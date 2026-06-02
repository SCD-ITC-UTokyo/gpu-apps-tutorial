#!/usr/bin/env python3
"""
diffusion アプリのビルド構成を生成する。

yaml (app/implementations/machines) を読み込み、指定 variant × machine × mode × compiler
に対する CMakeLists.txt または Makefile を variant ディレクトリに書き出す。

使い方:
    ./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu
        (default は cmake。CMakeLists.txt + ビルド用コマンドを出力)
    ./configure.py --variant F/openmp-cpu --machine wisteria --mode cpu --compiler gcc --build make
    ./configure.py --list                              # 使用可能な variant/machine 一覧
    ./configure.py --help
"""
import argparse
import fnmatch
import os
import socket
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML が必要です。pip install pyyaml", file=sys.stderr)
    sys.exit(1)

APP_ROOT = Path(__file__).resolve().parent
APP_NAME = APP_ROOT.name


# ---------------------------------------------------------------------------
# 共通ユーティリティ
# ---------------------------------------------------------------------------
def fail(msg: str) -> None:
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def load_yaml(path: Path) -> dict:
    if not path.exists():
        fail(f"yaml not found: {path}")
    with open(path) as f:
        return yaml.safe_load(f)


def list_machines() -> List[str]:
    md = APP_ROOT / "machines"
    return sorted(p.stem for p in md.glob("*.yaml"))


def detect_primary_group() -> Optional[str]:
    """`id -gn` で primary group 名を取得。失敗時は None。
    Linux クラスタでは primary group が課金グループ (gtXX/gjXX/grXX 等) に
    設定されていることが多く、yaml の placeholder を上書きするのに使える。
    """
    out = _run_cmd(["id", "-gn"])
    return out or None


def is_placeholder_group(g: str) -> bool:
    """yaml の default_group が明らかな placeholder か判定 (gr52, xxx, none)。"""
    return g.lower() in ("gr52", "xxx", "none", "")


def detect_kokkos_root() -> Optional[str]:
    """環境変数 Kokkos_ROOT から Kokkos install prefix を取得。未設定なら None。
    CMake の find_package(Kokkos) も同じ変数を参照するため、--group の `id -gn` に
    相当する「自動検出元」として使う。
    """
    root = (os.environ.get("Kokkos_ROOT") or "").strip()
    return root or None


def is_placeholder_kokkos_root(p: str) -> bool:
    """machine yaml の kokkos.root が placeholder か判定 (/xxx/ を含む、空、none)。"""
    if not p or not p.strip():
        return True
    low = p.lower()
    return "/xxx/" in low or low.strip("/ ") in ("xxx", "none")


def detect_actual_machine() -> Optional[str]:
    """現在の hostname を各 machine yaml の host_patterns と照合し、
    マッチした machine 名を返す。マッチが無ければ None (=ローカル開発機等)。
    環境変数 DIFFUSION_SKIP_HOSTCHECK=1 でスキップ可能。
    """
    if os.environ.get("DIFFUSION_SKIP_HOSTCHECK"):
        return None
    host = socket.gethostname().lower()
    for m in list_machines():
        mpath = APP_ROOT / "machines" / f"{m}.yaml"
        try:
            mdata = yaml.safe_load(open(mpath))
        except Exception:
            continue
        patterns = (mdata or {}).get("host_patterns") or []
        for p in patterns:
            if fnmatch.fnmatch(host, p.lower()):
                return m
    return None


# ---------------------------------------------------------------------------
# ローカル機の情報を自動検出 (machine="local" 用)
# 検出失敗時は静かに空文字を返し、yaml の description を fallback として使う
# ---------------------------------------------------------------------------
def _run_cmd(cmd: List[str], timeout: float = 2.0) -> str:
    # Python 3.6 互換: capture_output / text は 3.7+ なので PIPE + universal_newlines を使う
    try:
        r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           universal_newlines=True, timeout=timeout)
        if r.returncode == 0:
            return (r.stdout or "").strip()
    except (FileNotFoundError, subprocess.TimeoutExpired, OSError):
        pass
    return ""


def detect_local_info() -> str:
    """OS + CPU + GPU + NVHPC バージョンを検出して結合した文字列を返す。"""
    parts: List[str] = []

    # OS (Linux: /etc/os-release)
    try:
        with open("/etc/os-release") as f:
            for line in f:
                if line.startswith("PRETTY_NAME="):
                    parts.append(line.split("=", 1)[1].strip().strip('"'))
                    break
    except OSError:
        pass

    # CPU model
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("model name"):
                    cpu = line.split(":", 1)[1].strip()
                    # core 数を補足
                    ncores = sum(1 for ln in open("/proc/cpuinfo") if ln.startswith("processor"))
                    parts.append(f"{cpu} ({ncores} cores)")
                    break
    except OSError:
        pass

    # GPU (nvidia-smi)
    gpu = _run_cmd(["nvidia-smi", "--query-gpu=name,compute_cap", "--format=csv,noheader"])
    if gpu:
        # 1 行目だけ (複数 GPU 時は最初を代表として)
        first = gpu.split("\n")[0].strip()
        parts.append(f"GPU: {first}")

    # NVHPC version
    nvhpc = _run_cmd(["nvc++", "--version"])
    if nvhpc:
        # "nvc++ 26.3-0 64-bit target on x86-64 Linux -tp znver5" の 1 行目
        for line in nvhpc.split("\n"):
            line = line.strip()
            if line.startswith("nvc++"):
                # バージョン部分だけ抽出
                tokens = line.split()
                if len(tokens) >= 2:
                    parts.append(f"NVHPC {tokens[1]}")
                else:
                    parts.append(f"NVHPC ?")
                break

    return " | ".join(parts)


def list_variants() -> List[str]:
    out = []
    for lang_dir in ["C++", "F"]:
        lang_root = APP_ROOT / lang_dir
        if not lang_root.exists():
            continue
        for impl_dir in sorted(lang_root.iterdir()):
            if not impl_dir.is_dir():
                continue
            if impl_dir.name == "kokkos":
                # Kokkos: 4 階層 <policy>/<sub>
                for policy in sorted(impl_dir.iterdir()):
                    if not policy.is_dir():
                        continue
                    for sub in sorted(policy.iterdir()):
                        if sub.is_dir() and (sub / "src").exists():
                            out.append(f"{lang_dir}/kokkos/{policy.name}/{sub.name}")
                continue
            children = [c for c in impl_dir.iterdir() if c.is_dir()]
            children_with_src = [c for c in children if (c / "src").exists()]
            if children_with_src:
                for c in sorted(children_with_src):
                    out.append(f"{lang_dir}/{impl_dir.name}/{c.name}")
            elif (impl_dir / "src").exists():
                out.append(f"{lang_dir}/{impl_dir.name}")
    return out


def parse_variant(variant: str) -> Tuple[str, str, Optional[str], Optional[str]]:
    """variant パス解析:
       - 2 部: lang/impl (e.g., C++/openmp-cpu) → (lang, impl, None, None)
       - 3 部: lang/impl/variant (e.g., C++/openacc/auto.def) → (lang, impl, variant, None)
       - 4 部: lang/kokkos/policy/sub (e.g., C++/kokkos/range/baseline) → (lang, 'kokkos', policy, sub)
    """
    parts = variant.rstrip("/").split("/")
    if len(parts) == 4:
        return parts[0], parts[1], parts[2], parts[3]
    if len(parts) == 3:
        return parts[0], parts[1], parts[2], None
    if len(parts) == 2:
        return parts[0], parts[1], None, None
    fail(f"--variant の形式は 'C++/openacc/auto.def', 'C++/openmp-cpu', "
         f"または 'C++/kokkos/range/baseline' のみ。受け取った値: '{variant}'")
    return "", "", None, None


def lang_from_dir(lang_dir: str) -> str:
    if lang_dir == "C++":
        return "cpp"
    if lang_dir == "F":
        return "f90"
    fail(f"unknown language dir: '{lang_dir}'. C++ or F を指定。")
    return ""


def resolve_cpu_compiler(machine: dict, machine_name: str, requested: Optional[str],
                         lang: str) -> Tuple[str, dict]:
    cpu_block = machine.get("cpu") or {}
    compilers = cpu_block.get("compilers") or {}
    if not compilers:
        fail(f"machine '{machine_name}' に CPU compiler 定義が無い")

    default = cpu_block.get("default")
    name = requested or default
    if name not in compilers:
        avail = list(compilers.keys())
        fail(f"compiler '{name}' は machine '{machine_name}' に定義されていない。"
             f"使用可能: {avail}。default: {default}")

    compiler = compilers[name]
    required_keys = ["cxx", "cc"] if lang == "cpp" else ["f90"]
    missing = [k for k in required_keys if k not in compiler]
    if missing:
        alts = [c for c, cfg in compilers.items() if all(k in cfg for k in required_keys)]
        lang_desc = "C/C++" if lang == "cpp" else "Fortran"
        fail(f"compiler '{name}' は {lang_desc} をサポートしない (欠落キー: {missing})。"
             f"machine '{machine_name}' で {lang_desc} に使えるのは: {alts}")

    return name, compiler


# ---------------------------------------------------------------------------
# Build config 解決 (Make/CMake 共通)
# ---------------------------------------------------------------------------
def resolve_build_config(args, app, impls, machine, machine_name) -> dict:
    """yaml と CLI 引数から実効的なビルド設定を構築する。
    Make/CMake 両方の生成器が同じ dict を消費する。"""
    lang_dir, impl_name, variant_key, sub_key = parse_variant(args.variant)
    lang = lang_from_dir(lang_dir)

    impl_defs = impls.get("implementations") or {}
    if impl_name not in impl_defs:
        fail(f"impl '{impl_name}' は implementations.yaml に未定義。"
             f"使用可能: {list(impl_defs.keys())}")
    impl = impl_defs[impl_name]

    impl_lang = impl.get("lang")
    if impl_lang and impl_lang != lang:
        fail(f"impl '{impl_name}' は言語 '{impl_lang}' 専用。'{lang}' では使えない。")

    machine_modes = machine.get("modes", [])
    if args.mode not in machine_modes:
        hint = ""
        if args.mode == "uni" and machine_name == "wisteria":
            hint = (" (Wisteria の NVHPC 24.1 は `-gpu=mem:unified:nomanagedalloc` 構文非対応。"
                    "代わりに `--mode gpu` を使うと managed メモリで同等動作します)")
        elif args.mode == "uni":
            hint = (" (uni mode は NVHPC 24.5+ の unified memory 構文を要求します。"
                    "古い NVHPC では `--mode gpu` の managed メモリで代用してください)")
        fail(f"mode '{args.mode}' は machine '{machine_name}' でサポートされない。"
             f"使用可能: {machine_modes}{hint}")
    impl_modes = impl.get("needs_mode", [])
    if args.mode not in impl_modes:
        fail(f"impl '{impl_name}' は mode '{args.mode}' でビルドできない。"
             f"サポート: {impl_modes}")

    is_kokkos = (impl_name == "kokkos")

    # variant_dir parts: 通常 [lang_dir, impl_name, variant_key]、
    # Kokkos のみ [lang_dir, 'kokkos', policy, sub] の 4 階層
    variant_dir_parts = [lang_dir, impl_name]
    if variant_key is not None:
        variant_dir_parts.append(variant_key)
    if sub_key is not None:
        variant_dir_parts.append(sub_key)
    variant_dir = APP_ROOT.joinpath(*variant_dir_parts)
    if not variant_dir.exists():
        fail(f"variant ディレクトリが存在しない: {variant_dir}")
    if not (variant_dir / "src").exists():
        fail(f"src/ が見つからない: {variant_dir}/src")

    # ===== Kokkos の特別処理 =====
    if is_kokkos:
        if args.build != "cmake":
            fail("kokkos は CMake 強制 (--build cmake のみサポート)")
        kokkos_block = machine.get("kokkos") or {}
        if not kokkos_block:
            fail(f"machine '{machine_name}' に kokkos 設定が無い (machines/{machine_name}.yaml に kokkos.root を追加してください)")
        # Kokkos install prefix (link 先) の解決順 — --group と同じ思想:
        #   1. --kokkos-root CLI で明示指定された値があれば最優先
        #   2. なければ環境変数 Kokkos_ROOT を自動検出 (yaml が placeholder の時)
        #   3. それも無理なら machine yaml の kokkos.root (placeholder ならビルド前に要編集)
        yaml_kokkos_root = kokkos_block.get("root", "")
        cli_kokkos_root = getattr(args, "kokkos_root", None)
        detected_kokkos_root = detect_kokkos_root()
        kr_is_placeholder = is_placeholder_kokkos_root(yaml_kokkos_root)
        if cli_kokkos_root:
            kokkos_root = cli_kokkos_root
            kokkos_root_unresolved = False
            kokkos_root_source = "cli"
            if detected_kokkos_root and detected_kokkos_root != cli_kokkos_root:
                print(f"  [WARN] --kokkos-root '{cli_kokkos_root}' は環境変数 Kokkos_ROOT "
                      f"'{detected_kokkos_root}' と異なります。指定値を使用します。")
        elif kr_is_placeholder and detected_kokkos_root:
            kokkos_root = detected_kokkos_root
            kokkos_root_unresolved = False
            kokkos_root_source = "env"
        elif kr_is_placeholder and not detected_kokkos_root:
            kokkos_root = yaml_kokkos_root
            kokkos_root_unresolved = True
            kokkos_root_source = "placeholder"
        else:
            kokkos_root = yaml_kokkos_root
            kokkos_root_unresolved = False
            kokkos_root_source = "yaml"
        cxx_standard = kokkos_block.get("cxx_standard", 17)

        policy = variant_key   # range / mdrange / team
        sub = sub_key          # baseline / cpu-sweep / uvm / ...

        # UVM variant は GPU mode のみ
        is_uvm = sub.startswith("uvm")
        if is_uvm and args.mode not in ("gpu", "uni"):
            fail(f"kokkos uvm variant ({policy}/{sub}) は GPU mode のみ (CudaUVMSpace 要 CUDA backend)。"
                 f"--mode={args.mode} ではビルド不可。")

        # Sweep variant のマクロ既定値
        macros_default = impl.get("macros_default") or {}
        active_macros: List[Tuple[str, Union[str, int]]] = []
        if "tile-sweep" in sub:
            for k in ("_NX", "_NY", "_NZ"):
                if k in macros_default:
                    active_macros.append((k, macros_default[k]))
        if "chunk-sweep" in sub:
            if "_CHUNK" in macros_default:
                active_macros.append(("_CHUNK", macros_default["_CHUNK"]))

        # Kokkos C++ sources
        sources_cpp = app.get("sources", {}).get("cpp", [])
        # 拡張子変換 (.c → .cpp、Kokkos variant はすべて .cpp)
        sources = []
        for s in sources_cpp:
            if s.endswith(".c"):
                sources.append(s[:-2] + ".cpp")
            else:
                sources.append(s)

        target_base = app.get("target", "a.out")

        return {
            "is_kokkos": True,
            "kokkos_root": kokkos_root,
            "kokkos_root_unresolved": kokkos_root_unresolved,
            "kokkos_root_source": kokkos_root_source,
            "cxx_standard": cxx_standard,
            "policy": policy,
            "sub": sub,
            "is_uvm": is_uvm,
            "active_macros": active_macros,
            "lang_dir": lang_dir,
            "lang": lang,
            "impl_name": impl_name,
            "variant_key": variant_key,
            "sub_key": sub_key,
            "variant_dir": variant_dir,
            "compiler_name": "kokkos-managed",
            "sources": sources,
            "target_base": target_base,
            "default_fp": app.get("default_fp", 32),
            "libs": app.get("libs", []) or [],
        }

    # ===== 非 Kokkos: variant_key を implementations.yaml で参照 =====
    variant_cfg = {}
    if variant_key is not None:
        variant_defs = impls.get("variants") or {}
        if variant_key not in variant_defs:
            fail(f"variant '{variant_key}' は implementations.yaml に未定義。"
                 f"使用可能: {list(variant_defs.keys())}")
        variant_cfg = variant_defs[variant_key]

    is_cpu = (args.mode == "cpu")
    link_flags: List[str] = []

    if is_cpu:
        compiler_name, compiler = resolve_cpu_compiler(
            machine, machine_name, args.compiler, lang)
        cc_cmd = compiler.get("cc", {}).get("cmd", "")
        cxx_cmd = compiler.get("cxx", {}).get("cmd", "")
        f90_cmd = compiler.get("f90", {}).get("cmd", "")
        cc_flags = list(compiler.get("cc", {}).get("flags", []))
        cxx_flags = list(compiler.get("cxx", {}).get("flags", []))
        f90_flags = list(compiler.get("f90", {}).get("flags", []))
        omp_flag = compiler.get("openmp_flag") or ""
        common = list(compiler.get("common_flags", []))
        if omp_flag:
            for fl in [cc_flags, cxx_flags, f90_flags]:
                fl.append(omp_flag)
            link_flags.append(omp_flag)
        cc_flags = common + cc_flags
        cxx_flags = common + cxx_flags
        f90_flags = common + f90_flags
        f_module_flag = compiler.get("fortran_module_flag", "")
        f_include_flag = compiler.get("fortran_include_flag", "")
    else:
        if args.compiler:
            print(f"WARNING: GPU mode では --compiler は無視される (machine.gpu の compiler を使用)",
                  file=sys.stderr)
        compiler_name = "nvhpc"
        gpu_block = machine.get("gpu") or {}
        if not gpu_block:
            fail(f"machine '{machine_name}' に gpu compiler 定義が無い")
        gpu_compiler = gpu_block.get("compiler") or {}
        cc_cmd = gpu_compiler.get("cc", {}).get("cmd", "")
        cxx_cmd = gpu_compiler.get("cxx", {}).get("cmd", "")
        f90_cmd = gpu_compiler.get("f90", {}).get("cmd", "")
        cc_flags = list(gpu_compiler.get("cc", {}).get("flags", []))
        cxx_flags = list(gpu_compiler.get("cxx", {}).get("flags", []))
        f90_flags = list(gpu_compiler.get("f90", {}).get("flags", []))
        common = list(gpu_block.get("common_flags", []))
        f_module_flag = gpu_block.get("fortran_module_flag", "")
        f_include_flag = gpu_block.get("fortran_include_flag", "")

        parallel = impl.get("parallel_flag", "")
        memory_key_for_summary = "unified" if args.mode == "uni" else variant_cfg.get("memory", "managed")
        memory_key = memory_key_for_summary
        memory_models = gpu_block.get("memory_models") or {}
        if memory_key not in memory_models:
            avail = list(memory_models.keys())
            fail(f"memory model '{memory_key}' は machine '{machine_name}' で未サポート。"
                 f"使用可能: {avail}")
        memory_flag = memory_models[memory_key]

        for tok in (parallel + " " + memory_flag).split():
            for fl in [cc_flags, cxx_flags, f90_flags]:
                fl.append(tok)
            link_flags.append(tok)
        cc_flags = common + cc_flags
        cxx_flags = common + cxx_flags
        f90_flags = common + f90_flags

    nthreads = variant_cfg.get("nthreads")
    use_nthreads = (nthreads is not None and impl_name in ("openmp-target", "openacc"))

    if f_module_flag:
        for tok in f_module_flag.split():
            f90_flags.append(tok)
    if f_include_flag:
        for tok in f_include_flag.split():
            f90_flags.append(tok)

    sources_cpp = app.get("sources", {}).get("cpp", [])
    sources_f90 = app.get("sources", {}).get("f90", [])
    if lang == "cpp":
        # impl.lang == "cpp" (stdpar 等) は .c → .cpp 変換 (実ファイル拡張子に合わせる)
        if impl_lang == "cpp":
            sources = [s[:-2] + ".cpp" if s.endswith(".c") else s for s in sources_cpp]
        else:
            sources = sources_cpp
    else:
        sources = sources_f90
    if not sources:
        fail(f"app.yaml にソース ({lang}) が定義されていない")

    objs = [s.rsplit(".", 1)[0] + ".o" for s in sources]
    target_base = app.get("target", "a.out")
    libs = app.get("libs", []) or []

    # defines: (name, var_name_or_None)
    # var_name is the variable name in the build system (Make 変数 or CMake 変数)
    defines: List[Tuple[str, Optional[str]]] = [
        ("FP", "FP"),
        ("BENCHMARK_MODE", None),
    ]
    if use_nthreads:
        defines.append(("NTHREADS", "NTHREADS"))

    return {
        "is_kokkos": False,
        "lang_dir": lang_dir,
        "lang": lang,
        "impl_name": impl_name,
        "variant_key": variant_key,
        "variant_dir": variant_dir,
        "compiler_name": compiler_name,
        "is_cpu": is_cpu,
        "cc_cmd": cc_cmd,
        "cxx_cmd": cxx_cmd,
        "f90_cmd": f90_cmd,
        "cc_flags": cc_flags,
        "cxx_flags": cxx_flags,
        "f90_flags": f90_flags,
        "link_flags": link_flags,
        "defines": defines,
        "sources": sources,
        "objs": objs,
        "target_base": target_base,
        "use_nthreads": use_nthreads,
        "default_nthreads": nthreads if use_nthreads else 128,
        "default_fp": app.get("default_fp", 32),
        "libs": libs,
        "memory_key": (None if is_cpu else memory_key_for_summary),
    }


# ---------------------------------------------------------------------------
# Job script 生成 (machine yaml の job ブロックから PBS / PJM / bash を吐く)
# ---------------------------------------------------------------------------
def generate_job_script(cfg: dict, args, machine: dict, machine_name: str,
                        build_dir: Optional[str]) -> Tuple[Path, str, str]:
    """run.<machine>.<mode>.sh を variant_dir に書き出す。

    machine yaml の job ブロックを参照:
      job.scheduler  : pbs | pjm | none
      job.per_mode.<mode>: { queue, omp_threads, modules: [...] }
      job.default_group, default_walltime
      job.env: [VAR=VALUE, ...]

    戻り値: (出力パス, scheduler 名)。scheduler=none ならパスは返すが空ディレクティブ。
    """
    job = machine.get("job") or {}
    scheduler = (job.get("scheduler") or "none").lower()
    per_mode = (job.get("per_mode") or {}).get(args.mode) or {}
    mode = args.mode
    variant_dir = cfg["variant_dir"]

    # 環境設定 (module load 等) はユーザ責務 — ビルド env と実行 env を揃えるため、
    # configure.py は推奨例をコメントとしてのみ提供し、自動でロードはしない。
    module_examples = list(per_mode.get("modules", []))

    # group_list 解決順 (--group と同じ思想):
    #   1. --group CLI で明示指定された値があれば最優先
    #   2. なければ yaml.default_group が placeholder の時に `id -gn` で検出
    #   3. それも無理なら placeholder のまま (要手動編集)
    # 注: PJM/PBS のディレクティブ行末にインラインコメント (`#`) を置くと
    # スケジューラが option として解釈してエラーになる。コメントは別行に置く。
    yaml_group = job.get("default_group", "xxx")
    is_placeholder = is_placeholder_group(yaml_group)
    detected_group = detect_primary_group()  # 常に検出 (CLI 値との比較用)
    cli_group = getattr(args, "group", None)

    if cli_group:
        # CLI 明示指定が最優先
        resolved_group = cli_group
        group_unresolved = False
        # 自動検出値と違う場合は標準出力に警告 (バッチスケジューラがある時のみ意味あり)
        if scheduler in ("pbs", "pjm") and detected_group and detected_group != cli_group:
            print(f"  [WARN] --group '{cli_group}' は `id -gn` 検出値 '{detected_group}' と異なります。"
                  f"指定値 '{cli_group}' を使用します。")
        group_pre_comment = (
            f"# group_list: '{cli_group}' (--group で明示指定)"
            if not detected_group or detected_group == cli_group
            else f"# group_list: '{cli_group}' (--group で明示指定、自動検出 '{detected_group}' とは異なる)"
        )
    elif is_placeholder and detected_group:
        # yaml が placeholder で自動検出が成功
        resolved_group = detected_group
        group_unresolved = False
        group_pre_comment = f"# group_list: primary group ({detected_group}) を自動検出。違う場合は --group で明示指定"
    elif is_placeholder and not detected_group:
        # 検出失敗 (placeholder のまま)
        resolved_group = yaml_group
        group_unresolved = True
        group_pre_comment = "# group_list: !!! 手動編集が必要 (自動検出失敗) !!!"
    else:
        # yaml に明示値あり、CLI 指定無し
        resolved_group = yaml_group
        group_unresolved = False
        group_pre_comment = f"# group_list: '{yaml_group}' (machine yaml の default_group)"

    # バイナリ相対パスとデフォルト引数 (N=64)
    fp = cfg.get("default_fp", 32)
    nthr = cfg.get("default_nthreads") if cfg.get("use_nthreads") else None
    bin_suffix = f".{nthr}" if nthr else ""
    bin_name = f"{cfg['target_base']}.{mode}.{fp}{bin_suffix}"
    bin_rel = f"./{build_dir}/{bin_name}" if build_dir else f"./{bin_name}"

    lines: List[str] = ["#!/bin/bash"]

    # group_list 検出失敗時: 先頭に目立つ警告ヘッダを置く
    if group_unresolved and scheduler in ("pbs", "pjm"):
        lines += [
            "#",
            "# ============================================================",
            "# !!! 警告: 課金グループ (group_list) の自動検出に失敗しました !!!",
            "# ============================================================",
            f"# yaml の default_group は placeholder ('{yaml_group}')、",
            "# `id -gn` も有効な値を返しませんでした。",
            "#",
            "# 投入前に下記の行を編集してください:",
            "#   PBS:  '#PBS -W group_list=<YOUR_GROUP>'",
            "#   PJM:  '#PJM -g <YOUR_GROUP>'",
            "#",
            "# 自分のグループは `id -Gn` または所属機関のポータルで確認できます。",
            "# ============================================================",
            "",
        ]

    def env_setup_block(workdir_var: str) -> List[str]:
        """module load 等の env 設定はユーザ責務とし、推奨例をコメントとして埋める。"""
        block = [
            "",
            "################################################################",
            "# 環境設定: コンピュートノードは module 未ロード状態で起動します。",
            "# ビルド時と同じ環境を再現してください (以下はこの machine の推奨例)。",
            "# 必要に応じて編集 / コメントアウト解除して使ってください。",
            "#",
        ]
        if module_examples:
            block.append("#   module purge")
            for m in module_examples:
                block.append(f"#   module load {m}")
        else:
            block.append("#   (推奨 module なし)")
        block += [
            "################################################################",
            "",
            f"cd ${{{workdir_var}}}",
        ]
        return block

    if scheduler == "pbs":
        q = per_mode.get("queue", "debug-g")
        omp = per_mode.get("omp_threads", 72)
        walltime = job.get("default_walltime", "00:10:00")
        lines += [
            group_pre_comment,
            f"#PBS -N \"{cfg['target_base']}\"",
            f"#PBS -q {q}",
            f"#PBS -l select=1:ompthreads={omp}",
            f"#PBS -l walltime={walltime}",
            f"#PBS -W group_list={resolved_group}",
            f"#PBS -j oe",
        ]
        lines += env_setup_block("PBS_O_WORKDIR")

    elif scheduler == "pjm":
        q = per_mode.get("queue", "debug-a")
        omp = per_mode.get("omp_threads", 72)
        walltime = job.get("default_walltime", "00:10:00")
        lines += [
            group_pre_comment,
            f"#PJM -N \"{cfg['target_base']}\"",
            f"#PJM -L rscgrp={q}",
            f"#PJM -L node=1",
            f"#PJM --omp thread={omp}",
            f"#PJM -L elapse={walltime}",
            f"#PJM -g {resolved_group}",
            f"#PJM -j",
        ]
        lines += env_setup_block("PJM_O_WORKDIR")

    else:
        lines.append("# ローカル実行 (バッチスケジューラ無し)")

    # 環境変数
    for env in (job.get("env") or []):
        lines.append(f"export {env}")
    if (job.get("env")):
        lines.append("")

    # group 検出失敗時の bash guard: スクリプトが編集されないまま実行されたら exit
    if group_unresolved and scheduler in ("pbs", "pjm"):
        guard_pattern = yaml_group
        lines += [
            "",
            f"# group_list 検出失敗時の安全装置: placeholder のままなら実行を止める",
            f"if grep -qE '(group_list|-g)[ =]{guard_pattern}([^A-Za-z0-9]|$)' \"$0\"; then",
            f"  echo 'ERROR: 課金グループが placeholder ({guard_pattern}) のままです。スクリプト先頭の指示に従って編集してください。' >&2",
            f"  exit 1",
            f"fi",
        ]

    lines += [
        "",
        "# === 実行 (N = nx*ny*nz の N; 64 → 64x64x64 立方体) ===",
        f"N=64",
        f"{bin_rel} $N",
    ]

    content = "\n".join(lines) + "\n"
    out_path = variant_dir / f"run.{machine_name}.{mode}.sh"
    return out_path, content, scheduler


# ---------------------------------------------------------------------------
# Makefile 生成
# ---------------------------------------------------------------------------
def generate_makefile(cfg: dict, args, machine_name: str) -> Tuple[Path, str]:
    lang = cfg["lang"]
    variant_dir = cfg["variant_dir"]
    mk_path = variant_dir / f"Makefile.gen.{machine_name}.{args.mode}"

    # 言語別フラグに defines (-DFP=$(FP) 等) を追加
    cc_flags = list(cfg["cc_flags"])
    cxx_flags = list(cfg["cxx_flags"])
    f90_flags = list(cfg["f90_flags"])
    for name, var in cfg["defines"]:
        flag = f"-D{name}=$({var})" if var else f"-D{name}"
        for fl in [cc_flags, cxx_flags, f90_flags]:
            fl.append(flag)

    # target 名 (Make 変数で組み立て)
    target = f"{cfg['target_base']}.{args.mode}.$(FP)"
    if cfg["use_nthreads"]:
        target = f"{cfg['target_base']}.{args.mode}.$(FP).$(NTHREADS)"

    ld = "$(CXX)" if lang == "cpp" else "$(F90)"
    ldflags = "$(CXXFLAGS)" if lang == "cpp" else "$(F90FLAGS)"

    if lang == "cpp":
        obj_rule = (
            "%.o: src/%.c\n"
            "\t$(CC) $(CFLAGS) -c $< -o $@\n"
            "\n"
            "%.o: src/%.cpp\n"
            "\t$(CXX) $(CXXFLAGS) -c $< -o $@\n"
        )
    else:
        obj_rule = (
            "mod:\n"
            "\tmkdir -p mod\n"
            "\n"
            "%.o: src/%.f90 | mod\n"
            "\t$(F90) $(F90FLAGS) -c $< -o $@\n"
        )

    lines = [
        f"# Generated by configure.py at {APP_ROOT}/configure.py",
        f"# Source yamls: app.yaml, implementations.yaml, machines/{machine_name}.yaml",
        f"# Config: variant={args.variant} machine={machine_name} mode={args.mode} compiler={cfg['compiler_name']}",
        f"# DO NOT EDIT — re-run configure.py to regenerate.",
        f"",
        f"FP        ?= {cfg['default_fp']}",
        f"NTHREADS  ?= {cfg['default_nthreads']}",
        f"",
        f"# === Compilers ===",
        f"CC        = {cfg['cc_cmd']}",
        f"CXX       = {cfg['cxx_cmd']}",
        f"F90       = {cfg['f90_cmd']}",
        f"",
        f"# === Flags ===",
        f"CFLAGS    = {' '.join(cc_flags)}",
        f"CXXFLAGS  = {' '.join(cxx_flags)}",
        f"F90FLAGS  = {' '.join(f90_flags)}",
        f"",
        f"# === Build ===",
        f"OBJS      = {' '.join(cfg['objs'])}",
        f"TARGET    = {target}",
        f"LIBS      = {' '.join(cfg['libs'])}",
        f"",
        f".PHONY: all clean show_config",
        f"",
        f"all: show_config $(TARGET)",
        f"",
        f"show_config:",
        f"\t@echo '--- build config ---'",
        f"\t@echo 'MACHINE   = {machine_name}'",
        f"\t@echo 'MODE      = {args.mode}'",
        f"\t@echo 'COMPILER  = {cfg['compiler_name']}'",
        f"\t@echo 'FP        = $(FP)'",
        f"\t@echo 'TARGET    = $(TARGET)'",
        f"",
        obj_rule,
        f"$(TARGET): $(OBJS)",
        f"\t{ld} {ldflags} $(OBJS) $(LIBS) -o $@",
        f"",
        f"clean:",
        f"\trm -f *.o $(TARGET) *.mod mod/*.mod",
        f"\t-rmdir mod 2>/dev/null || true",
        f"",
    ]
    return mk_path, "\n".join(lines)


# ---------------------------------------------------------------------------
# CMakeLists.txt 生成
# ---------------------------------------------------------------------------
def generate_cmake(cfg: dict, args, machine_name: str) -> Tuple[Path, str]:
    lang = cfg["lang"]
    variant_dir = cfg["variant_dir"]
    cmake_path = variant_dir / "CMakeLists.txt"

    # 言語別フラグ。Fortran のモジュール出力先は CMAKE_Fortran_MODULE_DIRECTORY で
    # 管理するため、yaml の fortran_module_flag / fortran_include_flag 由来のフラグは除く。
    cc_flags = list(cfg["cc_flags"])
    cxx_flags = list(cfg["cxx_flags"])
    def _strip_fortran_mod_flags(flags: List[str]) -> List[str]:
        # Fortran モジュール出力先指定は CMake が自動付与するので除く。
        # 対象: -Jmod / -J mod / -module mod / -Mmod / -Imod / -I mod
        out = []
        skip_next = False
        for f in flags:
            if skip_next:
                skip_next = False
                continue
            if f in ("-J", "-module", "-Mmod"):
                skip_next = True   # 直後の path も捨てる
                continue
            if f.startswith("-J") or f.startswith("-module"):
                continue   # -Jmod や -modulemod のような連結形
            if f == "-Imod":
                continue
            out.append(f)
        return out
    f90_flags = _strip_fortran_mod_flags(list(cfg["f90_flags"]))

    # CMake で必要な language の決定 (sources の拡張子で判定)
    src_ext = {s.rsplit(".", 1)[-1].lower() for s in cfg["sources"]}
    cmake_langs = []
    if "c" in src_ext:
        cmake_langs.append("C")
    if "cpp" in src_ext or "cxx" in src_ext or "cc" in src_ext:
        cmake_langs.append("CXX")
    if "f90" in src_ext or "f" in src_ext or "for" in src_ext:
        cmake_langs.append("Fortran")
    if not cmake_langs:
        cmake_langs = ["C"]  # fallback
    cmake_langs_str = " ".join(cmake_langs)

    # defines を CMake 表現に変換: FP=${FP}, BENCHMARK_MODE, NTHREADS=${NTHREADS}
    define_strs = []
    for name, var in cfg["defines"]:
        if var:
            define_strs.append(f"{name}=${{{var}}}")
        else:
            define_strs.append(name)

    # 出力名 (CMake 変数で組み立て)
    target_name = APP_NAME  # CMake target 名は APP_NAME (binary file 名は OUTPUT_NAME で動的)
    output_name = f"{cfg['target_base']}.{args.mode}.${{FP}}"
    if cfg["use_nthreads"]:
        output_name = f"{cfg['target_base']}.{args.mode}.${{FP}}.${{NTHREADS}}"

    # sources は src/ 配下
    src_paths = [f"src/{s}" for s in cfg["sources"]]

    lines = [
        f"cmake_minimum_required(VERSION 3.20)",
        f"",
        f"# Generated by configure.py at {APP_ROOT}/configure.py",
        f"# Source yamls: app.yaml, implementations.yaml, machines/{machine_name}.yaml",
        f"# Config: variant={args.variant} machine={machine_name} mode={args.mode} compiler={cfg['compiler_name']}",
        f"# DO NOT EDIT — re-run configure.py to regenerate.",
        f"",
        f"# Compilers (must be set before project())",
    ]
    if cfg["cc_cmd"]:
        lines.append(f'set(CMAKE_C_COMPILER "{cfg["cc_cmd"]}")')
    if cfg["cxx_cmd"]:
        lines.append(f'set(CMAKE_CXX_COMPILER "{cfg["cxx_cmd"]}")')
    if cfg["f90_cmd"]:
        lines.append(f'set(CMAKE_Fortran_COMPILER "{cfg["f90_cmd"]}")')

    lines += [
        f"",
        f"project({APP_NAME} LANGUAGES {cmake_langs_str})",
        f"",
        f"# Cache variables (override via -DFP=64 etc.)",
        f'set(FP "{cfg["default_fp"]}" CACHE STRING "Floating point bits (32 or 64)")',
        f'set(NTHREADS "{cfg["default_nthreads"]}" CACHE STRING "Threads (for omp-target/openacc opt variant)")',
        f"",
        f"# Per-language compile flags",
    ]
    if "C" in cmake_langs:
        lines.append(f'set(CMAKE_C_FLAGS "{" ".join(cc_flags)}")')
    if "CXX" in cmake_langs:
        lines.append(f'set(CMAKE_CXX_FLAGS "{" ".join(cxx_flags)}")')
    if "Fortran" in cmake_langs:
        lines.append(f'set(CMAKE_Fortran_FLAGS "{" ".join(f90_flags)}")')
        lines.append(f'set(CMAKE_Fortran_MODULE_DIRECTORY "${{CMAKE_BINARY_DIR}}/mod")')

    lines += [
        f"",
        f"# Defines (compile-time macros)",
        f"add_compile_definitions({' '.join(define_strs)})",
        f"",
        f"# Include dirs",
        f'include_directories(".")',
        f"",
    ]
    if cfg["link_flags"]:
        lines.append(f"# Link flags (parallel + memory model, etc.)")
        lines.append(f"add_link_options({' '.join(cfg['link_flags'])})")
        lines.append(f"")

    lines += [
        f"# Target",
        f"add_executable({target_name} {' '.join(src_paths)})",
        f'set_target_properties({target_name} PROPERTIES OUTPUT_NAME "{output_name}")',
    ]
    if cfg["libs"]:
        lines.append(f"target_link_libraries({target_name} PRIVATE {' '.join(cfg['libs'])})")

    lines.append("")
    return cmake_path, "\n".join(lines)


# ---------------------------------------------------------------------------
# Kokkos CMakeLists.txt 生成
# ---------------------------------------------------------------------------
def generate_cmake_kokkos(cfg: dict, args, machine_name: str) -> Tuple[Path, str]:
    variant_dir = cfg["variant_dir"]
    cmake_path = variant_dir / "CMakeLists.txt"

    target_name = APP_NAME      # CMake target 名 (= APP_NAME)
    output_name = f"{cfg['target_base']}.{args.mode}.${{FP}}"
    src_paths = [f"src/{s}" for s in cfg["sources"]]

    # tile/chunk マクロ (sweep variant のみ)
    macro_defs = [f"{n}=${{{n}}}" for n, _ in cfg["active_macros"]]
    macro_cache_lines = []
    for name, default in cfg["active_macros"]:
        macro_cache_lines.append(
            f'set({name} "{default}" CACHE STRING "{name} for {cfg["sub"]}")'
        )

    define_strs = ["FP=${FP}", "BENCHMARK_MODE"] + macro_defs

    # Kokkos install prefix の挿入行 (解決元 / 未解決で出し分け)
    kroot = cfg["kokkos_root"]
    kroot_src = cfg.get("kokkos_root_source", "yaml")
    src_label = {"cli": "--kokkos-root 指定", "env": "環境変数 Kokkos_ROOT",
                 "yaml": "machine yaml の kokkos.root"}.get(kroot_src, kroot_src)
    if cfg.get("kokkos_root_unresolved"):
        kokkos_install_block = [
            "# !!! Kokkos install prefix 未解決 !!!",
            f"#   machine yaml の kokkos.root が placeholder ('{kroot}')、",
            "#   かつ環境変数 Kokkos_ROOT も未設定です。次のいずれかで指定してください:",
            "#     - configure.py を --kokkos-root /path/to/kokkos 付きで再実行",
            "#     - export Kokkos_ROOT=/path/to/kokkos してから configure.py を再実行",
            f"#     - machines/{machine_name}.yaml の kokkos.root を実際のパスに編集",
            f'list(APPEND CMAKE_PREFIX_PATH "{kroot}")  # <-- placeholder。要編集',
            "find_package(Kokkos REQUIRED)",
        ]
    else:
        kokkos_install_block = [
            f"# Kokkos installation ({src_label} 由来)",
            f'list(APPEND CMAKE_PREFIX_PATH "{kroot}")',
            "find_package(Kokkos REQUIRED)",
        ]

    lines = [
        f"cmake_minimum_required(VERSION 3.20)",
        f"",
        f"# Generated by configure.py at {APP_ROOT}/configure.py",
        f"# Source yamls: app.yaml, implementations.yaml, machines/{machine_name}.yaml",
        f"# Config: variant={args.variant} machine={machine_name} mode={args.mode}",
        f"#         (Kokkos: policy={cfg['policy']} sub={cfg['sub']} uvm={cfg['is_uvm']})",
        f"# DO NOT EDIT — re-run configure.py to regenerate.",
        f"",
        f"project({APP_NAME} LANGUAGES CXX)",
        f"",
        f"set(CMAKE_CXX_STANDARD {cfg['cxx_standard']})",
        f"set(CMAKE_CXX_STANDARD_REQUIRED ON)",
        f"set(CMAKE_CXX_EXTENSIONS OFF)",
        f"",
        *kokkos_install_block,
        f"",
        f"# Cache variables (cmake -DFP=64 等で上書き可)",
        f'set(FP "{cfg["default_fp"]}" CACHE STRING "Floating point bits (32 or 64)")',
    ]
    lines.extend(macro_cache_lines)

    lines += [
        f"",
        f"# Target",
        f"add_executable({target_name} {' '.join(src_paths)})",
        f"target_link_libraries({target_name} PRIVATE Kokkos::kokkos)",
        f"target_compile_definitions({target_name} PRIVATE {' '.join(define_strs)})",
        f'set_target_properties({target_name} PROPERTIES OUTPUT_NAME "{output_name}")',
        f"",
    ]
    if cfg["libs"]:
        lines.append(f"target_link_libraries({target_name} PRIVATE {' '.join(cfg['libs'])})")
        lines.append("")

    return cmake_path, "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI / main
# ---------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    desc = (f"{APP_NAME} アプリのビルド構成生成器。\n"
            f"yaml (app/implementations/machines) を読み、CMakeLists.txt または "
            f"Makefile.<machine>.<mode> を variant 配下に書き出す。")
    epilog = (
        "例:\n"
        "  ./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu\n"
        "      (default は cmake)\n"
        "  ./configure.py --variant F/openmp-cpu --machine wisteria --mode cpu --compiler gcc --build make\n"
        "  ./configure.py --list\n"
        "\n"
        "生成後のビルド (cmake):\n"
        "  cd <variant_dir>\n"
        "  cmake -B build-<machine>-<mode> -S .\n"
        "  cmake --build build-<machine>-<mode>\n"
        "\n"
        "生成後のビルド (make):\n"
        "  cd <variant_dir> && make -f Makefile.gen.<machine>.<mode>\n"
    )
    p = argparse.ArgumentParser(
        prog="configure.py",
        description=desc,
        epilog=epilog,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--variant",
        help="variant パス。例: 'C++/openmp-target/auto.def', 'F/openmp-cpu' (--list で一覧)",
    )
    p.add_argument(
        "--machine",
        help="machine 名。machines/<name>.yaml に対応 (--list で一覧)",
    )
    p.add_argument(
        "--mode",
        choices=["gpu", "cpu", "uni"],
        help="ビルドモード。gpu=GPU offload、cpu=CPU OpenMP、uni=GPU unified memory",
    )
    p.add_argument(
        "--compiler",
        default=None,
        help="CPU compiler 名 (cpu mode のみ有効)。未指定時は machine yaml の default を使用。"
             "未登録 compiler 指定時は使用可能名を列挙してエラー終了。",
    )
    p.add_argument(
        "--build",
        choices=["cmake", "make"],
        default="cmake",
        help="ビルドシステム選択 (default: cmake)。Kokkos は CMake 強制 (現状未実装)。"
             "非Kokkos は cmake と make の両方からビルド可。",
    )
    p.add_argument(
        "--fp",
        type=int,
        choices=[32, 64],
        default=None,
        help="浮動小数精度の default 値を指定 (32 or 64)。"
             "省略時は app.yaml の default_fp (32)。cmake -DFP=N でも上書き可。",
    )
    p.add_argument(
        "--nthreads",
        type=int,
        default=None,
        help="NTHREADS の default 値を指定 (auto.opt/manu.opt variant の openmp-target/openacc のみ実効)。"
             "省略時は 128。cmake -DNTHREADS=N でも上書き可。",
    )
    p.add_argument(
        "--tile-nx",
        type=int,
        default=None,
        help="_NX (Kokkos mdrange tile-sweep) の default 値 (cmake -D_NX=N でも上書き可)。",
    )
    p.add_argument(
        "--tile-ny",
        type=int,
        default=None,
        help="_NY (Kokkos mdrange tile-sweep) の default 値。",
    )
    p.add_argument(
        "--tile-nz",
        type=int,
        default=None,
        help="_NZ (Kokkos mdrange tile-sweep) の default 値。",
    )
    p.add_argument(
        "--chunk",
        type=int,
        default=None,
        help="_CHUNK (Kokkos team chunk-sweep) の default 値 (cmake -D_CHUNK=N でも上書き可)。",
    )
    p.add_argument(
        "--opt-level",
        choices=["O0", "O1", "O2", "O3", "O4", "fast"],
        default=None,
        help="ベース最適化レベルを上書き (default: machine yaml の common_flags の -O3)。"
             "例: --opt-level O2、--opt-level fast (NVHPC の -fast)。"
             "元 hairdesc_diffusion の range/uvm-sweep variant が O2/O3/O4/fast を sweep していた相当。",
    )
    p.add_argument(
        "--extra-cflags",
        default=None,
        metavar="STR",
        help='ベースフラグに追加する任意の compile flag (CC/CXX/F90 すべてに適用)。'
             '空白区切りで複数指定可。例: --extra-cflags="-xCORE-AVX512" '
             '--extra-cflags="-march=sapphirerapids -funroll-loops"。'
             '元 range/cpu-sweep variant の compile flag sweep 相当 (arch / vector ext)。',
    )
    p.add_argument(
        "--list",
        action="store_true",
        help="使用可能な variant / machine の一覧を表示して終了。",
    )
    p.add_argument(
        "--info",
        action="store_true",
        help="設定一覧 (machine, device, compiler, 最適化, 精度等) を表示。"
             "単独で全体サマリ、--machine 指定で機種詳細、"
             "--variant + --machine + --mode で解決された構成詳細を表示。",
    )
    p.add_argument(
        "--group",
        default=None,
        metavar="NAME",
        help="ジョブスクリプトの課金グループ (PBS group_list / PJM -g) を明示指定。"
             "省略時は `id -gn` で primary group を自動検出。"
             "明示値が自動検出値と異なる場合は標準出力に警告を表示。",
    )
    p.add_argument(
        "--kokkos-root",
        default=None,
        metavar="PATH",
        help="Kokkos install prefix (CMake find_package(Kokkos) のリンク先) を明示指定。"
             "省略時は環境変数 Kokkos_ROOT を自動検出し、無ければ machine yaml の kokkos.root を使用。"
             "明示値が環境変数と異なる場合は警告を表示。kokkos variant のみ実効。",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="生成内容を標準出力に表示するだけで、ファイルは書き出さない。",
    )
    return p


def cmd_list() -> None:
    print("=== machines ===")
    for m in list_machines():
        print(f"  {m}")
    print("\n=== variants ===")
    for v in list_variants():
        print(f"  {v}")


def cmd_info(args, app, impls, machine, machine_name) -> None:
    """設定一覧を表示。引数の指定レベルに応じて詳細度が変わる。"""
    # ----- 全体サマリ (machine 指定なし) -----
    if not machine_name:
        print(f"=== {APP_NAME} app yaml 設定一覧 ===\n")

        # app.yaml
        print(f"## app.yaml")
        print(f"  target           : {app.get('target')}")
        print(f"  default_fp       : {app.get('default_fp')}")
        print(f"  libs             : {app.get('libs') or '(none)'}")
        print(f"  sources (cpp)    : {' '.join(app.get('sources', {}).get('cpp', []))}")
        print(f"  sources (f90)    : {' '.join(app.get('sources', {}).get('f90', []))}")
        print()

        # implementations
        print(f"## implementations")
        for name, impl in (impls.get("implementations") or {}).items():
            lang = impl.get("lang") or "all"
            modes = impl.get("needs_mode", [])
            flag = impl.get("parallel_flag", "—") or "—"
            print(f"  {name:20s} lang={lang:6s} modes={str(modes):20s} flag={flag}")
        print()

        # variants (非 Kokkos)
        print(f"## non-Kokkos variants (memory model × optimization)")
        for name, var in (impls.get("variants") or {}).items():
            mem = var.get("memory", "—")
            nthr = f"NTHREADS={var.get('nthreads')}" if var.get("nthreads") else ""
            print(f"  {name:12s} memory={mem:10s} {nthr}")
        print()

        # Kokkos macros
        macros = (impls.get("implementations") or {}).get("kokkos", {}).get("macros_default") or {}
        if macros:
            print(f"## Kokkos sweep variant の既定マクロ")
            for k, v in macros.items():
                print(f"  {k} = {v}")
            print()

        # machines
        print(f"## machines ({len(list_machines())} 個)")
        for m in list_machines():
            mpath = APP_ROOT / "machines" / f"{m}.yaml"
            mdata = load_yaml(mpath)
            modes = mdata.get("modes", [])
            cpu_def = (mdata.get("cpu") or {}).get("default", "—")
            cpu_list = list((mdata.get("cpu") or {}).get("compilers", {}).keys())
            kokkos = "✓" if mdata.get("kokkos") else "—"
            print(f"  {m:10s} modes={str(modes):24s} cpu_default={cpu_def:8s} cpu={cpu_list}  kokkos={kokkos}")
        print()

        # 最適化・精度オプション
        print(f"## ビルド時のオプション (CMake -D で上書き可能)")
        print(f"  FP             : {app.get('default_fp', 32)} (32 / 64 — float / double 切替)")
        print(f"  NTHREADS       : 128 (default; auto.opt variant の openmp-target/openacc のみ実効)")
        print(f"  _NX/_NY/_NZ    : {macros.get('_NX')}/{macros.get('_NY')}/{macros.get('_NZ')} (Kokkos mdrange/cpu-tile-sweep のみ)")
        print(f"  _CHUNK         : {macros.get('_CHUNK')} (Kokkos team/cpu-chunk-sweep のみ)")
        print()

        # ヘルプ
        print(f"## 使い方の例")
        print(f"  ./configure.py --info --machine local")
        print(f"      → machine 'local' の詳細 (compiler 全部、GPU 情報、Kokkos 等)")
        print(f"  ./configure.py --info --variant C++/openacc/auto.def --machine local --mode gpu")
        print(f"      → この組合せで解決された compiler/flags/binary 名等の詳細")
        print(f"  ./configure.py --list")
        print(f"      → variant / machine の一覧 (省略表示)")
        return

    # ----- machine 詳細 (variant 指定なし) -----
    if not args.variant or not args.mode:
        print(f"=== machine '{machine_name}' の詳細 ===\n")
        yaml_desc = machine.get('description', '—')
        print(f"description : {yaml_desc}")
        if machine_name == "local":
            detected = detect_local_info()
            if detected:
                print(f"detected    : {detected}")
        print(f"scheduler   : {machine.get('scheduler', '—')}")
        print(f"modes       : {machine.get('modes', [])}")
        print()

        # CPU section
        cpu = machine.get("cpu") or {}
        if cpu:
            print(f"## CPU compilers (default: {cpu.get('default')})")
            for name, c in (cpu.get("compilers") or {}).items():
                langs = [k for k in ["cxx", "cc", "f90"] if k in c]
                print(f"  --compiler {name}")
                for k in langs:
                    cmd = c[k].get("cmd", "")
                    flags = " ".join(c[k].get("flags", []))
                    print(f"    {k:4s}: {cmd} {flags}".rstrip())
                print(f"    openmp_flag: {c.get('openmp_flag', '—')!r}")
                if "fortran_module_flag" in c:
                    print(f"    fortran_module_flag: {c.get('fortran_module_flag', '—')!r}")
                common = c.get("common_flags", [])
                if common:
                    print(f"    common_flags: {' '.join(common)}")
            print()

        # GPU section
        gpu = machine.get("gpu") or {}
        if gpu:
            print(f"## GPU compiler (NVHPC)")
            gc = gpu.get("compiler") or {}
            for k in ("cxx", "cc", "f90"):
                if k in gc:
                    cmd = gc[k].get("cmd", "")
                    flags = " ".join(gc[k].get("flags", []))
                    print(f"  {k:4s}: {cmd} {flags}".rstrip())
            common = gpu.get("common_flags", [])
            if common:
                print(f"  common_flags: {' '.join(common)}")
            mm = gpu.get("memory_models") or {}
            if mm:
                print(f"  memory_models:")
                for name, flag in mm.items():
                    print(f"    {name:10s}: {flag}")
            print()

        # Kokkos
        kk = machine.get("kokkos") or {}
        if kk:
            print(f"## Kokkos")
            for k, v in kk.items():
                print(f"  {k:14s}: {v}")
            print()
        else:
            print(f"## Kokkos: 未設定 (kokkos variant は使えない)\n")
        return

    # ----- 具体的構成詳細 (variant + machine + mode 指定) -----
    cfg = resolve_build_config(args, app, impls, machine, machine_name)
    print(f"=== 解決された具体構成 ===\n")
    print(f"## 入力")
    print(f"  variant  : {args.variant}")
    print(f"  machine  : {machine_name}")
    print(f"  mode     : {args.mode}")
    print(f"  compiler : {args.compiler or '(default)'}")
    print(f"  build    : {args.build}")
    print()

    print(f"## 解決された設定")
    print(f"  is_kokkos          : {cfg.get('is_kokkos')}")
    print(f"  compiler_name      : {cfg.get('compiler_name')}")
    print(f"  variant_dir        : {cfg['variant_dir'].relative_to(APP_ROOT)}")
    if cfg.get("is_kokkos"):
        print(f"  kokkos_root        : {cfg.get('kokkos_root')}")
        print(f"  cxx_standard       : {cfg.get('cxx_standard')}")
        print(f"  policy             : {cfg.get('policy')}")
        print(f"  sub                : {cfg.get('sub')}")
        print(f"  is_uvm             : {cfg.get('is_uvm')}")
        if cfg.get('active_macros'):
            print(f"  active_macros      : {dict(cfg['active_macros'])}")
    else:
        print(f"  cc / cxx / f90     : {cfg['cc_cmd']!r} / {cfg['cxx_cmd']!r} / {cfg['f90_cmd']!r}")
        print(f"  cc_flags           : {' '.join(cfg.get('cc_flags', []))}")
        print(f"  cxx_flags          : {' '.join(cfg.get('cxx_flags', []))}")
        print(f"  f90_flags          : {' '.join(cfg.get('f90_flags', []))}")
        print(f"  link_flags         : {' '.join(cfg.get('link_flags', []))}")
        defines = cfg.get("defines", [])
        define_strs = [f"{n}=${{{v}}}" if v else n for n, v in defines]
        print(f"  defines            : {' '.join(define_strs)}")
    print(f"  sources            : {' '.join(cfg.get('sources', []))}")
    print(f"  default_fp         : {cfg.get('default_fp')}")
    print(f"  target_base        : {cfg.get('target_base')}")
    print()

    # 出力ファイルとビルドコマンド
    print(f"## 生成されるファイル")
    if cfg.get("is_kokkos") or args.build == "cmake":
        out = cfg["variant_dir"] / "CMakeLists.txt"
    else:
        out = cfg["variant_dir"] / f"Makefile.gen.{machine_name}.{args.mode}"
    print(f"  {out.relative_to(APP_ROOT)}")
    print()
    print(f"## 想定バイナリ名")
    suffix = ""
    if cfg.get('use_nthreads'):
        suffix = ".$(NTHREADS)"
    print(f"  {cfg['target_base']}.{args.mode}.$(FP){suffix}")
    print(f"  (例: {cfg['target_base']}.{args.mode}.{cfg.get('default_fp', 32)})")


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.list:
        cmd_list()
        return 0

    # --info: 段階的な詳細表示
    if args.info:
        app = load_yaml(APP_ROOT / "app.yaml")
        impls = load_yaml(APP_ROOT / "implementations.yaml")
        machine = {}
        if args.machine:
            machine_path = APP_ROOT / "machines" / f"{args.machine}.yaml"
            if not machine_path.exists():
                fail(f"machine yaml not found: {machine_path}. 使用可能: {list_machines()}")
            machine = load_yaml(machine_path)
        cmd_info(args, app, impls, machine, args.machine)
        return 0

    if not args.variant or not args.machine or not args.mode:
        parser.print_help(sys.stderr)
        print("\nERROR: --list, --info, または (--variant + --machine + --mode) のいずれかが必要", file=sys.stderr)
        return 2

    app = load_yaml(APP_ROOT / "app.yaml")
    impls = load_yaml(APP_ROOT / "implementations.yaml")
    machine_path = APP_ROOT / "machines" / f"{args.machine}.yaml"
    if not machine_path.exists():
        avail = list_machines()
        fail(f"machine yaml not found: {machine_path}. 使用可能: {avail}")
    machine = load_yaml(machine_path)

    # 実行ホストと --machine の不整合を検出 (--dry-run は許容)
    actual = detect_actual_machine()
    if actual and actual != args.machine and not args.dry_run:
        host = socket.gethostname()
        fail(f"現在のホスト '{host}' は machine '{actual}' に該当しますが、"
             f"--machine '{args.machine}' が指定されています。"
             f"間違ったマシン用の構成を生成しようとしている可能性があります。"
             f"\n  - 現在のホストで動かすなら: --machine {actual}"
             f"\n  - 他マシン向けに生成だけしたいなら: --dry-run を併用するか、"
             f"環境変数 DIFFUSION_SKIP_HOSTCHECK=1 を設定")

    cfg = resolve_build_config(args, app, impls, machine, args.machine)

    # --opt-level: 既存 flags 中の -O\d を新しい opt level に置換 (なければ追加)
    def _apply_opt_level(flags: List[str], new_level: str) -> Tuple[List[str], str]:
        """flags を opt-level 置換、(新 flags, 旧 level 文字列) を返す"""
        old_level = ""
        out = []
        for f in flags:
            if (f.startswith("-O") and len(f) >= 3 and (f[2:].isdigit() or f == "-Ofast")) or f == "-fast":
                old_level = f
                continue
            out.append(f)
        new_flag = "-fast" if new_level == "fast" else f"-{new_level}"
        out.append(new_flag)
        return out, old_level

    # --opt-level: 非 Kokkos のみ (Kokkos はインストール時に決定)
    opt_level_old = None
    if args.opt_level is not None:
        if cfg.get("is_kokkos"):
            fail(f"--opt-level は非 Kokkos variant のみ実効です。"
                 f"variant '{args.variant}' (Kokkos) では指定不可。"
                 f"Kokkos の最適化は spack install kokkos ... cxxstd=N cflags='...' 等で"
                 f"インストール時に決定されます。"
                 f"対応 variant: 非 Kokkos (openmp-cpu, openmp-target, openacc, stdpar, do-concurrent)")
        for key in ("cc_flags", "cxx_flags", "f90_flags"):
            cfg[key], oldv = _apply_opt_level(cfg.get(key, []), args.opt_level)
            opt_level_old = opt_level_old or oldv

    # --extra-cflags: 非 Kokkos のみ
    if args.extra_cflags:
        if cfg.get("is_kokkos"):
            fail(f"--extra-cflags は非 Kokkos variant のみ実効です。"
                 f"variant '{args.variant}' (Kokkos) では指定不可。"
                 f"Kokkos に追加 flag を渡す場合は spack install 時の cflags 等で指定。"
                 f"対応 variant: 非 Kokkos (openmp-cpu, openmp-target, openacc, stdpar, do-concurrent)")
        extra_tokens = args.extra_cflags.split()
        for key in ("cc_flags", "cxx_flags", "f90_flags"):
            cfg[key] = list(cfg.get(key, [])) + extra_tokens

    cfg["_opt_level_old"] = opt_level_old
    cfg["_opt_level_new"] = args.opt_level
    cfg["_extra_cflags"] = args.extra_cflags

    # CLI オプションで default 値を上書き (非対応の組み合わせは fail で拒否)
    if args.fp is not None:
        cfg["default_fp"] = args.fp

    # --nthreads: omp-target/openacc の auto.opt/manu.opt variant のみ実効
    if args.nthreads is not None:
        if cfg.get("use_nthreads"):
            cfg["default_nthreads"] = args.nthreads
        else:
            fail(f"--nthreads は variant '{args.variant}' では実効しません。"
                 f"対応: openmp-target/openacc の auto.opt または manu.opt のみ。"
                 f"指定された variant の impl='{cfg.get('impl_name')}', variant_key='{cfg.get('variant_key')}' は対象外。")

    # --tile-nx/ny/nz: Kokkos mdrange の tile-sweep 系 variant のみ
    tile_args = {"--tile-nx": args.tile_nx, "--tile-ny": args.tile_ny, "--tile-nz": args.tile_nz}
    tile_given = [k for k, v in tile_args.items() if v is not None]
    if tile_given:
        if not cfg.get("is_kokkos"):
            fail(f"{', '.join(tile_given)} は Kokkos の tile-sweep variant のみ実効。"
                 f"variant '{args.variant}' (非 Kokkos) では指定不可。"
                 f"対応 variant: C++/kokkos/mdrange/cpu-tile-sweep, C++/kokkos/mdrange/uvm-tile-sweep")
        active_names = {n for n, _ in cfg.get("active_macros", [])}
        if not {"_NX", "_NY", "_NZ"} & active_names:
            fail(f"{', '.join(tile_given)} は variant '{args.variant}' では実効しません "
                 f"(active macros: {list(active_names) or '(なし)'})。"
                 f"対応 variant: C++/kokkos/mdrange/cpu-tile-sweep, C++/kokkos/mdrange/uvm-tile-sweep")

    # --chunk: Kokkos team の chunk-sweep 系 variant のみ
    if args.chunk is not None:
        if not cfg.get("is_kokkos"):
            fail(f"--chunk は Kokkos の chunk-sweep variant のみ実効。"
                 f"variant '{args.variant}' (非 Kokkos) では指定不可。"
                 f"対応 variant: C++/kokkos/team/cpu-chunk-sweep, C++/kokkos/team/uvm-chunk-sweep")
        active_names = {n for n, _ in cfg.get("active_macros", [])}
        if "_CHUNK" not in active_names:
            fail(f"--chunk は variant '{args.variant}' では実効しません "
                 f"(active macros: {list(active_names) or '(なし)'})。"
                 f"対応 variant: C++/kokkos/team/cpu-chunk-sweep, C++/kokkos/team/uvm-chunk-sweep")

    # active_macros を --tile-* / --chunk で上書き
    if cfg.get("is_kokkos") and cfg.get("active_macros"):
        override_map = {"_NX": args.tile_nx, "_NY": args.tile_ny,
                        "_NZ": args.tile_nz, "_CHUNK": args.chunk}
        new_macros = []
        for name, value in cfg["active_macros"]:
            if override_map.get(name) is not None:
                value = override_map[name]
            new_macros.append((name, value))
        cfg["active_macros"] = new_macros

    if cfg.get("is_kokkos"):
        # Kokkos は cmake 強制 (resolve_build_config で検証済)
        out_path, content = generate_cmake_kokkos(cfg, args, args.machine)
        build_dir = f"build-{args.machine}-{args.mode}"
    elif args.build == "cmake":
        out_path, content = generate_cmake(cfg, args, args.machine)
        build_dir = f"build-{args.machine}-{args.mode}"
    else:
        out_path, content = generate_makefile(cfg, args, args.machine)
        build_dir = None

    if args.dry_run:
        print(f"# would write: {out_path}")
        print(content)
        return 0

    out_path.write_text(content)
    rel = out_path.relative_to(APP_ROOT)
    print(f"Generated: {rel}")
    print()
    # machine = "local" は実行ホストの情報を自動検出して表示
    machine_desc = machine.get('description', '')
    if args.machine == "local":
        detected = detect_local_info()
        if detected:
            machine_desc = f"detected: {detected}"

    print(f"--- Build configuration ---")
    print(f"  variant       : {args.variant}")
    print(f"  machine       : {args.machine}  ({machine_desc})")
    print(f"  mode          : {args.mode}  (gpu=GPU offload, cpu=CPU OpenMP, uni=GPU unified memory)")
    print(f"  language      : {cfg['lang']} ({cfg['lang_dir']})")
    print(f"  implementation: {cfg['impl_name']}")
    print(f"  compiler      : {cfg['compiler_name']}", end="")
    if cfg.get("is_kokkos"):
        print(f"  (Kokkos-managed)")
    elif cfg.get("is_cpu"):
        print(f"  ({cfg.get('cc_cmd', '?')} / {cfg.get('cxx_cmd', '?')} / {cfg.get('f90_cmd', '?')})")
    else:
        print(f"  ({cfg.get('cc_cmd', '?')} / {cfg.get('cxx_cmd', '?')} / {cfg.get('f90_cmd', '?')})")
    print(f"  build system  : {args.build}")
    print(f"  precision     : FP={cfg.get('default_fp', 32)} (default; cmake -DFP=64 で double 精度に変更可)")
    if cfg.get("is_kokkos"):
        policy_descs = {"range": "parallel_for (RangePolicy)",
                        "mdrange": "MDRangePolicy",
                        "team": "TeamPolicy + TeamThreadRange"}
        policy = cfg.get('policy', '?')
        print(f"  policy        : {policy}  ({policy_descs.get(policy, '?')})")
        print(f"  memory layout : {'CudaUVMSpace (UVM)' if cfg.get('is_uvm') else 'default View'}")
        if cfg.get('active_macros'):
            macros_str = ", ".join(f"{n}={v}" for n, v in cfg['active_macros'])
            print(f"  macros (default): {macros_str}  (cmake -D で上書き可)")
        kroot = cfg.get('kokkos_root', '')
        if len(kroot) > 70:
            kroot = "..." + kroot[-67:]
        kroot_src = cfg.get('kokkos_root_source', 'yaml')
        src_label = {"cli": "--kokkos-root 明示指定", "env": "環境変数 Kokkos_ROOT 自動検出",
                     "yaml": "machine yaml の kokkos.root"}.get(kroot_src, kroot_src)
        if cfg.get('kokkos_root_unresolved'):
            print(f"  ! kokkos root : '{kroot}' (placeholder。--kokkos-root か環境変数 Kokkos_ROOT で指定してください)")
        else:
            print(f"  kokkos root   : {kroot}  ({src_label})")
        print(f"  cxx_standard  : C++{cfg.get('cxx_standard', '?')}")
    else:
        # 非 Kokkos
        # ベース opt level (machine yaml common_flags 由来、--opt-level で上書き可)
        # cxx_flags の中から -O\d / -fast を探して表示
        base_opt = None
        for f in cfg.get("cxx_flags", []):
            if (f.startswith("-O") and len(f) >= 3 and (f[2:].isdigit() or f == "-Ofast")) or f == "-fast":
                base_opt = f
                break
        if cfg.get("_opt_level_new"):
            print(f"  opt level     : {base_opt}  (--opt-level={cfg['_opt_level_new']} で上書き; 元 {cfg['_opt_level_old'] or '(なし)'})")
        else:
            print(f"  opt level     : {base_opt}  (machine yaml の common_flags 由来; --opt-level で上書き可)")
        if cfg.get("_extra_cflags"):
            print(f"  extra cflags  : {cfg['_extra_cflags']}  (全言語の flags に追加)")
        if cfg.get('use_nthreads'):
            print(f"  variant tuning: NTHREADS={cfg.get('default_nthreads', 128)}  (--nthreads N または cmake -DNTHREADS=N で上書き可)")
        else:
            print(f"  variant tuning: (variant 固有の最適化なし)")
        mem_key = cfg.get('memory_key')
        if mem_key:
            mem_descs = {"managed": "GPU/CPU 共有メモリ (自動転送)",
                          "separate": "明示的データ管理 (手動データ転送)",
                          "unified": "Unified memory (NVHPC unified)"}
            print(f"  memory model  : {mem_key}  ({mem_descs.get(mem_key, '')})")
    print(f"  target binary : {cfg.get('target_base', '?')}.{args.mode}.<FP>" +
          (".<NTHREADS>" if cfg.get('use_nthreads') else ""))
    print(f"  sources       : {' '.join(cfg.get('sources', []))}")
    print()
    print(f"次の手順:")
    print(f"  cd {out_path.parent.relative_to(APP_ROOT)}")
    if args.build == "cmake":
        print(f"  cmake -B {build_dir} -S .")
        print(f"  cmake --build {build_dir}")
        print(f"  # バイナリ: {build_dir}/{cfg['target_base']}.{args.mode}.<FP>")
    else:
        print(f"  make -f {out_path.name}")

    # ジョブスクリプトを variant_dir に書き出し
    job_path, job_content, scheduler = generate_job_script(
        cfg, args, machine, args.machine, build_dir)
    job_path.write_text(job_content)
    job_path.chmod(0o755)
    job_rel = job_path.relative_to(APP_ROOT)
    job_block = machine.get("job") or {}
    submit_cmd = job_block.get("submit_cmd")
    status_cmd = job_block.get("status_cmd", "")
    print()
    print(f"Generated job script: {job_rel}")
    if scheduler == "pbs" or scheduler == "pjm":
        sched_label = "PBS Pro" if scheduler == "pbs" else "PJM (Fujitsu)"
        print(f"  scheduler   : {sched_label}")
        print(f"  投入        : {submit_cmd} {job_path.name}")
        if status_cmd:
            print(f"  状態確認    : {status_cmd}")
        yaml_group = job_block.get("default_group", "xxx")
        cli_group = getattr(args, "group", None)
        detected = detect_primary_group()
        if cli_group:
            if detected and detected != cli_group:
                print(f"  group_list  : '{cli_group}' (--group 明示指定、自動検出 '{detected}' と異なる)")
            else:
                print(f"  group_list  : '{cli_group}' (--group 明示指定)")
        elif is_placeholder_group(yaml_group):
            if detected:
                print(f"  group_list  : '{detected}' (primary group を自動検出。違う場合は --group で明示指定)")
            else:
                print(f"  ! group_list は yaml で '{yaml_group}' に設定。実機では --group で指定するかスクリプトを編集してください。")
        else:
            print(f"  group_list  : '{yaml_group}' (machine yaml の default_group)")
    else:
        print(f"  実行        : bash {job_path.name}  (またはバイナリを直接実行)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""
results/<machine>/*.log を fem.csv 形式 (18 列) に集約変換する。

バイナリ自身が 10 カラム CSV を出力する (`# binary,NP,time_sec,...`):
    # binary,NP,time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time
    /path/to/C++/kokkos/range/baseline/build-miyabi-gpu/fem.gpu.64,274625,4.164441e-02,...

このスクリプトはそれを読み、バイナリパスから category/machine/mode/language/impl/
variant/memory_model/optimization_type/optimization_param/fp の 10 カラムを推論して
18 カラム CSV に変換出力する (diffusion/summary/parse_logs.py と同じ役割)。

使い方:
    # results/miyabi/ 配下の全 .log/.csv を変換
    python3 parse_logs.py --input ../results/miyabi --output ../results/miyabi/summary.csv

    # 既存の fem.csv に追記
    python3 parse_logs.py --input ../results/miyabi --append fem.csv

    # 個別ファイル指定
    python3 parse_logs.py file1.log file2.log --output combined.csv

旧形式ログについて:
    Kokkos 実装はかつて 10 カラム CSV を出力せず、
        *** matrix conn. <sec> sec. / <iter> <resid> / *** solver <sec> sec.
    というテキストだけを出していた。その形式のログも読めるよう、10 カラム CSV が
    1 行も無いファイルは自動的に旧形式として解析し、非 Kokkos 実装と同じ FLOP 式
        FLOP = ITER*(NP*14 + NPLU*2) + NP*3 + NPLU*2,  NPLU(n) = (3n-2)^3 - n^3
    で performance_gflops を復元する (要 --nside)。現在のバイナリは CSV を直接
    出力するので、新規計測でこの経路を使う必要はない。
"""
from __future__ import annotations
import argparse
import csv
import os
import re
import sys
from pathlib import Path

# fem.csv の 18 カラムスキーマ
COLS = [
    "category", "machine", "mode", "language",
    "impl", "variant", "memory_model",
    "optimization_type", "optimization_param", "fp",
    "N",
    "time_sec", "performance_gflops", "error",
    "real_sec", "user_sec", "sys_sec",
    "source_file",
]

# バイナリパスから言語・impl・variant を抜き出す regex
# 例: .../C++/openmp-target/auto.def/build-miyabi-gpu/fem.gpu.32
#     .../C++/kokkos/range/baseline/build-miyabi-gpu/fem.gpu.64  (Kokkos は 4 階層)
PATH_RE = re.compile(
    r"/(?P<lang>C\+\+|F)/(?P<impl>[^/]+)(?:/(?P<variant>[^/]+(?:/[^/]+)?))?"
    r"/build-(?P<machine>[^-]+)-(?P<mode>[^/]+)/(?P<binname>[^/]+)$"
)
BIN_RE = re.compile(
    r"^(?P<target>\w+)\.(?P<mode>gpu|cpu|uni)\.(?P<fp>\d+)(?:\.(?P<nthr>\d+))?$"
)


def nplu(n: int) -> int:
    """構造格子六面体メッシュの非対角非零要素数 (1 辺 n 節点)。"""
    return (3 * n - 2) ** 3 - n ** 3


def infer_meta(binary_path: str, log_file: Path) -> dict:
    """バイナリパス + log ファイル位置から 10 カラムのメタ情報を推論。"""
    meta = {c: "" for c in COLS}

    parts = log_file.resolve().parts
    machine_from_log = ""
    if "results" in parts:
        idx = parts.index("results")
        if idx + 1 < len(parts):
            machine_from_log = parts[idx + 1]

    path_str = binary_path.replace("\\", "/")
    m = PATH_RE.search(path_str)
    if m:
        lang_dir = m.group("lang")
        impl = m.group("impl")
        meta["language"] = "cpp" if lang_dir == "C++" else "f90"
        meta["impl"] = impl
        meta["variant"] = m.group("variant") or ""
        meta["machine"] = m.group("machine")
        meta["mode"] = m.group("mode")
        meta["category"] = "Kokkos" if impl == "kokkos" else "non-Kokkos"
        binname = m.group("binname")
    else:
        binname = os.path.basename(path_str)
        if machine_from_log:
            meta["machine"] = machine_from_log

    mb = BIN_RE.match(os.path.basename(binname))
    if mb:
        meta["mode"] = meta["mode"] or mb.group("mode")
        meta["fp"] = mb.group("fp")
        if mb.group("nthr"):
            meta["optimization_param"] = f"NTHREADS={mb.group('nthr')}"
            meta["optimization_type"] = "nthreads"

    v = meta.get("variant", "")
    if v.startswith("auto"):
        meta["memory_model"] = "managed"
    elif v.startswith("manu"):
        meta["memory_model"] = "separate"
    elif meta["mode"] == "uni":
        meta["memory_model"] = "unified"
    elif meta["impl"] == "openmp-cpu" or meta["mode"] == "cpu":
        meta["memory_model"] = "cpu-only"

    if meta["impl"] == "kokkos":
        meta["memory_model"] = "kokkos-uvm" if "uvm" in v else "kokkos-default"
        if "tile-sweep" in v:
            meta["optimization_type"] = "tile-sweep"
        elif "chunk-sweep" in v:
            meta["optimization_type"] = "chunk-sweep"
        elif "cpu-sweep" in v or "uvm-sweep" in v:
            meta["optimization_type"] = "opt-level-sweep"
        else:
            meta["optimization_type"] = "baseline"
    elif not meta.get("optimization_type"):
        if v.endswith(".opt"):
            meta["optimization_type"] = "nthreads"
            if not meta["optimization_param"]:
                meta["optimization_param"] = "NTHREADS=128"
        else:
            meta["optimization_type"] = "baseline"

    return meta


def parse_csv_log(log_file: Path) -> list[dict]:
    """10 カラム CSV 形式のログから 18 カラム dict のリストを返す。"""
    rows = []
    with open(log_file) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = [x.strip() for x in line.split(",")]
            if len(fields) < 10:
                continue
            try:
                N = int(fields[1])
            except ValueError:
                continue
            meta = infer_meta(fields[0], log_file)
            meta["N"] = N
            meta["time_sec"] = fields[2]
            meta["performance_gflops"] = fields[3]
            meta["error"] = fields[4]
            meta["real_sec"] = fields[5]
            meta["user_sec"] = fields[6]
            meta["sys_sec"] = fields[7]
            # fields[8]=num_time_steps_logged, [9]=last_sim_time は 18 カラムに含まれない
            meta["source_file"] = str(log_file)
            rows.append(meta)
    return rows


def parse_legacy_log(log_file: Path, nside: int, binary: str) -> list[dict]:
    """旧 Kokkos ログ (10 カラム CSV 無し) から FLOP を復元して 18 カラムを作る。"""
    txt = open(log_file).read()
    it = re.findall(r"^(\d+) ([0-9.eE+-]+)\s*$", txt, re.M)
    m_s = re.search(r"\*\*\* solver\s+([0-9.eE+-]+) sec", txt)
    if not it or not m_s:
        return []
    iters, resid = int(it[-1][0]), float(it[-1][1])
    solver_s = float(m_s.group(1))
    NP, P = nside ** 3, nplu(nside)
    flop = iters * (NP * 14 + P * 2) + NP * 3 + P * 2
    meta = infer_meta(binary, log_file)
    meta.update({
        "N": NP, "time_sec": f"{solver_s:.6e}",
        "performance_gflops": f"{flop / solver_s * 1e-9:.6e}",
        "error": f"{resid:.6e}", "real_sec": "", "user_sec": "", "sys_sec": "",
        "source_file": str(log_file),
    })
    return [meta]


def collect_logs(paths: list[Path]) -> list[Path]:
    out = []
    for p in paths:
        if p.is_dir():
            out.extend(sorted(p.rglob("*.log")))
            out.extend(sorted(p.rglob("*.csv")))
        elif p.is_file():
            out.append(p)
    return [f for f in out if f.suffix in (".log", ".csv") and f.stat().st_size > 0]


def main() -> int:
    ap = argparse.ArgumentParser(
        description="results/<machine>/*.log を fem.csv 形式 (18 列) に集約変換")
    ap.add_argument("inputs", nargs="*", help="入力ファイル or ディレクトリ")
    ap.add_argument("--input", action="append", default=[],
                    help="入力ディレクトリ (複数指定可)")
    ap.add_argument("--output", default=None, help="出力 CSV パス (省略時は stdout)")
    ap.add_argument("--append", default=None, metavar="EXISTING_CSV",
                    help="既存 CSV (例: summary/fem.csv) に追記 (header はスキップ)")
    ap.add_argument("--nside", type=int, default=None,
                    help="旧形式ログ用: 1 辺の節点数 (65 → 65^3)")
    ap.add_argument("--binary", default=None,
                    help="旧形式ログ用: 対応するバイナリパス (メタ情報の推論に使う)")
    args = ap.parse_args()

    paths = [Path(p) for p in args.inputs + args.input]
    if not paths:
        ap.error("入力ファイル or --input ディレクトリを指定")

    log_files = collect_logs(paths)
    if not log_files:
        print("WARNING: 入力に該当する *.log/*.csv が見つからない", file=sys.stderr)
        return 1

    all_rows = []
    for lf in log_files:
        rows = parse_csv_log(lf)
        if not rows:
            # 10 カラム CSV が無い → 旧形式ログとして解析
            if args.nside is None:
                print(f"WARNING: {lf}: 10 カラム CSV が無い。旧形式なら --nside を指定",
                      file=sys.stderr)
                continue
            rows = parse_legacy_log(lf, args.nside, args.binary or str(lf))
            if not rows:
                print(f"WARNING: {lf}: 解析できません", file=sys.stderr)
                continue
        all_rows.extend(rows)
        print(f"  {lf}: {len(rows)} rows", file=sys.stderr)

    print(f"合計 {len(all_rows)} 行抽出", file=sys.stderr)

    if args.append:
        with open(args.append, "a", newline="") as f:
            csv.DictWriter(f, fieldnames=COLS, lineterminator="\n").writerows(all_rows)
        print(f"追記: {args.append}", file=sys.stderr)
    else:
        out = open(args.output, "w", newline="") if args.output else sys.stdout
        w = csv.DictWriter(out, fieldnames=COLS, lineterminator="\n")
        w.writeheader()
        w.writerows(all_rows)
        if args.output:
            out.close()
            print(f"出力: {args.output}", file=sys.stderr)
    return 0


sys.exit(main())

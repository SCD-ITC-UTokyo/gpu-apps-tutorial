#!/usr/bin/env python3
"""
nbody のベンチ出力 (`<name>_run.csv`) を nbody.csv 形式 (25 カラム) に集約変換する。

nbody バイナリ (C++ / Fortran とも common/io.hpp の write_log) は、次のヘッダ付き
CSV を出力する:
    exec,N,time[s],step,time_per_step[s],interactions_per_sec,Flop/s,FP_L,FP_M
      [,dt,energy_error_worst,energy_error_final,virial_ratio_final]
    ./build-local-gpu/nbody.gpu.32.64,1024,2.11e+00,8192,2.58e-04,4.02e+09,8.85e+10,32,64

末尾 4 列 (dt, energy_error_worst, energy_error_final, virial_ratio_final) は
CALCULATE_POTENTIAL かつ **非 BENCHMARK_MODE** ビルドの時だけ出力される。
(既定の BENCHMARK_MODE ビルドでは 9 列のみ。)

このスクリプトはそれを読み、バイナリ/ログのパスから
category/machine/mode/language/impl/variant/memory_model/optimization_type/optimization_param
を推論し、出力の各指標を nbody.csv の対応列にマップする。

埋まらない列について:
  - real_sec / user_sec / sys_sec: プログラムは出力しない (実行ラッパの `time` 等が必要)。空のまま。
  - energy_error_* / virial_ratio_final / dt / error: 非 BENCHMARK_MODE ビルドの出力にのみ存在。
    BENCHMARK_MODE ビルドでは空のまま。

使い方:
    # results/local/ 配下の全 *_run.csv を変換
    python3 parse_logs.py --input ../results/local --output ../results/local/summary.csv
    # 既存の nbody.csv に追記 (header はスキップ)
    python3 parse_logs.py --input ../results/local --append nbody.csv
    # 個別ファイル指定
    python3 parse_logs.py file1_run.csv --output combined.csv
"""
from __future__ import annotations
import argparse
import csv
import os
import re
import sys
from pathlib import Path

# 出力スキーマ = 実 nbody.csv の 25 カラム (順序も一致させる)。
# fp は精度構成ラベル (低精度部 FP_L / 標準精度部 FP_M を明示)。高精度部 FP_H は常に 64 固定。
COLS = [
    "category", "machine", "mode", "language",
    "impl", "variant", "memory_model",
    "optimization_type", "optimization_param", "fp", "N",
    "time_sec", "performance_gflops", "error",
    "real_sec", "user_sec", "sys_sec",
    "source_file",
    "energy_error_final", "virial_ratio_final", "dt", "energy_error_worst",
    "interactions_per_sec", "step", "time_per_step_sec",
]

# build-<machine>-<mode> ディレクトリを含むフルパス用
PATH_RE = re.compile(
    r"/(?P<lang>C\+\+|F)/(?P<impl>[^/]+)(?:/(?P<variant>[^/]+(?:/[^/]+)?))?"
    r"/build-(?P<machine>[^-]+)-(?P<mode>[^/]+)/(?P<binname>[^/]+)$"
)
# nbody バイナリ名: target[.bench].mode.FP_L[.FP_M][.NTHREADS] 例 nbody.gpu.32.64
BIN_RE = re.compile(
    r"^(?P<target>\w+)(?:\.bench)?\.(?P<mode>gpu|cpu|uni)"
    r"\.(?P<fp_l>\d+)(?:\.(?P<fp_m>\d+))?(?:\.(?P<nthr>\d+))?$"
)

# variant 階層を打ち切る (これ以降は variant 名ではない) ディレクトリ名
_STOP_DIRS = {"src", "log", "logs", "results"}


def fp_label(fp_l: str, fp_m: str) -> str:
    """低精度部 FP_L / 標準精度部 FP_M から精度構成ラベルを作る。"""
    if fp_l == fp_m:
        return f"{fp_l} (FP_L=FP_M={fp_l})"
    return f"{fp_l}_{fp_m} (FP_L={fp_l}, FP_M={fp_m})"


def _scan_path(path: str) -> dict:
    """パス中の C++/F 以降から lang/impl/variant、build-<machine>-<mode> から machine/mode を拾う。
    exec パス (相対) でもログファイルパスでも動くよう寛容に走査する。"""
    out = {}
    parts = path.replace("\\", "/").split("/")
    for i, p in enumerate(parts):
        if p in ("C++", "F"):
            out["language"] = "cpp" if p == "C++" else "f90"
            if i + 1 < len(parts):
                out["impl"] = parts[i + 1]
            rest = []
            for q in parts[i + 2:]:
                if q.startswith("build-") or q in _STOP_DIRS or q.endswith(".csv"):
                    break
                rest.append(q)
            if rest:
                out["variant"] = "/".join(rest)   # kokkos は policy/sub の 2 階層
            break
    for p in parts:
        mb = re.match(r"^build-(?P<machine>[^-]+)-(?P<mode>.+)$", p)
        if mb:
            out["machine"] = mb.group("machine")
            out["mode"] = mb.group("mode")
            break
    return out


def infer_meta(exec_path: str, log_file: Path) -> dict:
    """exec パス + ログファイル位置から meta 列 (指標以外) を推論。"""
    meta = {c: "" for c in COLS}

    # results/<machine>/ から machine を補足
    rparts = log_file.resolve().parts
    machine_from_log = ""
    if "results" in rparts:
        idx = rparts.index("results")
        if idx + 1 < len(rparts):
            machine_from_log = rparts[idx + 1]

    # exec パス → ログファイルパスの順に走査 (どちらかに variant 情報が含まれていれば拾う)
    scanned = {}
    m = PATH_RE.search(exec_path.replace("\\", "/"))
    if m:
        scanned = {
            "language": "cpp" if m.group("lang") == "C++" else "f90",
            "impl": m.group("impl"), "variant": m.group("variant") or "",
            "machine": m.group("machine"), "mode": m.group("mode"),
        }
    else:
        scanned = _scan_path(exec_path)
        for k, v in _scan_path(str(log_file.resolve())).items():
            scanned.setdefault(k, v)   # exec で取れなかったものをログパスで補完

    for k in ("language", "impl", "variant", "machine", "mode"):
        if scanned.get(k):
            meta[k] = scanned[k]
    if not meta["machine"] and machine_from_log:
        meta["machine"] = machine_from_log
    if meta["impl"]:
        meta["category"] = "Kokkos" if meta["impl"] == "kokkos" else "non-Kokkos"

    # バイナリ名から mode / nthreads を補完
    binname = os.path.basename(exec_path.replace("\\", "/"))
    mb = BIN_RE.match(binname)
    if mb:
        meta["mode"] = meta["mode"] or mb.group("mode")
        if mb.group("nthr"):
            meta["optimization_param"] = f"NTHREADS={mb.group('nthr')}"
            meta["optimization_type"] = "nthreads"

    # variant 名から memory_model / optimization_type を推論
    v = meta.get("variant", "")
    if v.startswith("auto"):
        meta["memory_model"] = "unified" if "auto" in v else "managed"
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
        meta["optimization_type"] = "あり" if v.endswith(".opt") else "baseline"

    return meta


def _gflops(flops: str) -> str:
    try:
        return repr(float(flops) / 1e9)
    except (ValueError, TypeError):
        return ""


def parse_log_file(log_file: Path) -> list:
    """nbody の <name>_run.csv 1 ファイルを 25 カラム dict のリストに変換。"""
    rows = []
    with open(log_file) as f:
        for line in f:
            line = line.strip()
            # 空行・コメント・ヘッダ行 (exec,N,...) はスキップ
            if not line or line.startswith("#") or line.startswith("exec,"):
                continue
            fields = [x.strip() for x in line.split(",")]
            if len(fields) < 9:        # nbody は最低 9 列 (BENCHMARK_MODE)
                continue
            try:
                N = int(fields[1])
            except ValueError:
                continue               # ヘッダや不正行を確実に除外

            meta = infer_meta(fields[0], log_file)
            meta["N"] = str(N)
            meta["time_sec"] = fields[2]
            meta["step"] = fields[3]
            meta["time_per_step_sec"] = fields[4]
            meta["interactions_per_sec"] = fields[5]
            meta["performance_gflops"] = _gflops(fields[6])
            meta["fp"] = fp_label(fields[7], fields[8])
            # 保存量系 (非 BENCHMARK_MODE ビルドのみ存在)
            if len(fields) >= 13:
                meta["dt"] = fields[9]
                meta["energy_error_worst"] = fields[10]
                meta["energy_error_final"] = fields[11]
                meta["virial_ratio_final"] = fields[12]
                meta["error"] = fields[11]      # legacy 互換: error = energy_error_final
            # real/user/sys_sec はプログラム出力に無いため空のまま
            meta["source_file"] = str(log_file)
            rows.append(meta)
    return rows


def collect_logs(paths: list) -> list:
    out = []
    for p in paths:
        if p.is_dir():
            out.extend(sorted(p.rglob("*_run.csv")))
            out.extend(sorted(p.rglob("*.log")))
        elif p.is_file():
            out.append(p)
    return [f for f in out if f.suffix in (".log", ".csv") and f.stat().st_size > 0]


def main() -> int:
    ap = argparse.ArgumentParser(
        description="nbody の <name>_run.csv を nbody.csv 形式 (25 列) に集約変換")
    ap.add_argument("inputs", nargs="*", help="入力ファイル or ディレクトリ")
    ap.add_argument("--input", action="append", default=[],
                    help="入力ディレクトリ (複数指定可)")
    ap.add_argument("--output", default=None, help="出力 CSV パス (省略時は stdout)")
    ap.add_argument("--append", default=None, metavar="EXISTING_CSV",
                    help="既存 CSV (例: summary/nbody.csv) に追記 (header はスキップ)")
    args = ap.parse_args()

    paths = [Path(p) for p in args.inputs + args.input]
    if not paths:
        ap.error("入力ファイル or --input ディレクトリを指定")

    log_files = collect_logs(paths)
    if not log_files:
        print("WARNING: 入力に該当する *_run.csv / *.log が見つからない", file=sys.stderr)
        return 1

    all_rows = []
    for lf in log_files:
        rows = parse_log_file(lf)
        all_rows.extend(rows)
        print(f"  {lf}: {len(rows)} rows", file=sys.stderr)
    print(f"合計 {len(all_rows)} 行抽出", file=sys.stderr)

    if args.append:
        with open(args.append, "a", newline="", encoding="utf-8-sig") as f:
            csv.DictWriter(f, fieldnames=COLS).writerows(all_rows)
        print(f"追記: {args.append}", file=sys.stderr)
    else:
        out = open(args.output, "w", newline="", encoding="utf-8-sig") if args.output else sys.stdout
        w = csv.DictWriter(out, fieldnames=COLS)
        w.writeheader()
        w.writerows(all_rows)
        if args.output:
            out.close()
            print(f"出力: {args.output}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""
results/<machine>/*.log を nbody.csv 形式 (21 カラム) に集約変換する。

バイナリ自身は 10 カラム CSV を出力 (`# binary,N(=nx*ny*nz),time_sec,...`):
    # binary,N(=nx*ny*nz),time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time
    ./build-local-gpu/nbody.gpu.32,262144,6.685e-02,4.176e+01,1.214e-05,...

このスクリプトはそれを読み、バイナリパスから category/machine/mode/language/impl/variant/
memory_model/optimization_type/optimization_param/fp/nx/ny/nz の 11 カラムを推論して
21 カラム CSV に変換出力する。

使い方:
    # results/local/ 配下の全 .log/.csv を変換
    python3 parse_logs.py --input ../results/local --output ../results/local/summary.csv

    # 既存の nbody.csv に追記
    python3 parse_logs.py --input ../results/local --append nbody.csv

    # 個別ファイル指定
    python3 parse_logs.py file1.log file2.log --output combined.csv
"""
from __future__ import annotations
import argparse
import csv
import math
import os
import re
import sys
from pathlib import Path
from typing import Iterable

# nbody.csv の 21 カラムスキーマ
COLS = [
    "category", "machine", "mode", "language",
    "impl", "variant", "memory_model",
    "optimization_type", "optimization_param", "fp",
    "nx", "ny", "nz", "N",
    "time_sec", "performance_gflops", "error",
    "real_sec", "user_sec", "sys_sec",
    "source_file",
]

# バイナリパスから言語・impl・variant を抜き出す regex
# 例: .../C++/openmp-target/auto.def/build-local-gpu/nbody.gpu.32
PATH_RE = re.compile(
    r"/(?P<lang>C\+\+|F)/(?P<impl>[^/]+)(?:/(?P<variant>[^/]+(?:/[^/]+)?))?/build-(?P<machine>[^-]+)-(?P<mode>[^/]+)/(?P<binname>[^/]+)$"
)
# Kokkos は 4 階層 (kokkos/<policy>/<sub>) なので variant が "policy/sub" 形式
BIN_RE = re.compile(
    r"^(?P<target>\w+)\.(?P<mode>gpu|cpu|uni)\.(?P<fp>\d+)(?:\.(?P<nthr>\d+))?$"
)


def infer_meta(binary_path: str, log_file: Path) -> dict:
    """バイナリパス + log ファイル位置から 11 カラムのメタ情報を推論。"""
    meta = {c: "" for c in COLS}

    # log ファイル所在の親ディレクトリから machine を補足推論 (results/<machine>/)
    parts = log_file.resolve().parts
    machine_from_log = ""
    if "results" in parts:
        idx = parts.index("results")
        if idx + 1 < len(parts):
            machine_from_log = parts[idx + 1]

    # バイナリパスを正規化
    path_str = binary_path.replace("\\", "/")

    m = PATH_RE.search(path_str)
    if m:
        lang_dir = m.group("lang")
        impl = m.group("impl")
        variant_raw = m.group("variant") or ""
        machine = m.group("machine")
        mode_from_dir = m.group("mode")
        binname = m.group("binname")

        meta["language"] = "cpp" if lang_dir == "C++" else "f90"
        meta["impl"] = impl
        meta["variant"] = variant_raw
        meta["machine"] = machine
        meta["mode"] = mode_from_dir
        meta["category"] = "Kokkos" if impl == "kokkos" else "non-Kokkos"
    else:
        binname = os.path.basename(path_str)
        if machine_from_log:
            meta["machine"] = machine_from_log

    # バイナリ名から target/mode/fp/nthreads を取得
    mb = BIN_RE.match(os.path.basename(binname))
    if mb:
        meta["mode"] = meta["mode"] or mb.group("mode")
        meta["fp"] = mb.group("fp")
        if mb.group("nthr"):
            meta["optimization_param"] = f"NTHREADS={mb.group('nthr')}"
            meta["optimization_type"] = "nthreads"

    # variant 名から memory_model と optimization_type を推論
    v = meta.get("variant", "")
    if v.startswith("auto"):
        meta["memory_model"] = "managed"
    elif v.startswith("manu"):
        meta["memory_model"] = "separate"
    elif meta["mode"] == "uni":
        meta["memory_model"] = "unified"
    elif meta["impl"] == "openmp-cpu" or meta["mode"] == "cpu":
        meta["memory_model"] = "cpu-only"

    # Kokkos の memory model + optimization_type
    if meta["impl"] == "kokkos":
        if "uvm" in v:
            meta["memory_model"] = "kokkos-uvm"
        else:
            meta["memory_model"] = "kokkos-default"
        if "tile-sweep" in v:
            meta["optimization_type"] = "tile-sweep"
        elif "chunk-sweep" in v:
            meta["optimization_type"] = "chunk-sweep"
        elif "cpu-sweep" in v or "uvm-sweep" in v:
            meta["optimization_type"] = "opt-level-sweep"
        elif "fused" in v:
            meta["optimization_type"] = "fused"
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


def parse_log_file(log_file: Path) -> list[dict]:
    """1 つのログから 21 カラム形式の dict のリストを返す。"""
    rows = []
    with open(log_file) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = [x.strip() for x in line.split(",")]
            if len(fields) < 10:
                continue
            binary_path = fields[0]
            try:
                N = int(fields[1])
            except ValueError:
                continue

            meta = infer_meta(binary_path, log_file)
            # 立方体仮定で nx, ny, nz を計算
            n_side = round(N ** (1.0 / 3.0))
            if n_side ** 3 == N:
                meta["nx"] = meta["ny"] = meta["nz"] = n_side

            meta["N"] = N
            meta["time_sec"] = fields[2]
            meta["performance_gflops"] = fields[3]
            meta["error"] = fields[4]
            meta["real_sec"] = fields[5]
            meta["user_sec"] = fields[6]
            meta["sys_sec"] = fields[7]
            # fields[8] = num_time_steps_logged, [9] = last_sim_time は nbody.csv 21 カラムに含まれない
            meta["source_file"] = str(log_file)
            rows.append(meta)
    return rows


def collect_logs(paths: list[Path]) -> list[Path]:
    """ディレクトリ or ファイルのリストから全 *.log / *.csv を集める。"""
    out = []
    for p in paths:
        if p.is_dir():
            out.extend(sorted(p.rglob("*.log")))
            out.extend(sorted(p.rglob("*.csv")))
        elif p.is_file():
            out.append(p)
    # .gitkeep 等は除外
    return [f for f in out if f.suffix in (".log", ".csv") and f.stat().st_size > 0]


def main() -> int:
    ap = argparse.ArgumentParser(
        description="results/<machine>/*.log を nbody.csv 形式 (21 列) に集約変換")
    ap.add_argument("inputs", nargs="*", help="入力ファイル or ディレクトリ")
    ap.add_argument("--input", action="append", default=[],
                    help="入力ディレクトリ (複数指定可)")
    ap.add_argument("--output", default=None,
                    help="出力 CSV パス (省略時は stdout)")
    ap.add_argument("--append", default=None, metavar="EXISTING_CSV",
                    help="既存 CSV (例: summary/nbody.csv) に追記 (header はスキップ)")
    args = ap.parse_args()

    paths = [Path(p) for p in args.inputs + args.input]
    if not paths:
        ap.error("入力ファイル or --input ディレクトリを指定")

    log_files = collect_logs(paths)
    if not log_files:
        print(f"WARNING: 入力に該当する *.log/*.csv が見つからない", file=sys.stderr)
        return 1

    all_rows = []
    for lf in log_files:
        rows = parse_log_file(lf)
        all_rows.extend(rows)
        print(f"  {lf}: {len(rows)} rows", file=sys.stderr)

    print(f"合計 {len(all_rows)} 行抽出", file=sys.stderr)

    # 出力
    if args.append:
        # 既存 CSV に追記 (header をスキップ)
        with open(args.append, "a", newline="") as f:
            w = csv.DictWriter(f, fieldnames=COLS)
            w.writerows(all_rows)
        print(f"追記: {args.append}", file=sys.stderr)
    else:
        out = open(args.output, "w", newline="") if args.output else sys.stdout
        w = csv.DictWriter(out, fieldnames=COLS)
        w.writeheader()
        w.writerows(all_rows)
        if args.output:
            out.close()
            print(f"出力: {args.output}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    sys.exit(main())

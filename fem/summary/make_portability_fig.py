#!/usr/bin/env python3
"""
fem の性能可搬性 (performance portability) 図の生成器

  問題サイズ N を固定し、6 つの実装
    do concurrent / Kokkos / OpenACC / OpenMP (CPU) / OpenMP target / stdpar
  を Wisteria (NVIDIA A100) と Miyabi (NVIDIA H200) で並べて比較する。

  N は「全実装 × 両機種でデータが揃っている」ものを自動選択する
  (揃い方が同じなら大きい N)。--N で明示指定も可能。


  1 枚の図の中身:
    パネル = 精度 (FP32 / FP32-64 混合 / FP64)
    x      = 実装、棒 = 機種 (A100 / H200)、y = 性能 [GFLOPS] (log)
    棒の値は variant / メモリモデル / 言語 (C++ or Fortran) /
    スレッド数などを振った中の best 値。棒の上のラベルは
    GFLOPS 値と、その best を出した言語。

使い方 (このディレクトリで):
    python3 make_portability_fig.py            # N は自動選択
    python3 make_portability_fig.py --N 35937  # N を明示指定

    入力 : ./fem.csv
    出力 : ./figs/fem_portability.{png,pdf}
           ./figs/fem_portability_best.csv  (棒の元になった best 値)
"""
from __future__ import annotations

import argparse
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

APP = "fem"
APP_TITLE = "FEM (CG solver)"
HERE = os.path.dirname(os.path.abspath(__file__))

# N の自動選択に設ける上限 (None なら制限なし)。
#   fem は最大 N (2,146,689) でもまだ性能が伸びている領域なので制限しない。
N_LIMIT = None

# csv の machine 値 -> (凡例名, 色)
MACHINES = {
    "wisteria": ("Wisteria (A100)", "#4c78a8"),
    "miyabi":   ("Miyabi (H200)",   "#e45756"),
}

# 図に並べる実装と表示名 (この順に左から並ぶ)
IMPLS = [
    ("do-concurrent", "do concurrent\n(Fortran)"),
    ("kokkos",        "Kokkos\n(C++)"),
    ("openacc",       "OpenACC"),
    ("openmp-cpu",    "OpenMP\n(CPU)"),
    ("openmp-target", "OpenMP\ntarget"),
    ("stdpar",        "stdpar\n(C++)"),
]

LANG_TAG = {"cpp": "C++", "f90": "F"}
FP_ORDER = ["FP32", "FP32/64 mixed", "FP64"]


def fp_label(raw) -> str:
    """'32_64 (FP_L=32, FP_M=64)' のような値も FP32 / FP64 / mixed に正規化する。"""
    head = str(raw).split()[0]
    return {"32": "FP32", "64": "FP64", "32_64": "FP32/64 mixed"}.get(head, f"FP{head}")


def load() -> pd.DataFrame:
    """variant / メモリモデル / 言語 / チューニングパラメータの best を返す。"""
    csv = os.path.join(HERE, f"{APP}.csv")
    df = pd.read_csv(csv, encoding="utf-8-sig")
    df["N"] = pd.to_numeric(df["N"], errors="coerce")
    df["performance_gflops"] = pd.to_numeric(df["performance_gflops"], errors="coerce")
    df = df[df["N"].notna() & df["performance_gflops"].notna()]
    df = df[df["machine"].isin(MACHINES) & df["impl"].isin(dict(IMPLS))]
    df["fp_label"] = df["fp"].map(fp_label)
    idx = df.groupby(["machine", "impl", "fp_label", "N"])["performance_gflops"].idxmax()
    best = df.loc[idx, ["machine", "impl", "language", "fp_label", "N",
                        "performance_gflops", "variant", "memory_model",
                        "optimization_param"]]
    return best.rename(columns={"performance_gflops": "gflops"}).reset_index(drop=True)


def pick_N(best: pd.DataFrame) -> int:
    """全実装 × 両機種が揃う N を選ぶ (揃い方が同じなら大きい N)。"""
    need = len(IMPLS) * len(MACHINES)
    cand = best[best["N"] < N_LIMIT] if N_LIMIT else best
    if N_LIMIT:
        print(f"  N の上限 {N_LIMIT:,} を適用 (性能飽和を避けるため)")
    scores = []
    for N, g in cand.groupby("N"):
        # 精度パネルごとの充足数を合計したものをカバレッジ指標にする
        cov = sum(len(set(zip(p["impl"], p["machine"])))
                  for _, p in g.groupby("fp_label"))
        npanel = g["fp_label"].nunique()
        scores.append((cov, int(N), npanel, need * npanel))
    scores.sort(reverse=True)
    cov, N, npanel, full = scores[0]
    tag = "全実装×両機種そろい" if cov == full else f"充足 {cov}/{full}"
    print(f"  選択した N = {N:,}  ({tag}, 精度パネル {npanel} 枚)")
    return N


def draw(best: pd.DataFrame, N: int, outdir: str) -> list[str]:
    sub = best[best["N"] == N]
    fps = [f for f in FP_ORDER if f in set(sub["fp_label"])]
    fig, axes = plt.subplots(1, len(fps), figsize=(6.0 * len(fps), 5.6),
                             sharey=True, squeeze=False)
    axes = axes[0]

    x = np.arange(len(IMPLS))
    width = 0.38
    for ax, fp in zip(axes, fps):
        p = sub[sub["fp_label"] == fp]
        for k, (machine, (mlabel, color)) in enumerate(MACHINES.items()):
            vals, tags = [], []
            for impl, _ in IMPLS:
                r = p[(p["impl"] == impl) & (p["machine"] == machine)]
                vals.append(float(r["gflops"].iloc[0]) if len(r) else np.nan)
                tags.append(LANG_TAG.get(r["language"].iloc[0], "") if len(r) else "n/a")
            pos = x + (k - 0.5) * width
            ax.bar(pos, vals, width, color=color, label=mlabel,
                   edgecolor="white", linewidth=0.6, zorder=3)
            for xi, v, t in zip(pos, vals, tags):
                if np.isnan(v):
                    # log 軸では y=0 を指定できないので軸座標で下端に置く
                    ax.annotate("no data", (xi, 0.0),
                                xycoords=("data", "axes fraction"),
                                xytext=(0, 12), textcoords="offset points",
                                rotation=90, fontsize=7.5, ha="center",
                                va="bottom", color="0.55")
                else:
                    lab = f"{v:,.0f}" if v >= 10 else f"{v:.2f}"
                    ax.annotate(f"{lab} ({t})", (xi, v), xytext=(0, 3),
                                textcoords="offset points", rotation=90, fontsize=7.5,
                                ha="center", va="bottom", color="0.25")
        ax.set_yscale("log")
        ax.set_xticks(x, [lbl for _, lbl in IMPLS], fontsize=8.5)
        ax.grid(True, axis="y", which="both", ls=":", alpha=0.45, zorder=0)
        ax.set_axisbelow(True)
        ax.set_title(fp, fontsize=11)
        ax.margins(y=0.45)
    axes[0].set_ylabel("performance [GFLOPS]  (log scale)")

    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, 0.89),
               ncol=2, frameon=False, fontsize=10)
    fig.suptitle(f"{APP_TITLE} — implementation comparison at fixed "
                 f"N = {N:,}\nWisteria (A100) vs Miyabi (H200), "
                 "best over variants / memory models / languages", fontsize=12)
    fig.subplots_adjust(top=0.78)

    written = []
    for ext in ("png", "pdf"):
        path = os.path.join(outdir, f"{APP}_portability.{ext}")
        fig.savefig(path, dpi=150, bbox_inches="tight")
        written.append(path)
    plt.close(fig)

    csv_out = os.path.join(outdir, f"{APP}_portability_best.csv")
    sub.sort_values(["fp_label", "impl", "machine"]).to_csv(csv_out, index=False)
    written.append(csv_out)
    return written


def main() -> None:
    ap = argparse.ArgumentParser(description=f"{APP} の性能可搬性図を作る")
    ap.add_argument("--N", type=int, default=None, help="固定する問題サイズ N")
    args = ap.parse_args()

    print(f"[{APP}]")
    outdir = os.path.join(HERE, "figs")
    os.makedirs(outdir, exist_ok=True)
    best = load()
    N = args.N if args.N is not None else pick_N(best)
    for p in draw(best, N, outdir):
        print("  ->", os.path.relpath(p, HERE))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
diffusion の性能可搬性 (performance portability) 図の生成器

  問題サイズ N を固定し、6 つの実装
    do concurrent / Kokkos / OpenACC / OpenMP (CPU) / OpenMP target / stdpar
  を Wisteria (NVIDIA A100) と Miyabi (NVIDIA GH200) で並べて比較する。

  N は「全実装 × 両機種でデータが揃っている」ものを自動選択する
  (揃い方が同じなら大きい N)。--N で明示指定も可能。

  diffusion は N=1.34e8 まで行くと do concurrent / stdpar がピークを過ぎて
  性能が落ちるため、N < 1e8 に限定して選ぶ。

  1 枚の図の中身:
    パネル = 精度 (FP32 / FP32-64 混合 / FP64)
    x      = 実装、棒 = 機種 (A100 / GH200)、y = 性能 [GFLOPS] (log)
    棒の値は variant / メモリモデル / 言語 (C++ or Fortran) /
    スレッド数などを振った中の best 値。棒の上のラベルは
    GFLOPS 値と、その best を出した言語。

使い方 (このディレクトリで):
    python3 make_portability_fig.py            # N は自動選択、C++ に統一
    python3 make_portability_fig.py --lang all # 言語を問わず best (旧挙動)
    python3 make_portability_fig.py --N 2097152  # N を明示指定

    入力 : ./diffusion.csv
    出力 : ./figs/diffusion_portability.{png,pdf}
           ./figs/diffusion_portability_best.csv  (棒の元になった best 値)
"""
from __future__ import annotations

import argparse
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
import numpy as np
import pandas as pd

APP = "diffusion"
APP_TITLE = "Diffusion (3D stencil)"
HERE = os.path.dirname(os.path.abspath(__file__))

# N の自動選択に設ける上限 (None なら制限なし)。
#   N=1.34e8 ではメモリ逼迫で do concurrent / stdpar の性能が低下するため上限を置く。
N_LIMIT = 100_000_000

# csv の machine 値 -> (凡例名, 色)
MACHINES = {
    "wisteria": ("Wisteria (A100)", "#4c78a8"),
    "miyabi":   ("Miyabi (GH200)",   "#e45756"),
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

# CPU パーティションでの実行 (OpenMP CPU など) は GPU と土俵が違うので、
# 機種の色を保ったまま淡色 + ハッチで塗り分ける。
CPU_FACE = {"wisteria": "#b6cce0", "miyabi": "#f6bcbb"}
CPU_HATCH = "//"

LANG_TAG = {"cpp": "C++", "f90": "F"}
FP_ORDER = ["FP32", "FP32/64 mixed", "FP64"]


def fp_label(raw) -> str:
    """'32_64 (FP_L=32, FP_M=64)' のような値も FP32 / FP64 / mixed に正規化する。"""
    head = str(raw).split()[0]
    return {"32": "FP32", "64": "FP64", "32_64": "FP32/64 mixed"}.get(head, f"FP{head}")


def load(lang: str = "cpp") -> tuple[pd.DataFrame, list[str]]:
    """variant / メモリモデル / チューニングパラメータの best を返す。

    lang に "cpp" / "f90" を渡すとその言語の実装だけで best を取る。
    言語をそろえないと、同じ棒の左右 (Wisteria / Miyabi) で別の言語が選ばれて
    比較にならないことがある (例: diffusion FP32 の OpenMP CPU は
    Wisteria が Fortran、Miyabi が C++ で best になる)。

    指定言語の実装が存在しない impl (do concurrent は Fortran のみ、
    Kokkos / stdpar は C++ のみ) は、元の言語のまま残す。
    戻り値の 2 つめはそのフォールバックした impl の一覧。
    """
    csv = os.path.join(HERE, f"{APP}.csv")
    df = pd.read_csv(csv, encoding="utf-8-sig")
    df["N"] = pd.to_numeric(df["N"], errors="coerce")
    df["performance_gflops"] = pd.to_numeric(df["performance_gflops"], errors="coerce")
    df = df[df["N"].notna() & df["performance_gflops"].notna()]
    df = df[df["machine"].isin(MACHINES) & df["impl"].isin(dict(IMPLS))]
    df["fp_label"] = df["fp"].map(fp_label)

    fallback: list[str] = []
    if lang != "all":
        have = df.groupby("impl")["language"].agg(set).to_dict()
        fallback = [i for i, _ in IMPLS if i in have and lang not in have[i]]
        selectable = df["impl"].map(lambda i: lang in have.get(i, set()))
        df = df[(~selectable) | (df["language"] == lang)]

    idx = df.groupby(["machine", "impl", "fp_label", "N"])["performance_gflops"].idxmax()
    best = df.loc[idx, ["machine", "impl", "language", "mode", "fp_label", "N",
                        "performance_gflops", "variant", "memory_model",
                        "optimization_param"]]
    return (best.rename(columns={"performance_gflops": "gflops"}).reset_index(drop=True),
            fallback)


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


def lang_policy(lang: str, fallback: list[str] | None) -> str:
    """図のサブタイトルに出す「何の中の best か」の説明文を組み立てる。"""
    if lang == "all":
        return "best over variants / memory models / languages"
    name = {"cpp": "C++", "f90": "Fortran"}[lang]
    text = f"best over variants / memory models — {name} only"
    labels = [lbl.replace("\n", " ") for impl, lbl in IMPLS if impl in (fallback or [])]
    if labels:
        text += f" ({', '.join(labels)}: no {name} implementation)"
    return text


def draw(best: pd.DataFrame, N: int, outdir: str,
         lang: str = "cpp", fallback: list[str] | None = None) -> list[str]:
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
            vals, tags, modes = [], [], []
            for impl, _ in IMPLS:
                r = p[(p["impl"] == impl) & (p["machine"] == machine)]
                vals.append(float(r["gflops"].iloc[0]) if len(r) else np.nan)
                tags.append(LANG_TAG.get(r["language"].iloc[0], "") if len(r) else "n/a")
                modes.append(r["mode"].iloc[0] if len(r) else "")
            pos = x + (k - 0.5) * width
            # CPU 実行の棒だけ淡色 + ハッチ + 機種色の枠にする
            faces = [CPU_FACE[machine] if m == "cpu" else color for m in modes]
            edges = [color if m == "cpu" else "white" for m in modes]
            bars = ax.bar(pos, vals, width, color=faces, label=mlabel,
                          edgecolor=edges, linewidth=0.6, zorder=3)
            for b, m in zip(bars, modes):
                if m == "cpu":
                    b.set_hatch(CPU_HATCH)
                    b.set_linewidth(1.0)
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
    # CPU 実行が図中にあるときだけ、その凡例も出す
    if (sub["mode"] == "cpu").any():
        for machine, (mlabel, color) in MACHINES.items():
            handles.append(Patch(facecolor=CPU_FACE[machine], edgecolor=color,
                                 hatch=CPU_HATCH, linewidth=1.0))
            labels.append(f"{mlabel} — CPU partition")
    # CPU 実行の凡例を足すと 2 行になるので、その分パネルを下げる
    ncol = 2 if len(handles) > 2 else 2
    fig.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, 0.905),
               ncol=ncol, frameon=False, fontsize=9.5)
    fig.suptitle(f"{APP_TITLE} — implementation comparison at fixed "
                 f"N = {N:,}\nWisteria (A100) vs Miyabi (GH200), "
                 f"{lang_policy(lang, fallback)}", fontsize=12)
    fig.subplots_adjust(top=0.72 if len(handles) > 2 else 0.78)

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
    ap.add_argument("--lang", choices=["cpp", "f90", "all"], default="cpp",
                    help="best を取る言語 (既定 cpp)。all は言語を問わない旧挙動")
    args = ap.parse_args()

    print(f"[{APP}]")
    outdir = os.path.join(HERE, "figs")
    os.makedirs(outdir, exist_ok=True)
    best, fallback = load(args.lang)
    if args.lang == "all":
        print("  言語: 指定なし (C++ / Fortran の best)")
    else:
        name = {"cpp": "C++", "f90": "Fortran"}[args.lang]
        print(f"  言語: {name} に統一")
        for impl in fallback:
            only = "Fortran" if args.lang == "cpp" else "C++"
            print(f"    - {impl} は {name} 実装が無いため {only} のまま")
    N = args.N if args.N is not None else pick_N(best)
    for p in draw(best, N, outdir, args.lang, fallback):
        print("  ->", os.path.relpath(p, HERE))


if __name__ == "__main__":
    main()

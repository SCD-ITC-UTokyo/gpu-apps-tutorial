#!/bin/bash
# group_list: 'kudoyk' (--group で明示指定)
#PJM -N "diffusion"
#PJM -L rscgrp=debug-a
#PJM -L node=1
#PJM --omp thread=72
#PJM -L elapse=00:10:00
#PJM -g kudoyk
#PJM -j

################################################################
# 環境設定: コンピュートノードは module 未ロード状態で起動します。
# ビルド時と同じ環境を再現してください (以下はこの machine の推奨例)。
# 必要に応じて編集 / コメントアウト解除して使ってください。
#
#   module purge
#   module load nvidia
################################################################

cd ${PJM_O_WORKDIR}
export OMP_PROC_BIND=spread
export OMP_PLACES=threads


# === 実行 (N = nx*ny*nz の N; 64 → 64x64x64 立方体) ===
N=64
./build-wisteria-gpu/diffusion.gpu.32 $N

#!/bin/bash
# group_list: 'gr52' (--group で明示指定、自動検出 'kudoyk' とは異なる)
#PBS -N "diffusion"
#PBS -q debug-g
#PBS -l select=1:ompthreads=72
#PBS -l walltime=00:10:00
#PBS -W group_list=gr52
#PBS -j oe

################################################################
# 環境設定: コンピュートノードは module 未ロード状態で起動します。
# ビルド時と同じ環境を再現してください (以下はこの machine の推奨例)。
# 必要に応じて編集 / コメントアウト解除して使ってください。
#
#   module purge
#   module load nvidia/25.9
################################################################

cd ${PBS_O_WORKDIR}
export OMP_PROC_BIND=spread
export OMP_PLACES=threads


# === 実行 (N = nx*ny*nz の N; 64 → 64x64x64 立方体) ===
N=64
./build-miyabi-uni/diffusion.uni.32 $N

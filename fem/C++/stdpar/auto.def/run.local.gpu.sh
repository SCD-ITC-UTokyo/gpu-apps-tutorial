#!/bin/bash
# ローカル実行 (バッチスケジューラ無し)

# === 実行 (N = nx*ny*nz の N; 64 → 64x64x64 立方体) ===
N=64
./build-local-gpu/fem.gpu.32 $N

# Kokkos mdrange — `MDRangePolicy`

N 次元ループを **直接 N 次元 range として表現**するパターン。`MDRangePolicy<Rank<3>>` で 3D
ループ全体を Kokkos に渡すと、Kokkos が tile size を決めて並列化する。tile size を明示すれば
チューニング可能。

## variant

| variant | メモリ | tile macros | 特徴 |
|---|---|---|---|
| `baseline` | default | なし (Kokkos 任せ) | デフォルト tile (Kokkos が決定) |
| `cpu-tile-sweep` | default | `_NX, _NY, _NZ` | **明示的に tile size を指定** (CMake `-D_NX=N` で上書き) |
| `uvm` | `CudaUVMSpace` | なし | UVM の baseline |
| `uvm-tile-sweep` | CudaUVMSpace | `_NX, _NY, _NZ` | UVM + tile macros |

## コード例 (`baseline/src/diffusion.cpp`)

```cpp
using mdrange_policy = Kokkos::MDRangePolicy<Kokkos::Rank<3>>;
Kokkos::parallel_for("diffusion3d", mdrange_policy({0,0,0}, {nz,ny,nx}),
  KOKKOS_LAMBDA(const int k, const int j, const int i) { 
      const int ix = nx * ny * (k + mgn) + nx * j + i;
      fn[ix] = cc*f[ix] + ...;
  });
```

3 つの index `(k, j, i)` を lambda が直接受け取る。`range` 版より自然。

`cpu-tile-sweep/src/diffusion.cpp` は tile size を渡す:
```cpp
Kokkos::parallel_for("diffusion3d",
  mdrange_policy({0,0,0}, {nz,ny,nx}, {_NX,_NY,_NZ}),  // ← tile size を 3 番目に
  KOKKOS_LAMBDA(const int k, const int j, const int i) { ... });
```

`_NX, _NY, _NZ` はコンパイル時マクロ。CMake から `cmake -B build -S . -D_NX=4 -D_NY=8 -D_NZ=64` で上書き。

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant C++/kokkos/mdrange/cpu-tile-sweep --machine local --mode gpu

cd C++/kokkos/mdrange/cpu-tile-sweep
# デフォルト tile size (_NX=2 _NY=4 _NZ=16)
cmake -B build-default -S .
cmake --build build-default
./build-default/diffusion.gpu.32 64

# tile size を変更
cmake -B build-tuned -S . -D_NX=4 -D_NY=8 -D_NZ=64
cmake --build build-tuned
./build-tuned/diffusion.gpu.32 64
```

元プロジェクトの `sh/<machine>/tilesize_compiles_*.sh` には 19 種類の tile combination が
記載されているので、組織的な sweep の参考に。

## 学習のポイント

- `range` よりも **N 次元構造を自然に書ける** (lambda が複数 index を取る)
- tile size の選択が **性能に大きく影響** (CPU では cache、GPU では occupancy)
- baseline (tile 自動) と tile-sweep (tile 明示) を比較して、Kokkos の自動最適化と
  手動チューニングのトレードオフを観察
- UVM 版は CUDA backend 必須 (`--mode cpu` でエラー)

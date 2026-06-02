# Kokkos team — `TeamPolicy + TeamThreadRange`

CUDA の block + thread 階層に似た **hierarchical parallelism** パターン。
外側を `TeamPolicy` (block 相当)、内側を `TeamThreadRange` (thread 相当) で表現する。

## variant

| variant | メモリ | chunk macro | 特徴 |
|---|---|---|---|
| `baseline` | default | `chunk=32` ハードコード | TeamPolicy の出発点 |
| `cpu-chunk-sweep` | default | `_CHUNK` macro | chunk size を CMake から上書き可 |
| `uvm` | `CudaUVMSpace` | `chunk=32` | UVM の baseline |
| `uvm-chunk-sweep` | CudaUVMSpace | `_CHUNK` | UVM + chunk sweep |

## コード例 (`baseline/src/diffusion.cpp`)

```cpp
const int chunk = 32;     // baseline は固定、cpu-chunk-sweep は _CHUNK

Kokkos::parallel_for("diffusion3d",
  Kokkos::TeamPolicy<>((nz*ny + chunk - 1) / chunk, Kokkos::AUTO()),
  KOKKOS_LAMBDA(const Kokkos::TeamPolicy<>::member_type& team) {
      const int block_id = team.league_rank();      // CUDA の blockIdx 相当
      Kokkos::parallel_for(
        Kokkos::TeamThreadRange(team, chunk),
        [&](const int local_id) {
            const int kj = block_id * chunk + local_id;
            if (kj >= nz*ny) return;
            const int k = kj / ny;
            const int j = kj % ny;
            for (int i = 0; i < nx; i++) {
                // ...stencil 計算...
            }
        });
  });
```

`league_rank()` は block ID、`TeamThreadRange` 内で並列スレッド展開。
`Kokkos::AUTO()` は team size を Kokkos に決めさせる。

`cpu-chunk-sweep` では `const int chunk = _CHUNK;` でマクロ参照。
CMake から `-D_CHUNK=128` 等で上書き。

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant C++/kokkos/team/cpu-chunk-sweep --machine local --mode gpu

cd C++/kokkos/team/cpu-chunk-sweep
# デフォルト (_CHUNK=32)
cmake -B build -S .
cmake --build build
./build/diffusion.gpu.32 64

# chunk 変更
cmake -B build-128 -S . -D_CHUNK=128
cmake --build build-128
./build-128/diffusion.gpu.32 64
```

元プロジェクトの `sh/<machine>/chunksize_compiles_*.sh` には chunk 1〜512 の sweep が
記載されている。

## 学習のポイント

- **block-thread 階層**を明示的にコードに書く (CUDA に慣れた人には自然)
- `chunk` サイズが **GPU 上での block size 相当**。性能に直結
- `range` や `mdrange` より低レベル、より細かい制御が可能
- 同じソースが Kokkos の backend 切替で CPU / GPU / AMD GPU でも動く portability
- chunk size 32 〜 256 程度が典型的な GPU の sweet spot (要 benchmark)

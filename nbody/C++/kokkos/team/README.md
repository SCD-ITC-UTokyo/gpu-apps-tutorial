# Kokkos team (nbody) — `TeamPolicy + TeamThreadRange`

CUDA の block + thread 階層に似た **hierarchical parallelism** パターン。
外側を `TeamPolicy` (league = block 群)、内側を `TeamThreadRange` (team 内 thread) で表現する。
nbody では i 粒子を chunk 単位の team に分割し、各スレッドが 1 i 粒子の O(N²) 計算を担当する。

## コード例 (`baseline/src/base/nbody.cpp`)

```cpp
using team_policy = Kokkos::TeamPolicy<>;
using member_type = team_policy::member_type;
const int chunk       = NCHUNK;                   // #define NCHUNK (128) — ハードコード
const int league_size = (Ni + chunk - 1) / chunk;
team_policy policy(league_size, Kokkos::AUTO());  // team size は Kokkos 任せ

Kokkos::parallel_for("calc_acc", policy,
  KOKKOS_LAMBDA(const member_type& team) {
    const int begin = team.league_rank() * chunk;          // CUDA の blockIdx 相当
    const int end   = (begin + chunk < Ni) ? (begin + chunk) : Ni;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end),
      [&] (const int i) {
        const auto pi = ipos(i);
        decltype(iacc(0)) ai = { ... };
        for (auto j = 0; j < Nj; j++) {                    // O(N²) の内側は手書き for
            const auto pj = jpos(j);
            /* ...particle-particle interaction... */
        }
        iacc(i) = ai;
      });
  });
Kokkos::fence();
```

`league_rank()` が block ID、`TeamThreadRange` が team 内のスレッド分担。`calc_acc` / `kick` /
`drift` などが同じ team パターンで書かれています。

## chunk マクロ (※ diffusion とは扱いが違う)

chunk size はソース先頭で **ハードコード**されています:

```cpp
#define NCHUNK (128)
```

> **注意:** chunk は `_CHUNK` マクロではなく `#define NCHUNK (128)` で固定です。
> そのため `configure.py --chunk N` (= `-D_CHUNK=N`) は **現状このアプリでは効きません**。
> chunk を変えるには `base/nbody.cpp` の `#define NCHUNK` を編集してください。

## variant

| variant | メモリ | 特徴 |
|---|---|---|
| `baseline` | default | `NCHUNK=128` ハードコード |
| `cpu-chunk-sweep` | default | ※ **baseline と同一ソース** (NCHUNK 固定。`_CHUNK` 上書きは無効) |
| `uvm` | `CudaUVMSpace` | UVM の baseline (差分は `KV*1D` 型定義) |
| `uvm-chunk-sweep` | CudaUVMSpace | uvm と同一ソース |

## ビルド & 実行

```bash
cd apps/nbody
./configure.py --variant C++/kokkos/team/baseline --machine local --mode gpu
cd C++/kokkos/team/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# chunk を変えるには base/nbody.cpp の #define NCHUNK を編集して再ビルド
```

## 学習のポイント

- **league / team / thread の階層**を明示的に書く (CUDA の block/thread に対応)
- `league_rank()` が block ID、`TeamThreadRange` が team 内のスレッド分担。各スレッドが 1 i 粒子を担当
- chunk サイズ (`NCHUNK`) が team あたりの担当粒子数。GPU 性能に直結 (要 benchmark)
- `range` / `mdrange` より低レベルで細かい制御が可能
- 同じソースが Kokkos backend 切替で CPU / GPU いずれでも動く portability

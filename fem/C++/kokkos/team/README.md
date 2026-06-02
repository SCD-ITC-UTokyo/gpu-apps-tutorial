# Kokkos team (fem) — `TeamPolicy + TeamThreadRange`

CUDA の block + thread 階層に似た **hierarchical parallelism** パターン。
外側を `TeamPolicy` (league = block 群)、内側を `TeamThreadRange` (team 内 thread) で表現する。
fem では CG ソルバ・行列組み立ての各ループを、chunk 単位の team に分割して並列化する。

## コード例 (`baseline/src/solver_CG.cpp`)

各 team が `chunk` 個の節点を担当し、team 内スレッドで `[begin, end)` を分担:

```cpp
// kokkos_settings.h:  using team_policy = Kokkos::TeamPolicy<>;
//                     using member_type = team_policy::member_type;
const int chunk       = CHUNK_SIZE;                 // ソルバの chunk (既定 512)
const int league_size = (N + chunk - 1) / chunk;
team_policy policy_t(league_size, Kokkos::AUTO());  // team size は Kokkos 任せ

Kokkos::parallel_reduce("RHO", policy_t,
  KOKKOS_LAMBDA(const member_type& team, KREAL &sum_outer) {
    const int begin = team.league_rank() * chunk;   // CUDA の blockIdx 相当
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    KREAL sum_local = 0.0;
    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(team, begin, end),
      [&] (const int i, KREAL &sum_inner) {
        sum_inner += WW(R, i) * WW(Z, i);
      }, sum_local);
    sum_outer += sum_local;
  }, RHO);
```

`parallel_for` (例: SpMV `{q}=[A]{p}`) も同型で、内側 `TeamThreadRange` の lambda 内に
CSR 行の手書き for を残します。

## chunk マクロ (※ diffusion とは名前が違う)

`src/kokkos_settings.h` で 2 種類の chunk size を定義 (`#ifdef` で上書き可):

```cpp
#ifdef _CHUNK_SIZE
  static int CHUNK_SIZE = _CHUNK_SIZE;
#else
  static int CHUNK_SIZE = 512;          // CG ソルバの chunk
#endif
#ifdef _CHUNK_SIZE_ASS
  static int CHUNK_SIZE_ASS = _CHUNK_SIZE_ASS;
#else
  static int CHUNK_SIZE_ASS = 32;       // 行列組み立ての chunk
#endif
```

> **注意:** chunk マクロは `_CHUNK_SIZE` / `_CHUNK_SIZE_ASS` です (diffusion の `_CHUNK` ではない)。
> そのため `configure.py --chunk N` (= `-D_CHUNK=N`) は **現状このアプリでは効きません**。
> chunk を変えるには CMake で直接 `-D_CHUNK_SIZE=128 -D_CHUNK_SIZE_ASS=64` を渡すか、
> `kokkos_settings.h` の既定値を編集してください。

## variant

| variant | メモリ | 特徴 |
|---|---|---|
| `baseline` | default | `CHUNK_SIZE=512` / `CHUNK_SIZE_ASS=32` (マクロ未指定時の既定) |
| `cpu-chunk-sweep` | default | ※ **baseline と同一ソース**。chunk は `_CHUNK_SIZE`(_ASS) で上書き |
| `uvm` | `CudaUVMSpace` | UVM の baseline (差分は `kokkos_settings.h` の View 定義) |
| `uvm-chunk-sweep` | CudaUVMSpace | uvm と同一ソース |

## ビルド & 実行

```bash
cd apps/fem
./configure.py --variant C++/kokkos/team/baseline --machine local --mode gpu
cd C++/kokkos/team/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# chunk を変える例 (configure.py --chunk ではなく CMake で直接):
cmake -B build-256 -S . -D_CHUNK_SIZE=256 -D_CHUNK_SIZE_ASS=64 && cmake --build build-256
```

## 学習のポイント

- **league / team / thread の階層**を明示的に書く (CUDA の block/thread に対応)
- `league_rank()` が block ID、`TeamThreadRange` が team 内のスレッド分担
- chunk サイズが team あたりの担当要素数。GPU 性能に直結 (要 benchmark)
- `range` / `mdrange` より低レベルで細かい制御が可能
- 同じソースが Kokkos backend 切替で CPU / GPU いずれでも動く portability

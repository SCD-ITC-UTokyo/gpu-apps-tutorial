# C++/kokkos — Kokkos (C++ performance portability)

Kokkos C++ ライブラリで GPU/CPU 横断ポータブルに並列化する fem (3D Poisson FEM + CG) 実装。
pragma の代わりに **`Kokkos::parallel_for` / `parallel_reduce` + `KOKKOS_LAMBDA`** を使い、
**View** という抽象データコンテナでメモリ空間を管理する。

## 構造 (3 ディレクトリ)

```
kokkos/
├── README.md                                          # ← このファイル
├── range/         # parallel_for (RangePolicy) — 整数 extent を直接渡す最短形
│   ├── baseline/, cpu-sweep/, uvm/, uvm-sweep/
│   └── README.md
├── mdrange/       # ※ 実体は RangePolicy<> を明示構築 (MDRangePolicy ではない。下記注意)
│   ├── baseline/, cpu-tile-sweep/, uvm/, uvm-tile-sweep/
│   └── README.md
└── team/          # TeamPolicy + TeamThreadRange — hierarchical parallelism
    ├── baseline/, cpu-chunk-sweep/, uvm/, uvm-chunk-sweep/
    └── README.md
```

> **このアプリ固有の注意 (diffusion との違い)**
> fem の `mdrange/` と `team/` は diffusion の同名ディレクトリと実装が異なります。
> - **`mdrange/` は `MDRangePolicy` を使っていません。** 実体は `using policy_1d = Kokkos::RangePolicy<>;`
>   で、`range/` が整数 extent を暗黙変換するのに対し、`policy_1d(0, N)` と **RangePolicy を明示構築**して
>   いるだけです (節点配列が 1 次元のため N 次元 range の出番が無い)。tile マクロ `_NX/_NY/_NZ` は
>   ソースから参照されておらず、`cpu-tile-sweep` も baseline と同等です。
> - **`team/` の chunk マクロは `_CHUNK` ではありません。** ソルバは `_CHUNK_SIZE` (既定 512)、
>   行列組み立ては `_CHUNK_SIZE_ASS` (既定 32) を読みます。そのため `configure.py --chunk N`
>   (= `-D_CHUNK=N`) は **現状このアプリでは効きません**。`team/baseline` と `team/cpu-chunk-sweep`
>   はソースが同一です。

## 軸の意味

| 軸 | 値 | 意味 (fem での実態) |
|---|---|---|
| **policy** | `range` | `parallel_for("...", N, ...)` — 整数 extent を直接渡す (RangePolicy に暗黙変換) |
| | `mdrange` | `parallel_for("...", policy_1d(0,N), ...)` — RangePolicy を明示構築 (※MDRange 不使用) |
| | `team` | `TeamPolicy + TeamThreadRange` — league/team/thread の階層並列 |
| **メモリ** | (default) | `Kokkos::View<T*>` — Kokkos が backend に合わせて自動配置 |
| | `uvm` | `Kokkos::View<T*, KOKKOS_DEV>` (`KOKKOS_DEV = Device<Cuda, CudaUVMSpace>`) |
| **sweep** | `cpu-sweep` | range の compile flag sweep 用 (`configure.py --opt-level` / `--extra-cflags`) |
| | `cpu-tile-sweep` | ※ tile マクロ未配線 (baseline と同等) |
| | `cpu-chunk-sweep` | ※ baseline と同一ソース (chunk は `_CHUNK_SIZE` を直接編集) |

## 依存

- **Kokkos 4.x / 5.x** がインストール済 (`KokkosConfig.cmake` が見つかること)
- machines/<machine>.yaml の `kokkos.root`、または `configure.py --kokkos-root` / 環境変数 `Kokkos_ROOT` で
  install prefix を指定

## ビルド & 実行

Kokkos は **CMake 必須** (Make 非対応)。

```bash
cd fem
./configure.py --variant C++/kokkos/range/baseline --machine local --mode gpu
cd C++/kokkos/range/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# バイナリ: build-local-gpu/fem.gpu.32  (実行は run.<machine>.<mode>.sh 参照)
```

**UVM variant (`*/uvm`, `*/uvm-*`) は GPU mode 必須** (`--mode cpu` はエラー終了)。

## 学習順序のおすすめ

1. `range/baseline` (Kokkos の "Hello World"、RangePolicy)
2. `range/uvm` で UVM のオーバーヘッドを観察
3. `mdrange/baseline` で **RangePolicy を明示構築する書き方** (range の暗黙形との対比)
4. `team/baseline` で hierarchical parallelism (CUDA 風の league/team/thread)

詳細は各 policy の README (`range/README.md`, `mdrange/README.md`, `team/README.md`) を参照。

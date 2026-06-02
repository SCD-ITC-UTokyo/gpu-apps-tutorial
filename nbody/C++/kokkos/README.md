# C++/kokkos — Kokkos (C++ performance portability)

Kokkos C++ ライブラリで GPU/CPU 横断ポータブルに並列化する nbody (直接法 N 体重力計算) 実装。
pragma の代わりに **`Kokkos::parallel_for` + `KOKKOS_LAMBDA`** を使い、**View** という抽象データ
コンテナでメモリ空間を管理する。

## 構造 (3 ディレクトリ)

```
kokkos/
├── README.md                                          # ← このファイル
├── range/         # parallel_for (RangePolicy) — 整数 extent を直接渡す最短形
│   ├── baseline/, cpu-sweep/, uvm/, uvm-sweep/
│   └── README.md
├── mdrange/       # ※ 実体は RangePolicy を明示構築 (MDRangePolicy ではない。下記注意)
│   ├── baseline/, cpu-tile-sweep/, uvm/, uvm-tile-sweep/
│   └── README.md
└── team/          # TeamPolicy + TeamThreadRange — hierarchical parallelism
    ├── baseline/, cpu-chunk-sweep/, uvm/, uvm-chunk-sweep/
    └── README.md
```

> **このアプリ固有の注意 (diffusion との違い)**
> nbody の `mdrange/` と `team/` は diffusion の同名ディレクトリと実装が異なります。
> - **`mdrange/` は `MDRangePolicy` を使っていません。** 実体は `Kokkos::parallel_for("calc_acc",
>   Kokkos::RangePolicy(0, Ni), ...)` で、`range/` が整数 extent を暗黙変換するのに対し、
>   **RangePolicy を明示構築**しているだけです (粒子配列が 1 次元のため N 次元 range の出番が無い)。
>   tile マクロ `_NX/_NY/_NZ` はソースから参照されておらず、`cpu-tile-sweep` も baseline と同等です。
> - **`team/` の chunk は `#define NCHUNK (128)` でハードコード**されています。`_CHUNK` マクロは
>   読まないため `configure.py --chunk N` (= `-D_CHUNK=N`) は **現状効きません**。
>   `team/baseline` と `team/cpu-chunk-sweep` はソースが同一です。

## 軸の意味

| 軸 | 値 | 意味 (nbody での実態) |
|---|---|---|
| **policy** | `range` | `parallel_for("calc_acc", Ni, ...)` — 整数 extent を直接渡す (RangePolicy に暗黙変換) |
| | `mdrange` | `parallel_for("calc_acc", Kokkos::RangePolicy(0, Ni), ...)` — 明示構築 (※MDRange 不使用) |
| | `team` | `TeamPolicy + TeamThreadRange` — league/team/thread の階層並列 |
| **メモリ** | (default) | `Kokkos::View<type::position*>` 等 |
| | `uvm` | `Kokkos::View<type::position*, Kokkos::CudaUVMSpace>` 等 |
| **sweep** | `cpu-sweep` | range の compile flag sweep 用 (`configure.py --opt-level` / `--extra-cflags`) |
| | `cpu-tile-sweep` | ※ tile マクロ未配線 (baseline と同等) |
| | `cpu-chunk-sweep` | ※ baseline と同一ソース (chunk は `NCHUNK` ハードコード) |

## 依存

- **Kokkos 4.x / 5.x** がインストール済 (`KokkosConfig.cmake` が見つかること)
- machines/<machine>.yaml の `kokkos.root`、または `configure.py --kokkos-root` / 環境変数 `Kokkos_ROOT` で
  install prefix を指定

## ビルド & 実行

Kokkos は **CMake 必須** (Make 非対応)。

```bash
cd apps/nbody
./configure.py --variant C++/kokkos/range/baseline --machine local --mode gpu
cd C++/kokkos/range/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# バイナリ: build-local-gpu/nbody.gpu.32  (実行オプションは run.<machine>.<mode>.sh 参照)
```

**UVM variant (`*/uvm`, `*/uvm-*`) は GPU mode 必須** (`--mode cpu` はエラー終了)。

## 学習順序のおすすめ

1. `range/baseline` (Kokkos の "Hello World"、RangePolicy + O(N²) 内側ループ)
2. `range/uvm` で UVM のオーバーヘッドを観察
3. `mdrange/baseline` で **RangePolicy を明示構築する書き方** (range の暗黙形との対比)
4. `team/baseline` で hierarchical parallelism (CUDA 風の league/team/thread)

詳細は各 policy の README (`range/README.md`, `mdrange/README.md`, `team/README.md`) を参照。

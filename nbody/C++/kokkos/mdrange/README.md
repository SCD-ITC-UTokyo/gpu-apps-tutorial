# Kokkos mdrange (nbody) — ※実体は `RangePolicy` の明示構築

> **重要 — このディレクトリは `MDRangePolicy` を使っていません。**
> diffusion の `mdrange/` は `MDRangePolicy<Rank<3>>` で 3D range を表現しますが、nbody は粒子が
> 1 次元配列のため N 次元 range の出番がありません。ここでの実装は
> `Kokkos::parallel_for("calc_acc", Kokkos::RangePolicy(0, Ni), ...)` で、**RangePolicy を明示的に
> オブジェクト構築**しているだけです。
> `range/` 版が整数 extent を `RangePolicy<>(0, Ni)` に暗黙変換するのに対し、こちらは同じものを
> 手で書いた形 — 両者は等価で、`MDRangePolicy` / `Rank<>` はソースに一切現れません。

## range との唯一の違い

`range/baseline` と `mdrange/baseline` の差は、policy の渡し方だけです:

```cpp
// range/baseline — 整数 extent を直接 (暗黙変換)
Kokkos::parallel_for("calc_acc", Ni, KOKKOS_LAMBDA(...) { ... });

// mdrange/baseline — RangePolicy を明示構築
Kokkos::parallel_for("calc_acc", Kokkos::RangePolicy(0, Ni), KOKKOS_LAMBDA(...) { ... });
```

これは `calc_acc` / `trim_acc` / `kick` / `drift` などすべての parallel_for に共通します。
カーネル本体 (O(N²) の内側 j 粒子ループなど) は `range/` と同じです。
詳細なコード例は [`../range/README.md`](../range/README.md) を参照。

## variant

| variant | メモリ | 特徴 |
|---|---|---|
| `baseline` | default | `Kokkos::RangePolicy(0, Ni)` を明示構築 |
| `cpu-tile-sweep` | default | ※ **tile マクロ `_NX/_NY/_NZ` はソースが参照しておらず、baseline と同等**。`configure.py --tile-nx` 等の `-D_NX=…` は現状無効 |
| `uvm` | `CudaUVMSpace` | 粒子 View を UVM 空間に (差分は `base/nbody.cpp` の `KV*1D` 型定義) |
| `uvm-tile-sweep` | CudaUVMSpace | 同上 (tile マクロ未配線) |

## ビルド & 実行

```bash
cd apps/nbody
./configure.py --variant C++/kokkos/mdrange/baseline --machine local --mode gpu
cd C++/kokkos/mdrange/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# バイナリ: build-local-gpu/nbody.gpu.32  (実行オプションは run.<machine>.<mode>.sh 参照)
```

## 学習のポイント

- **`RangePolicy` を明示的に書く形**を学べる (`range/` の暗黙変換と対比)
- 本来 `MDRangePolicy` は 2D/3D 構造を持つ問題向け。nbody の粒子配列は 1 次元なので
  MDRange の利点が無く、このディレクトリは実質 range と等価
- tile size の概念は適用されない (1 次元のため)。`cpu-tile-sweep` の tile マクロは未配線

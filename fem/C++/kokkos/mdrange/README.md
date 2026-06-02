# Kokkos mdrange (fem) — ※実体は `RangePolicy<>` の明示構築

> **重要 — このディレクトリは `MDRangePolicy` を使っていません。**
> diffusion の `mdrange/` は `MDRangePolicy<Rank<3>>` で 3D range を表現しますが、fem は節点・要素が
> 1 次元配列のため N 次元 range の出番がありません。ここでの実装は
> `using policy_1d = Kokkos::RangePolicy<>;` で、`parallel_for("...", policy_1d(0, N), ...)` と
> **RangePolicy を明示的にオブジェクト構築**しているだけです。
> `range/` 版が整数 extent を `RangePolicy<>(0, N)` に暗黙変換するのに対し、こちらは同じものを
> 手で書いた形 — 両者は等価で、`MDRangePolicy` / `Rank<>` はソースに一切現れません。

## range との唯一の違い

`range/baseline` と `mdrange/baseline` の差は、policy の渡し方だけです:

```cpp
// range/baseline — 整数 extent を直接 (暗黙変換)
Kokkos::parallel_for("q", N, KOKKOS_LAMBDA(const int j) { ... });

// mdrange/baseline — RangePolicy を明示構築 (policy_1d = Kokkos::RangePolicy<>)
Kokkos::parallel_for("q", policy_1d(0, N), KOKKOS_LAMBDA(const int j) { ... });
```

`policy_1d` は `src/kokkos_settings.h` で定義:
```cpp
using policy_1d = Kokkos::RangePolicy<>;
```

カーネル本体 (SpMV の内側 CSR ループ、`parallel_reduce` による内積など) は `range/` と同じです。
詳細なコード例は [`../range/README.md`](../range/README.md) を参照。

## variant

| variant | メモリ | 特徴 |
|---|---|---|
| `baseline` | default | `policy_1d(0, N)` で RangePolicy を明示構築 |
| `cpu-tile-sweep` | default | ※ **tile マクロ `_NX/_NY/_NZ` はソースが参照しておらず、baseline と同等**。`configure.py --tile-nx` 等の `-D_NX=…` は現状無効 |
| `uvm` | `CudaUVMSpace` | View を UVM 空間に (差分は `kokkos_settings.h` の View 定義) |
| `uvm-tile-sweep` | CudaUVMSpace | 同上 (tile マクロ未配線) |

## ビルド & 実行

```bash
cd apps/fem
./configure.py --variant C++/kokkos/mdrange/baseline --machine local --mode gpu
cd C++/kokkos/mdrange/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# バイナリ: build-local-gpu/fem.gpu.32  (実行は run.<machine>.<mode>.sh 参照)
```

## 学習のポイント

- **`RangePolicy` を明示的に書く形**を学べる (`range/` の暗黙変換と対比)
- 本来 `MDRangePolicy` は 2D/3D 構造を持つ問題向け。fem の CG は 1 次元配列演算なので
  MDRange の利点が無く、このディレクトリは実質 range と等価
- tile size の概念は適用されない (1 次元のため)。`cpu-tile-sweep` の tile マクロは未配線

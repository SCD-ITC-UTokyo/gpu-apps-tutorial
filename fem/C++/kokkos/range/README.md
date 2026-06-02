# Kokkos range — `parallel_for` / `parallel_reduce` (RangePolicy)

Kokkos の最も基本的な並列化パターン: `parallel_for("name", N, KOKKOS_LAMBDA(int i){...})`。
fem (3D Poisson FEM + CG ソルバ) では、節点・要素の 1 次元 index を range で並列化し、
内側ループ (疎行列の非ゼロ走査など) は lambda 内に手書き for として残します。

> **Note — ソース上に `RangePolicy` の文字列は現れません。**
> この variant は `Kokkos::parallel_for("q", N, ...)` のように実行ポリシーの代わりに
> ループ長 (整数 `N`) を直接渡しています。しかし実体はこのディレクトリ名どおり **RangePolicy** で、
> Kokkos が整数の extent を受け取ると内部で `Kokkos::RangePolicy<>(0, N)` に暗黙変換します
> (`parallel_for(N, f)` ≡ `parallel_for(RangePolicy<>(0, N), f)`)。
> 内積などの総和は `parallel_reduce("RHO", N, ...)` で書きますが、これも同じく RangePolicy です。
> `mdrange` の `MDRangePolicy` を明示構築するのと対比すると理解しやすいです。

## variant

| variant | メモリ | 特徴 |
|---|---|---|
| `baseline` | default `View<T*>` | 出発点。`parallel_for` / `parallel_reduce` を RangePolicy で実行 |
| `cpu-sweep` | default | (`baseline` と同ソース) compile flag sweep 用 (`configure.py --opt-level` / `--extra-cflags`) |
| `uvm` | `Kokkos::CudaUVMSpace` | View を UVM 空間に置き、転送 overhead を観察 |
| `uvm-sweep` | CudaUVMSpace | (`uvm` と同ソース) UVM + sweep |

> diffusion と違い、fem の range には `*-fused` variant はありません (もともと 1 次元 index 走査のため融合対象が無い)。

## コード例

並列化の対象は主に **行列組み立て (`mat_ass_*`)** と **CG ソルバ (`solver_CG.cpp`)** です。

疎行列ベクトル積 `{q} = [A]{p}` (`solver_CG.cpp`) — 外側 (節点 `j`) を RangePolicy 並列化、
内側 (CSR 行の非ゼロ `k`) は手書き for:
```cpp
Kokkos::parallel_for("q", N, KOKKOS_LAMBDA(const int j) {
    KREAL WVAL = D(j) * WW(P, j);
    for (int k = indexLU(j); k < indexLU(j+1); k++) {   // 内側ループは手書き
        int i = itemLU(k);
        WVAL += AMAT(k) * WW(P, i);
    }
    WW(Q, j) = WVAL;
});
Kokkos::fence();
```

内積 (総和) は `parallel_reduce` — これも RangePolicy:
```cpp
Kokkos::parallel_reduce("RHO", N, KOKKOS_LAMBDA(const int i, KREAL &RHO_l) {
    RHO_l += WW(R, i) * WW(Z, i);
}, RHO);
```

要素ループ (行列組み立て, `mat_ass_main.cpp`) も同型 — 要素数 `inum` を並列化:
```cpp
Kokkos::parallel_for("mat_ass_main", inum, KOKKOS_LAMBDA(const int icel0){ ... });
```

## UVM 版との違い

View 型の切り替えは `src/kokkos_settings.h` に集約されています。
`baseline` はデフォルト空間:
```cpp
template <typename T> using View1D = Kokkos::View<T*>;
```
`uvm` は CudaUVMSpace を持つ Device に置き換えるだけで、`parallel_for` 側のコードは不変:
```cpp
using KOKKOS_DEV = Kokkos::Device<Kokkos::Cuda, Kokkos::CudaUVMSpace>;
template <typename T> using View1D = Kokkos::View<T*, KOKKOS_DEV>;
```

## ビルド & 実行

```bash
cd apps/fem
./configure.py --variant C++/kokkos/range/baseline --machine local --mode gpu
#   (Kokkos の install prefix は --kokkos-root か環境変数 Kokkos_ROOT で指定)
cd C++/kokkos/range/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# バイナリ: build-local-gpu/fem.gpu.32
# 実行方法は同時に生成される run.<machine>.<mode>.sh を参照
```

## 学習のポイント

- **最もシンプル** な Kokkos パターン。`parallel_for` / `parallel_reduce` の第 2 引数 (整数) が並列度
- ソースに `RangePolicy` は書かないが、整数 extent が `RangePolicy<>(0, N)` に暗黙変換される
- 疎行列の非ゼロ走査のような **不規則な内側ループは手書き for** として lambda 内に残す
- 総和 (内積・ノルム) は `parallel_reduce` で書く (同じ RangePolicy)
- `uvm` との差は `kokkos_settings.h` の View 定義だけ — カーネル本体は共通

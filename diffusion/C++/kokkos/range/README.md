# Kokkos range — `parallel_for` (RangePolicy)

Kokkos の最も基本的な並列化パターン: `parallel_for("name", N, KOKKOS_LAMBDA(int i){...})`。
1 次元 range をベースに、内側ループは手書きで残す形式 (lambda 内 for ループ)。

> **Note — ソース上に `RangePolicy` の文字列は現れません。**
> この variant は `Kokkos::parallel_for("name", nz, ...)` のように実行ポリシーの代わりに
> ループ長 (整数) を直接渡しています。しかし実体はこのディレクトリ名どおり **RangePolicy** で、
> Kokkos が整数の extent を受け取ると内部で `Kokkos::RangePolicy<>(0, nz)` に暗黙変換します
> (`parallel_for(N, f)` ≡ `parallel_for(RangePolicy<>(0, N), f)`)。
> つまり「`RangePolicy` を明示的に書かない最短記法」であり、`mdrange` の `MDRangePolicy` や
> `team` の `TeamPolicy` を明示構築するのと対比すると理解しやすいです。

## variant

| variant | メモリ | ループ形 | 特徴 |
|---|---|---|---|
| `baseline` | default | 外側 for を `parallel_for` 化、内 2 ループは手書き | 出発点 |
| `cpu-sweep` | default | (`baseline` と同ソース) | compile flag sweep 用 (`sh/options_*.sh`) |
| `uvm` | `Kokkos::CudaUVMSpace` | 同上 | UVM の overhead を観察 |
| `uvm-sweep` | CudaUVMSpace | (`uvm` と同ソース) | UVM + sweep |
| `baseline-fused` | default | 3 重ループを 1 重に融合 (`parallel_for("…", nx*ny*nz, …)`) | indexing が増えるが GPU 効率向上の可能性 |
| `uvm-fused` | CudaUVMSpace | 同上 (fused) | UVM + fused |

## コード例 (`baseline/src/diffusion.cpp`)

```cpp
Kokkos::parallel_for("diffusion3d", nz,
  KOKKOS_LAMBDA(const int k) { 
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            const int ix = ...;
            fn[ix] = cc*f[ix] + ...;
        }
    }
});
```

`baseline-fused/src/diffusion.cpp` だと:
```cpp
Kokkos::parallel_for("diffusion3d", nx*ny*nz,
  KOKKOS_LAMBDA(const int ijk) { 
      const int i = ijk % nx;
      const int j = (ijk / nx) % ny;
      const int k = ijk / (nx * ny);
      ...
  });
```

UVM 版 (`uvm/src/diffusion.cpp`) は View 型が変わる:
```cpp
double diffusion3d(..., Kokkos::View<float*, Kokkos::CudaUVMSpace> f,
                        Kokkos::View<float*, Kokkos::CudaUVMSpace> fn)
```

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant C++/kokkos/range/baseline --machine local --mode gpu
cd C++/kokkos/range/baseline
cmake -B build -S . && cmake --build build
./build/diffusion.gpu.32 64
```

## 学習のポイント

- **最もシンプル** な Kokkos パターン。`parallel_for` の第 2 引数が並列度
- 内側ループは普通の C++ for として書く (lambda 内)
- `mdrange` と比べると **tile size 制御不可** (Kokkos に任せる)
- `team` と比べると **明示的な block 階層なし**
- ループ融合 (`-fused`) は GPU では効果が大きいことがある (occupancy 向上)

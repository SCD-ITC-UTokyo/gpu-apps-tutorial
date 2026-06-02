# Kokkos range — `parallel_for` (RangePolicy)

Kokkos の最も基本的な並列化パターン: `parallel_for("name", N, KOKKOS_LAMBDA(int i){...})`。
nbody (直接法 N 体重力計算) では、i 粒子の 1 次元 index を range で並列化し、
内側の j 粒子ループ (O(N²) の相互作用) は lambda 内に手書き for として残します。

> **Note — ソース上に `RangePolicy` の文字列は現れません。**
> この variant は `Kokkos::parallel_for("calc_acc", Ni, ...)` のように実行ポリシーの代わりに
> ループ長 (i 粒子数 `Ni`) を直接渡しています。しかし実体はこのディレクトリ名どおり **RangePolicy** で、
> Kokkos が整数の extent を受け取ると内部で `Kokkos::RangePolicy<>(0, Ni)` に暗黙変換します
> (`parallel_for(N, f)` ≡ `parallel_for(RangePolicy<>(0, N), f)`)。
> `mdrange` の `MDRangePolicy` を明示構築するのと対比すると理解しやすいです。

## variant

| variant | メモリ | 特徴 |
|---|---|---|
| `baseline` | default `View<T*>` | 出発点。i 粒子ループを RangePolicy で並列化、j 粒子ループは手書き |
| `cpu-sweep` | default | (`baseline` と同ソース) compile flag sweep 用 (`configure.py --opt-level` / `--extra-cflags`) |
| `uvm` | `Kokkos::CudaUVMSpace` | 粒子 View を UVM 空間に置き、転送 overhead を観察 |
| `uvm-sweep` | CudaUVMSpace | (`uvm` と同ソース) UVM + sweep |

> diffusion と違い、nbody の range には `*-fused` variant はありません (相互作用ループの構造上、融合対象が無い)。

## コード例 (`base/nbody.cpp`)

重力加速度の計算 `calc_acc` — 外側 (i 粒子 `Ni`) を RangePolicy 並列化、
内側 (j 粒子 `Nj`) は手書き for で全粒子対を走査:
```cpp
Kokkos::parallel_for("calc_acc", Ni,
  KOKKOS_LAMBDA(std::remove_const_t<decltype(Ni)> i) {
    const auto pi = ipos(i);
    decltype(iacc(0)) ai = { ... };                 // 加速度の蓄積先
    for (auto j = 0; j < Nj; j++) {                  // 内側ループは手書き (O(N²))
        const auto pj = jpos(j);
        const auto dx = pj.x - pi.x; /* ... */
        const auto r_inv = 1 / std::sqrt(eps2 + dx*dx + dy*dy + dz*dz);
        const auto alp = pj.w * (r_inv * r_inv * r_inv);
        ai.x += alp * dx;  ai.y += alp * dy;  ai.z += alp * dz;
    }
    iacc(i) = ai;
  });
Kokkos::fence();
```

時間積分 (`kick` / `drift`) も同型 — 粒子数 `num` を RangePolicy 並列化:
```cpp
Kokkos::parallel_for("kick", num,
  KOKKOS_LAMBDA(std::remove_const_t<decltype(num)> i) {
    auto vi = vel(i);
    const auto ai = acc(i);
    vi.x += ai.x * dt; /* ... */
    vel(i) = vi;
  });
```

## UVM 版との違い

粒子 View の型定義 (`base/nbody.cpp` 冒頭) を切り替えるだけで、カーネル側は不変。
`baseline` はデフォルト空間:
```cpp
using KVpos1D = Kokkos::View<type::position*>;
using KVvel1D = Kokkos::View<type::velocity*>;
using KVacc1D = Kokkos::View<type::acceleration*>;
```
`uvm` は同じ View に CudaUVMSpace を付けるだけ:
```cpp
using KVpos1D = Kokkos::View<type::position*, Kokkos::CudaUVMSpace>;
using KVvel1D = Kokkos::View<type::velocity*, Kokkos::CudaUVMSpace>;
using KVacc1D = Kokkos::View<type::acceleration*, Kokkos::CudaUVMSpace>;
```

## ビルド & 実行

```bash
cd apps/nbody
./configure.py --variant C++/kokkos/range/baseline --machine local --mode gpu
#   (Kokkos の install prefix は --kokkos-root か環境変数 Kokkos_ROOT で指定)
cd C++/kokkos/range/baseline
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
# バイナリ: build-local-gpu/nbody.gpu.32
# 実行方法 (粒子数などのオプション) は同時に生成される run.<machine>.<mode>.sh を参照
```

## 学習のポイント

- **最もシンプル** な Kokkos パターン。`parallel_for` の第 2 引数 (整数) が並列度
- ソースに `RangePolicy` は書かないが、整数 extent が `RangePolicy<>(0, Ni)` に暗黙変換される
- O(N²) の **内側 j 粒子ループは手書き for** として lambda 内に残す (各 i スレッドが全 j を走査)
- 1 スレッド = 1 i 粒子。加速度はスレッドローカルに蓄積してから View へ書き戻す
- `uvm` との差は粒子 View の型定義だけ — カーネル本体は共通

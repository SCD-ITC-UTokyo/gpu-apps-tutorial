# C++/stdpar — C++ 言語標準 (std::execution::par)

C++17 の `std::execution::par` (parallel execution policy) で並列化する実装。
**pragma を一切使わない**で、STL アルゴリズムだけで GPU 化できる (NVHPC が `-stdpar=gpu` で
GPU offload に変換)。

## 構造

```
stdpar/
├── README.md                        # ← このファイル
├── auto.def/, auto.opt/             # メモリ管理 = managed
└── 各 variant: src/{*.cpp,*.h} + Makefile.miyabi[.gpu|.cpu|.uni] + Makefile.wisteria[.gpu|.cpu] + sh/
```

stdpar は GPU 専用記法ではないため `manu.*` 変種は無し (`auto.def`/`auto.opt` のみ)。
C++ 限定。

## 見どころ (src/diffusion.cpp)

```cpp
#include <execution>
#include <algorithm>
#include <boost/iterator/counting_iterator.hpp>

std::for_each_n(
    std::execution::par,
    boost::iterators::counting_iterator<int32_t>(0),
    nx*ny*nz,
    [&](int iter) {
        int ix = iter + nx*ny*mgn;
        int k = (ix/(nx*ny)) - mgn;
        int j = (ix%(nx*ny)) / nx;
        int i = (ix%(nx*ny)) % nx;
        // ...stencil 計算...
        fn[ix] = cc*f[ix] + ...;
    });
```

ポイント:
- **pragma が無い**: 並列化は `std::execution::par` という第一引数の指定だけ
- 3 重ループは **1 次元 (`counting_iterator`) に潰して**インデックス分解
- boost の `counting_iterator` で 0..N-1 のイテレータを作る (C++23 では `std::ranges::iota_view` で代替可)

ループ融合 (155 行、他の impl の 89-93 行より大幅に長い) になっているのは STL idiom の制約上。

## 依存

- **NVHPC 22.x+** (stdpar GPU 対応)
- **boost** (header-only。`<boost/iterator/counting_iterator.hpp>` のみ使用)
  - 多くの環境で `apt install libboost-dev` または `module load boost` で OK
  - app.yaml の `libs` には現状未記載 (header-only なので link 不要)

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant C++/stdpar/auto.def --machine local --mode gpu

cd C++/stdpar/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

CPU 版 (比較用): `./configure.py --variant C++/stdpar/auto.def --machine local --mode cpu`
で stdpar を CPU マルチコアで実行できる。同じソース・同じバイナリ生成で flag 違い。

## 他 impl との比較ポイント

- **vs `openmp-target`**: pragma が不要、その代わりループ構造を STL に書き換え必須
- **vs `openmp-cpu`**: 同じ `std::execution::par` がそのまま CPU マルチコア並列にもなる
- **vs `do-concurrent`** (Fortran 側の対応物): どちらも「言語標準で GPU offload」の代表例
- **boost 依存**: portability の観点で wisteria/miyabi など環境ごとに boost の有無を確認要

C++23 では boost なしで書けるようになる予定 (`std::ranges::iota_view` 等)。

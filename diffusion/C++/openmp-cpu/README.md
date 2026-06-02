# C++/openmp-cpu — OpenMP CPU マルチコア (出発点)

OpenMP の `#pragma omp parallel for` で CPU マルチコア並列化したベースライン実装。
OpenMP を知っている読者にとっては自然な出発点。他の impl (`openmp-target`, `openacc`,
`stdpar`, `kokkos`) との比較で「GPU 化に何が必要か」を読み解く土台になる。

## 構造

```
openmp-cpu/
├── src/
│   ├── diffusion.c, diffusion.h   # 計算カーネル (本体)
│   ├── main.c                     # main + 時間積分
│   ├── misc.c, misc.h             # 入出力・ユーティリティ + FP 切替 typedef
└── sh/miyabi/                     # 元プロジェクトのジョブスクリプト類 (リファレンス)
```

variant 階層なし (`src/` 直下にソースを置くフラット形式)。

## 見どころ (src/diffusion.c)

3 箇所の並列ループに `#pragma omp parallel for collapse(3)` を付与:

```c
#pragma omp parallel for collapse(3)
for(int k = 0; k < nz; k++) {
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            fn[ix] = cc*f[ix] + ce*f[ip] + cw*f[im] + cn*f[jp] + cs*f[jm] + ct*f[kp] + cb*f[km];
        }
    }
}
```

`init` と `err` (誤差計算) にも同じ pragma。`err` は `reduction(+:ferr)` 付き。

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant C++/openmp-cpu --machine local --mode cpu

cd C++/openmp-cpu
cmake -B build-local-cpu -S .          # CMake (default)
cmake --build build-local-cpu

./build-local-cpu/diffusion.cpu.32 64
```

FP 切替: `cmake -B build-local-cpu-fp64 -S . -DFP=64` で double 精度。
スレッド数制御: `OMP_NUM_THREADS=16 ./build-local-cpu/diffusion.cpu.32 64`

## 他 impl との比較ポイント

`openmp-target/auto.def` の `src/diffusion.c` と diff すると、**pragma 行のみ**が違うことが分かる:

```diff
- #pragma omp parallel for collapse(3)              # ← この impl (CPU)
+ #pragma omp target teams loop collapse(3)         # ← openmp-target (GPU)
```

つまり OpenMP target は OpenMP CPU の自然な延長 (移行コスト最小)。

`openacc/auto.def` と diff すると `#pragma omp parallel for` ⇔ `#pragma acc kernels` + `#pragma acc loop independent collapse(3)` の違い。

`stdpar/auto.def` と比べると、pragma が消えて `std::for_each_n` + `std::execution::par` という STL idiom に変わる (構造が大きく違う)。

## machines/compilers

| machine | default compiler | 備考 |
|---|---|---|
| `local` | nvhpc (`-mp=multicore`) | NVHPC を CPU OpenMP に流用 |
| `miyabi` | intel (`icpx -qopenmp`) | |
| `wisteria` | gcc (`g++ -fopenmp`) | `--compiler fccpx` で Fujitsu に切替可 |

`./configure.py --variant C++/openmp-cpu --machine wisteria --mode cpu --compiler fccpx`
で FCCpx ビルド (実機要)。

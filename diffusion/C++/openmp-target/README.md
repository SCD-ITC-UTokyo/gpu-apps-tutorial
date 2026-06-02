# C++/openmp-target — OpenMP target offloading

OpenMP 5.0+ の `omp target` ディレクティブで GPU に offload する実装。OpenMP CPU からの
**移行コストが最小**で、pragma 行 1-2 行の変更で GPU 化できる。

## 構造

```
openmp-target/
├── README.md                        # ← このファイル
├── auto.def/, auto.opt/             # メモリ管理 = managed
├── manu.def/, manu.opt/             # メモリ管理 = separate (手動)
└── 各 variant: src/{*.c,*.h} + Makefile.miyabi[.gpu|.uni] + Makefile.wisteria[.gpu] + sh/
```

## variant 詳細

| variant | メモリモデル | コンパイラフラグ追加 | バイナリ名 |
|---|---|---|---|
| `auto.def` | managed (`-gpu=mem:managed`) | — | `diffusion.gpu.32` |
| `auto.opt` | managed | `-DNTHREADS=128` | `diffusion.gpu.32.128` |
| `manu.def` | separate (`-gpu=mem:separate`) | — | `diffusion.gpu.32` |
| `manu.opt` | separate | (現状 `manu.def` と同内容、互換維持) | `diffusion.gpu.32` |

memory model 別フラグは `machines/<machine>.yaml` の `gpu.memory_models` から決定:
- miyabi/local (NVHPC 24.5+): `-gpu=mem:managed`, `-gpu=mem:separate`, `-gpu=mem:unified:nomanagedalloc`
- wisteria (NVHPC 24.1): `-gpu=managed`, `-gpu=nomanaged` (uni 未対応)

## 見どころ (src/diffusion.c)

`#pragma omp target teams loop collapse(3)` が並列ループを GPU 化する核:

```c
#pragma omp target teams loop collapse(3)
for(int k = 0; k < nz; k++) {
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            fn[ix] = cc*f[ix] + ce*f[ip] + cw*f[im] + cn*f[jp] + cs*f[jm] + ct*f[kp] + cb*f[km];
        }
    }
}
```

`init` と `err` も同様 (`err` は `reduction(+:ferr)` 付き)。

managed/separate は **コンパイラフラグで切替**するのでソース上の差はなし (data map は暗黙)。

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu

cd C++/openmp-target/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

NTHREADS 上書き (auto.opt のみ実効): `cmake -B ... -DNTHREADS=256`
unified memory (`--mode uni`、Miyabi/local のみ): `./configure.py ... --mode uni`

## 他 impl との比較ポイント

`openmp-cpu` との diff: pragma 行のみ違う (`parallel for collapse(3)` ⇔ `target teams loop collapse(3)`)。

`openacc/auto.def` と比べると、OpenMP target は **メタディレクティブ**で host/device を同時記述、
OpenACC は `#pragma acc kernels` + `#pragma acc loop` の **2 行構成** で意味は近い。

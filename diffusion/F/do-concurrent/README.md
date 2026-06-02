# F/do-concurrent — Fortran 言語標準 (do concurrent)

Fortran 2008 で導入された `do concurrent` 構文で並列化する実装。
**pragma を一切使わない**で、`do` ループを並列化可能と宣言する。
NVHPC が `-stdpar=gpu` で GPU offload に変換 (C++ の `stdpar` の Fortran 版)。

## 構造

```
do-concurrent/
├── README.md
├── auto.def/, auto.opt/             # メモリ管理 = managed
└── 各 variant: src/{*.f90} + Makefile.miyabi[.gpu|.cpu|.uni] + Makefile.wisteria[.gpu|.cpu] + sh/
```

do-concurrent は GPU 専用記法ではないため `manu.*` 変種は無し (`auto.def`/`auto.opt` のみ)。
Fortran 限定。

## 見どころ (src/diffusion.f90)

```fortran
do concurrent (k = 1:nz, j = 1:ny, i = 1:nx)
   fn(i,j,k) = cc*f(i,j,k) + ce*f(i+1,j,k) + cw*f(i-1,j,k) + &
               cn*f(i,j+1,k) + cs*f(i,j-1,k) + ct*f(i,j,k+1) + cb*f(i,j,k-1)
end do
```

ポイント:
- **`!$omp` / `!$acc` のような pragma が無い** — 並列化は `do concurrent` 構文そのもの
- `(k=1:nz, j=1:ny, i=1:nx)` で 3 重ループを 1 文に
- コンパイラ (NVHPC) が `-stdpar=gpu` フラグの下で GPU offload に変換

`err` (reduction) は:
```fortran
do concurrent (k = 1:nz, j = 1:ny, i = 1:nx) reduce(+:ferr)
   ferr = ferr + (f(i,j,k) - f0)**2
end do
```

`reduce` 句 (Fortran 2018+) で reduction も標準構文。

## 依存

- **NVHPC 22.x+** (do concurrent + stdpar GPU 対応)
- Intel/GFortran も `do concurrent` 構文は受け付けるが GPU offload は NVHPC のみ
- ローカル機の Intel Fortran (`ifx`) も最近 GPU offload 対応開始

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant F/do-concurrent/auto.def --machine local --mode gpu

cd F/do-concurrent/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

CPU 版 (比較用): `./configure.py --variant F/do-concurrent/auto.def --machine local --mode cpu`
で do-concurrent を CPU マルチコアで実行できる (同じソース・同じバイナリ生成で flag 違い)。

## 他 impl との比較ポイント

- **vs `F/openmp-target`**: pragma 不要、Fortran 2008 言語標準
- **vs `F/openmp-cpu`**: `!$omp parallel do` を消して `do` を `do concurrent` に変えるだけ
- **vs `C++/stdpar`**: 「言語標準で GPU offload」の代表例の対 — Fortran の純粋さは特筆
- **vs `F/openacc`**: do-concurrent は本来 portable、acc は NVIDIA 中心

教材的には: pragma 駆動 (omp/acc) → 言語標準 (do concurrent) という流れで進化を見せる位置づけ。

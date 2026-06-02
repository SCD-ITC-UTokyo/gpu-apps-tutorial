# F/openacc — Fortran OpenACC

Fortran 版の OpenACC GPU offload。`!$acc kernels` + `!$acc loop independent collapse(3)`
で並列ループを GPU 化する。C++ 版の `openacc` と完全に対応。

## 構造

```
openacc/
├── README.md
├── auto.def/, auto.opt/             # managed メモリ
├── manu.def/, manu.opt/             # separate メモリ
└── 各 variant: src/{*.f90} + Makefile.miyabi[.gpu|.uni] + Makefile.wisteria[.gpu] + sh/
```

## variant 詳細

| variant | メモリ | 備考 |
|---|---|---|
| `auto.def` | managed | デフォルト |
| `auto.opt` | managed | `-DNTHREADS=128` |
| `manu.def` | separate | 手動メモリ管理 |
| `manu.opt` | separate | (manu.def と同内容) |

## 見どころ (src/diffusion.f90)

```fortran
!$acc kernels
!$acc loop independent collapse(3)
do k = 1, nz
  do j = 1, ny
    do i = 1, nx
       fn(i,j,k) = cc*f(i,j,k) + ...
    end do
  end do
end do
!$acc end kernels
```

`err` (reduction):
```fortran
!$acc kernels
!$acc loop independent reduction(+:ferr) collapse(3)
do k = 1, nz
  ...
!$acc end kernels
```

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant F/openacc/auto.def --machine local --mode gpu

cd F/openacc/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

コンパイルフラグは `-mp -acc=gpu -gpu=mem:managed` (NVHPC)。

## 他 impl との比較ポイント

- **vs `F/openmp-target`**: 同等の機能、別構文 (`!$acc kernels`+`!$acc loop` ⇔ `!$omp target teams loop`)
- **vs `F/openmp-cpu`**: 並列化ディレクティブを切り替え、ループ構造はそのまま
- **vs `F/do-concurrent`**: do-concurrent はディレクティブなし、純粋な Fortran 2008 構文に
- **vs C++ 版**: pragma sigil (`!$acc` ⇔ `#pragma acc`) のみ違う、構造は同等

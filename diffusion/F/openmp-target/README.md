# F/openmp-target — Fortran OpenMP target offloading

Fortran 版の OpenMP target offload。`!$omp target teams loop collapse(3)` で GPU 化する。
C++ 版の `openmp-target` と完全に対応する構造。

## 構造

```
openmp-target/
├── README.md
├── auto.def/, auto.opt/             # メモリ管理 = managed
├── manu.def/, manu.opt/             # メモリ管理 = separate
└── 各 variant: src/{*.f90} + Makefile.miyabi[.gpu|.uni] + Makefile.wisteria[.gpu] + sh/
```

## variant 詳細

| variant | メモリ | コンパイラフラグ追加 | バイナリ名 |
|---|---|---|---|
| `auto.def` | managed | — | `diffusion.gpu.32` |
| `auto.opt` | managed | `-DNTHREADS=128` | `diffusion.gpu.32.128` |
| `manu.def` | separate | — | `diffusion.gpu.32` |
| `manu.opt` | separate | (manu.def と同内容) | `diffusion.gpu.32` |

## 見どころ (src/diffusion.f90)

```fortran
!$omp target teams loop collapse(3) private(i,j,k,w,e,n,s,b,t)
do k = 1, nz
  do j = 1, ny
    do i = 1, nx
       fn(i,j,k) = cc*f(i,j,k) + ce*f(e,j,k) + ...
    end do
  end do
end do
!$omp end target teams loop
```

`err` (誤差計算) では reduction も併用:
```fortran
!$omp target teams loop collapse(3) reduction(+:ferr) private(i,j,k,x,y,z,f0)
```

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant F/openmp-target/auto.def --machine local --mode gpu

cd F/openmp-target/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

NTHREADS 上書き (auto.opt のみ): `cmake -B ... -DNTHREADS=256`
unified memory: `--mode uni` (Miyabi/local のみ、Wisteria 非対応)

## 他 impl との比較ポイント

- **vs `F/openmp-cpu`**: pragma 1 行を `!$omp parallel do collapse(3)` → `!$omp target teams loop collapse(3)` に変えるだけで GPU 化
- **vs `F/openacc`**: 同等の GPU 並列化を別構文で
- **vs `F/do-concurrent`**: do-concurrent は pragma 不要、`do concurrent (i=1:nx, j=1:ny, k=1:nz)` 構文に
- **vs C++ 版 `openmp-target`**: pragma 構文の sigil (`#pragma omp` ⇔ `!$omp`) と loop syntax (C の `for` ⇔ Fortran `do`) のみ違う、教材として並べて見ると言語非依存性が分かる

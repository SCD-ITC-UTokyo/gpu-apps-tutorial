# F/openmp-cpu — Fortran OpenMP CPU マルチコア (出発点)

`!$omp parallel do collapse(3)` で Fortran do ループを CPU マルチコア並列化したベースライン。
他の Fortran impl (`openmp-target`, `openacc`, `do-concurrent`) との比較で GPU 化の効果を見る土台。

## 構造

```
openmp-cpu/
├── src/
│   ├── diffusion.f90    # 計算カーネル + init + err
│   ├── main.f90         # main + 時間積分
│   └── misc.f90         # 入出力・ユーティリティ + FP 切替
└── sh/                  # 元プロジェクトのジョブスクリプト類 (リファレンス)
```

variant 階層なし。

## 見どころ (src/diffusion.f90)

3 箇所の並列ループに OpenMP directive:

```fortran
!$omp parallel do private(i,j,k,w,e,n,s,b,t) collapse(3)
do k = 1, nz
  do j = 1, ny
    do i = 1, nx
       fn(i,j,k) = cc*f(i,j,k) + ce*f(e,j,k) + cw*f(w,j,k) + ...
    end do
  end do
end do
!$omp end parallel do
```

`init` と `err` (reduction) にも同じパターン。

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant F/openmp-cpu --machine local --mode cpu

cd F/openmp-cpu
cmake -B build-local-cpu -S .
cmake --build build-local-cpu
./build-local-cpu/diffusion.cpu.32 64
```

FP 切替: `cmake -B ... -DFP=64`。スレッド数: `OMP_NUM_THREADS=16 ./diffusion.cpu.32 64`。

## 他 impl との比較ポイント

`F/openmp-target/auto.def/src/diffusion.f90` と diff すると **pragma 行だけ**違う:

```diff
- !$omp parallel do private(i,j,k,w,e,n,s,b,t) collapse(3)              # ← この impl
+ !$omp target teams loop collapse(3) private(i,j,k,w,e,n,s,b,t)        # ← openmp-target
```

`F/do-concurrent/auto.def` と比べると `!$omp parallel do` が消えて `do concurrent (i=1:nx, j=1:ny, k=1:nz)` という Fortran 2008 言語標準構文に。

## machines/compilers

| machine | default compiler | 備考 |
|---|---|---|
| `local` | nvhpc (`-mp=multicore`) | NVHPC 流用 |
| `miyabi` | intel (`ifx -qopenmp`) | |
| `wisteria` | gcc (`gfortran -fopenmp`) | `--compiler frtpx` で Fujitsu Fortran も指定可 (要実機) |

注: 過去 `Makefile.{miyabi,wisteria}.gpu` (NVHPC `-mp=multicore` の CPU build にも関わらず `.gpu` 名)
が残骸として存在したが、cleanup 時に削除。現在の `.miyabi`/`.miyabi.cpu`/`.wisteria`/`.wisteria.cpu` のみ。

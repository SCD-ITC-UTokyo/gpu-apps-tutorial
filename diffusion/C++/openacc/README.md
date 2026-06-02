# C++/openacc — OpenACC

OpenACC で GPU offload する実装。`#pragma acc kernels` + `#pragma acc loop independent collapse(3)`
の 2 行で並列ループを GPU 化する。NVIDIA HPC SDK では `-acc=gpu` で有効化。

## 構造

```
openacc/
├── README.md                        # ← このファイル
├── auto.def/, auto.opt/             # メモリ管理 = managed
├── manu.def/, manu.opt/             # メモリ管理 = separate
└── 各 variant: src/{*.c,*.h} + Makefile.miyabi[.gpu|.uni] + Makefile.wisteria[.gpu] + sh/
```

## variant 詳細

OpenMP target と同じ命名:

| variant | メモリ | 備考 |
|---|---|---|
| `auto.def` | managed | デフォルト |
| `auto.opt` | managed | `-DNTHREADS=128` |
| `manu.def` | separate | 手動メモリ管理 |
| `manu.opt` | separate | (manu.def と同内容) |

## 見どころ (src/diffusion.c)

```c
#pragma acc kernels
#pragma acc loop independent collapse(3)
for(int k = 0; k < nz; k++) {
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            fn[ix] = cc*f[ix] + ...;
        }
    }
}
// pragma acc end kernels
```

`#pragma acc kernels` の領域内をコンパイラが並列化対象として解析、`#pragma acc loop independent`
で安全に並列化できる旨をコンパイラに伝える。`reduction(+:ferr)` は `#pragma acc loop` 側に書く。

## ビルド & 実行

```bash
cd apps/diffusion
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu

cd C++/openacc/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

コンパイルフラグは `-mp -acc=gpu -gpu=mem:managed` (NVHPC, machine yaml 由来)。

### メモリモデルの切替 (managed / separate / unified)

メモリモデルは **ソース共通でコンパイルフラグだけが変わります** (machine yaml の `gpu.memory_models` 由来)。
variant ディレクトリには `Makefile.<machine>.uni` も同梱されており、unified は Makefile / フラグの差し替えだけで試せます。

| 指定 | フラグ (NVHPC) | 備考 |
|---|---|---|
| `--mode gpu` + `auto.*` | `-gpu=mem:managed` | managed (デフォルト) |
| `--mode gpu` + `manu.*` | `-gpu=mem:separate` | 手動データ管理 |
| `--mode uni` | `-gpu=mem:unified:nomanagedalloc` | unified。**Miyabi / local のみ** (Wisteria は NVHPC 24.1 が未対応) |

unified の例 (ソースは managed と同一、フラグのみ差し替え):
```bash
./configure.py --variant C++/openacc/auto.def --machine local --mode uni
```

## 他 impl との比較ポイント

- **vs `openmp-target`**: 同等の GPU 並列化を別構文で実現。NVIDIA 系列では性能差は小さいことが多い
- **vs `openmp-cpu`**: OpenMP CPU からは pragma を `omp parallel for` → `acc kernels`+`acc loop` に置換
- 言語標準 (`stdpar`) と違い、明示的にデータ転送を制御できる (separate モード)

OpenACC は AMD/Intel が GPU を公式にサポートしていないため、portability では `openmp-target` / `kokkos` に劣る (AMD GPU で OpenACC が動くのは HPE コンパイラ等に限られ、AMD 公式のサポートではない)。
NVIDIA 環境では成熟度が高く、`-Minfo=accel` で詳細な並列化情報が得られる。

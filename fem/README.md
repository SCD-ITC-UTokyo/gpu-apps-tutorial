# fem — 3D 有限要素法 (CG ソルバ)

`hairdesc_fem` の Miyabi/Wisteria 版コードを GPU プログラミングモデルの**横断比較**のために再構成したアプリです。同じ FEM 問題 (3D 熱伝導 / 構造解析、Conjugate Gradient ソルバ) を 5 つの方式で実装しています。

| 方式 | C/C++ | Fortran | 特徴 |
|---|:---:|:---:|---|
| OpenMP CPU | ✓ (C) | ✓ | OpenMP の伝統的な CPU 並列。出発点 |
| OpenMP target | ✓ (C) | ✓ | `omp target` で GPU 化 (移行コスト最小) |
| OpenACC | ✓ (C) | ✓ | `acc parallel loop`。NVIDIA 中心 |
| 言語標準 stdpar | ✓ (C++) | — | C++17 `std::execution::par`。pragma 不要 |
| 言語標準 do concurrent | — | ✓ | Fortran 2008 `do concurrent` |
| Kokkos | ✓ (C++) | — | C++ ライブラリ。CPU/GPU portability |

`C/C++` 列のカッコ内は実際のソース言語: openmp-cpu/openmp-target/openacc は **C コード** (`.c`)、stdpar と Kokkos は **C++ コード** (`.cpp`)。

---

## このアプリで学べること

- 同じ反復ソルバ (CG) を **OpenMP CPU → OpenMP target → OpenACC → stdpar / do concurrent → Kokkos** の順に書き換える際の差分
- diffusion (memory-bound stencil) と異なり、**スパース疎行列ベクトル積 (SpMV)** が支配的なアプリで GPU offload がどう効くかの観察
- Conjugate Gradient ソルバの反復回数・残差収束パターン

### スコープ

精度は **FP32 / FP64** のみ。FP16/BF16/tensor core (TF32 等) は対象外です。

---

## はじめての方へ (5 分で動かす)

### 必要なもの

| 項目 | 必須/任意 | 備考 |
|---|---|---|
| Linux (Ubuntu 24.04 で検証) | 必須 | WSL2 でも可 |
| 動作検証済みスパコン: Miyabi / Wisteria-A | — | ビルド〜ジョブ実行まで確認済み (`machines/{miyabi,wisteria}.yaml`) |
| NVIDIA HPC SDK (NVHPC) 24.1 以降 | 必須 (GPU 系) | OpenMP target / OpenACC / stdpar / do-concurrent に |
| CMake 3.20 以降 | 必須 | `apt install cmake` |
| Python 3.6 以降 + PyYAML | 必須 | `pip install pyyaml` |
| Kokkos 4.x / 5.x | 任意 | Kokkos 実装を試したい場合 |

### 最短経路 (ローカル GPU で OpenACC を動かす)

```bash
# 1. アプリのルートに移動
cd apps/fem

# 2. 構成を生成
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu

# 3. ビルド
cd C++/openacc/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu

# 4. 実行 (引数は INPUT.DAT のパス、cube.0 を CWD に置く)
cp ../../input/cube.0 .
./build-local-gpu/fem.gpu.32 ../../input/INPUT.DAT
```

標準出力 (10 列 CSV):
```
# binary,NP,time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time
./build-local-gpu/fem.gpu.32,274625, 3.21e-01, 1.47e+01, 8.03e-09, 1.34e+00, 4.32e-01, 2.18e-01,267, 8.76e-02
```

---

## ディレクトリ構成

```
apps/fem/
├── README.md                           # ← このファイル
├── app.yaml                            # アプリのメタ情報 (ソース、TARGET 名)
├── implementations.yaml                # impl × variant の軸定義
├── configure.py                        # ビルド構成生成器 (yaml → CMakeLists/Makefile)
├── machines/                           # マシン固有設定
│   ├── miyabi.yaml                     # 東大 Miyabi (Intel CPU + NVHPC 24.5+/GH200, PBS Pro)
│   ├── wisteria.yaml                   # 東大 Wisteria (GCC/FCCpx + NVHPC 24.1/A100, PJM)
│   └── local.yaml                      # ローカル開発機 (バッチ無し、直接実行)
├── input/                              # FEM 入力データ (fem 特有)
│   ├── INPUT.DAT                       # 制御パラメータ (mesh, ITER, COND, RESID)
│   └── cube.0                          # 3D 立方体メッシュ (65^3 = 274625 節点)
├── C++/                                # C/C++ 実装
│   ├── openmp-cpu/                     # OpenMP CPU ベースライン
│   ├── openacc/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── openmp-target/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── stdpar/{auto.def,auto.opt}/     # std::execution::par
│   └── kokkos/
│       ├── range/{baseline, cpu-sweep, uvm, uvm-sweep}/
│       ├── mdrange/{baseline, cpu-tile-sweep, uvm, uvm-tile-sweep}/
│       └── team/{baseline, cpu-chunk-sweep, uvm, uvm-chunk-sweep}/
├── F/                                  # Fortran 実装 (.f 固定形式)
│   ├── openmp-cpu/
│   ├── openacc/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── openmp-target/{auto.def,auto.opt,manu.def,manu.opt}/
│   └── do-concurrent/{auto.def,auto.opt}/
├── results/                            # 読者の実行結果出力先 (空)
│   └── {miyabi,wisteria,local}/
└── summary/                            # 既測データ + 可視化
    ├── parse_logs.py                   # results/*.log → CSV 変換
    ├── visualize.py                    # ダッシュボード生成器
    ├── template_fem.csv                # 既測データテンプレート
    ├── results/{miyabi,wisteria}/      # 既測ログ
    └── orig-scripts/                   # 元プロジェクトのジョブスクリプト類
```

---

## fem 固有の入力 (INPUT.DAT)

```
cube.0       # mesh ファイル名 (CWD 相対パス)
2000         # CG 最大反復数 (ITER)
1.0 1.0      # COND, QVOL (熱伝導率と熱源)
1.0e-08      # 収束判定残差 (RESID)
```

実行時は `<binary> input/INPUT.DAT` のように **INPUT.DAT のパスを引数で指定**します。INPUT.DAT 内に書かれた mesh ファイル (`cube.0`) は **CWD 相対**で探されるため、バイナリと同じディレクトリにコピーするか symlink を張ってください。

`cube.0` を別 mesh に差し替えれば、より大きな問題も解けます (現状は 65^3 = 274625 節点)。

---

## variant 命名の意味

`<lang>/<impl>/<variant>` の階層 (Kokkos のみ 4 階層):

| 軸 | 値 | 意味 |
|---|---|---|
| `<lang>` | `C++` / `F` | C/C++ ソース、Fortran ソース |
| `<impl>` | `openmp-cpu` | OpenMP CPU マルチコア (ベースライン) |
| | `openmp-target` | OpenMP target offloading (5.0+) |
| | `openacc` | OpenACC |
| | `stdpar` | C++ 言語標準 (`std::execution::par`) |
| | `do-concurrent` | Fortran 言語標準 (`do concurrent`) |
| | `kokkos` | Kokkos (C++ のみ、4 階層命名) |
| `<variant>` | `auto.def` | 自動メモリ管理 (managed) + デフォルト並列度 |
| (非 Kokkos) | `auto.opt` | managed + `NTHREADS=128` (omp-target/openacc のみ実効) |
| | `manu.def` | 手動メモリ管理 (separate) + デフォルト並列度 |
| | `manu.opt` | 手動メモリ管理 (separate) + `NTHREADS=128` |
| `<policy>/<sub>` | `range/baseline` | Kokkos `parallel_for` (RangePolicy)、デフォルトメモリ |
| (Kokkos) | `range/uvm` | RangePolicy + `CudaUVMSpace` |
| | `mdrange/cpu-tile-sweep` | `MDRangePolicy` + `_NX/_NY/_NZ` マクロ |
| | `team/cpu-chunk-sweep` | `TeamPolicy + TeamThreadRange`、`_CHUNK` マクロ |

注: `openmp-cpu` は variant 階層を持たず、`stdpar` と `do-concurrent` は `auto.def` / `auto.opt` のみ。

---

## ビルド方法 (例: Miyabi で C++ OpenACC の GPU managed メモリ)

### Step 1: 構成生成

```bash
cd apps/fem
./configure.py --variant C++/openacc/auto.def --machine miyabi --mode gpu
```

`CMakeLists.txt` と `run.miyabi.gpu.sh` が `C++/openacc/auto.def/` に生成されます。

### Step 2: CMake ビルド (default、推奨)

```bash
cd C++/openacc/auto.def
cmake -B build-miyabi-gpu -S .
cmake --build build-miyabi-gpu
```

バイナリは `build-miyabi-gpu/fem.gpu.32` (FP32 default、`-DFP=64` で FP64) に生成されます。

### Make ビルドする場合 (`--build make` 指定時)

```bash
./configure.py --variant C++/openacc/auto.def --machine miyabi --mode gpu --build make
cd C++/openacc/auto.def
make -f Makefile.gen.miyabi.gpu
```

### Step 3: 実行 (ジョブスクリプトを投入)

`configure.py` が `run.miyabi.gpu.sh` を自動生成します。**INPUT.DAT のパスと cube.0 配置を含むよう編集が必要**です。

```bash
# Miyabi (PBS Pro)
# 1. run.miyabi.gpu.sh の課金グループ (group_list) を確認/編集
# 2. 末尾の実行行を以下のように書き換え:
#       cp ../../input/cube.0 .
#       ./build-miyabi-gpu/fem.gpu.32 ../../input/INPUT.DAT
qsub run.miyabi.gpu.sh
qstat -u $USER
```

### 出力フィールド (10 列 CSV、`BENCHMARK_MODE` 時)

| カラム | fem での意味 | 単位 |
|---|---|---|
| binary | 実行バイナリのパス | string |
| **NP** | **節点数 (= DOF)、mesh で決まる (cube.0 = 274625)** | int |
| time_sec | CG ソルバ計算時間 | 秒 |
| performance_gflops | 演算性能 (FLOP / solver_time) | GFLOPS |
| **error** | **CG 最終残差** (収束時 ≤ INPUT.DAT の RESID) | float |
| real_sec | プログラム全体の wall clock 時間 | 秒 |
| user_sec | user CPU 時間 (Fortran は cpu_time() 代用) | 秒 |
| sys_sec | sys CPU 時間 (Fortran は 0.0 固定) | 秒 |
| **num_time_steps_logged** | **CG 実反復回数 (ITERactual)** | int |
| **last_sim_time** | **matrix assembly 時間** (fem 特有) | 秒 |

太字は diffusion と意味が異なる箇所:
- diffusion: `N` = 立方体 1 辺 (引数指定)、`error` = 解析解との L2 誤差、`num_time_steps_logged` = 時間積分ステップ数、`last_sim_time` = 最終 simulation 時刻
- fem: `NP` = 全節点数、`error` = CG 残差、`num_time_steps_logged` = CG 反復数、`last_sim_time` = matrix assembly 時間

CSV の列名・スキーマは diffusion と同一構造ですが、**ファイルとツールはアプリごとに独立**しています:

| アプリ | CSV | ツール |
|---|---|---|
| diffusion | `apps/diffusion/summary/diffusion.csv` | `apps/diffusion/summary/{parse_logs.py, visualize.py}` |
| fem | `apps/fem/summary/fem.csv` | `apps/fem/summary/{parse_logs.py, visualize.py}` |

fem 用 visualize.py のデフォルト入出力は `fem.csv` / `fem_dashboard.html`。

### Step 4: クリーンアップ

```bash
rm -rf build-miyabi-gpu
```

---

## ビルド方法 (例: Miyabi で Kokkos)

Kokkos は C++ performance portability ライブラリで、**ビルドは CMake 必須**。事前に Kokkos のインストールが必要です。

### 前提: Kokkos のインストール

```bash
# spack 例 (Miyabi GH200 = sm_90):
spack install kokkos +cuda +openmp +wrapper cuda_arch=90 cxxstd=17

# spack 不可な環境 (Wisteria 等) の手動ビルド:
git clone -b 4.7.02 https://github.com/kokkos/kokkos.git
mkdir kokkos-build && cd kokkos-build
cmake ../kokkos \
  -DCMAKE_INSTALL_PREFIX=$HOME/kokkos/4.7.02 \
  -DCMAKE_CXX_COMPILER=$(pwd)/../kokkos/bin/nvcc_wrapper \
  -DKokkos_ENABLE_CUDA=ON -DKokkos_ENABLE_OPENMP=ON \
  -DKokkos_ENABLE_CUDA_LAMBDA=ON \
  -DKokkos_ARCH_AMPERE80=ON \
  -DCMAKE_CXX_STANDARD=17
make -j install
```

インストール先を `machines/miyabi.yaml` の `kokkos.root` に記述します。

### Kokkos variant の選び方

| 名前 | policy | メモリ | 特徴 |
|---|---|---|---|
| `range/baseline` | `parallel_for` (RangePolicy) | デフォルト | 最もシンプルな出発点 |
| `range/cpu-sweep` | RangePolicy | デフォルト | compile flag sweep 用 |
| `range/uvm` | RangePolicy | **CudaUVMSpace** | UVM のオーバーヘッドを観察 |
| `range/uvm-sweep` | RangePolicy | CudaUVMSpace | UVM + sweep |

> **`range/*` の `RangePolicy` について:** ソースには `RangePolicy` の文字列は現れず、`Kokkos::parallel_for("q", N, ...)` のようにループ長 (整数) を直接渡しています。Kokkos が内部で `RangePolicy<>(0, N)` に暗黙変換するためで、実体はディレクトリ名どおり RangePolicy です。詳細は [`C++/kokkos/range/README.md`](C++/kokkos/range/README.md) を参照。
| `mdrange/baseline` | `MDRangePolicy` | デフォルト | MDRange の baseline |
| `mdrange/cpu-tile-sweep` | MDRangePolicy | デフォルト | `_NX/_NY/_NZ` マクロで tile size 指定 |
| `mdrange/uvm` | MDRangePolicy | CudaUVMSpace | |
| `mdrange/uvm-tile-sweep` | MDRangePolicy | CudaUVMSpace | UVM + tile macros |
| `team/baseline` | `TeamPolicy + TeamThreadRange` | デフォルト | chunk=32 ハードコード |
| `team/cpu-chunk-sweep` | TeamPolicy | デフォルト | `_CHUNK` マクロで chunk size 指定 |
| `team/uvm` | TeamPolicy | CudaUVMSpace | |
| `team/uvm-chunk-sweep` | TeamPolicy | CudaUVMSpace | UVM + chunk macro |

注: **UVM variant は GPU mode 必須** (`CudaUVMSpace` は CUDA backend 要)。

---

## configure.py の使い方

`configure.py` は variant / machine / mode などを受け取って variant ディレクトリに 2 種のファイルを生成します:

1. **ビルドファイル**: `CMakeLists.txt` (default) または `Makefile.gen.<machine>.<mode>`
2. **ジョブスクリプト**: `run.<machine>.<mode>.sh` — machine yaml の `job:` ブロックから自動生成 (Miyabi → PBS Pro `qsub`、Wisteria → PJM `pjsub`、local → 直接 bash)

### 基本コマンド

```bash
# ヘルプ
./configure.py --help

# 使える variant / machine の一覧
./configure.py --list

# 設定の一覧表示
./configure.py --info
./configure.py --info --machine miyabi
./configure.py --info --variant C++/openacc/auto.opt --machine local --mode gpu

# 例: ローカル機の GPU で OpenACC をビルド
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu

# 例: Wisteria CPU で Fortran OpenMP CPU をビルド (GCC 明示、Make 指定)
./configure.py --variant F/openmp-cpu --machine wisteria --mode cpu --compiler gcc --build make

# 内容だけ確認 (ファイル書き込まない)
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu --dry-run
```

詳細は `./configure.py --help` を参照。

### 主要オプション

| 引数 | 必須 | 値 | 説明 |
|---|---|---|---|
| `--variant` | yes | `C++/openacc/auto.def` 等 | ビルドする variant のパス |
| `--machine` | yes | `local` / `miyabi` / `wisteria` | 対象マシン |
| `--mode` | yes | `gpu` / `cpu` / `uni` | GPU offload / CPU OpenMP / GPU unified memory |
| `--compiler` | no | `intel` / `gcc` / `fccpx` / `frtpx` / `nvhpc` | CPU compiler 上書き |
| `--build` | no | `cmake` (default) / `make` | ビルドシステム (Kokkos は cmake 強制) |
| `--fp` | no | `32` / `64` | 浮動小数精度 |
| `--nthreads` | no | int | NTHREADS デフォルト (opt variant のみ実効) |
| `--tile-nx/ny/nz` | no | int | Kokkos mdrange tile-sweep の `_NX/_NY/_NZ` |
| `--chunk` | no | int | Kokkos team chunk-sweep の `_CHUNK` |
| `--opt-level` | no | `O0`〜`O4` / `fast` | ベース最適化レベル (非 Kokkos のみ) |

---

## マシン × compiler の対応表

| machine | default compiler | 使用可能 compiler | GPU compiler | サポート mode |
|---|---|---|---|---|
| `local` | gcc | gcc, nvhpc | nvhpc (auto-detect) | gpu, cpu, uni |
| `miyabi` | intel | intel, gcc | nvhpc 24.5+ (GH200) | gpu, cpu, uni |
| `wisteria` | gcc | gcc, fccpx (C/C++), frtpx (Fortran) | nvhpc 24.1 (A100) | gpu, cpu |

`uni` (unified memory) は GPU mode の派生。NVHPC 24.1 では未対応のため Wisteria では使えない。

---

## エラー時の挙動

`configure.py` は不整合を検出すると使用可能な選択肢を列挙してエラー終了します。

| エラー | 例 |
|---|---|
| 未登録 compiler | `compiler 'intel' は machine 'wisteria' に定義されていない。使用可能: ['gcc', 'fccpx', 'frtpx']` |
| 言語非対応 compiler | `compiler 'fccpx' は Fortran をサポートしない。Fortran に使えるのは: ['gcc', 'frtpx']` |
| machine が mode 未サポート | `mode 'uni' は machine 'wisteria' でサポートされない。使用可能: ['gpu', 'cpu']` |
| impl と mode の不整合 | `impl 'openmp-cpu' は mode 'gpu' でビルドできない。サポート: ['cpu']` |
| Kokkos UVM を CPU mode で | `kokkos uvm variant (range/uvm) は GPU mode のみ` |
| Kokkos に make 指定 | `kokkos は CMake 強制 (--build cmake のみサポート)` |

---

## fem 固有の既知の修正点 (mizuho 由来コードからの変更)

| ファイル | 修正内容 | 理由 |
|---|---|---|
| `F/*/src/pfem_fem_util.f` | `use pfem_util` のみとし、配列再宣言を削除 | NVHPC は use-associated symbol の再宣言を許可しない |
| `F/*/src/mat_ass_init.f` | `use pfem_all` → `use pfem_util + use pfem_fem_util` | `pfem_all` モジュールは存在しない (旧コードのタイポ?) |
| `C++/{openacc,openmp-target}/*/src/solver_CG.c` | CG 関数末尾に `ITERactual = ITER;` を追加 | CG パラメータの `ITER` がローカル shadow になり global ITERactual に反映されないため |
| `F/*/src/solver_CG.f` | 同上 (Fortran 版) | 同上 |
| `C++/*/src/test1.c`, `F/*/src/test1.f` | BENCHMARK_MODE 時の出力を 10 列 CSV に変更 | diffusion と統一スキーマ。`real_sec`/`user_sec`/`sys_sec`/`ITERactual`/`mat_sec` を出力 |

---

## 性能評価 (ダッシュボード)

`summary/visualize.py` を使うと 10 列 CSV を読み込んでインタラクティブダッシュボードが生成可能です。`category` カラムで `diffusion` / `fem` を区別する想定:

```bash
cd apps/fem/summary
python3 visualize.py
# → fem_dashboard.html (同ディレクトリ) が生成される
```

`template_fem.csv` は既測データのテンプレート。

---

## 詳しく知りたい人向け

- diffusion アプリの README (`apps/diffusion/README.md`) と相補的: 同じ configure.py / 同じ yaml 仕様 / 同じ CSV 形式 で 2 アプリを比較できます
- 各 impl の固有事情は `apps/diffusion/<lang>/<impl>/README.md` 等を参照 (fem 側の impl 別 README は今後整備予定)

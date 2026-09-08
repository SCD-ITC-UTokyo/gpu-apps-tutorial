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
| Python 3.8 以降 + PyYAML | 必須 | `pip install pyyaml` |
| pandas | 任意 | `summary/visualize.py` でダッシュボードを再生成する場合のみ |
| Kokkos 4.x / 5.x | 任意 | Kokkos 実装を試したい場合 |

### 最短経路 (ローカル GPU で OpenACC を動かす)

```bash
# 1. アプリのルートに移動
cd fem

# 2. 構成を生成
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu

# 3. ビルド
cd C++/openacc/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu

# 4. 実行 (引数は INPUT.DAT のパス。mesh (cube.0) は CWD 相対で読まれる)
#    variant ディレクトリは 3 階層なので input/ は ../../../ にある
cp ../../../input/INPUT.DAT ../../../input/cube.0 .
./build-local-gpu/fem.gpu.32 INPUT.DAT
```

標準出力 (10 列 CSV):
```
# binary,NP,time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time
./build-local-gpu/fem.gpu.32,274625, 3.21e-01, 1.47e+01, 1.000e-08, 1.34e+00, 4.32e-01, 2.18e-01,2000, 8.76e-02
```

---

## ディレクトリ構成

```
fem/
├── README.md                           # ← このファイル
├── app.yaml                            # アプリのメタ情報 (ソース、TARGET 名)
├── implementations.yaml                # impl × variant の軸定義
├── configure.py                        # ビルド構成生成器 (yaml → CMakeLists/Makefile)
├── docs/
│   └── build-tutorial.html             # ビジュアル版チュートリアル
├── machines/                           # マシン固有設定
│   ├── miyabi.yaml                     # 東大 Miyabi (Intel CPU + NVHPC 24.5+/GH200, PBS Pro)
│   ├── wisteria.yaml                   # 東大 Wisteria (GCC/FCCpx + NVHPC 24.1/A100, PJM)
│   └── local.yaml                      # ローカル開発機 (バッチ無し、直接実行)
├── input/                              # FEM 入力データ (fem 特有)
│   ├── INPUT.DAT                       # 制御パラメータ (mesh, ITER, COND, RESID)
│   ├── cube.0                          # 3D 立方体メッシュ (65^3 = 274625 節点)
│   └── gen_cube.py                     # 別サイズの cube.N 生成 (例: gen_cube.py 33 cube.0)
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
    ├── fem.csv                         # 統合 CSV (Kokkos + 非 Kokkos、18 列)
    ├── fem_dashboard.html              # インタラクティブ可視化
    ├── parse_logs.py                   # 実行ログ → fem.csv 形式に変換
    ├── parse_kokkos_logs.py            # Kokkos 実行ログ用
    └── visualize.py                    # ダッシュボード生成器
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
cd fem
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

`configure.py` が `run.miyabi.gpu.sh` を自動生成します。**入力のコピーと実行行は生成済みなので編集は不要**です
(課金グループも `id -gn` の primary group が埋まります。違う場合は `--group` で指定)。生成される実行部:

```bash
# === 実行 ===
#   argv[1] は制御ファイル (INPUT.DAT)。メッシュファイル名 (cube.0) は
#   INPUT.DAT の 1 行目に書かれており、カレントディレクトリから読まれる。
#   そのため入力一式をカレントにコピーしてから実行する。
INPUT_DIR=../../../input
cp -f ${INPUT_DIR}/INPUT.DAT ${INPUT_DIR}/cube.0 .
#   別サイズのメッシュを使う場合 (例: 33^3 節点):
#     python3 ${INPUT_DIR}/gen_cube.py 33 cube.0
./build-miyabi-gpu/fem.gpu.32 INPUT.DAT
```

```bash
qsub run.miyabi.gpu.sh                 # ジョブ投入
qstat -u $USER                         # 状態確認
# 完了後、fem.o<jobid> に標準出力 (10 列 CSV) が出力される
```

### 出力フィールド (10 列 CSV、`BENCHMARK_MODE` 時)

| カラム | fem での意味 | 単位 |
|---|---|---|
| binary | 実行バイナリのパス | string |
| **NP** | **節点数 (= DOF)、mesh で決まる (cube.0 = 274625)** | int |
| time_sec | CG ソルバ計算時間 | 秒 |
| performance_gflops | 演算性能 (FLOP / solver_time) | GFLOPS |
| **error** | **INPUT.DAT の RESID 設定値** (収束後の残差ではない。下の注意を参照) | float |
| real_sec | プログラム全体の wall clock 時間 | 秒 |
| user_sec | user CPU 時間 (Fortran は cpu_time() 代用) | 秒 |
| sys_sec | sys CPU 時間 (Fortran は 0.0 固定) | 秒 |
| **num_time_steps_logged** | **INPUT.DAT の最大反復数 ITER** (実反復数ではない。下の注意を参照) | int |
| **last_sim_time** | **matrix assembly 時間** (fem 特有) | 秒 |

太字は diffusion と意味が異なる箇所:
- diffusion: `N` = 立方体 1 辺 (引数指定)、`error` = 解析解との L2 誤差、`num_time_steps_logged` = 時間積分ステップ数、`last_sim_time` = 最終 simulation 時刻
- fem: `NP` = 全節点数、`error` = CG の収束判定値、`num_time_steps_logged` = CG 最大反復数、`last_sim_time` = matrix assembly 時間

> **注意 (`error` と `num_time_steps_logged`)**
> `SOLVE11()` は `ITERactual = ITER` としていますが、`CG()` は `ITER` と `RESID` を値渡しで
> 受け取るため、収束した反復数・残差は呼び出し元に戻りません。したがって CSV の
> `error` は INPUT.DAT の収束判定値 (既定 `1.0e-08`)、`num_time_steps_logged` は
> INPUT.DAT の最大反復数 (既定 `2000`) がそのまま出ます。
> 実際の反復履歴が必要な場合は `BENCHMARK_MODE` を外してビルドしてください
> (標準出力に `<反復数> <残差>` が 1 行ずつ出ます。同梱 `cube.0` + 既定設定なら
> 197 反復・残差 8.292404e-09 で収束します)。


CSV の列名・スキーマは diffusion と同一構造ですが、**ファイルとツールはアプリごとに独立**しています:

| アプリ | CSV | ツール |
|---|---|---|
| diffusion | `diffusion/summary/diffusion.csv` | `diffusion/summary/{parse_logs.py, visualize.py}` |
| fem | `fem/summary/fem.csv` | `fem/summary/{parse_logs.py, visualize.py}` |

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

### `--queue` / `--group` / `--kokkos-root`

| オプション | 既定の挙動 |
|---|---|
| `--queue NAME` | 省略時は machine yaml の `job.per_mode.<mode>.queue`。講習会などで専用キューを使うときに指定します (例: `--queue tutorial-g`)。使えるキュー名は `--info --machine <name>` で一覧できます |
| `--group NAME` | 省略時は `id -gn` で課金グループを自動取得。取得できない場合はジョブスクリプト先頭に警告を出し、投入前の編集を促します |
| `--kokkos-root PATH` | 省略時は **machine yaml の `kokkos.root`** を使い、それが placeholder (`/xxx/` 等) のときだけ自動検出に回ります。自動検出は環境変数 `Kokkos_ROOT` / `KOKKOS_ROOT` → `CMAKE_PREFIX_PATH` → `~/kokkos/*`, `~/.local`, `/opt/kokkos*`, `/usr/local`, spack の順に探索し、`KokkosConfig.cmake` を持つ prefix を採用します。どれも見つからない場合は指定を促して終了します |

`--group` は明示値が `id -gn` の検出値と異なる場合に警告を出したうえで、指定値を優先します。
`--kokkos-root` は明示値に `KokkosConfig.cmake` が無いときに警告を出し、そのまま指定値を使います
(machine yaml の `kokkos.root` に `KokkosConfig.cmake` が無い場合は、自動検出値へフォールバックして警告します)。

講習会などで専用のキューと課金グループが割り当てられている場合は、両方をまとめて渡せます。

```bash
./configure.py --variant C++/openacc/auto.def --machine miyabi --mode gpu \
    --queue tutorial-g --group gt00
```

生成されたジョブスクリプトには由来がコメントとして残ります (スケジューラがディレクティブ
行末のコメントを option と誤解するため、コメントは必ず別行に置いています)。

```bash
# group_list: 'gt00' (--group で明示指定、自動検出 'gz00' とは異なる)
#PBS -N "fem"
# queue: 'tutorial-g' (--queue で明示指定。machine yaml の既定は 'debug-g')
#PBS -q tutorial-g
```

`--queue` はキューの概念があるスケジューラ (`pbs` / `pjm`) の machine でのみ有効で、
`local` など `scheduler: none` の machine に渡すと ERROR で拒否します。

投入前に反映結果を確かめたいときは `--dry-job` を使います。ジョブスクリプトを
書き出さずに内容だけ標準出力に表示します (`--dry-run` はビルドファイル側の同等機能で、
併用すると両方表示します)。

```bash
./configure.py --variant C++/openacc/auto.def --machine miyabi --mode gpu \
    --queue tutorial-g --group gt00 --dry-job
```

`--dry-run` / `--dry-job` はホストと `--machine` の不整合チェックも免除されるため、
手元の環境から別マシン向けの内容を確認する用途にも使えます。

### 演習モード (`--exercise`)

「ビルド設定は GPU 版、ソースは CPU 版」の状態を用意して、**受講者が自分で
ディレクティブを書き込む**ための作業ディレクトリを作ります。講習会で手作業の
ファイルコピーを案内していた手順を、オプション 1 つで再現できます。

```bash
./configure.py --variant C++/openacc/auto.def --machine miyabi --mode gpu --exercise
```

これで `C++/openacc/auto.def.exercise/` が作られます。

| 中身 | 由来 |
|---|---|
| `src/` | `fem` の **CPU OpenMP 版** (`<lang>/openmp-cpu/src`) のコピー。ディレクティブ未記述 |
| `CMakeLists.txt` / `Makefile.gen.*` / `run.*.sh` | 指定した variant (`C++/openacc/auto.def`) と `--mode` のもの。つまり GPU 向け |
| `EXERCISE.md` | 由来・答え合わせ・初期化のコマンドを書いた説明ファイル |

参照実装 (答え) は元の variant にそのまま残るので、差分で確認できます。

```bash
diff -ru C++/openacc/auto.def.exercise/src C++/openacc/auto.def/src
```

作った演習ディレクトリは普通の variant として扱われます。`--list` にも
`← 演習用 (--exercise で作成)` 付きで表示され、2 回目以降は次のように直接指定できます
(このときソースは触られません)。

```bash
./configure.py --variant C++/openacc/auto.def.exercise --machine miyabi --mode gpu
```

| オプション | 意味 |
|---|---|
| `--exercise` | 演習ディレクトリを作る。**既存の `src/` は上書きしない** (受講者の編集を守る) |
| `--exercise-reset` | 演習ソースを削除してコピー元から作り直す (編集は失われる) |
| `--exercise-from VARIANT` | コピー元の variant を明示指定 (既定は同言語の `openmp-cpu`) |

補足:

- `--clean` / `--allclean` は演習ディレクトリの生成物だけを削除し、`src/` と
  `EXERCISE.md` は残します
- コピー元は同じ言語である必要があります (C++ のソースを Fortran のビルドに
  渡すような指定は ERROR で拒否)
- Kokkos variant を演習対象にする場合は、ソース構成が異なるため
  `--exercise-from C++/kokkos/range/baseline` のように Kokkos variant の明示が必要です
- `<lang>/<impl>` 形式の variant (`C++/openmp-cpu` 等) はコピー元そのものなので
  演習対象にできません
- 初回セットアップはディレクトリ作成を伴うため `--dry-run` / `--dry-job` とは併用
  できません (作成後なら併用可)


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

# ビルドファイルの内容だけ確認 (書き込まない)
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu --dry-run

# ジョブスクリプトの内容だけ確認 (書き込まない)
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu --dry-job
```

詳細は `./configure.py --help` を参照。

### 生成物の削除 (`--clean` / `--allclean`)

`configure.py --clean` で、`configure.py` とビルドが作った生成物を削除します。
**実行結果の出力ファイルと `results/` は削除しません**。結果まで消して完全に初期状態へ
戻したいときは `--allclean` を使います (`--allclean` でも `results/` は削除しません)。
`summary/`, `machines/`, `docs/`, 各 variant の `src/`・`sh/`、手書きの
`Makefile.<machine>[.<mode>]` はどちらでも対象外です。

| 削除されるもの | 例 |
|---|---|
| configure.py の生成物 | `CMakeLists.txt` (先頭に `Generated by configure.py` を含むものだけ)、`Makefile.gen.<machine>.<mode>`、`run.<machine>.<mode>.sh` |
| ビルド生成物 | `build-<machine>-<mode>/`、`*.o`、`*.mod`、`mod/`、バイナリ (`fem.<mode>.<FP>...`) |
| ジョブ出力ログ | `fem.o<jobid>` (PBS)、`fem.<jobid>.out` / `.err` (PJM) |
| 実行時生成物 | `INPUT.DAT` / `cube.0` (run スクリプトが `input/` からコピーしたもの。原本は `input/` に残る) |
| 実行結果 (`--allclean` のみ) | `test.inp` (`output_ucd`)、`log.log` (`test1`) |

```bash
# 削除対象の一覧だけ表示 (削除しない)
./configure.py --clean --dry-run

# 確認プロンプトの後に削除 (実行結果は残る)
./configure.py --clean

# 実行結果まで削除 (results/ は残る)
./configure.py --allclean

# 確認なしで削除 (バッチ/スクリプトから)
./configure.py --clean --yes

# 対象を絞る (variant / machine / mode は任意に組み合わせ可)
./configure.py --clean --variant F/openmp-cpu --machine wisteria --mode cpu
```

対話できない環境 (パイプ経由等) で `--yes` を付けずに実行した場合は、誤削除を防ぐため
ERROR で終了します。


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
| `--opt-level` | no | `O0`〜`O4` / `fast` | ベース最適化レベルの上書き。Kokkos variant でもアプリのコンパイル単位には適用される (Kokkos 本体はインストール時の設定) |
| `--extra-cflags` | no | str | 追加 compile flag (例: `--extra-cflags="-march=sapphirerapids"`)。Kokkos variant での扱いは `--opt-level` と同じ |
| `--kokkos-root` | no | str | Kokkos の install prefix (kokkos variant のみ実効) |
| `--exercise` | no | flag | 演習用 `<variant>.exercise` を作る (ビルド設定は指定 variant、`src/` は CPU OpenMP 版のコピー) |
| `--exercise-reset` | no | flag | 演習ソースをコピー元から作り直す (編集は失われる) |
| `--exercise-from` | no | str | 演習ソースのコピー元 variant (既定: 同言語の `openmp-cpu`) |
| `--queue` | no | str | 投入キュー (PBS `-q` / PJM `-L rscgrp`) の明示指定。例: `--queue tutorial-g` |
| `--group` | no | str | 課金グループ (PBS `group_list` / PJM `-g`) の明示指定。例: `--group gt00` |
| `--dry-run` | no | flag | ビルドファイルの生成内容を表示するだけで書き込まない |
| `--dry-job` | no | flag | ジョブスクリプトの生成内容を表示するだけで書き込まない (`--dry-run` と併用で両方) |
| `--clean` | no | flag | 生成物を削除 (実行結果と `results/` は残す)。`--dry-run` で一覧のみ、`--yes` で確認省略、`--variant`/`--machine`/`--mode` で対象を絞る |
| `--allclean` | no | flag | `--clean` に加えて実行結果の出力ファイル (`test.inp`, `log.log`) も削除 (`results/` は残す) |
| `--yes`, `-y` | no | flag | `--clean` / `--allclean` の確認プロンプトを省略 (非対話環境では必須) |

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

`summary/visualize.py` が `summary/fem.csv` (18 列、現在 455 行) を読み込んでインタラクティブ
ダッシュボードを生成します。`category` カラムは `Kokkos` / `non-Kokkos` の区別で、CSV とツールは
アプリごとに独立しています (アプリ横断の統合 CSV はありません):

```bash
cd fem/summary
python3 visualize.py
# → fem_dashboard.html (同ディレクトリ) が生成される
# 別の CSV / 出力先を指定する場合: python3 visualize.py path/to/data.csv path/to/dash.html
```

フィルタ軸は `machine` / `mode` / `language` / `impl` / `variant` / `memory_model` /
`optimization_param` / `fp` の 8 軸 (`category` と `optimization_type` は他の軸から一意に決まるため
フィルタからは外し、色分け軸としてのみ残しています)。fem は mesh で問題サイズが決まるため
`nx/ny/nz` 列は持たず、`N` (= NP) のみです。

---

## 詳しく知りたい人向け

- diffusion アプリの README (`diffusion/README.md`) と相補的: 同じ configure.py / 同じ yaml 仕様 / 同じ CSV 形式 で 2 アプリを比較できます
- 各 impl の固有事情は `diffusion/<lang>/<impl>/README.md` 等を参照。fem 側は Kokkos のみ
  (`C++/kokkos/README.md`, `C++/kokkos/{range,mdrange,team}/README.md`) が整備済みで、
  非 Kokkos impl の README は今後整備予定

# diffusion — 3D 拡散方程式

3 次元拡散方程式を有限差分法で時間積分するベンチマーク。複数の GPU プログラミングモデル
(OpenMP CPU / OpenMP target / OpenACC / 言語標準 (stdpar・do concurrent) / Kokkos) で
同じ問題を実装し、比較できるよう整備したアプリ。

**数値モデル**: 拡散方程式 ∂f/∂t = κ ∇²f (3D, Dirichlet 境界条件, float/double 切替可)

---

## このアプリで学べること

OpenMP 経験者が GPU プログラミングモデルを横断的に学べるよう、**同じ拡散方程式**を 5 つの方式で
実装してあります。各 impl は **pragma 1〜3 行レベルの違い**で済むようコードを揃えています:

| 方式 | C/C++ | Fortran | 特徴 |
|---|:---:|:---:|---|
| **OpenMP CPU** | ✓ (C) | ✓ | OpenMP の伝統的な CPU 並列。出発点 |
| **OpenMP target** | ✓ (C) | ✓ | `omp target` ディレクティブで GPU 化 (移行コスト最小) |
| **OpenACC** | ✓ (C) | ✓ | `acc kernels` ディレクティブ。NVIDIA 中心 |
| **言語標準 stdpar** | ✓ (C++) | — | C++17 `std::execution::par`。pragma 不要 |
| **言語標準 do concurrent** | — | ✓ | Fortran 2008 `do concurrent`。pragma 不要 |
| **Kokkos** | ✓ (C++) | — | C++ ライブラリ。CPU/GPU/AMD/Intel どこでも動く portability |

`C/C++` 列のカッコ内は実際のソース言語: openmp-cpu/openmp-target/openacc は **C コード** (`.c`)、stdpar と Kokkos は **C++ コード** (`.cpp`)。
ディレクトリ名 `C++/` は両者をまとめる「C/C++ コンパイラで処理できる言語ディレクトリ」の意味で、実体は C と C++ の混在です。

OpenMP CPU の `#pragma omp parallel for collapse(3)` と OpenMP target の
`#pragma omp target teams loop collapse(3)` を見比べると、移行に必要な変更が一目で分かります。

### スコープ

精度は **FP32 / FP64** のみ。FP16/BF16/tensor core (TF32 等) は対象外です。

---

## はじめての方へ (5 分で動かす)

> **📘 ビジュアル版チュートリアル**: より分かりやすい HTML 版を `docs/build-tutorial.html` に用意しました。
> ブラウザで開くと、ステップごとのカード、コード比較、フロー図、トラブルシューティングなどを
> 視覚的に確認できます。`open apps/diffusion/docs/build-tutorial.html` または GitHub Pages で公開可能。


### 必要なもの

- **OS**: Linux (Ubuntu 24.04 で開発・検証済)
- **動作検証済みスパコン**: Miyabi (NVHPC / NVIDIA GH200, PBS Pro) / Wisteria-A (NVHPC / NVIDIA A100, PJM) — いずれもビルド〜ジョブ実行まで確認済み (`machines/miyabi.yaml`, `machines/wisteria.yaml`)
- **コンパイラ**: 以下のいずれか
  - **NVIDIA HPC SDK (NVHPC)** 24.1 以降 — OpenMP target / OpenACC / stdpar / do-concurrent に必須
  - **Intel oneAPI / GCC** — CPU OpenMP のみなら可
- **CMake** 3.20 以降 (推奨)
- **Python** 3.8 以降 (`configure.py` 実行用)
- **PyYAML** (`pip install pyyaml`)
- **Kokkos** (オプション、Kokkos 実装を試したい場合のみ) — spack 等でインストール

### 最短経路 (ローカル GPU で OpenMP target を動かす)

NVHPC が `/opt/nvidia/hpc_sdk/` 等にインストール済みであることが前提です。

```bash
# 1. リポジトリのルートに移動
cd apps/diffusion

# 2. 構成を生成 (CMake ファイルを作る)
./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu

# 3. ビルドディレクトリに移動してビルド
cd C++/openmp-target/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu

# 4. 実行 (引数はグリッドサイズ N)
./build-local-gpu/diffusion.gpu.32 64
# 出力例: ./build-local-gpu/diffusion.gpu.32,  64, 6.685e-02, 4.175e+11, 1.214e-05
#         [バイナリ名]                          [N] [計算時間 s] [FLOPS]   [誤差]
```

うまく動かない場合: [トラブルシューティング](#トラブルシューティング) を参照。

### 次に試すこと

最短経路でビルド成功したら、次のステップで比較学習に進めます:

1. **CPU 版と比較**: 同じ問題を CPU で動かして GPU の効果を体感
   ```bash
   cd apps/diffusion
   ./configure.py --variant C++/openmp-cpu --machine local --mode cpu
   cd C++/openmp-cpu && cmake -B build-local-cpu && cmake --build build-local-cpu
   ./build-local-cpu/diffusion.cpu.32 64
   ```

2. **他の方式と pragma を見比べる**: ソースコードの差分を見る
   ```bash
   diff C++/openmp-cpu/src/diffusion.c \
        C++/openmp-target/auto.def/src/diffusion.c
   # → 違いは pragma 行だけ ! (collapse(3) parallel for ⇔ target teams loop)
   ```

3. **言語標準で書いてみる**: pragma 不要の stdpar
   ```bash
   ./configure.py --variant C++/stdpar/auto.def --machine local --mode gpu
   cd C++/stdpar/auto.def && cmake -B build-local-gpu && cmake --build build-local-gpu
   ./build-local-gpu/diffusion.gpu.32 64
   ```

4. **Kokkos で portability を体験**: 同じバイナリが CPU でも GPU でも動く (`OMP_NUM_THREADS` 等で制御)
   → [Kokkos の使い方](#kokkos-のビルド方法) を参照

5. **Fortran 版**: F90 でも同じことができる
   ```bash
   ./configure.py --variant F/openmp-target/auto.def --machine local --mode gpu
   cd F/openmp-target/auto.def && cmake -B build-local-gpu && cmake --build build-local-gpu
   ./build-local-gpu/diffusion.gpu.32 64
   ```

### トラブルシューティング

| 症状 | 原因と対処 |
|---|---|
| `ERROR: PyYAML が必要です` | `pip install pyyaml` |
| `nvc++: command not found` | NVHPC のパスが通っていない。`source /opt/nvidia/hpc_sdk/Linux_x86_64/<ver>/comm_libs/<ver>/nvshmem/etc/nvshmem-config-h.cmake` 等を環境に追加 |
| `cmake: command not found` | CMake 未インストール。`apt install cmake` 等 |
| 数値結果がおかしい (NaN 等) | `-DFP=64` で double 精度に切り替えて確認 |
| GPU が認識されない | `nvidia-smi` で GPU 認識を確認、`nvc++ -mp=gpu -gpu=cc80 ...` 等 GPU の compute capability を明示 |

---

## ディレクトリ構成

```
apps/diffusion/
├── README.md                           # ← このファイル
├── app.yaml                            # アプリのメタ情報 (ソース、TARGET 名)
├── implementations.yaml                # impl × variant の軸定義
├── configure.py                        # ビルド構成生成器 (yaml → CMakeLists/Makefile)
├── machines/                           # マシン固有設定 (compiler/flags + job ブロック)
│   ├── miyabi.yaml                     # 東大 Miyabi (Intel CPU + NVHPC 24.5+/GH200, PBS Pro)
│   ├── wisteria.yaml                   # 東大 Wisteria (GCC/FCCpx + NVHPC 24.1/A100, PJM)
│   └── local.yaml                      # ローカル開発機 (バッチ無し、直接実行)
├── C++/                                # C/C++ 実装
│   ├── README は各 impl/ 配下にあります
│   ├── openmp-cpu/                     # OpenMP CPU ベースライン
│   ├── openacc/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── openmp-target/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── stdpar/{auto.def,auto.opt}/     # std::execution::par
│   └── kokkos/
│       ├── range/{baseline, cpu-sweep, uvm, uvm-sweep, baseline-fused, uvm-fused}/
│       ├── mdrange/{baseline, cpu-tile-sweep, uvm, uvm-tile-sweep}/
│       └── team/{baseline, cpu-chunk-sweep, uvm, uvm-chunk-sweep}/
├── F/                                  # Fortran 実装
│   ├── openmp-cpu/
│   ├── openacc/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── openmp-target/{auto.def,auto.opt,manu.def,manu.opt}/
│   └── do-concurrent/{auto.def,auto.opt}/
├── results/                            # 読者の実行結果出力先 (空)
│   └── {miyabi,wisteria,local}/
└── summary/                            # 既測の公開データ + 可視化
    ├── diffusion.csv                   # 統合 CSV (Kokkos + 非 Kokkos、907 行)
    ├── diffusion_dashboard.html        # インタラクティブ可視化
    ├── visualize.py                    # ダッシュボード生成器
    ├── results/{miyabi,wisteria}/      # 既測ログ
    ├── orig-scripts/                   # 元プロジェクトのジョブスクリプト類
    └── docs/                           # PDF マニュアル
```

各 impl ディレクトリには独自の README があります (例: `C++/openmp-target/README.md`)。
impl 固有の説明・コード見どころ・variant 詳細はそちらを参照。

---

## variant 命名の意味

`<lang>/<impl>/<variant>` の階層 (Kokkos のみ 4 階層):

| 軸 | 値 | 意味 |
|---|---|---|
| `<lang>` | `C++` (ディレクトリ名) | **C/C++ ソース** (`*.c`, `*.cpp`) — C++ コンパイラで処理する言語ディレクトリ |
| | `F` | Fortran ソース (`*.f90`) |
| `<impl>` | `openmp-cpu` | OpenMP CPU マルチコア (ベースライン) |
| | `openmp-target` | OpenMP target offloading (5.0+) |
| | `openacc` | OpenACC |
| | `stdpar` | C++ 言語標準 (`std::execution::par`) |
| | `do-concurrent` | Fortran 言語標準 (`do concurrent`) |
| | `kokkos` | Kokkos (C++ のみ、4 階層命名) |
| `<variant>` | `auto.def` | 自動メモリ管理 (managed) + デフォルト並列度 |
| (非 Kokkos) | `auto.opt` | 自動メモリ管理 + `NTHREADS=128` (omp-target/openacc のみ実効) |
| | `manu.def` | 手動メモリ管理 (separate) + デフォルト並列度 |
| | `manu.opt` | 手動メモリ管理 (separate) + `NTHREADS=128` (omp-target/openacc のみ実効) |
| `<policy>/<sub>` | `range/baseline` | Kokkos `parallel_for` (RangePolicy)、デフォルトメモリ |
| (Kokkos) | `range/uvm` | RangePolicy + `CudaUVMSpace` |
| | `range/cpu-sweep` | RangePolicy + compile flag sweep (元 sh/options_*.sh 参照) |
| | `range/baseline-fused` | ループ融合バリアント |
| | `mdrange/...` | `MDRangePolicy`、tile sweep には `_NX/_NY/_NZ` マクロ |
| | `team/...` | `TeamPolicy + TeamThreadRange`、chunk sweep には `_CHUNK` マクロ |

注: `openmp-cpu` は variant 階層を持たず (`<lang>/openmp-cpu/src/` 直下にソース)、
`stdpar` と `do-concurrent` は `auto.def` / `auto.opt` のみ。

---

## ビルド方法 (例: Miyabi で C++ stdpar の GPU unified memory ビルド)

ビルド・実行手順を 1 つの具体例で示します。同じパターンが他の variant/machine/mode にも適用できます (例の引数を差し替えるだけ)。

### Step 1: 構成生成

```bash
cd apps/diffusion
./configure.py --variant C++/stdpar/auto.def --machine miyabi --mode uni
```

標準出力例:
```
Generated: C++/stdpar/auto.def/CMakeLists.txt

--- Build configuration ---
  variant       : C++/stdpar/auto.def
  machine       : miyabi  (東大 Miyabi (PBS Pro、Miyabi-G=GH200/NVHPC 24.5+、Miyabi-C=Intel Xeon))
  mode          : uni  (gpu=GPU offload, cpu=CPU OpenMP, uni=GPU unified memory)
  language      : cpp (C++)
  implementation: stdpar
  compiler      : nvhpc  (nvc / nvc++ / nvfortran)
  build system  : cmake
  precision     : FP=32 (default; cmake -DFP=64 で double 精度に変更可)
  optimization  : (variant-specific 最適化なし)
  memory model  : unified  (Unified memory (NVHPC unified))
  target binary : diffusion.uni.<FP>
  sources       : misc.cpp diffusion.cpp main.cpp

次の手順:
  cd C++/stdpar/auto.def
  cmake -B build-miyabi-uni -S .
  cmake --build build-miyabi-uni
  # バイナリ: build-miyabi-uni/diffusion.uni.<FP>
```

### Step 2-a: CMake ビルド (default、推奨)

```bash
cd C++/stdpar/auto.def
cmake -B build-miyabi-uni -S .
cmake --build build-miyabi-uni
```

標準出力例 (configure フェーズ):
```
-- The CXX compiler identification is NVHPC 24.5.0
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: .../build-miyabi-uni
```
標準出力例 (build フェーズ):
```
[ 25%] Building CXX object .../misc.cpp.o
[ 50%] Building CXX object .../diffusion.cpp.o
[ 75%] Building CXX object .../main.cpp.o
[100%] Linking CXX executable diffusion.uni.32
[100%] Built target diffusion
```

バイナリは `build-miyabi-uni/diffusion.uni.32` に生成されます。

### Step 2-b: Make ビルド (`--build make` 指定時、Kokkos 以外で利用可)

```bash
./configure.py --variant C++/stdpar/auto.def --machine miyabi --mode uni --build make
cd C++/stdpar/auto.def
make -f Makefile.gen.miyabi.uni
```

標準出力例:
```
--- build config ---
MACHINE   = miyabi
MODE      = uni
COMPILER  = nvhpc
FP        = 32
TARGET    = diffusion.uni.32
nvc++ -tp=native -O3 -std=c++17 -stdpar=gpu -gpu=mem:unified:nomanagedalloc -c src/misc.cpp -o misc.o
nvc++ -tp=native -O3 ... -c src/diffusion.cpp -o diffusion.o
nvc++ -tp=native -O3 ... -c src/main.cpp -o main.o
nvc++ ... misc.o diffusion.o main.o  -o diffusion.uni.32
```

バイナリは variant 直下 (`diffusion.uni.32`) に生成されます。`make -f Makefile.gen.miyabi.uni FP=64` で精度切替も可能。

### Step 3: 実行

`configure.py` は variant ディレクトリに `run.<machine>.<mode>.sh` を**自動生成**します
(machine yaml の `job:` ブロックから PBS / PJM / local 用の雛形を組み立て)。
このスクリプトを編集 (課金グループ等) してジョブを投入します。

```bash
# Miyabi (PBS Pro) — uni mode の例
# 課金グループ (group_list) を自分のものに書き換える
qsub run.miyabi.uni.sh
qstat -u $USER                                   # 状態確認
# 完了後、run.miyabi.uni.sh.o<jobid> に標準出力 (10 カラム CSV) が出力される
```

Wisteria の場合は `run.wisteria.<mode>.sh` が PJM 形式で生成されるので `pjsub run.wisteria.<mode>.sh` で投入。
ローカル機 (`--machine local`) では `run.local.<mode>.sh` が単純な bash スクリプトとして生成され、`bash run.local.<mode>.sh` または直接バイナリを呼ぶことで実行できます:

```bash
# ローカル機 / 対話ノードで直接実行する場合
./build-miyabi-uni/diffusion.uni.32 64    # CMake ビルド
./diffusion.uni.32 64                     # Make ビルド (variant 直下に生成)
```

引数 `64` は **N (= nx × ny × nz の N、つまり 64×64×64 立方体)** を指定 (`./binary 64` で 262,144 格子点)。

標準出力例:
```
# binary,N(=nx*ny*nz),time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time
./build-miyabi-uni/diffusion.uni.32,262144, 6.685e-02, 4.176e+01, 1.214e-05, 2.118e-01, 7.210e-02, 1.018e-01, 8192, 2.000e+00
```

出力フィールド (CSV、カンマ区切り、`BENCHMARK_MODE` 時):

| カラム | 内容 | 単位 |
|---|---|---|
| binary | 実行バイナリのパス | string |
| **N = nx × ny × nz** | 総格子点数 (引数 N で `nx=ny=nz=N` の cubic、例: 64 → 262144) | int |
| time_sec | カーネル計算時間 | 秒 |
| performance_gflops | 演算性能 | GFLOPS |
| error | 解析解との L2 誤差 (FP32 で 1e-5、FP64 で 1e-7 程度) | float |
| real_sec | プログラム全体の wall clock 時間 | 秒 |
| user_sec | user CPU 時間 (Fortran は cpu_time() 代用、user+sys 合算近似) | 秒 |
| sys_sec | sys CPU 時間 (Fortran は分離不可のため 0.0 固定) | 秒 |
| num_time_steps_logged | 時間積分のステップ数 (icnt) | int |
| last_sim_time | 最終 simulation 時間 (time) | float |

`#` で始まる 1 行目は CSV のコメント行 (列名)。`awk '!/^#/'` や `pandas.read_csv(comment='#')` でデータ行だけ抽出できます。

### Step 4: クリーンアップ

```bash
# CMake
rm -rf build-miyabi-uni

# Make
make -f Makefile.gen.miyabi.uni clean
```

---

## ビルド方法 (例: Miyabi で Kokkos ビルド)

Kokkos は C++ performance portability ライブラリで、**ビルドは CMake 必須** (Make 非対応)。
事前に Kokkos がインストールされている必要があります。

### 前提: Kokkos のインストール例 (spack)

```bash
# CUDA + OpenMP backend で Kokkos 5.0.2 をビルド (Miyabi GH200 の場合 cuda_arch=90)
spack install kokkos +cuda +openmp +wrapper cuda_arch=90 cxxstd=17
```

インストール先 (`KOKKOS_ROOT`) を `machines/miyabi.yaml` の `kokkos.root` に記述:

```yaml
# machines/miyabi.yaml
kokkos:
  root: /xxx/kokkos/5.0.2/              # 実機の Kokkos インストール先パス
  cxx_standard: 17
```

### Step 1: 構成生成

```bash
cd apps/diffusion
./configure.py --variant C++/kokkos/range/baseline --machine miyabi --mode gpu
```

標準出力例:
```
Generated: C++/kokkos/range/baseline/CMakeLists.txt

--- Build configuration ---
  variant       : C++/kokkos/range/baseline
  machine       : miyabi  (東大 Miyabi (PBS Pro、Miyabi-G=GH200/NVHPC 24.5+、Miyabi-C=Intel Xeon))
  mode          : gpu  (gpu=GPU offload, cpu=CPU OpenMP, uni=GPU unified memory)
  language      : cpp (C++)
  implementation: kokkos
  compiler      : kokkos-managed  (Kokkos-managed)
  build system  : cmake
  precision     : FP=32 (default; cmake -DFP=64 で double 精度に変更可)
  policy        : range  (parallel_for (RangePolicy))
  memory layout : default View
  kokkos root   : /xxx/kokkos/5.0.2/
  cxx_standard  : C++17
  target binary : diffusion.gpu.<FP>
  sources       : misc.cpp diffusion.cpp main.cpp

次の手順:
  cd C++/kokkos/range/baseline
  cmake -B build-miyabi-gpu -S .
  cmake --build build-miyabi-gpu
```

### Step 2: CMake ビルド (Kokkos は cmake 強制)

```bash
cd C++/kokkos/range/baseline
cmake -B build-miyabi-gpu -S .
cmake --build build-miyabi-gpu
```

標準出力例:
```
-- Enabled Kokkos devices: OPENMP;CUDA
-- kokkos_launch_compiler is enabled globally. C++ compiler commands with -DKOKKOS_DEPENDENCE
   will be redirected to the appropriate compiler for Kokkos
-- Configuring done (0.3s)
[ 25%] Building CXX object .../misc.cpp.o
[ 50%] Building CXX object .../diffusion.cpp.o
[ 75%] Building CXX object .../main.cpp.o
[100%] Linking CXX executable diffusion.gpu.32
[100%] Built target diffusion
```

### Step 3: 実行

```bash
./build-miyabi-gpu/diffusion.gpu.32 64
```

標準出力例:
```
# binary,N(=nx*ny*nz),time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time
./build-miyabi-gpu/diffusion.gpu.32,262144, 4.500e-02, 6.205e+02, 1.146e-05, 2.500e-01, 1.700e-01, 8.000e-02, 8192, 2.000e+00
```

(参考までに Kokkos 起動時に出る情報行は Kokkos 自身のログで、`# ...` 列名行と CSV データ行とは別)

### Kokkos variant の選び方

| 名前 | policy | メモリ | 特徴 |
|---|---|---|---|
| `range/baseline` | `parallel_for` (RangePolicy) | デフォルト | 最もシンプルな出発点 |
| `range/cpu-sweep` | RangePolicy | デフォルト | compile flag sweep 用 |
| `range/uvm` | RangePolicy | **CudaUVMSpace** | UVM のオーバーヘッドを観察 |
| `range/uvm-sweep` | RangePolicy | CudaUVMSpace | UVM + sweep |
| `range/baseline-fused` | RangePolicy | デフォルト | 3 重ループを 1 重に融合 |
| `range/uvm-fused` | RangePolicy | CudaUVMSpace | UVM + 融合 |
| `mdrange/baseline` | `MDRangePolicy` | デフォルト | MDRange の baseline |
| `mdrange/cpu-tile-sweep` | MDRangePolicy | デフォルト | **`_NX/_NY/_NZ` マクロで tile size 指定** |
| `mdrange/uvm` | MDRangePolicy | CudaUVMSpace | |
| `mdrange/uvm-tile-sweep` | MDRangePolicy | CudaUVMSpace | UVM + tile macros |
| `team/baseline` | `TeamPolicy + TeamThreadRange` | デフォルト | chunk=32 ハードコード |
| `team/cpu-chunk-sweep` | TeamPolicy | デフォルト | **`_CHUNK` マクロで chunk size 指定** |
| `team/uvm` | TeamPolicy | CudaUVMSpace | |
| `team/uvm-chunk-sweep` | TeamPolicy | CudaUVMSpace | UVM + chunk macro |

注: **UVM variant は GPU mode (または `uni`) 必須** (`CudaUVMSpace` は CUDA backend 要)。
`--mode cpu` で UVM variant を指定すると `configure.py` がエラー終了します。

### Kokkos backend と実行モード

Kokkos は **インストール時に決まった backend** で動作します:
- `+cuda +openmp` でインストール → CUDA (GPU) と OpenMP (CPU) の両方が使える
- 実行時の選択: `Kokkos::initialize()` がデフォルトで CUDA を選ぶ。`OMP_NUM_THREADS` 等で OpenMP 並行制御

`--mode gpu`/`--mode cpu`/`--mode uni` は本リポジトリでは **OUTPUT_NAME のサフィックス**にだけ反映され、バイナリの動作自体は Kokkos 任せです。

### tile / chunk 等の sweep 変種

```bash
# tile sweep 例 (Kokkos mdrange)
./configure.py --variant C++/kokkos/mdrange/cpu-tile-sweep --machine miyabi --mode gpu --tile-nx 4 --tile-ny 8 --tile-nz 64
cd C++/kokkos/mdrange/cpu-tile-sweep
cmake -B build-miyabi-gpu -S . && cmake --build build-miyabi-gpu

# chunk sweep 例 (Kokkos team)
./configure.py --variant C++/kokkos/team/cpu-chunk-sweep --machine miyabi --mode gpu --chunk 128
cd C++/kokkos/team/cpu-chunk-sweep
cmake -B build-miyabi-gpu -S . && cmake --build build-miyabi-gpu
```

CMake 段で上書きする方法も可能 (`cmake -B build -S . -D_NX=4 -D_NY=8 -D_NZ=64`、`-D_CHUNK=128`)。

---

## configure.py の使い方

`configure.py` は variant / machine / mode などを受け取って、以下を variant ディレクトリに生成します:

1. **ビルドファイル**: `CMakeLists.txt` (default) または `Makefile.gen.<machine>.<mode>`
2. **ジョブスクリプト**: `run.<machine>.<mode>.sh` — machine yaml の `job:` ブロックから自動生成
   - Miyabi → PBS Pro (`#PBS` ディレクティブ + `qsub`)
   - Wisteria → PJM (`#PJM` ディレクティブ + `pjsub`)
   - local → バッチ無しの bash スクリプト

### 基本コマンド

```bash
# ヘルプ
./configure.py --help

# 使える variant / machine の一覧
./configure.py --list

# 設定の一覧表示 (machine 指定で詳細、--variant も指定で具体構成を表示)
./configure.py --info
./configure.py --info --machine miyabi
./configure.py --info --variant C++/openacc/auto.opt --machine local --mode gpu

# 例: ローカル機の GPU で OpenMP target をビルド (CMake、default)
./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu

# 例: Wisteria CPU で Fortran OpenMP CPU をビルド (GCC を明示、Make 指定)
./configure.py --variant F/openmp-cpu --machine wisteria --mode cpu --compiler gcc --build make

# 例: Miyabi で C++ stdpar の GPU unified memory ビルド
./configure.py --variant C++/stdpar/auto.def --machine miyabi --mode uni

# 内容だけ確認 (ファイル書き込まない)
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu --dry-run
```

### 引数の意味

| 引数 | 必須 | 値 | 説明 |
|---|---|---|---|
| `--variant` | yes | `C++/openmp-target/auto.def` 等 | ビルドする variant のパス |
| `--machine` | yes | `local` / `miyabi` / `wisteria` | 対象マシン (`machines/<name>.yaml`) |
| `--mode` | yes | `gpu` / `cpu` / `uni` | GPU offload / CPU OpenMP / GPU unified memory |
| `--compiler` | no | `intel` / `gcc` / `fccpx` / `frtpx` / `nvhpc` | CPU compiler 上書き (cpu mode のみ) |
| `--build` | no | `cmake` (default) / `make` | ビルドシステム。**Kokkos は cmake 強制** |
| `--fp` | no | `32` / `64` | 浮動小数精度のデフォルト値を上書き (cmake -DFP=N でも可) |
| `--nthreads` | no | int | NTHREADS のデフォルト値 (omp-target/openacc の opt variant のみ実効) |
| `--tile-nx` / `--tile-ny` / `--tile-nz` | no | int | Kokkos mdrange tile-sweep の `_NX/_NY/_NZ` |
| `--chunk` | no | int | Kokkos team chunk-sweep の `_CHUNK` |
| `--opt-level` | no | `O0`〜`O4` / `fast` | ベース最適化レベル (`-O3` を上書き; 非 Kokkos のみ) |
| `--extra-cflags` | no | str | 任意の追加 compile flag (例: `-xCORE-AVX512 -march=native`; 非 Kokkos のみ) |

### CLI オプションと variant 種別の対応表

オプションごとに使える variant が分かれます。**対応しない組み合わせは `configure.py` が ERROR で拒否します** (静かに無視はしない)。

| オプション | 非 Kokkos<br>(omp-cpu/target, acc, stdpar, do-concurrent) | Kokkos<br>(range/mdrange/team) | 理由 |
|---|:---:|:---:|---|
| `--fp 32\|64` | ✓ 全 variant | ✓ 全 variant | FP マクロは misc.h で全 variant 共通 |
| `--nthreads N` | ✓ **`auto.opt` / `manu.opt` の `openmp-target` / `openacc` のみ** | ✗ | `vector_length(NTHREADS)` を source で使うのが opt variant の omp-target/openacc のみ |
| `--tile-nx/ny/nz N` | ✗ | ✓ **`mdrange/cpu-tile-sweep`, `mdrange/uvm-tile-sweep` のみ** | source の `_NX/_NY/_NZ` macro はこの 2 variant のみ |
| `--chunk N` | ✗ | ✓ **`team/cpu-chunk-sweep`, `team/uvm-chunk-sweep` のみ** | source の `_CHUNK` macro はこの 2 variant のみ |
| `--opt-level O0..O4\|fast` | ✓ 全 variant | ✗ | Kokkos は **インストール時** に opt が決定 (`spack install kokkos ... cflags=...`) |
| `--extra-cflags "..."` | ✓ 全 variant | ✗ | 同上 (Kokkos インストール時に決定) |

#### 対応しない組み合わせを指定したときの ERROR 例

```
$ ./configure.py --variant C++/kokkos/range/baseline --machine local --mode gpu --opt-level fast
ERROR: --opt-level は非 Kokkos variant のみ実効です。variant 'C++/kokkos/range/baseline' (Kokkos) では指定不可。
       Kokkos の最適化は spack install kokkos ... cxxstd=N cflags='...' 等でインストール時に決定されます。
       対応 variant: 非 Kokkos (openmp-cpu, openmp-target, openacc, stdpar, do-concurrent)

$ ./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu --nthreads 256
ERROR: --nthreads は variant 'C++/openmp-target/auto.def' では実効しません。
       対応: openmp-target/openacc の auto.opt または manu.opt のみ。

$ ./configure.py --variant C++/openacc/auto.def --machine local --mode gpu --tile-nx 4
ERROR: --tile-nx は Kokkos の tile-sweep variant のみ実効。variant 'C++/openacc/auto.def' (非 Kokkos) では指定不可。
       対応 variant: C++/kokkos/mdrange/cpu-tile-sweep, C++/kokkos/mdrange/uvm-tile-sweep
```

Kokkos 側で最適化レベルや arch flag を変えたい場合は、**Kokkos を spack で再 install** が正攻法です:

```bash
# 例: Hopper 向け Kokkos、C++17 + 特定の cflags で再ビルド
spack install kokkos +cuda +openmp +wrapper cuda_arch=90 cxxstd=17 cflags="-O2"
```
| `--list` | no | — | 使用可能な variant / machine を表示 |
| `--info` | no | — | 設定一覧 (machine, compiler, 最適化, 精度) を 3 階層で表示 |
| `--dry-run` | no | — | 生成内容を stdout に表示、書き込まない |

### CLI オプション例 (精度・最適化を直接指定)

```bash
# 倍精度 (FP=64) でビルド構成生成
./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu --fp 64
# → バイナリ名: diffusion.gpu.64

# NTHREADS=256 を default に
./configure.py --variant C++/openacc/auto.opt --machine local --mode gpu --nthreads 256
# → バイナリ名: diffusion.gpu.32.256

# 倍精度 + NTHREADS の組み合わせ
./configure.py --variant C++/openmp-target/auto.opt --machine local --mode gpu --fp 64 --nthreads 256
# → バイナリ名: diffusion.gpu.64.256

# Kokkos tile sweep のデフォルト tile size を指定
./configure.py --variant C++/kokkos/mdrange/cpu-tile-sweep --machine local --mode gpu --tile-nx 4 --tile-ny 8 --tile-nz 64

# Kokkos chunk sweep のデフォルト chunk size を指定
./configure.py --variant C++/kokkos/team/cpu-chunk-sweep --machine local --mode gpu --chunk 256

# ベース最適化レベルを変更 (default は machine yaml の common_flags の -O3)
./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu --opt-level fast
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu --opt-level O2

# arch / vector ext 等の任意フラグを追加 (元 hairdesc_diffusion の range/cpu-sweep 相当)
./configure.py --variant C++/openacc/auto.def --machine miyabi --mode cpu --compiler intel \
    --extra-cflags="-xCORE-AVX512"
./configure.py --variant C++/openacc/auto.def --machine miyabi --mode cpu --compiler intel \
    --opt-level O3 --extra-cflags="-march=sapphirerapids -funroll-loops"
```

これらのオプションは生成される CMakeLists.txt の `CACHE STRING` 既定値や `CMAKE_<LANG>_FLAGS` を変えるだけなので、cmake -D で後から上書きも可能です (`--opt-level` と `--extra-cflags` は CMake 変数化していないので configure.py 再実行で変更)。

**注意**: `--opt-level` と `--extra-cflags` は **Kokkos 以外** で実効します。Kokkos は **インストール時に最適化が決定**するため (`spack install kokkos +cuda +openmp ... cxxstd=20` 等)、configure.py からの上書きは無効です (警告を表示)。

### machine 情報の自動検出 (`local` のみ)

`--machine local` 指定時は、実行ホストから OS / CPU / GPU / NVHPC バージョンを自動検出して表示します:

```
$ ./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu
...
  machine       : local  (detected: Ubuntu 24.04.4 LTS | AMD EPYC 9135 16-Core Processor (16 cores) | GPU: NVIDIA RTX PRO 4000 Blackwell, 12.0 | NVHPC 26.3-0)
```

リモート機 (`miyabi`, `wisteria`) は yaml の `description` を使います (ログイン前提でローカルから検出できないため)。

---

## 性能評価

ビルドして実行した結果は `summary/diffusion.csv` に統合されており、[ダッシュボード](summary/diffusion_dashboard.html) からインタラクティブに閲覧できます。

### ダッシュボードの主な特徴

| 機能 | 説明 |
|---|---|
| **連動フィルタ** | category / machine / mode / language / impl / variant / memory_model / optimization_type / fp の 10 軸 |
| **件数表示** | 各フィルタ項目に該当行数 `(N)` を表示。0 件の項目は自動的に灰色化 |
| **X/Y 軸選択** | time_sec, performance_gflops, error, real_sec, N, nx 等から自由選択 |
| **色分け** | impl, variant, memory_model, optimization_type 等で系列を分けられる |
| **グラフ種別** | 散布図 / 折れ線 / 棒 / 箱ひげ / ヒストグラム |
| **対数軸切替** | X/Y それぞれ線形/対数 |

### 既測データの概要

| 項目 | 値 |
|---|---|
| 統合 CSV | `summary/diffusion.csv` (168 KB、907 行) |
| 非 Kokkos 行 | 642 (mizuho 由来、Miyabi/Wisteria 実機計測) |
| Kokkos 行 | 265 (kokkos_combined.csv 由来) |
| grid サイズ N | 非 Kokkos: nx=32/64/128/256/512、Kokkos: nx=128 のみ |
| machine | miyabi, wisteria |
| 精度 | FP=32 / FP=64 |
| impl 種類 | openmp-cpu, openmp-target, openacc, stdpar, do-concurrent, kokkos (6 種) |

### 統合 CSV のカラム (17 列)

```
category, machine, mode, language, impl, variant, memory_model,
optimization_type, optimization_param, fp, nx, ny, nz, N,
time_sec, performance_gflops, error
```

加えて `real_sec, user_sec, sys_sec, source_file` の 4 列 (Kokkos 行のみ実値、mizuho 行は空)。

### ダッシュボードの典型的な使い方

| 観察したいこと | 操作 |
|---|---|
| 「全 impl の性能スケーリング (N→GFLOPS)」 | X=N (log), Y=performance_gflops (log), 色=impl |
| 「FP32 vs FP64 の精度比較」 | X=fp, Y=error (log), 色=impl, グラフ=box |
| 「mdrange の tile size の影響」 | filter=impl:kokkos+variant:mdrange/cpu-tile-sweep、X=optimization_param、Y=performance_gflops |
| 「machine 別の baseline 比較」 | filter=optimization_type:baseline、X=impl、Y=performance_gflops、グラフ=bar |
| 「auto.def vs manu.def のメモリモデル効果」 | filter=variant:{auto.def, manu.def}、X=N、Y=performance_gflops、色=memory_model |

### ダッシュボードの再生成

新しい計測データを `summary/diffusion.csv` に追加した後は、以下で HTML を再生成:

```bash
cd apps/diffusion/summary
python3 visualize.py
# → diffusion_dashboard.html (同ディレクトリ) が更新される
# ブラウザが自動で開く (失敗時は手動でファイルを開く)
```

別の CSV / 出力先を指定:

```bash
python3 apps/diffusion/summary/visualize.py path/to/data.csv path/to/dash.html
```

---

## エラー時の挙動

`configure.py` は不整合を検出すると使用可能な選択肢を列挙してエラー終了する:

| エラー | 例 |
|---|---|
| 未登録 compiler | `compiler 'intel' は machine 'wisteria' に定義されていない。使用可能: ['gcc', 'fccpx', 'frtpx']` |
| 言語非対応 compiler | `compiler 'fccpx' は Fortran をサポートしない。Fortran に使えるのは: ['gcc', 'frtpx']` |
| machine が mode 未サポート | `mode 'uni' は machine 'wisteria' でサポートされない。使用可能: ['gpu', 'cpu']` |
| impl と mode の不整合 | `impl 'openmp-cpu' は mode 'gpu' でビルドできない。サポート: ['cpu']` |
| Kokkos UVM を CPU mode で | `kokkos uvm variant (range/uvm) は GPU mode のみ` |
| Kokkos に make 指定 | `kokkos は CMake 強制 (--build cmake のみサポート)` |
| 不正な variant 形式 | `--variant の形式は 'C++/openacc/auto.def' / 'C++/openmp-cpu' / 'C++/kokkos/range/baseline'` |

---

## マシン × compiler の対応表

| machine | default compiler | 使用可能 compiler | GPU compiler | サポート mode |
|---|---|---|---|---|
| `local` | gcc | gcc, nvhpc | nvhpc (auto-detect) | gpu, cpu, uni |
| `miyabi` | intel | intel, gcc | nvhpc 24.5+ (GH200) | gpu, cpu, uni |
| `wisteria` | gcc | gcc, fccpx (C/C++), frtpx (Fortran) | nvhpc 24.1 (A100) | gpu, cpu |

`uni` (unified memory) は GPU mode の派生。NVHPC 24.1 では未対応のため Wisteria では使えない。
Kokkos は machine yaml に `kokkos.root` (インストール先パス) と `cxx_standard` を要設定。

---

## 動作確認済みの組み合わせ

| variant | machine | mode | build | 結果 |
|---|---|---|---|---|
| `C++/openmp-target/auto.def` | local | gpu | cmake | ✅ OpenMP target offload (RTX PRO 4000) |
| `C++/openmp-target/auto.def` | local | gpu | make  | ✅ 同等動作 |
| `C++/openmp-cpu`             | local | cpu | cmake | ✅ `FP=32` / `FP=64` 両方検証 |
| `F/openmp-cpu` | local | cpu | cmake | ✅ Fortran modules を CMake が解決 |
| `F/openmp-cpu` | local | cpu | make  | ✅ NVHPC multicore |
| `C++/kokkos/range/baseline`   | local | gpu | cmake | ✅ Kokkos 5.0.2 (CUDA+OpenMP) |
| `C++/kokkos/range/uvm`        | local | gpu | cmake | ✅ CudaUVMSpace |
| `C++/kokkos/mdrange/cpu-tile-sweep` | local | gpu | cmake | ✅ tile macros 既定 `_NX=2 _NY=4 _NZ=16` |
| `C++/kokkos/team/cpu-chunk-sweep` | local | gpu | cmake | ✅ `_CHUNK=32` 既定 |

ドライラン全数検証 (非Kokkos): **396 組み合わせ**、OK 212 / SKIP 184 / FAIL 0
Kokkos variant: 14 種、いずれも cmake 強制で生成可能。

---

## 詳しく知りたい人向け

各 impl の固有事情は impl ディレクトリ配下の README を参照:

- C++ — `C++/openmp-cpu/README.md`, `C++/openmp-target/README.md`, `C++/openacc/README.md`, `C++/stdpar/README.md`, `C++/kokkos/README.md`
- Fortran — `F/openmp-cpu/README.md`, `F/openmp-target/README.md`, `F/openacc/README.md`, `F/do-concurrent/README.md`
- Kokkos の policy 別 — `C++/kokkos/range/README.md`, `C++/kokkos/mdrange/README.md`, `C++/kokkos/team/README.md`

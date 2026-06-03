# nbody — direct N-body 重力多体問題

直接法重力多体計算 (2nd-order leapfrog 軌道積分) を GPU プログラミングモデルの横断比較のために再構成したアプリ。同じ物理問題を 5 つの方式で実装し、性能と精度 (energy_error / virial_ratio) を比較できます。

| 方式 | C/C++ | Fortran | 特徴 |
|---|:---:|:---:|---|
| OpenMP CPU | ✓ (C++) | ✓ | OpenMP の CPU 並列ベースライン |
| OpenMP target | ✓ (C++) | ✓ | `omp target` で GPU 化 |
| OpenACC | ✓ (C++) | ✓ | `acc parallel loop` (NVIDIA 中心) |
| 言語標準 stdpar | ✓ (C++) | — | C++17 `std::execution::par` |
| 言語標準 do concurrent | — | ✓ | Fortran 2008 `do concurrent` |
| Kokkos | ✓ (C++) | — | C++ ライブラリ。CPU/GPU portability |

---

## このアプリで学べること

- 同じ重力多体計算を 5 つのプログラミングモデルで書き分けたときの差分
- **compute-bound** な処理 (O(N^2) の粒子相互作用) で GPU のスループットがどう効くか — diffusion (memory-bound stencil)、fem (sparse SpMV) と相補的
- **混合精度** (FP_L = 低精度部、FP_M = 標準精度部、FP_H = 高精度部) による速度 vs 精度トレードオフ
- leapfrog 積分の保存量 (エネルギー、virial 比) で物理的妥当性を確認

### スコープ

精度は **FP32 / FP64 / 混合 (32+64)** の 3 通り。`FP_H = 64` (高精度部、保存量計算等) は常に FP64 固定。

---

## はじめての方へ (5 分で動かす)

### 必要なもの

| 項目 | 必須/任意 | 備考 |
|---|---|---|
| Linux | 必須 | WSL2 でも可 |
| 動作検証済みスパコン: Miyabi / Wisteria-A | — | ビルド〜ジョブ実行まで確認済み (`machines/{miyabi,wisteria}.yaml`) |
| NVIDIA HPC SDK (NVHPC) 24.1 以降 | 必須 (GPU 系) | `nvc++` 等 |
| **HDF5** 1.14 以降 | 必須 | 出力ファイル形式 |
| **Boost** 1.83 以降 | 必須 | program_options, filesystem, system, timer, chrono |
| CMake 3.20 以降 | 必須 | `apt install cmake` |
| Python 3.6 以降 + PyYAML | 必須 | configure.py 用 |
| Kokkos 4.x / 5.x | 任意 | Kokkos 実装を試したい場合 |

### 最短経路 (ローカル GPU で OpenACC を動かす)

```bash
# 1. アプリのルートに移動
cd apps/nbody

# 2. 構成を生成
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu

# 3. ビルド (Boost と HDF5 がリンクできる前提)
cd C++/openacc/auto.def
cmake -B build-local-gpu -S .
cmake --build build-local-gpu

# 4. 実行 (boost_program_options で引数指定)
./build-local-gpu/nbody.gpu.32.32 --num=1024 --finish=10.0 --eps=0.03125 --eta=0.5 --interval=1.0
```

---

## ディレクトリ構成

```
apps/nbody/
├── README.md                           # ← このファイル
├── app.yaml                            # ソース・include_dirs・libs・defines_default
├── implementations.yaml                # impl × variant の軸定義
├── configure.py                        # nbody 拡張版 (FP_L/FP_M、include_dirs、defines_default 対応)
├── machines/                           # マシン固有設定 (HDF5/Boost module 含む)
│   ├── miyabi.yaml
│   ├── wisteria.yaml
│   └── local.yaml
├── C++/                                # C/C++ 実装
│   ├── openmp-cpu/                     # baseline (src/base/, src/common/, src/util/)
│   ├── openacc/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── openmp-target/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── stdpar/{auto.def,auto.opt}/
│   └── kokkos/
│       ├── range/{baseline, cpu-sweep, uvm, uvm-sweep}/
│       ├── mdrange/{baseline, cpu-tile-sweep, uvm, uvm-tile-sweep}/
│       └── team/{baseline, cpu-chunk-sweep, uvm, uvm-chunk-sweep}/
├── F/                                  # Fortran 実装
│   ├── openmp-cpu/
│   ├── openacc/{auto.def,auto.opt,manu.def,manu.opt}/
│   ├── openmp-target/{auto.def,auto.opt,manu.def,manu.opt}/
│   └── do-concurrent/{auto.def,auto.opt}/
├── results/                            # 読者の実行結果出力先 (空)
│   └── {miyabi,wisteria,local}/
└── summary/                            # 既測データ + 可視化 (nbody 独立)
    ├── nbody.csv                       # 既測 (8356 行、25 列)
    ├── nbody_dashboard.html
    ├── parse_logs.py                   # results/*.log → nbody.csv 形式に変換
    ├── visualize.py                    # ダッシュボード生成器
    ├── legacy-csv/                     # 古いバージョンの CSV (nbody_v3-v6 等)
    ├── results/{miyabi,wisteria}/      # 既測ログ
    └── orig-scripts/                   # 元プロジェクトのジョブスクリプト類
```

### ソース構造 (diffusion/fem と異なる)

nbody は単一コンパイル単位 `base/nbody.cpp` (または `base/nbody.f90`) と **ヘッダ多数** (`common/*.hpp`, `util/*.hpp`) で構成。configure.py の `include_dirs` で `src/`, `src/common/`, `src/util/` を include path に追加します。

---

## 混合精度 (nbody 固有)

nbody は 3 種類の浮動小数精度を使い分けます (define で制御):

| マクロ | 意味 | 既定値 |
|---|---|---|
| `FP_L` | 重力計算の一部分のみ (low precision) | 32 |
| `FP_M` | 粒子データの格納，軌道計算，重力計算の一部等 (mid precision) | 32 |
| `FP_H` | 保存量計算等 (high, 固定) | 64 |

組合せ例:
- 単精度: `--fp 32` (FP_L=FP_M=32)
- 倍精度: `--fp 64` (FP_L=FP_M=64)
- **混合**: cmake -DFP_L=32 -DFP_M=64 で生成後に上書き

バイナリ名は `nbody.{mode}.{FP_L}.{FP_M}` (例: `nbody.gpu.32.64`)。

---

## variant 命名の意味

`<lang>/<impl>/<variant>` の階層 (Kokkos のみ 4 階層):

- `<impl>` = `openmp-cpu` / `openmp-target` / `openacc` / `stdpar` / `do-concurrent` / `kokkos`
- `<variant>` (非 Kokkos) = `auto.def` / `auto.opt` / `manu.def` / `manu.opt`
- `<policy>/<sub>` (Kokkos) = `{range,mdrange,team}/{baseline, cpu-sweep/tile/chunk, uvm, uvm-...}` (sub 名は policy 別)

旧 nbody の `A1-F4` 表記は次のように対応:

| 旧 ID | 新 (non-Kokkos) | 新 (Kokkos) |
|---|---|---|
| A1 | `C++/openacc/auto.def` | `C++/kokkos/range/baseline` |
| A2 | `C++/openacc/auto.opt` | `C++/kokkos/range/cpu-sweep` |
| A3 | `C++/openacc/manu.def` | `C++/kokkos/range/uvm` |
| A4 | `C++/openacc/manu.opt` | `C++/kokkos/range/uvm-sweep` |
| B1-B4 | `C++/openmp-target/{auto.def, auto.opt, manu.def, manu.opt}` | `C++/kokkos/mdrange/{baseline, cpu-tile-sweep, uvm, uvm-tile-sweep}` |
| C1-C4 | `C++/stdpar/{auto.def, auto.opt}` (2 only) | `C++/kokkos/team/{baseline, cpu-chunk-sweep, uvm, uvm-chunk-sweep}` |
| D1-D4 | `F/openacc/{auto.def, auto.opt, manu.def, manu.opt}` | — |
| E1-E4 | `F/openmp-target/{auto.def, ...}` | — |
| F1-F2 | `F/do-concurrent/{auto.def, auto.opt}` | — |

---

## 出力フィールド (25 列 CSV)

| カラム | nbody での意味 | 単位 |
|---|---|---|
| category, machine, mode, language, impl, variant, memory_model, optimization_type, optimization_param, fp, N, time_sec, performance_gflops, error, real_sec, user_sec, sys_sec, source_file | (diffusion/fem と共通の 18 列) | — |
| **`energy_error_final`** | 最終エネルギー誤差 (相対) | float |
| **`virial_ratio_final`** | virial 比 (理想は 0.5) | float |
| **`dt`** | 時間刻み | float |
| **`energy_error_worst`** | シミュレーション中の最悪エネルギー誤差 | float |
| **`interactions_per_sec`** | 粒子間相互作用処理数/秒 (nbody 性能の主要メトリクス) | int |
| **`step`** | 時間ステップ数 | int |
| **`time_per_step_sec`** | 1 step あたりの平均時間 | 秒 |

太字は nbody 固有 (diffusion/fem には無い列)。物理保存量で計算の妥当性を確認できます。

---

## machines yaml の特徴 (HDF5 / Boost 追加)

```yaml
job:
  per_mode:
    gpu: { queue: debug-g, omp_threads: 72, modules: ["nvidia/25.9", "hdf5/1.14.6"] }
    # Wisteria は boost も明示:
    # gpu: { queue: debug-a, ..., modules: ["nvidia", "hdf5/1.14.6", "boost/1.83"] }
```

Miyabi の Boost は default で見える前提、Wisteria は `boost/1.83` module を明示。

---

## configure.py 使い方 (nbody 拡張)

diffusion/fem と基本的に同じ。以下が nbody 固有:

```bash
# 単精度 (デフォルト、FP_L=FP_M=32)
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu

# 倍精度 (FP_L=FP_M=64)
./configure.py --variant C++/openacc/auto.def --machine local --mode gpu --fp 64

# 混合精度 (FP_L=32, FP_M=64) — cmake -D で上書き
cmake -B build-local-gpu -S . -DFP_L=32 -DFP_M=64
```

将来的に configure.py に `--fp-low` / `--fp-mid` オプションを追加予定。

---

## エラー時の挙動

`configure.py` は diffusion/fem と共通の検証ロジックを持ちます。nbody 固有としては:

- **Boost / HDF5 が見つからない**: cmake configure 時に `find_package(Boost ...)` 等のエラー → modules または環境変数で path を通す
- **混合精度の数値発散**: FP_L=32 で 大規模 N (10^6 以上) を実行すると累積誤差で energy_error が悪化することあり

---

## 性能評価 (ダッシュボード)

```bash
cd apps/nbody/summary
python3 visualize.py
# → nbody_dashboard.html (同ディレクトリ) が生成される
```

25 列の CSV から:
- X 軸 = `N` (log)、Y 軸 = `performance_gflops` (log)、色分け = `impl` で **scaling plot**
- X 軸 = `N`、Y 軸 = `energy_error_final` (log)、色分け = `fp` で **精度比較**
- フィルタで category=Kokkos と category=non-Kokkos の切替

---

## 詳しく知りたい人向け

- diffusion (`apps/diffusion/README.md`) / fem (`apps/fem/README.md`) と相補的。3 アプリで GPU プログラミングモデルの memory-bound / compute-bound / mixed 特性を一通り体験できる
- nbody の元コード: `hairdesc_nbody_cpp/`, `hairdesc_nbody_f90/`, `hairdesc_nbody_kokkos/` (リポジトリ親階層)

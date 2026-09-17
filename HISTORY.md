# 変更履歴・コードとデータの由来

各アプリの `README.md` と `docs/build-tutorial.html` は **現在の実装がどうなっているか**
だけを説明します。「以前はこうだった」「ここを変更した」という履歴はこのファイルに
分離してあります。実装を使うだけならこのファイルを読む必要はありません。

---

## 1. コードの由来

3 アプリ (diffusion / fem / nbody) は、それぞれ既存の実装を出発点に、
GPU プログラミングモデル比較の教材として再整備したものです。

| アプリ | 出発点 |
|---|---|
| diffusion | 3 次元拡散方程式の有限差分ベンチマーク |
| fem | 3 次元熱伝導 FEM (CG ソルバ)。`mizuho` 由来のコード |
| nbody | 直接法重力多体計算 |

共通して次の整備を行いました。

- `configure.py` による machine × mode × variant からのビルド設定生成
- `BENCHMARK_MODE` 時の 10 列 CSV 出力 (3 アプリ共通スキーマ)
- 同一問題を 5〜6 種のプログラミングモデルで実装した variant 群の整理

---

## 2. fem: 元コードからの変更

### ビルドを通すための修正

| ファイル | 修正内容 | 理由 |
|---|---|---|
| `F/*/src/pfem_fem_util.f` | `use pfem_util` のみとし、配列再宣言を削除 | NVHPC は use-associated symbol の再宣言を許可しない |
| `F/*/src/mat_ass_init.f` | `use pfem_all` → `use pfem_util + use pfem_fem_util` | `pfem_all` モジュールは存在しない (元コードのタイポと思われる) |

### 出力スキーマの統一

| ファイル | 修正内容 | 理由 |
|---|---|---|
| `C++/*/src/test1.c`, `F/*/src/test1.f` | `BENCHMARK_MODE` 時の出力を 10 列 CSV に変更 | diffusion と統一スキーマ。`real_sec`/`user_sec`/`sys_sec`/`ITERactual`/`mat_sec` を出力 |
| `C++/kokkos/*/*/src/{test1,solver_CG,solve11}.cpp`, `pfem_util.h` | Kokkos 版にも同じ 10 列 CSV 出力を追加 (`FLOP` / `RESIDactual` グローバルを新設) | Kokkos だけ CSV を出さず、GFlop/s を後段で解析式から復元する必要があった。式の取り違えで性能値が約 1/4.6 にずれた行が実在したため、コード側で FLOP を数えて直接出力する形にした |

### 反復回数・残差の報告

`CG()` は `ITER` / `RESID` を値渡しで受け取るためローカル shadow になり、収束した
反復数・残差が呼び出し元に戻りませんでした。CG 末尾でグローバル `ITERactual` /
`RESIDactual` に保存する形に統一しています (C / Fortran / Kokkos 共通)。
あわせて `solve11` が `ITERactual` を最大反復数の設定値で上書きしていた行を削除しました。

この修正前は、収束に失敗した run でも CSV には `error = 1.0e-08` (収束判定値)、
`num_time_steps_logged = 2000` (最大反復数) が出力され、成功した run と区別できませんでした。

### `C++/openmp-cpu` の収束不安定 (修正済み)

`C++/openmp-cpu/src/solver_CG.c` は CG 反復の全体を 1 つの `#pragma omp parallel`
領域で囲み、内側を `#pragma omp for` で分担する構成です。この中で共有スカラ
`RHO` / `C1` / `DNRM2` のゼロ初期化が **同期なしで全スレッドから実行**されていました。

```c
  RHO= 0.e0;                        /* 全スレッドが実行 (バリア無し) */
#pragma omp for reduction(+:RHO)    /* 先行スレッドはここで RHO に加算し終える */
  for(i=0;i<N;i++) RHO += ...;
```

`#pragma omp for` は**終了時**にのみ暗黙バリアを持ち、開始時には持ちません。
そのため先行スレッドがリダクション結果を共有 `RHO` に加算した後に、遅れた
スレッドが `RHO = 0.e0` を実行して結果を消す競合が発生します。内積が壊れると
CG の探索方向が破綻し、収束しません。

症状は実行ごとに変わり、8 スレッドで 10 回試すと 6〜7 回が最大反復数で打ち切り
(残差 1e+01〜1e+05 まで発散)、1 スレッドでは常に 197 反復で収束していました。
GPU 版や Fortran 版は各ループが独立した `omp target` / `!$omp parallel do` であり、
初期化が逐次部で行われるため影響を受けません。

修正は次の 2 点です。

- `RHO` / `C1` / `DNRM2` のゼロ初期化を `#pragma omp single` (暗黙バリア付き) で保護
- `ALPHA` / `BETA` を `private` 化 (領域内で全スレッドが同じ値を書くだけのため)

修正後は 8 スレッド・72 スレッドとも 10 回連続で 197 反復・残差 8.292404e-09 に収束し、
1 スレッドの結果と完全に一致します。

### メッシュの内部生成

以前は制御ファイル `INPUT.DAT` とメッシュファイル `cube.0` (65³ 節点、45 MB) を
読み込む方式でした。メッシュが完全に規則的な構造格子で 1 辺の節点数から決定できるため、
diffusion / nbody と同じく **引数だけで問題が決まる**形式に変更し、立方体メッシュは
実行時に内部生成するようにしました。生成されるデータは旧 `cube.0` と完全に同一です
(節点座標・要素結合・節点グループの全要素を照合して確認)。

---

## 3. nbody: 外部ライブラリ依存の廃止

以前は Boost (program_options / filesystem / system / timer / chrono) と HDF5 が
必須でしたが、diffusion・fem と揃えて外部ライブラリ依存をゼロにしました。

| 旧 | 新 |
|---|---|
| `boost::program_options` | 自作の引数パーサ (`common/cfg.hpp`) |
| `boost::timer::cpu_timer` | `std::chrono::steady_clock` と `getrusage` (`util/timer.hpp`) |
| `boost::filesystem` | POSIX の `mkdir` / `stat` (`common/io.hpp`) |
| `boost::iterators::counting_iterator` | `util::counting_iterator` (`util/counting_iterator.hpp`、stdpar 用) |
| `boost::math::constants::two_pi` | `2.0 * M_PI` |
| HDF5 | スナップショット出力ごと削除 (`util/hdf5.hpp` も廃止) |

計測値は変わりません。`cpu_timer` の wall clock は `steady_clock` と、user CPU 時間は
`getrusage` の `ru_utime` と同じものです。

HDF5 によるスナップショット出力 (旧 `dat/*.h5` と `*.xdmf`) は `BENCHMARK_MODE` を
定義しないビルドでのみ有効な経路で、`configure.py` は常に `BENCHMARK_MODE` を付けるため
本教材のビルドでは一度も実行されておらず、コードごと削除しました。

### Fortran 版の純 Fortran 化

以前の nbody の Fortran 版は物理計算の本体が C++ にあり、Fortran は `iso_c_binding` を
通じて呼び出すだけの混成ビルドでした。diffusion と fem の Fortran 実装が Fortran だけで
閉じているため、nbody も揃えて C++ 側の実装を Fortran に移しました。

---

## 4. diffusion / fem: Kokkos 実装の精度切り替え

`cmake -DFP=32` / `-DFP=64` は生成バイナリ名 (`diffusion.gpu.64` 等) を変えていましたが、
**Kokkos 実装だけが `FP` マクロを参照しておらず、計算精度が切り替わっていませんでした。**
非 Kokkos 実装はいずれも `#if FP == 32 / 64` の typedef 切り替えを持っており、
Kokkos だけが取り残されていた形です。

| アプリ | 修正前の Kokkos | 実際の計算精度 |
|---|---|---|
| diffusion | `float` を 5 ファイル × 14 variant にハードコード | 常に単精度 |
| fem | `precision.h` が `#define KREAL double` 固定 | 常に倍精度 |
| nbody | `common/type.hpp` が非 Kokkos とバイト単位で同一 | 正しく切り替わる (問題なし) |

### 発見のきっかけ

FP64 として計測したデータが FP32 と性能・誤差ともに完全に一致することから判明しました。

- diffusion: FP32 と FP64 で誤差が同一 (`4.467876e-06`)
- fem: FP32 と FP64 で CG 到達残差が同一 (`8.292404e-09`)

### 修正内容

非 Kokkos と同じ typedef 切り替えを Kokkos 側にも実装しました。

- **diffusion** (14 variant): `misc.h` に `#if FP == 32/64/128` の `flt` typedef を追加し、
  `diffusion.h` / `diffusion.cpp` / `misc.cpp` / `main.cpp` の `float` を `flt` に置換。
  `diffusion.h` と `diffusion.cpp` には `misc.h` の include を追加。
  `fminf` や `1.0F` といったリテラルは非 Kokkos 実装と同じく据え置き。
- **fem** (12 variant): `precision.h` に `KREAL` の切り替えを追加。あわせて
  `solver_CG.cpp` のリダクション変数 (`BNRM2` / `DNRM2` / `C1` / `RHO` / `ALPHA` / `BETA`) を
  `double` 固定から `KREAL` に変更 (非 Kokkos と同じ型)。これを直さないと FP32 ビルドで
  `parallel_reduce` のラムダ引数と結果変数の型が食い違いコンパイルできません。

### 検証

| アプリ | 修正前と同じ精度側 | もう一方の精度 | 判定 |
|---|---|---|---|
| diffusion (N=262144) | FP32 誤差 `1.146017e-05` = 既存データと一致 | FP64 誤差 `1.448939e-07` | 退行なし・切り替え成功 |
| fem (N=274625) | FP64 は 197 反復・残差 `8.292404e-09` = 既存と一致 | FP32 は 266 反復・残差 `8.915650e-09` | 同上 |

修正後の Kokkos FP64 の誤差は非 Kokkos FP64 とほぼ一致します
(diffusion N=2097152: Kokkos `4.691762e-06` / 非 Kokkos `4.690441e-06`)。
fem では FP32 が FP64 の 1.25〜2.9 倍の性能という妥当な関係になりました。

### データへの影響

- **diffusion**: 修正前は常に FP32 だったため、既存の `fp=32` 行 514 行はラベルどおり正しい。
  FP64 を新規に 210 行取得 (14 variant × 5 N × 3 回)。
- **fem**: 修正前は常に FP64 だったため、`fp=64` 行は正しい。一方 `fp=32` としていた
  Miyabi の 144 行は実体が FP64 の誤ラベルだったため削除し、正しい FP32 で取り直しました
  (12 variant × 4 N × 3 回)。Wisteria の Kokkos 16 行はすべて `fp=64` 表記でラベルは正しい。

---

## 5. 計測データの由来

各アプリの `summary/<app>.csv` は、複数の計測シリーズを 1 つに統合したものです。

| アプリ | 非 Kokkos 行 | Kokkos 行 |
|---|---|---|
| diffusion | 元の実機計測 (Miyabi/Wisteria) + 本リポジトリでの追試 | `kokkos_combined.csv` 由来の param sweep (両マシン、N=2097152 固定) + N 依存性追試 (FP32: Wisteria-A 8 variant / Miyabi-G 14 variant、FP64: Miyabi-G 14 variant。各 5 点 × 3 回) |
| fem | 元の実機計測 + 本リポジトリでの追試。Miyabi の CPU `openmp-cpu` 行は OpenMP 競合の修正後に取り直し (Miyabi-C / Intel / 112 スレッド、36 行)。N=2,146,689 は非 Kokkos 20 variant × FP32/64 × 3 回 = 120 行を新規取得 | 元の計測 (Miyabi 1 サイズ / Wisteria 4 variant × 4 サイズ) + Miyabi-G での N 依存性追試 (12 variant × FP32/64 × 4 サイズ × 3 回)。FP32 は精度切り替えの修正後に取り直し |
| nbody | Miyabi/Wisteria 実機計測 | 同左 |

`real_sec` / `user_sec` / `sys_sec` は、これらの値を出力するようになる前に取得された
古い行では空欄です。

### CPU 行の取り直し (fem)

`C++/openmp-cpu` の OpenMP 競合 (第 2 節参照) を修正したあと、Miyabi の CPU
`openmp-cpu` 行を取り直しました。旧行は次の理由で置き換えています。

- 旧行の `error` 列には**残差ではなく原点節点の解の値** (1.74e+03 / 1.39e+04 /
  1.11e+05) が入っており、収束したかどうかを事後に判定できなかった
- 性能値も新計測と最大 22 倍離れており (Fortran の小サイズ)、計測条件が不明

新計測は Miyabi-C (Intel Xeon、112 スレッド、icx/ifx) で FP32・FP64 × 3 サイズ ×
3 回 = 36 行を取得し、**全 36 回が収束** (残差 < 1.0e-08)、同一条件の 3 回で残差が
完全に一致することを確認済みです。`optimization_param` に `node=miyabi-c(Intel)` を
記録しています。

取り直せていないもの:

- **Wisteria の CPU `openmp-cpu` 行 6 行**: 競合の影響を受けた可能性がありますが、
  この環境から Wisteria に実行手段がありません。
- **Miyabi / Wisteria の CPU `stdpar`・`do-concurrent` 行 (計 24 行)**:
  現在の `implementations.yaml` では両 impl とも `needs_mode: [gpu, uni]` で
  cpu モードのビルドが拒否されるため、同じ構成を再現できません。
  旧行の `error` 列も解の値が入った古い系列です。
- **Kokkos の CPU 行 30 行**: 競合の影響は受けません (Kokkos は独自のソルバ実装)。
  取り直すには x86_64 向けの Kokkos ビルドが別途必要です。

### mode の定義に反する行の削除

`implementations.yaml` は当初から次の役割分担を宣言しています
(3 アプリ共通、各 README の「impl と mode の対応」節を参照)。

| impl | needs_mode |
|---|---|
| `openmp-cpu` | `cpu` |
| `openmp-target` / `openacc` / `stdpar` / `do-concurrent` | `gpu`, `uni` |
| `kokkos` | `gpu`, `cpu` |

一方 CSV には、この定義に反する行が計 1,963 行残っていました。性質の違う 2 種類です。

**(1) GPU 専用 impl の CPU 行 (1,899 行)** — `stdpar` / `do-concurrent` (3 アプリ)、
および nbody の `openacc` / `openmp-target` の CPU 計測です。現在の `configure.py` は
これらの cpu ビルドを拒否するため追試も追加計測もできず、ダッシュボード上では
「CPU の stdpar」として `openmp-cpu` と並んで見えるため、GPU 化の手段という位置づけを
誤解させる状態でした。CPU の比較対象を `openmp-cpu` に一本化するため削除しました。

**(2) `openmp-cpu` の GPU 行 (64 行、diffusion 40 / fem 24)** — `source_file` が
`Miyabi/C/log/bench.gpu.32_run.csv`、`memory_model` が `cpu-only` でありながら
性能が 320 GFlop/s (同条件の GPU 実測と同水準) でした。旧データのディレクトリ名 `C/` を
「CPU」と解釈して `impl=openmp-cpu` を推論したメタ情報の誤りで、実体は GPU の計測値です。
本来の impl を特定できないため削除しました。残していると CPU ベースラインの性能が
実際の 20 倍以上に見えてしまいます。

### nbody の CPU ベースライン計測

上記の削除にともない nbody の CPU ベースライン (`openmp-cpu`) が不在になるため、
新規に計測して追加しました。Miyabi は CPU が 2 種類あるので両方で取得しています。

| ノード | CPU | コンパイラ | スレッド数 | キュー |
|---|---|---|---|---|
| Miyabi-G | Grace (aarch64) | gcc | 72 | `debug-g` |
| Miyabi-C | Intel Xeon (x86_64) | icx / ifx | 112 | `debug-c` |

C++ と Fortran それぞれ 3 つの精度構成 (FP_L=FP_M=32 / FP_L=32,FP_M=64 / FP_L=FP_M=64)、
粒子数 1024〜4194304 の 13 点で計 12 本のジョブを実行しました。
`optimization_param` にノード種別を記録しています。

N=4194304 / FP32 での代表値は Grace が 850 GFlop/s、Intel が 681 GFlop/s で、
同条件の GPU (19〜27 TFlop/s) に対して 23〜32 倍の差という教材向きの比較になります。

### 既知のデータ品質の問題

現時点で未解消のものを記録しておきます。

- **fem**: Miyabi の Kokkos 60 行 (N=274625、FP64) の `performance_gflops` が、
  NPLU 項を含まない古い FLOP 式で算出されており、他の行の約 1/4.61 になっています。
  同一条件の solver 実時間は他の行と一致しているため、性能そのものではなく換算式の問題です。
- **fem**: `error` 列の意味が混在しています。Kokkos 行は CG の到達残差 (7.2e-09〜9.5e-09)、
  非 Kokkos 行は到達残差の行と原点節点の解の値の行 (最大 1.1e+05) が混在します。
  取り直した Miyabi の CPU 行と GPU 行は残差ですが、Wisteria の旧行は解の値です。
- **fem**: `optimization_param` 列に、他アプリではチューニング値が入るのに対し
  ノード名 (`node=miyabi-g(GH200-GPU)`) が入っています。
- **nbody**: 完全重複行が 222 種・余剰 530 行あります (すべて `time_sec` 空欄)。
  **全てが `source_file` = `legacy ...` の旧データ取り込み分**で、`results/miyabi` /
  `results/wisteria` のログ由来の行 (291 行) には重複はありません。内訳は
  `legacy B2/B4 OpenMP target` が 490 行、`legacy C2/C4 stdpar` が 40 行
  (機種ラベル別では miyabi 226 / wisteria 304)。該当行は `optimization_param` が
  すべて `-` に潰れており、元データで区別されていた最適化パラメータが取り込み時に
  失われて同一行へ縮退したものと見られます。平均を取る集計では二重計上になるため、
  `optimization_param` を復元するか legacy 行を重複除去する必要があります。
- **nbody**: `time_sec` 空欄の行が 2,652 行あります (すべて Kokkos GPU 行)。
  プログラムは実行時間を出力しないため `parse_logs.py` が埋められない列です。
- **diffusion**: Kokkos 4 行で `time_sec` / `performance_gflops` が空欄です。

### 実装側の非対称

計測ではなく実装そのものに差がある箇所です。意図的なものと未整備のものが混ざっています。

#### アプリ間で Kokkos の variant 数が違う

| アプリ | Kokkos variant 数 | 差分 |
|---|---:|---|
| diffusion | 14 | `range/baseline-fused` と `range/uvm-fused` を追加で持つ |
| fem | 12 | fused 系なし |
| nbody | 12 | fused 系なし |

diffusion のみループ融合版 (`fused`) を持ちます。fem は CG ソルバ、nbody は
相互作用計算と、融合対象のループ構造がアプリごとに異なるため、diffusion の
ステンシル計算に固有の最適化として入っているものです。
fem / nbody の `implementations.yaml` のコメントにも「diffusion にはあるが
このアプリには実装が無い」と明記してあります。

#### アプリ間で CPU ベースラインの言語が揃っている

`openmp-cpu` は 3 アプリとも C 版と Fortran 版の両方を持ちます。
nbody は計測行が無い状態が続いていましたが、今回 Miyabi の 2 ノードで取得しました
(上記「nbody の CPU ベースライン計測」参照)。

#### 出力経路はアプリ間で統一済み

以前は fem の Kokkos だけ 10 列 CSV を出さず、性能値を後段で解析式から復元して
いましたが、現在は 3 アプリとも計測値をコード側から直接出力します
(diffusion / fem は 10 列 CSV、nbody は `log/<file>_run.csv`)。

### 計測カバレッジの非対称

`README.md` は現在の実装を説明するもので、計測データがどこまで揃っているかは
このファイルに記録します。以下は実データを機械的に集計した結果です。

#### 環境由来 (計測では解消できない)

- **`uni` モードは Miyabi のみ**。Wisteria は NVHPC 24.1 に `mem:unified` が無く
  `machines/wisteria.yaml` が `modes: [gpu, cpu]` を宣言しています。実装上の差ではありません。
  該当は diffusion の 4 組、fem の 4 組 (`openmp-target` / `openacc` / `stdpar` /
  `do-concurrent` の uni)。

#### 計測が欠けている (impl, mode) — needs_mode は満たすがデータが無い

- **nbody の `uni` モード全般**: `openmp-target` / `openacc` / `stdpar` /
  `do-concurrent` の 4 組すべてで uni の計測がありません (mode は `cpu` と `gpu` のみ)。
  本リポジトリでの整理以前からの欠落で、Miyabi で流せば埋められます。

diffusion と fem には未計測の (impl, mode) はありません。

#### machine 間の非対称 (Miyabi にはあるが Wisteria に無い)

| アプリ | 対象 | 備考 |
|---|---|---|
| diffusion / fem / nbody | `uni` モード全般 | Wisteria が環境的に非対応 (上記) |
| diffusion | Kokkos の FP64 210 行 | 精度切り替えの修正後に Miyabi でのみ取得 |
| fem | `kokkos` の CPU 30 行 | Wisteria に Kokkos CPU 計測が無い |
| fem | `stdpar` の GPU 12 行 | N=2,146,689 のみ。Wisteria は uni も gpu も無い |
| nbody | `openmp-cpu` の CPU 156 行 | CPU ベースライン。Wisteria は未計測 |

逆に Wisteria にしか無い組み合わせは 3 アプリとも存在しません。

#### 同じ組み合わせでの N 点数の違い

| アプリ | 対象 | Miyabi | Wisteria |
|---|---|---:|---:|
| diffusion | `kokkos` GPU の sweep 系 6 組 | 5 点 | 1 点 |
| fem | `openacc` GPU 1 組 | 3 点 | 4 点 |
| nbody | `kokkos` CPU 12 組 | 13 点 | 10 点 |

fem の Kokkos GPU は variant 数自体が非対称で、Miyabi 12 variant × FP32/64 に対し
Wisteria は 4 variant × FP64 のみです。また fem の非 Kokkos は N=2,146,689 が
Miyabi のみ (Wisteria は OpenACC FP64 の 1 行だけ) です。

#### 計測条件が揃っていないもの

- **fem の CPU `openmp-cpu`**: Miyabi の 36 行は OpenMP 競合の修正後に
  Miyabi-C (Intel, 112 スレッド) で取り直し済みですが、Wisteria の 12 行は
  修正前の旧計測のままです。収束に失敗した run が含まれている可能性があります
  (旧計測は `error` 列に解の値が入っており事後判定ができません)。

---

## 6. variant 命名の対応表 (nbody)

計測データの `source_file` 列には旧 ID (`A1`〜`F4`) が現れます。現在の variant 名との対応:

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

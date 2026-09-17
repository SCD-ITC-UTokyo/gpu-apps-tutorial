# Miyabi (NVIDIA GH200) で追加計測が必要なデータ

`*/summary/*.csv` を Wisteria (A100) と突き合わせて洗い出した、Miyabi 側の欠測一覧。

集計時点: 2026-09-17 / 対象 CSV: `diffusion/summary/diffusion.csv`,
`fem/summary/fem.csv`, `nbody/summary/nbody.csv`

行数は (Wisteria 側の追加計測をマージした 2026-09-17 時点で)
diffusion 1,353 (Miyabi 912 / Wisteria 441) / fem 875 (684 / 191) /
nbody 7,923 (4,266 / 3,657)。Wisteria にしか無い組み合わせは 3 アプリとも存在しません。

---

## 解消済み

前回の一覧 (2026-09-16 集計) の優先度 A / B / C はすべて対応しました。

| 旧優先度 | 内容 | 対応 |
|---|---|---|
| A | diffusion Kokkos の FP64 が皆無 | **実装の不備が原因**。Kokkos が `FP` マクロを見ておらず常に単精度だった。精度切り替えを実装し、14 variant × 5 N × 3 回 = 210 行を取得 |
| B | fem OpenACC FP64 の N=2,146,689 が Wisteria のみ | C とあわせて解消 |
| C | fem の N=2,146,689 が Kokkos 以外全滅 | 非 Kokkos 20 variant × FP32/FP64 × 3 回 = 120 行を取得。全行が収束 (残差 < 1e-08) |

同じ調査で **fem の Kokkos も `FP` マクロ未対応 (常に倍精度)** と判明したため、
精度切り替えを実装のうえ FP32 を 144 行取り直しました。
詳細は [HISTORY.md](HISTORY.md) の第 4 章を参照。

---

## 優先度 A — 実装が無く計測できないもの

| アプリ | 対象 | 内容 |
|---|---|---|
| fem | `C++/openacc/manu.def`, `manu.opt` | GPU 実行時に `CUDA_ERROR_ILLEGAL_ADDRESS` で異常終了する。`solver_CG.c` の CG 関数が原因で、N=65 の小さい問題でも再現するため大 N 固有ではない。Fortran 版の `manu.*` は正常 |

これは計測ではなくコードの修正が必要です。

---

## 優先度 B — Miyabi で流せば埋まるもの

| アプリ | 対象 | 内容 |
|---|---|---|
| nbody | `uni` モード全般 | `openmp-target` / `openacc` / `stdpar` / `do-concurrent` の 4 impl すべてで uni の計測が無い (mode は cpu と gpu のみ)。diffusion / fem には uni 行がある |
| fem | `stdpar` GPU の小さい N | N=2,146,689 の 12 行のみ。N=4,913 / 35,937 / 274,625 が未計測 |
| diffusion | Kokkos FP64 の Wisteria 側 | Miyabi で 210 行取得済み。両機種比較には Wisteria でも必要 (Wisteria 側の作業) |

---

## 優先度 C — Wisteria 側の作業 (Miyabi では対応不可)

| アプリ | 対象 | 内容 |
|---|---|---|
| fem | Kokkos GPU | Miyabi 12 variant × FP32/64 に対し Wisteria は 4 variant × FP64 のみ |
| fem | Kokkos CPU | Miyabi のみ 30 行。Wisteria は 0 行 |
| fem | CPU `openmp-cpu` | Wisteria の 6 行は OpenMP 競合の修正前の計測。取り直しが必要 |
| nbody | CPU ベースライン `openmp-cpu` | Miyabi のみ 156 行 (Miyabi-G / Miyabi-C 両方)。Wisteria は 0 行 |
| diffusion | Kokkos sweep 系 6 variant の N 依存性 | Miyabi 5 点に対し Wisteria は 1 点 |

`uni` モードは Wisteria が環境的に非対応 (NVHPC 24.1 に `mem:unified` が無い) のため、
この差は計測では解消できません。

---

## 参考 — Miyabi 側で揃っているもの

- **未計測の (impl, mode) は diffusion / fem とも無し**。nbody のみ uni が未計測
- nbody: GPU 実行は Kokkos・非 Kokkos とも Wisteria と同数・同構成
- diffusion / fem: `uni` モードは Miyabi のみ存在 (diffusion 120 行 / fem 72 行)
- 各 sweep のパラメータ点数は Miyabi の方が多い

## 要確認の測定値

- diffusion Kokkos の最大 N (134,217,728, FP32) で variant 間の差が非常に大きい。
  ループ融合版 `range/baseline-fused` が 2,146 GFlop/s、タイル版
  `mdrange/cpu-tile-sweep` が 1,473 GFlop/s なのに対し、**非融合の
  `range/baseline` は 8.5 GFlop/s** と 250 倍の開きがある
  (`team/baseline` は 19.8、`mdrange/baseline` は 208.8)。
  最も遅いのは team 系ではなく非融合 range 系。
  この傾向は本リポジトリでの追試と既存データの双方で再現しており
  (N=2,097,152 で既存 Miyabi 値と 0.97〜1.09 倍で一致)、計測手順の問題ではない。
  プロファイルによる原因切り分けが望ましい

---

## 作業ツリーの同期 (2026-09-17, Wisteria 側の結果を統合したあと)

Miyabi の作業コピーを統合後の状態に更新しました。次に Miyabi で作業するときは、
この状態を持っていけば Wisteria 側の修正込みでビルドできます。

**取り込んだもの (Wisteria 由来)**

| 対象 | 内容 |
|---|---|
| `{diffusion,nbody}/C++/stdpar/auto.{def,opt}/src/` 4 ファイル | `std::execution::par` のラムダを `[&]` → `[=]` に変更。参照キャプチャだと GPU オフロードされず CPU へフォールバックしていた (Wisteria で diffusion stdpar FP32 が 2.4 → 948 GFLOPS) |
| `*/configure.py` 3 ファイル | Kokkos backend と `--mode` の整合性検査、`kokkos.root_per_mode`、言語別デフォルトコンパイラ (`default_f90`) |
| `*/machines/wisteria.yaml` 3 ファイル | Odyssey = A64FX 向け設定。**`miyabi.yaml` は変更なし** |
| `*/docs/build-tutorial.html`, `{diffusion,fem}/README.md` | 上記に対応する記述 |
| `wisteria.md` | Wisteria 側の欠測一覧と追加計測の記録 |
| `*/summary/{*.csv,*_dashboard.html,figs/,make_portability_fig.py}` | 両機種統合後の CSV・ダッシュボード・図 |

`configure.py` の Kokkos backend 検査は Miyabi でも効きます。GPU backend を持たない
Kokkos install に対して `--mode gpu` / `--mode uni` を指定するとエラーで止まり、
`--mode cpu` に GPU backend 付き install を指定すると警告が出ます。

計測ログ (`*/results/`) は転送量を抑えるため同期していません。各機種のログはそれぞれの
機種にのみ置いてあり、統合結果は `*/summary/*.csv` に入っています。

**Miyabi 側で変更なし**

`fem/C++/openmp-cpu/run_*/cube.0` の削除 (4 個, 計 180 MB) は Miyabi 側で実施済みの
ままマージされています。`miyabi.yaml` と Miyabi 用ビルド設定にも変更はありません。

**次に Miyabi で流すもの**

上記「優先度 B」のとおりです (nbody の `uni` モード、fem stdpar GPU の小さい N)。
性能可搬性図に残る "no data" 2 セルは**どちらも Wisteria 側**の作業なので、
Miyabi 側の追加計測は図の穴埋めには影響しません。

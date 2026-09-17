# GPU プログラミングモデル比較

本コード整備の一部は文部科学省「次世代HPC・AI開発支援拠点形成事業」の支援を受けています。

---

同じ数値計算問題を、複数の **GPU プログラミングモデル**
(OpenMP CPU / OpenMP target / OpenACC / 言語標準 stdpar・do concurrent / Kokkos) で実装し、
**書き方・性能・移植性を比較**するためのコード群です。問題の性質が異なる 3 つのアプリケーションを
収録しており、それぞれメモリ律速・不規則アクセス・演算律速という代表的な特性をカバーします。

ビルドは各アプリ diffusion/fem/nbody の `configure.py`（machine × mode × variant から CMake / Makefile とジョブスクリプトを生成）で行います。
動作検証済み環境: ローカル Linux (Ubuntu) / **Miyabi** (NVHPC, GH200, PBS Pro) / **Wisteria-A** (NVHPC, A100, PJM)。

---

## 3 つのアプリケーションと問題設定

### 1. diffusion — 3 次元拡散方程式（有限差分法）
3 次元拡散方程式 **∂f/∂t = κ ∇²f** を有限差分法（7 点ステンシル）で陽的に時間積分するベンチマーク。
Dirichlet 境界条件、精度は FP32 / FP64 切替可。

### 2. fem — 3 次元有限要素法（CG ソルバ）
3 次元の熱伝導／構造解析問題を有限要素法で離散化し、**共役勾配法 (Conjugate Gradient)** で解くアプリ。

### 3. nbody — 直接法重力多体問題
直接法による重力多体計算を、時間 2 次精度の leapfrog 法で軌道積分するアプリ。


---

各アプリの README と build-tutorial.html は現在の実装の説明に徹しています。
元コードからの変更点・計測データの由来・既知のデータ品質の問題は
[HISTORY.md](HISTORY.md) を参照してください。

## 各アプリの詳細・使い方・性能評価

クイックスタートや使い方/ビルド手順、ディレクトリ構造はアプリごとの README 及び build-tutorial.html をご覧ください。

| アプリ | 解説 (README) | 使い方 / ビルド手順 (HTML) | 性能評価ダッシュボード (HTML) |
|---|---|---|---|
| **diffusion** | [README](diffusion/README.md) | [build-tutorial.html](https://raw.githack.com/SCD-ITC-UTokyo/gpu-apps-tutorial/main/diffusion/docs/build-tutorial.html) | [diffusion_dashboard.html](https://raw.githack.com/SCD-ITC-UTokyo/gpu-apps-tutorial/main/diffusion/summary/diffusion_dashboard.html) |
| **fem** | [README](fem/README.md) | [build-tutorial.html](https://raw.githack.com/SCD-ITC-UTokyo/gpu-apps-tutorial/main/fem/docs/build-tutorial.html) | [fem_dashboard.html](https://raw.githack.com/SCD-ITC-UTokyo/gpu-apps-tutorial/main/fem/summary/fem_dashboard.html) |
| **nbody** | [README](nbody/README.md) | [build-tutorial.html](https://raw.githack.com/SCD-ITC-UTokyo/gpu-apps-tutorial/main/nbody/docs/build-tutorial.html) | [nbody_dashboard.html](https://raw.githack.com/SCD-ITC-UTokyo/gpu-apps-tutorial/main/nbody/summary/nbody_dashboard.html) |


## 性能可搬性 (performance portability) 図

問題サイズ N を固定して、6 つの実装
(do concurrent / Kokkos / OpenACC / OpenMP CPU / OpenMP target / stdpar) を
Wisteria (NVIDIA A100) と Miyabi (NVIDIA H200) で比較した図を、
**アプリごとに 1 枚**生成します。

作図スクリプトはアプリごとに `<app>/summary/make_portability_fig.py` に置いてあります。

```bash
python3 diffusion/summary/make_portability_fig.py        # N は自動選択
python3 fem/summary/make_portability_fig.py --N 35937    # N を明示指定
python3 nbody/summary/make_portability_fig.py
```

出力は `<app>/summary/figs/<app>_portability.{png,pdf}`、
図の元データ (プロットした best 値とその variant / メモリモデル) は
`<app>_portability_best.csv` に出ます。

図の読み方:

- パネル = 精度 (FP32 / FP32-64 混合 / FP64)、x 軸 = 実装、y 軸 = 性能 GFLOPS (log)
- 青 = Wisteria (A100)、赤 = Miyabi (H200)。同じ実装の 2 本の高さの比が可搬性を表す
- 棒の値は variant (auto/manu × def/opt)、メモリモデル (separate / managed / UVM)、
  言語 (C++ / Fortran)、スレッド数・タイル幅などを振った中の **best 値**。
  棒の上のラベルは GFLOPS 値と、その best を出した言語 (C++ / F)
- N は「全実装 × 両機種でデータが揃う」ものを自動選択 (揃い方が同じなら大きい N)。
  diffusion と nbody は N を大きくすると性能が飽和・低下して実装間の差が消えるため、
  上限を設けて選ぶ (各スクリプト中の `N_LIMIT`: diffusion < 1e8, nbody < 1e5, fem は制限なし)。
  現状の選択値は diffusion: 16,777,216 / fem: 274,625 / nbody: 65,536。
  `--N` で任意の N に変更可能
- データが無い組み合わせは "no data" と表示。現状残っているのは
  diffusion の Wisteria / Kokkos / FP64 と fem の Wisteria / Kokkos / FP32 の 2 つ
  (どちらも Kokkos の精度切替修正を入れた Wisteria 側の再計測で埋まる)

機種ごとの未計測データは [wisteria.md](wisteria.md) / [miyabi.md](miyabi.md) に
優先度付きでまとめてあります。

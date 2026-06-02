# GPU プログラミングモデル比較

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

### 3. nbody — 直接法 N 体重力多体問題
直接法による N 体重力計算を、2 次精度の leapfrog で軌道積分するアプリ。


---

## 各アプリの詳細・使い方・性能評価

クイックスタートや使い方/ビルド手順、ディレクトリ構造はアプリごとの README 及び build-tutorial.html をご覧ください。

| アプリ | 解説 (README) | 使い方 / ビルド手順 (HTML) | 性能評価ダッシュボード (HTML) |
|---|---|---|---|
| **diffusion** | [README](diffusion/README.md) | [build-tutorial.html](diffusion/docs/build-tutorial.html) | [diffusion_dashboard.html](diffusion/summary/diffusion_dashboard.html) |
| **fem** | [README](fem/README.md) | [build-tutorial.html](fem/docs/build-tutorial.html) | [fem_dashboard.html](fem/summary/fem_dashboard.html) |
| **nbody** | [README](nbody/README.md) | [build-tutorial.html](nbody/docs/build-tutorial.html) | [nbody_dashboard.html](nbody/summary/nbody_dashboard.html) |


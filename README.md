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
- **問題の特性**: 近傍参照のみの**ステンシル計算**で、典型的な **メモリ律速 (memory-bound)**。
  GPU では UVM／managed／unified などメモリモデルの影響や、ループ融合・tile/chunk 化の効果を観察できます。

### 2. fem — 3 次元有限要素法（CG ソルバ）
3 次元の熱伝導／構造解析問題を有限要素法で離散化し、**共役勾配法 (Conjugate Gradient)** で解くアプリ。
- **問題の特性**: 疎行列ベクトル積 (SpMV) を核とする **不規則メモリアクセス**。CSR 形式の非ゼロ走査を
  内側ループに持ち、行列組み立て (assembly) と反復ソルバの両方を並列化対象とします。

### 3. nbody — 直接法 N 体重力多体問題
直接法による N 体重力計算を、2 次精度の leapfrog で軌道積分するアプリ。
- **問題の特性**: 全粒子対 **O(N²)** の相互作用を計算する **演算律速 (compute-bound)**。
  混合精度 (i 粒子 FP_L / j 粒子 FP_M / 高精度部 FP_H) による速度・精度トレードオフや、
  energy_error / virial_ratio による精度評価ができます。

各アプリとも、上記の問題を **同一の 5 方式 + Kokkos の各 policy** で実装し、結果を比較できます。

---

## 各アプリの詳細・使い方・性能評価

| アプリ | 解説 (README) | 使い方 / ビルド手順 (HTML) | 性能評価ダッシュボード (HTML) |
|---|---|---|---|
| **diffusion** | [README](diffusion/README.md) | [build-tutorial.html](diffusion/docs/build-tutorial.html) | [diffusion_dashboard.html](diffusion/summary/diffusion_dashboard.html) |
| **fem** | [README](fem/README.md) | [build-tutorial.html](fem/docs/build-tutorial.html) | [fem_dashboard.html](fem/summary/fem_dashboard.html) |
| **nbody** | [README](nbody/README.md) | [build-tutorial.html](nbody/docs/build-tutorial.html) | [nbody_dashboard.html](nbody/summary/nbody_dashboard.html) |

> **HTML の閲覧について（GitHub 上での注意）**
> GitHub は `.md` を自動でレンダリング表示しますが、**`.html` はクリックするとソース表示**になり、
> ページとしては描画されません。レンダリングした状態で見るには、次のいずれかを使ってください。
> - **ローカル**: リポジトリを clone し、`.html` をブラウザで直接開く（最も確実）
> - **GitHub Pages**: Pages を有効化すると `https://<org>.github.io/<repo>/apps/diffusion/docs/build-tutorial.html` で閲覧可
> - **htmlpreview**: `https://htmlpreview.github.io/?https://github.com/<org>/<repo>/blob/<branch>/apps/diffusion/docs/build-tutorial.html`
>   （`<org>/<repo>/<branch>` を自分のものに置換）

---

## クイックスタート（例: diffusion を GPU でビルド）

```bash
cd diffusion
./configure.py --variant C++/openmp-target/auto.def --machine local --mode gpu
cd C++/openmp-target/auto.def
cmake -B build-local-gpu -S . && cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

利用可能な variant / machine の一覧は各アプリで `./configure.py --list`、
設定の詳細は `./configure.py --info` で確認できます。

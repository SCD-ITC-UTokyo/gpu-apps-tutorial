# Wisteria の計測状況 (2026-09-16 追加計測後)

`*/summary/*.csv` を Miyabi と突き合わせて洗い出した Wisteria 側の欠測一覧と、
2026-09-16 に実施した追加計測の結果。性能可搬性図 (`*/summary/make_portability_fig.py`) の
穴埋めと "best over variants" の公平性確保が目的。

対象 CSV: `diffusion/summary/diffusion.csv`, `fem/summary/fem.csv`, `nbody/summary/nbody.csv`

測定環境:

| 用途 | ノード / 環境 |
|---|---|
| GPU | Wisteria-A (Aquarius), `short-a`, NVIDIA A100-SXM4-40GB, driver 535.54.03 |
| CPU | Wisteria-O (Odyssey), `short-o`, Fujitsu A64FX 48 コア, `module load fj` (tcsds-1.2.43) |
| NVHPC | `nvidia/24.1` (GPU 系のビルド・実行) |
| Kokkos (GPU) | 4.7.02 `OPENMP`+`CUDA` (AMPERE80), `gcc/12.2.0` + `cuda/12.2`, `~/kokkos/4.7.02` |
| Kokkos (CPU) | 4.7.02 `OPENMP`+`SERIAL` (A64FX), `FCCpx -Nclang`, `~/kokkos/4.7.02-a64fx` ← 今回新規作成 |

各アプリの測定条件・注意点は `*/results/wisteria/README.md` に記載しています。

---

## 今回埋めた欠測

### 優先度 A (図が "no data" になっていたもの)

| アプリ | 実装 | 内容 | 状態 |
|---|---|---|---|
| fem | stdpar (C++) | `auto.def` / `auto.opt` × FP32/FP64 × N=17³/33³/65³/129³ (16 行) | **完了** |
| nbody | OpenMP CPU (C++/Fortran) | 3 精度 × N=1,024〜4,194,304 の 13 点 (78 行) | **完了** |
| fem | Kokkos FP32 | — | **測定不能** (下記) |

fem の Kokkos は `C++/kokkos/*/*/src/precision.h` が `#define KREAL double` 固定で、
非 Kokkos 実装のような `FP` マクロ切替を持ちません。`cmake -DFP=32` はバイナリ名を
`fem.gpu.32` に変えるだけで**演算は FP64** です。したがって「fem Kokkos の FP32」は
ソースを変更しない限り測定できません。

> **Miyabi 側の要修正点**: `fem.csv` の Miyabi Kokkos "FP32" 144 行は、同一条件の FP64 行と
> 性能が 3 桁一致します (例: range/baseline N=274,625 で 83.95 vs 83.98 GFLOPS)。
> 同じ FP64 実行に `fp=32` のラベルが付いたものと判断できます。

代わりに fem Kokkos は **FP64 の variant 網羅**を進めました (既存 4 variant → 12 variant、
+32 行)。追加したのは `mdrange/uvm`, `team/uvm`, `range/cpu-sweep`, `range/uvm-sweep`,
`mdrange/cpu-tile-sweep`, `mdrange/uvm-tile-sweep`, `team/cpu-chunk-sweep`, `team/uvm-chunk-sweep`。

### 優先度 B (best 値の公平性)

| アプリ | 内容 | 状態 |
|---|---|---|
| diffusion | stdpar の `auto.def` 追加 + `auto.opt` の再計測 (FP32/FP64 × 5 N、20 行) | **完了** (下記の不具合修正を伴う) |
| diffusion | Kokkos sweep 6 variant の N 依存性 → 上位 3 パラメータ × 4 N (48 行) | **完了** |
| nbody | Kokkos CPU: `range/cpu-sweep` の `-O4` (10 点 × 3 精度)、baseline 系 3 variant の N=1M/2M/4M (3 点 × 3 精度) | **完了** (A64FX 版 Kokkos を新規構築) |
| diffusion | Kokkos baseline 系の探索点 (Miyabi 16〜17 行 / Wisteria 6〜7 行) | 未実施 (下記) |
| fem | OpenMP CPU のスレッド数点 (Miyabi 9 行 / Wisteria 3 行) | 未実施 (下記) |

### 優先度 C (mode=uni) — **A100 では測定不能**

`-gpu=mem:unified` を **NVHPC 25.9** (Wisteria に module あり) でビルドして A100 ノードで
実行したところ、実行時に以下で落ちます:

```
Accelerator Fatal Error: The application was compiled with -gpu=unified,
but this platform does not support Unified Memory.
```

A100 (driver 535.54.03) はアドレス変換ハードウェア (ATS/HMM) を持たないため、
コンパイラを新しくしても unified memory 実行はできません。
`machines/wisteria.yaml` の `modes: [gpu, cpu]` (uni 非対応) は現状の実機と一致しています。

---

## 今回見つかった不具合

### diffusion の stdpar は A100 で実行できなかった (修正済み)

`summary/diffusion.csv` の Wisteria stdpar 行 (`auto.opt` のみ) は全 N で **1.2〜2.4 GFLOPS** と、
同じマシンの OpenACC / OpenMP target (500〜700 GFLOPS) より 2 桁遅く、CPU フォールバックでした。
`-stdpar=gpu -gpu=cc80` でビルドし直すと、今度は実行時に落ちます:

```
thrust::system::system_error: parallel_for: failed to synchronize:
cudaErrorIllegalAddress: an illegal memory access was encountered
```

原因は `diffusion/C++/stdpar/*/src/diffusion.cpp` の 3 箇所のラムダが **参照キャプチャ `[&]`**
だったことです。Miyabi (GH200) はホストスタックをデバイスから参照できるため動きますが、
A100 では不正アクセスになります (この落とし穴は `docs/build-tutorial.html` にも記載があります)。

**値キャプチャ `[=]` に変更**して測定しました。原本は `src/diffusion.cpp.orig-capture-by-ref`
に退避してあります。計算内容が変わっていないことは FP64 の `error` 列が Miyabi の stdpar /
openacc / openmp-target と完全一致することで確認済みです。

| FP | N=32,768 | 262,144 | 2,097,152 | 16,777,216 | 134,217,728 |
|---|---:|---:|---:|---:|---:|
| FP32 | 39.0 | 239.1 | 723.0 | 947.6 | 1074.9 |
| FP64 | 37.7 | 220.4 | 594.9 | 759.9 | 808.1 |

(GFLOPS。旧データの 1.2〜2.4 GFLOPS の 300〜500 倍)

同じ問題が nbody にもあります (次項)。fem の stdpar は最初から `[=]` なので影響はありません。

### nbody の stdpar も A100 では動かなかった (修正済み)

diffusion と同じ `[&]` キャプチャの問題が `nbody/C++/stdpar/*/src/base/nbody.cpp` にもあります
(5 箇所)。Aquarius ノードで実行すると同じ `cudaErrorIllegalAddress` で落ちます:

```
$ ./nbody/C++/stdpar/auto.def/build-wisteria-gpu/nbody.gpu.32.32 --num_min=1024 ...
terminate called after throwing an instance of 'thrust::system::system_error'
  what():  parallel_for: failed to synchronize: cudaErrorIllegalAddress
```

`[=]` に変えると正常に動き、N=65,536 で 3.49 TFLOPS (GPU 相当の値) が出ます。
そこで diffusion と同じく **`auto.def` / `auto.opt` の 5 箇所を `[=]` に修正**しました
(原本は `src/base/nbody.cpp.orig-capture-by-ref` に退避)。

なお `nbody.csv` にある Wisteria の stdpar 行 468 行は `source_file` が `legacy C1〜C4 stdpar`
で、現在のソース・現在の環境で再現したものではありません。

fem の stdpar は最初から `[=]` なので問題ありません。

### Kokkos バイナリはジョブスクリプトの推奨 module では起動できなかった (修正済み)

`machines/wisteria.yaml` の `job.per_mode.gpu.modules: ["nvidia"]` がそのまま
生成ジョブスクリプトの推奨例になりますが、Kokkos 実装は `gcc/12.2.0` + `cuda/12.2` で
ビルドするため、`module load nvidia` だけでは実行時に落ちます (Aquarius ノードで確認):

```
fem.gpu.32: /usr/lib64/libstdc++.so.6: version `GLIBCXX_3.4.29' not found
```

`module load gcc/12.2.0 cuda/12.2` なら正常に動きます。
machine yaml に `kokkos.modules_per_mode` を追加し、**variant が Kokkos のときは
そちらを推奨例として出す**ように `configure.py` を修正しました。

### Kokkos の install 先が 1 つしか書けなかった (修正済み)

Wisteria は GPU (Aquarius = x86 + A100) と CPU (Odyssey = A64FX) でアーキテクチャが違うため、
Kokkos も 2 つ必要です (今回 `~/kokkos/4.7.02` と `~/kokkos/4.7.02-a64fx` を用意)。
しかし `machines/wisteria.yaml` の `kokkos.root` は 1 つしか書けず、自動検出も
**mode を考慮せず `~/kokkos/*` の先頭に当たったものを返します**。実際、A64FX 版を入れた後は
`--mode gpu` でも A64FX 版 (CUDA backend 無し) が選ばれました:

```
$ ./configure.py --variant C++/kokkos/range/baseline --machine wisteria --mode gpu --dry-run
list(APPEND CMAKE_PREFIX_PATH "/home/z30133/kokkos/4.7.02-a64fx")   # ← CPU 用が選ばれている
  set(CMAKE_CXX_COMPILER ".../FCCpx")                               # ← A64FX 用コンパイラ
```

警告も出ないため、GPU のつもりで CPU 用 Kokkos をリンクしたバイナリができてしまいます。

修正として (1) machine yaml に `kokkos.root_per_mode.<mode>` を追加して mode ごとに
install 先を書けるようにし、(2) `configure.py` が install 済み Kokkos の
`include/KokkosCore_config.h` から backend を読んで `--mode` との整合性を検査するようにしました。

```
$ ./configure.py ... --mode gpu --kokkos-root ~/kokkos/4.7.02-a64fx
ERROR: Kokkos install '...4.7.02-a64fx' は GPU backend を持ちません (有効な backend: ['OPENMP', 'SERIAL'])。
       --mode gpu には CUDA/HIP/SYCL backend 付きの Kokkos が必要です。
$ ./configure.py ... --mode cpu --kokkos-root ~/kokkos/4.7.02
  [WARN] ... GPU backend ['CUDA'] を含むため、--mode cpu でも DefaultExecutionSpace は GPU になります。
```

### CPU mode の既定コンパイラ (gcc) では Odyssey 向けバイナリにならなかった (修正済み)

`machines/wisteria.yaml` の `cpu.default: gcc`、および `job.per_mode.cpu.modules: ["gcc/12.2.0"]`
は、ログインノードで使える gcc が **x86_64 用** であるため、`rscgrp=debug-o` (A64FX = aarch64) に
投入すると実行できません。

```
$ gcc -dumpmachine          # ログインノードの gcc/12.2.0
x86_64-redhat-linux
$ ./configure.py --variant C++/openmp-cpu --machine wisteria --mode cpu --dry-job
#PJM -L rscgrp=debug-o      # ← A64FX (aarch64) のキュー
#   module load gcc/12.2.0  # ← x86_64 用の gcc
```

aarch64 版 gcc は `/work/opt/local/aarch64/cores/gcc/12.2.0` に存在しますが、
ログインノード (x86_64) では実行できず、modulefile も Aquarius ツリーにしかありません。
Wisteria の CPU 実行は Fujitsu クロスコンパイラ (`module load fj` → `FCCpx` / `frtpx`) が必要で、
今回の計測もすべてそちらで行いました。

修正として `cpu.default: fccpx` / `cpu.default_f90: frtpx` (言語別の既定を `configure.py` が
見るようにした) と `job.per_mode.cpu.modules: ["fj"]` に変更し、
ドキュメントの例 (`diffusion/README.md`, `fem/README.md`, 3 アプリの `build-tutorial.html`) も
`--compiler frtpx` + 理由の説明に差し替えました。

### 既定 module のバージョンが README とずれていた (修正済み)

`job.per_mode.gpu.modules: ["nvidia"]` は `module load nvidia` = **nvidia/23.3 (default)** を指しますが、
yaml のコメントと README は NVHPC **24.1** 前提です (今回の計測もすべて `nvidia/24.1` で実施)。
`modules: ["nvidia/24.1"]` に固定しました。

### 既存の CPU フォールバック行は残してあります

diffusion の旧 stdpar 行 (`source_file` が `C2.C++.std.auto.opt/...`) は削除していません。
同じ `N`/`fp` で GFLOPS が 2 桁低い行として残るので、解析時は除外してください。

### 参考: 問題なかった項目

- ホスト判定 `host_patterns: ["wisteria*", "w[oa]*"]` — Aquarius 計算ノードは `wa01` で一致
- PJM 用ジョブスクリプト (`#PJM -L rscgrp/node/elapse`, `--omp thread`, `-g`, `${PJM_O_WORKDIR}`)、
  課金グループ自動検出、`--queue` / `--group` の上書き
- `modes: [gpu, cpu]` (uni 非対応) — 実機で `-gpu=mem:unified` が動かないことと一致
- GPU フラグ `-gpu=cc80` — stdpar / do-concurrent が逐次実行に退化しないために必須で、正しく入っている
- `omp_threads` (gpu=72 / cpu=48) は実機のコア数と一致

---

---

## 追加計測後の充足状況 (2026-09-16)

| アプリ | CSV 行数 | 性能可搬性図で Wisteria が欠けている組 |
|---|---|---|
| diffusion | 1,075 → **1,143** (+68) | Kokkos FP64 のみ (ソースが `float` 固定のため両機種とも測定不能) |
| fem | 707 → **755** (+48) | Kokkos FP32 のみ (ソースが `double` 固定のため測定不能) |
| nbody | 7,788 → **7,923** (+135) | **なし** (全実装 × 全精度そろい) |

図・ダッシュボードは再生成済みです (`*/summary/figs/*_portability.{png,pdf}`,
`*_portability_best.csv`, `*_dashboard.html`)。

追加計測で更新された best 値の例:

- diffusion Kokkos (Wisteria, FP32, N=16,777,216): 869 → **1,250 GFLOPS**
  (`mdrange/uvm-tile-sweep x1y2z64` を他の N でも測ったため)
- diffusion stdpar (Wisteria, FP32, N=16,777,216): 2.4 → **948 GFLOPS** (CPU フォールバックの解消)
- nbody OpenMP CPU (Wisteria, FP32, N=65,536): データ無し → **1,627 GFLOPS** (Fortran, A64FX)

## 残っている欠測

| アプリ | 内容 | 理由 |
|---|---|---|
| diffusion | Kokkos baseline 系 8 variant の探索点 (Miyabi 16〜17 行 / Wisteria 6〜7 行) | 差分は最適化レベル違いと繰り返し測定。best 値は今回追加した tile-sweep (1250 GFLOPS) が上回るため優先度低 |
| fem | OpenMP CPU の繰り返し測定 (Miyabi は同条件 3 回、Wisteria は 1 回) | 値のばらつき把握用。best 値には影響しない |
| fem | N=2,146,689 の非 Kokkos (openacc C++ FP64 以外) | 大きい問題サイズでの比較が必要になった段階で |
| diffusion / fem | **両機種とも** Kokkos の FP64 / FP32 | diffusion Kokkos は `float` 固定、fem Kokkos は `double` 固定。ソース側で `FP` マクロに対応させない限り測定不能 |
| nbody | Kokkos CPU の tile / chunk sweep の探索点 (Miyabi 100 行 / Wisteria 50 行) | パラメータ点数の差。必要になれば A64FX 版 Kokkos で追加可能 |

## データの取り違えに注意 (既存の課題)

diffusion / fem の OpenACC C++ `auto.def` (GPU) は、Wisteria だけ同一条件で 2 行あります。
出典が `Wisteria/A1.C++.acc.auto.def/log/` と `../results/wisteria/nonkokkos-openacc-auto.def.log`
の 2 系統で値も異なります (例: diffusion FP32 N=32,768 で 29.36 と 34.67 GFLOPS)。
どちらを正とするかは未決着です。

---

## マージ後の補足 (2026-09-17, Miyabi 側の結果を統合したあと)

この文書は Wisteria 単体での作業記録です。その後 Miyabi 側で **Kokkos の `FP` マクロ未対応**
(常に単精度 / 常に倍精度になっていた不具合) が修正され、再計測されたため、以下は解消済みです。

| 本文の記述 | 現状 |
|---|---|
| 「`fem.csv` の Miyabi Kokkos "FP32" 144 行は実は FP64」(上の引用ブロック) | **修正済み**。Miyabi 側で `precision.h` の `FP` 切替を実装し、該当 144 行を破棄して 264 行を取り直した |
| 「diffusion / fem の Kokkos は両機種とも FP64 / FP32 を測定不能」(残っている欠測の表) | **Miyabi は測定済み** (diffusion Kokkos FP64 = 210 行)。Kokkos ソース側の修正はマージ済みなので、**Wisteria 側も再ビルドすれば測定可能**になった |

マージ後の CSV 行数は diffusion 1,353 (Miyabi 912 / Wisteria 441) /
fem 875 (684 / 191) / nbody 7,923 (4,266 / 3,657) です。

性能可搬性図で残る "no data" は 2 セルのみ:

- diffusion: Wisteria / Kokkos / FP64
- fem: Wisteria / Kokkos / FP32

いずれも Kokkos の精度切替修正を取り込んだ Wisteria 側の再計測で埋まります。
詳細は [miyabi.md](miyabi.md) と [HISTORY.md](HISTORY.md) を参照してください。

### 作業ツリーの同期 (Wisteria 側)

Wisteria の作業コピーを統合後の状態に更新しました。次に Wisteria で作業するときは、
この状態を持っていけば Miyabi 側の修正込みでビルドできます。

**取り込んだもの (Miyabi 由来)**

| 対象 | 内容 |
|---|---|
| `{diffusion,fem}/C++/kokkos/**/src/` 94 ファイル | Kokkos の `FP` マクロ対応。**これが入るまで Wisteria の Kokkos は diffusion=常に単精度 / fem=常に倍精度**だった |
| `HISTORY.md`, `miyabi.md` | 変更履歴と Miyabi 側の欠測一覧 |
| `README.md` | 性能可搬性図の節 |
| `*/summary/{*.csv,*_dashboard.html,figs/}` | 両機種統合後の CSV・ダッシュボード・図 |

**削除した陳腐化ファイル**

| 対象 | 理由 |
|---|---|
| `fem/input/` (`cube.0` 45 MB, `INPUT.DAT`, `gen_cube.py`) | メッシュは実行時に内部生成する方式に変わったため不要 (`HISTORY.md` 第 2 章) |
| `fem/C++/openmp-cpu/run_*/cube.0` 4 個 (計 180 MB) | 同上 |
| `*/C++/**/src/*.orig-capture-by-ref` 4 個 | stdpar の capture 修正前バックアップ。修正はマージ済みで git 履歴から追える |
| `*/summary/*.csv.bak*` 4 個 | マージ前の CSV スナップショット。統合済み CSV が正 |

計測ログ (`*/results/`) は転送量を抑えるため同期していません。各機種のログはそれぞれの
機種にのみ置いてあり、統合結果は `*/summary/*.csv` に入っています。

**次に Wisteria で流すもの** (性能可搬性図の "no data" 2 セルを埋める)

1. Kokkos を再ビルドし直す必要はありません (ソースのみの修正)。`~/kokkos/4.7.02` (GPU) /
   `~/kokkos/4.7.02-a64fx` (CPU) はそのまま使えます
2. diffusion Kokkos の **FP64**: `cmake -DFP=64`。14 variant
3. fem Kokkos の **FP32**: `cmake -DFP=32`。`precision.h` が `FP` を見るようになったので
   今度は本当に単精度で走ります

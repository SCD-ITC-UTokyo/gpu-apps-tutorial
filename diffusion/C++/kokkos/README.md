# C++/kokkos — Kokkos (C++ performance portability)

Kokkos C++ ライブラリで GPU/CPU 横断ポータブルに並列化する実装。pragma の代わりに
**`Kokkos::parallel_for` + `KOKKOS_LAMBDA`** を使い、**View** という抽象データコンテナで
メモリ空間を管理する。

## 構造 (3 種類の並列化ポリシー)

```
kokkos/
├── README.md                                                  # ← このファイル
├── range/         # parallel_for (RangePolicy) — 最もシンプル
│   ├── baseline/, cpu-sweep/, uvm/, uvm-sweep/, baseline-fused/, uvm-fused/
│   └── README.md
├── mdrange/       # MDRangePolicy — N 次元 range、tile size 制御可
│   ├── baseline/, cpu-tile-sweep/, uvm/, uvm-tile-sweep/
│   └── README.md
└── team/          # TeamPolicy + TeamThreadRange — hierarchical parallelism
    ├── baseline/, cpu-chunk-sweep/, uvm/, uvm-chunk-sweep/
    └── README.md
```

各 variant 配下: `src/{*.cpp,*.h}` + `sh/<machine>/` (元プロジェクトの compile/run scripts)。

## 軸の意味

| 軸 | 値 | 意味 |
|---|---|---|
| **policy** | `range` | `parallel_for("...", N, KOKKOS_LAMBDA(int i){...})` — 最もシンプル、1 重ループに展開 |
| | `mdrange` | `MDRangePolicy<Rank<3>>` — N 次元ループを直接表現、tile size 指定可 |
| | `team` | `TeamPolicy + TeamThreadRange` — block-thread の階層並列 (CUDA の thread/block 概念に近い) |
| **メモリ** | (default) | `Kokkos::View<float*>` — Kokkos が backend に合わせて自動配置 |
| | `uvm` | `Kokkos::View<float*, Kokkos::CudaUVMSpace>` — CUDA UVM 明示 |
| **sweep** | (なし) | デフォルト | 比較対象のベースライン |
| | `cpu-sweep` | RangePolicy のみ | compile flag sweep の意図 (sh/options_*.sh 参照) |
| | `cpu-tile-sweep` | MDRange のみ | `_NX/_NY/_NZ` マクロで tile size 指定 |
| | `cpu-chunk-sweep` | TeamPolicy のみ | `_CHUNK` マクロで chunk size 指定 |
| **fused** | (なし) | デフォルト | 3 重ループのまま |
| | `-fused` (range のみ) | 3 重ループを 1 重に融合 (manual index 計算) |

## 依存

- **Kokkos 4.x / 5.x** がインストール済 (`KokkosConfig.cmake` が見つかること)
- machines/<machine>.yaml の `kokkos.root` に install path を記載

例: ローカル機の spack 環境
```yaml
# machines/local.yaml
kokkos:
  root: /home/<user>/spack/opt/spack/.../kokkos-5.0.2-.../
  cxx_standard: 20
```

Kokkos のインストール時に backend (CUDA/OpenMP/HIP/Serial) と GPU arch (`cuda_arch=120` 等) が
決まる。**インストール時の選択が実行時の挙動を支配** (本教材の build からは変更不可)。

## ビルド & 実行

Kokkos は **CMake 必須**。Make は非対応。

```bash
cd apps/diffusion
./configure.py --variant C++/kokkos/range/baseline --machine local --mode gpu

cd C++/kokkos/range/baseline
cmake -B build-local-gpu -S .
cmake --build build-local-gpu
./build-local-gpu/diffusion.gpu.32 64
```

**UVM variant (`*/uvm`, `*/uvm-*`) は GPU mode 必須**:
`./configure.py --variant C++/kokkos/range/uvm --machine local --mode cpu` はエラー終了。

### tile/chunk macros 上書き

`mdrange/cpu-tile-sweep`, `team/cpu-chunk-sweep` などは CMake 変数で macro を上書き可能:

```bash
cd C++/kokkos/mdrange/cpu-tile-sweep
cmake -B build-1 -S . -D_NX=4 -D_NY=8 -D_NZ=64
cmake --build build-1
```

既定値は `implementations.yaml` の `kokkos.macros_default` で定義 (`_NX=2 _NY=4 _NZ=16, _CHUNK=32`)。

## 他 impl との比較ポイント

- **vs `openmp-target`**: Kokkos は library、pragma 不要。ただしソースは `Kokkos::View` を使う書き換えが要
- **vs `stdpar`**: どちらも pragma 不要。Kokkos は **明示的にメモリ空間を選べる** (default vs UVM など)
- **vs CUDA**: Kokkos は CUDA を抽象化、AMD HIP や SYCL にも同じソースが流用可能

学習順序のおすすめ:
1. `range/baseline` (Kokkos の "Hello World")
2. `range/uvm` で UVM のオーバーヘッドを観察
3. `mdrange/baseline` で N 次元 range の自然な書き方
4. `team/baseline` で hierarchical parallelism (CUDA 風)
5. `*-sweep` 系で性能チューニング

詳細は各 policy の README (`range/README.md`, `mdrange/README.md`, `team/README.md`) を参照。

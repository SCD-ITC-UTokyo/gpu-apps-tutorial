#!/usr/bin/env python3
"""fem の Kokkos 実装の標準出力から 10 列ベンチマーク CSV を復元する。

fem の Kokkos 実装 (C++/kokkos/*/*) は非 Kokkos 実装と違い BENCHMARK_MODE を
持たず、10 列 CSV を出力しない。出力されるのは

    *** matrix conn. <sec> sec.
    <iter> <resid>            ... CG 反復履歴
    *** solver       <sec> sec.
    <node> <value>            ... 原点節点の解

だけなので、非 Kokkos 実装 (solver_CG.c) と同じ FLOP 式

    FLOP = ITER*(NP*14 + NPLU*2) + NP*3 + NPLU*2

を使って performance_gflops を復元する。NPLU (対角を除く非零要素数) は
構造格子六面体メッシュでは解析的に求まる:

    NPLU(n) = (3n-2)^3 - n^3        (n = 一辺の節点数)

この式は非 Kokkos 実装が実測で出力した GFlops から逆算した値と一致する
(65^3: 6,914,432)。
"""
import argparse, os, re, sys

def nplu(n):            # n = nodes per side
    return (3*n - 2)**3 - n**3

def parse(path):
    txt = open(path).read()
    it = re.findall(r'^(\d+) ([0-9.eE+-]+)\s*$', txt, re.M)
    if not it:
        return None
    iters, resid = int(it[-1][0]), float(it[-1][1])
    m_s = re.search(r'\*\*\* solver\s+([0-9.eE+-]+) sec', txt)
    m_m = re.search(r'\*\*\* matrix conn\.\s+([0-9.eE+-]+) sec', txt)
    if not m_s:
        return None
    return iters, resid, float(m_s.group(1)), float(m_m.group(1)) if m_m else 0.0

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="+", help="fem Kokkos の標準出力ファイル")
    ap.add_argument("--binary", required=True,
                    help="対応するバイナリの絶対パス (parse_logs.py が impl/variant を推論するのに使う)")
    ap.add_argument("--nside", type=int, required=True, help="一辺の節点数 (65 → 65^3)")
    ap.add_argument("--time-file", default=None, help="`/usr/bin/time -p` の出力 (real/user/sys)")
    a = ap.parse_args()

    n = a.nside
    NP, NPLU = n**3, nplu(n)
    real = user = sysc = 0.0
    if a.time_file and os.path.exists(a.time_file):
        for line in open(a.time_file):
            k, _, v = line.partition(" ")
            if k in ("real", "user", "sys"):
                v = float(v)
                real, user, sysc = (v, user, sysc) if k == "real" else \
                                   (real, v, sysc) if k == "user" else (real, user, v)

    print("# binary,NP,time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,"
          "num_time_steps_logged,last_sim_time")
    for lg in a.logs:
        r = parse(lg)
        if r is None:
            print(f"WARNING: {lg}: 解析できません", file=sys.stderr); continue
        iters, resid, solver_s, mat_s = r
        flop = iters * (NP*14 + NPLU*2) + NP*3 + NPLU*2
        print("%s,%d,%13.6e,%13.6e,%13.6e,%13.6e,%13.6e,%13.6e,%d,%13.6e" %
              (a.binary, NP, solver_s, flop/solver_s*1e-9, resid,
               real, user, sysc, iters, mat_s))

main()

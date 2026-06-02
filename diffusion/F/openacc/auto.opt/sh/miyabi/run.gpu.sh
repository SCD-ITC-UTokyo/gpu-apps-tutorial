#!/bin/bash

################################
## set job options.
################################

## for Wisteria ####

#### use CPU(Wisteria-Odyssey) [DISABLED] ####
##PJM -N "diffusion"
##PJM -L rscgrp=debug-o
##PJM -L node=1
##PJM --omp thread=48
##PJM -L elapse=00:10:00
##PJM -g gr52
##PJM -j

#### use GPU(Wisteria-Aquarius) [ENABLED] ####
#PJM -N "diffusion"
#PJM -L rscgrp=debug-a
#PJM -L node=1
#PJM --omp thread=72
#PJM -L elapse=00:10:00
#PJM -g gr52
#PJM -j

## for Miyabi ####

#### use CPU(Miyabi-C) [DISABLED] ####
##PBS -N "diffusion"
##PBS -q debug-c
##PBS -l select=1:ompthreads=112
##PBS -l walltime=00:10:00
##PBS -W group_list=gr52
##PBS -j oe

#### use GPU(Miyabi-G) [ENABLED] ####
#PBS -N "diffusion"
#PBS -q debug-g
#PBS -l select=1:ompthreads=72
#PBS -l walltime=00:10:00
#PBS -W group_list=gr52
#PBS -j oe

################################
## setup environment.
################################

# load modules.
case "$HOSTNAME" in
    wo*) # Wisteria-Odyssey(CPU)
	module purge
	module load gcc
	;;
    wa*) # Wisteria-Aquarius(GPU)
	module purge
	module load nvidia
	;;
    mc*) # Miyabi-C(CPU)
	module purge
	module load gcc
	;;
    mg*) # Miyabi-G(GPU)
	module purge
	module load nvidia/25.9
	;;
esac

# change to job submission dir.
if [ -n "${PJM_O_WORKDIR}" ]; then
    cd ${PJM_O_WORKDIR}
fi
if [ -n "${PBS_O_WORKDIR}" ]; then
    cd ${PBS_O_WORKDIR}
fi

################################
## build the program.
################################

if [ "$task" = "build" ]; then
#    make -C src -f Makefile.gpu FP=32 NTHREADS=32   clean install
#    make -C src -f Makefile.gpu FP=32 NTHREADS=64   clean install
#    make -C src -f Makefile.gpu FP=32 NTHREADS=128  clean install
#    make -C src -f Makefile.gpu FP=32 NTHREADS=256  clean install
#    make -C src -f Makefile.gpu FP=32 NTHREADS=512  clean install
#    make -C src -f Makefile.gpu FP=32 NTHREADS=1024 clean install
    make -C src -f Makefile.gpu FP=32 clean install
    make -C src -f Makefile.gpu FP=64 clean install
    exit 0
fi

################################
# benchmark the program.
################################

if [ "$task" = "bench" ]; then
    for f in 32 64; do
	for nx in 32 64 128 256 512; do
	    bin/diffusion.gpu.$f $nx
	done > log/bench.gpu.${f}_run.csv
    done
    exit 0
fi

################################
# optimize the program.
################################

if [ "$task" = "optim" ]; then
    for t in 32 64 128 256 512 1024; do
        for n in $(seq 1 11) ; do
            bin/diffusion.gpu.32.$t 256
        done
    done > log/optim.gpu
    exit 0
fi

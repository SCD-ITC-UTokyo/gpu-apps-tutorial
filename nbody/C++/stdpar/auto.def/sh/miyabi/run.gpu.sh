#!/bin/bash

################################
## set job options.
################################

## for Wisteria ####

#### use CPU(Wisteria-Odyssey) [DISABLED] ####
##PJM -N "nbody"
##PJM -L rscgrp=debug-o
##PJM -L node=1
##PJM --omp thread=48
##PJM -L elapse=00:10:00
##PJM -g xxx
##PJM -j

#### use GPU(Wisteria-Aquarius) [ENABLED] ####
#PJM -N "nbody"
#PJM -L rscgrp=debug-a
#PJM -L node=1
#PJM --omp thread=72
#PJM -L elapse=00:10:00
#PJM -g xxx
#PJM -j

## for Miyabi ####

#### use CPU(Miyabi-C) [DISABLED] ####
##PBS -N "nbody"
##PBS -q debug-c
##PBS -l select=1:ompthreads=112
##PBS -l walltime=00:10:00
##PBS -W group_list=xxx
##PBS -j oe

#### use GPU(Miyabi-G) [ENABLED] ####
#PBS -N "nbody"
#PBS -q debug-g
#PBS -l select=1:ompthreads=72
#PBS -l walltime=00:10:00
#PBS -W group_list=xxx
#PBS -j oe

################################
## setup environment.
################################

# load modules.
case "$HOSTNAME" in
    wo*) # Wisteria-Odyssey(CPU)
	module purge
	module load gcc
	module load hdf5
	;;
    wa*) # Wisteria-Aquarius(GPU)
	module purge
	module load nvidia/22.7
	module load hdf5/1.12.2
	;;
    mc*) # Miyabi-C(CPU)
	module purge
	module load gcc
        module load hdf5
	;;
    mg*) # Miyabi-G(GPU)
	module purge
	module load nvidia/25.9
        module load hdf5/1.14.6
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
    make -C src -f Makefile.gpu FP_L=32 FP_M=32 BENCH=1 clean install
    make -C src -f Makefile.gpu FP_L=64 FP_M=64 BENCH=1 clean install
    make -C src -f Makefile.gpu FP_L=32 FP_M=64 BENCH=1 clean install
    make -C src -f Makefile.gpu FP_L=32 FP_M=32 BENCH=0 clean install
    make -C src -f Makefile.gpu FP_L=64 FP_M=64 BENCH=0 clean install
    make -C src -f Makefile.gpu FP_L=32 FP_M=64 BENCH=0 clean install
    exit 0
fi

################################
# benchmark the program.
################################

if [ "$task" = "bench" ]; then
    for ff in 32.32 64.64 32.64; do
	rm -f log/bench.gpu.${ff}_run.csv
	bin/nbody.bench.gpu.$ff --file="bench.gpu.$ff" \
	    --num_min=1024 --num_max=4194304 --num_bin=13
    done
    exit 0
fi

################################
# validate the program.
################################

if [ "$task" = "valid" ]; then
    for ff in 32.32 64.64 32.64; do
	rm -f log/valid.gpu.${ff}_run.csv
	bin/nbody.gpu.$ff --file="valid.gpu.$ff"
    done
    exit 0
fi

#!/bin/bash
#PBS -q debug-g
#PBS -l select=1:ompthreads=72
#PBS -l walltime=00:10:00
#PBS -W group_list=xxx
#PBS -j oe

module purge
module load nvidia/25.9
module list

cd ${PBS_O_WORKDIR}

export OMP_PROC_BIND=spread
export OMP_PLACES=threads


time -p ./C1.out

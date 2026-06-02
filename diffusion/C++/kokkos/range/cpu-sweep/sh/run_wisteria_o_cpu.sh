#!/bin/bash
#PJM -L rscgrp=debug-o
#PJM -L node=1
#PJM --omp thread=48
#PJM -L elapse=00:10:00
#PJM -g xxx
#PJM -j
#PJM -S

module purge
module load fj/1.2.42
module list

export OMP_PROC_BIND=true

time -p ./A2.out

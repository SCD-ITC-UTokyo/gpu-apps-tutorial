#!/bin/bash
#PJM -L rscgrp=debug-a
#PJM -L node=1
#PJM --omp thread=72
#PJM -L elapse=00:10:00
#PJM -g xxx
#PJM -j
#PJM -S

module purge
module load nvidia/23.3
module list

export OMP_PROC_BIND=spread
export OMP_PLACES=threads

time -p ./B1.out

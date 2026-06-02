#!/bin/sh
#------ pjsub option --------#
#PJM -L rscgrp=debug-a
#PJM -L node=1
#PJM --omp thread=72
#PJM -L elapse=00:10:00
#PJM -g xxxx
#PJM -j
#PJM -S
#------- Program execution -------#
module purge
module load nvidia/23.3
module list

export OMP_NUM_THREADS=72
time -p ../solcpp

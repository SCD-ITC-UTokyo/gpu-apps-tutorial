#!/bin/sh
#------ pjsub option --------#
#PJM -L rscgrp=debug-o
#PJM -L node=1
#PJM --omp thread=48
#PJM -L elapse=00:10:00
#PJM -g xxxx
#PJM -j
#PJM -S
#------- Program execution -------#
module purge
module load fj/1.2.42
module list

export OMP_NUM_THREADS=48
time -p ../solcpp

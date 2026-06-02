#!/bin/sh
#------ pjsub option --------#
#PJM -L rscgrp=debug-o
#PJM -L node=1
#PJM --omp thread=48
#PJM -L elapse=00:10:00
#PJM -g xxx
#PJM -j
#PJM -S
#------- Program execution -------#

pairs=(
  "O2 -O2"
  "O3 -O3"
  "Kfast -Kfast"
)

module purge
module load fj/1.2.42
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02
CMAKE=/xxx/cmake/3.29.6/bin/cmake

for pair in "${pairs[@]}"; do
  read -r NAME OPTION <<< "$pair"
  echo "$NAME, OPTION=$OPTION"

  BUILD_DIR=build_wisteria_o_cpu_${NAME}
  COMPILE_LOG="cmake_wisteria_o_cpu_${NAME}.log"
  ${CMAKE} -S . -B ${BUILD_DIR} \
    -DCMAKE_CXX_COMPILER=FCC \
    -DCMAKE_CXX_FLAGS="-Nclang ${OPTION}" \
    -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT &> ${COMPILE_LOG}
  ${CMAKE} --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}
done


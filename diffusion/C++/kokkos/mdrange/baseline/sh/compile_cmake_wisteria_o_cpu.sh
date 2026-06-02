#!/bin/bash
#------ pjsub option --------#
#PJM -L rscgrp=debug-o
#PJM -L node=1
#PJM --omp thread=48
#PJM -L elapse=00:10:00
#PJM -g xxx
#PJM -j
#PJM -S
#------- Program execution -------#

module purge
module load fj/1.2.42
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02

CMAKE=/xxx/cmake/3.29.6/bin/cmake

BUILD_DIR=build_wisteria_o_cpu
COMPILE_LOG="cmake_wisteria_o_cpu.log"

${CMAKE} -S . -B ${BUILD_DIR} \
  -DCMAKE_CXX_COMPILER=FCC \
  -DCMAKE_CXX_FLAGS="-Nclang" \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT &> ${COMPILE_LOG}
${CMAKE} --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}
RUN_SCRIPT="run_wisteria_o_cpu.sh"
cp -a ${RUN_SCRIPT} ./${BUILD_DIR}/${RUN_SCRIPT}


BUILD_DIR=build_wisteria_o_cpu_Kfast
COMPILE_LOG="cmake_wisteria_o_cpu_Kfast.log"

${CMAKE} -S . -B ${BUILD_DIR} \
  -DCMAKE_CXX_COMPILER=FCC \
  -DCMAKE_CXX_FLAGS="-Nclang -Kfast" \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT &> ${COMPILE_LOG}
${CMAKE} --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}
RUN_SCRIPT="run_wisteria_o_cpu.sh"
cp -a ${RUN_SCRIPT} ./${BUILD_DIR}/${RUN_SCRIPT}

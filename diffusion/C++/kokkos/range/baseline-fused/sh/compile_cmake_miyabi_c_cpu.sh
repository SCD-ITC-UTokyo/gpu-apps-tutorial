#!/bin/bash

# Miyabi-C(CPU)

module purge
module load intel/2025.2.0
module list

export KOKKOS_DIR=/xxx/kokkos/5.0.2

BUILD_DIR=build_miyabi_c_cpu

COMPILE_LOG="cmake_miyabi_c_cpu.log"
cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_CXX_COMPILER=icpx \
  -DCMAKE_CXX_FLAGS="-O3 -march=sapphirerapids" \
  -DCMAKE_PREFIX_PATH=$KOKKOS_DIR &>  ${COMPILE_LOG}
cmake --build ${BUILD_DIR} -j  -v &>> ${COMPILE_LOG}

RUN_SCRIPT="run_miyabi_c_cpu.sh"
cp -a run_miyabi_c_cpu.sh ./${BUILD_DIR}/${RUN_SCRIPT}
pushd ${BUILD_DIR}
qsub ${RUN_SCRIPT}
popd

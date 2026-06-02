#!/bin/bash


module purge
module load nvidia/25.9
module list

export KOKKOS_ROOT=/xxx/kokkos/5.0.2omp/


COMPILE_LOG="cmake_miyabi_g_cpu.log"
BUILD_DIR=build_miyabi_g_cpu
cmake -S . -B ${BUILD_DIR}  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT &> ${COMPILE_LOG}
cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}

RUN_SCRIPT="run_miyabi_g_cpu.sh"
cp -a ${RUN_SCRIPT} ./${BUILD_DIR}/${RUN_SCRIPT}
pushd ${BUILD_DIR}
qsub ${RUN_SCRIPT}
popd

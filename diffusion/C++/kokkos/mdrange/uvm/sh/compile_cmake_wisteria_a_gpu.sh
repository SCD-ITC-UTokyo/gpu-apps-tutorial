#!/bin/bash


module purge
module load nvidia/23.3
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02


COMPILE_LOG="cmake_wisteria_a_gpu.log"
BUILD_DIR=build_wisteria_a_gpu
cmake -S . -B ${BUILD_DIR} \
 -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT &> ${COMPILE_LOG}
cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}

RUN_SCRIPT="run_wisteria_a_gpu.sh"
cp -a ${RUN_SCRIPT} ./${BUILD_DIR}/${RUN_SCRIPT}
pushd ${BUILD_DIR}
pjsub ${RUN_SCRIPT}
popd

COMPILE_LOG="cmake_wisteria_a_gpu_fast.log"
BUILD_DIR=build_wisteria_a_gpu_fast
cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT \
  -DCMAKE_CXX_FLAGS="-fast" &> ${COMPILE_LOG}
cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}

RUN_SCRIPT="run_wisteria_a_gpu.sh"
cp -a ${RUN_SCRIPT} ./${BUILD_DIR}/${RUN_SCRIPT}
pushd ${BUILD_DIR}
pjsub ${RUN_SCRIPT}
popd

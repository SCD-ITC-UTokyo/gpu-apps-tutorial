
#!/bin/bash

# NVIDIA HPC SDK
pairs=(
  "no1 -O2 -xCORE-AVX512"
  "no2 -O3 -xCORE-AVX512"
  "no3 -O2 -march=sapphirerapids"
  "no4 -O3 -march=sapphirerapids"
)

module purge
module load intel/2025.2.0
module list

export KOKKOS_DIR=/xxx/kokkos/5.0.2

for pair in "${pairs[@]}"; do
  read -r NAME OPTION1 OPTION2 <<< "$pair"
  echo "$NAME OPTION=${OPTION1} ${OPTION2}"

  BUILD_DIR=build_miyabi_c_cpu_${NAME}
  COMPILE_LOG="cmake_miyabi_c_cpu_${NAME}.log"
  #cmake -S . -B ${BUILD_DIR}  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT -DCMAKE_CXX_FLAGS="${OPTION}" &> ${COMPILE_LOG}
  #cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}
  cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_C_COMPILER=icx \
  -DCMAKE_CXX_COMPILER=icpx \
  -DCMAKE_CXX_FLAGS="${OPTION1} ${OPTION2}" \
  -DCMAKE_PREFIX_PATH=$KOKKOS_DIR &> ${COMPILE_LOG}
  cmake --build ${BUILD_DIR} -j  -v &> ${COMPILE_LOG}

  RUN_SCRIPT="run_miyabi_c_cpu.sh"
  cp -a ${RUN_SCRIPT} ./${BUILD_DIR}

  pushd ${BUILD_DIR}
  qsub ${RUN_SCRIPT}
  popd

done


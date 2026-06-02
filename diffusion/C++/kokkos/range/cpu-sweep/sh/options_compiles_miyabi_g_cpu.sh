
#!/bin/bash

pairs=(
  "O2 -O2"
  #"O3 -O3"
  #"O4 -O4"
  "fast -fast"
)

module purge
module load nvidia/25.9
module list

export KOKKOS_ROOT=/xxx/kokkos/5.0.2omp/

for pair in "${pairs[@]}"; do
  read -r NAME OPTION <<< "$pair"
  echo "$NAME, OPTION=$OPTION"

  BUILD_DIR=build_miyabi_g_cpu_${NAME}
  COMPILE_LOG="cmake_miyabi_c_cpu${NAME}.log"
  cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT \
  -DCMAKE_CXX_FLAGS="${OPTION}" &> ${COMPILE_LOG}
  cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}
  RUN_SCRIPT="run_miyabi_g_cpu.sh"
  cp -a ${RUN_SCRIPT} ./${BUILD_DIR}

  pushd ${BUILD_DIR}
  qsub ${RUN_SCRIPT}
  popd

done



#!/bin/bash

# NVIDIA HPC SDK
pairs=(
  "O2 -O2"
  "O3 -O3"
  "O4 -O4"
  "fast -fast"
)

module purge
module load nvidia/23.3
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02

for pair in "${pairs[@]}"; do
  read -r NAME OPTION <<< "$pair"
  echo "$NAME, OPTION=$OPTION"

  BUILD_DIR=build_wisteria_a_gpu_${NAME}
  COMPILE_LOG="cmake_wisteria_a_gpu_${NAME}.log"
  cmake -S . -B ${BUILD_DIR}  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT -DCMAKE_CXX_FLAGS="${OPTION}" &> ${COMPILE_LOG}
  cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}

  RUN_SCRIPT="run_wisteria_a_gpu.sh"
  cp -a run_wisteria_a_gpu.sh ./${BUILD_DIR}/${RUN_SCRIPT}
  pushd ${BUILD_DIR}
  pjsub ${RUN_SCRIPT}
  popd

done


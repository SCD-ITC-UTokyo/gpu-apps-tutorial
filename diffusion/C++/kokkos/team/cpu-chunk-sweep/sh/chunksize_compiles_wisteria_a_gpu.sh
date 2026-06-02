
#!/bin/bash

pairs=(
  "chunk1 -D_CHUNK=1"
  "chunk2 -D_CHUNK=2"
  "chunk4 -D_CHUNK=4"
  "chunk8 -D_CHUNK=8"
  "chunk16 -D_CHUNK=16"
  "chunk32 -D_CHUNK=32"
  "chunk64 -D_CHUNK=64"
  "chunk128 -D_CHUNK=128"
  "chunk256 -D_CHUNK=256"
  "chunk512 -D_CHUNK=512"
)

module purge
module load nvidia/23.3
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02

for pair in "${pairs[@]}"; do
  read -r NAME CHUNK <<< "$pair"
  echo "$NAME, $CHUNK"

  BUILD_DIR=build_wisteria_a_gpu_${NAME}
  COMPILE_LOG="cmake_wisteria_a_gpu_${NAME}.log"
  cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT \
  -DCMAKE_CXX_FLAGS="-fast ${CHUNK}" &> ${COMPILE_LOG}
  cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}

  RUN_SCRIPT="run_wisteria_a_gpu.sh"
  cp -a run_wisteria_a_gpu.sh ./${BUILD_DIR}/${RUN_SCRIPT}
  pushd ${BUILD_DIR}
  
  pjsub ${RUN_SCRIPT}
  popd

done



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
module load intel/2025.2.0
module list

export KOKKOS_DIR=/xxx/kokkos/5.0.2

for pair in "${pairs[@]}"; do
  read -r NAME CHUNK <<< "$pair"
  echo "$NAME, $CHUNK"

  BUILD_DIR=build_miyabi_c_cpu_${NAME}
  COMPILE_LOG="cmake_miyabi_c_cpu_${NAME}.log"
  cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_C_COMPILER=icx \
  -DCMAKE_CXX_COMPILER=icpx \
  -DCMAKE_CXX_FLAGS="-O3 -march=sapphirerapids ${CHUNK}" \
  -DCMAKE_PREFIX_PATH=$KOKKOS_DIR &> ${COMPILE_LOG}
  cmake --build ${BUILD_DIR} -j  -v &>> ${COMPILE_LOG}

  RUN_SCRIPT="run_miyabi_c_cpu.sh"
  cp -a run_miyabi_c_cpu.sh ./${BUILD_DIR}/${RUN_SCRIPT}
  pushd ${BUILD_DIR}
  
  qsub ${RUN_SCRIPT}
  popd

done


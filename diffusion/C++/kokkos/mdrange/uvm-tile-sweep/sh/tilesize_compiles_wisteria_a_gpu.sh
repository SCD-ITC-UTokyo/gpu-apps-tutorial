
#!/bin/bash

pairs=(
  "x1y2z16 -D_NX=1 -D_NY=2 -D_NZ=16"
  "x1y2z32 -D_NX=1 -D_NY=2 -D_NZ=32"
  "x1y2z64 -D_NX=1 -D_NY=2 -D_NZ=64"
  "x1y4z16 -D_NX=1 -D_NY=4 -D_NZ=16"
  "x1y4z32 -D_NX=1 -D_NY=4 -D_NZ=32"
  "x1y4z64 -D_NX=1 -D_NY=4 -D_NZ=64"
  "x1y8z16 -D_NX=1 -D_NY=8 -D_NZ=16"
  "x1y8z32 -D_NX=1 -D_NY=8 -D_NZ=32"
  "x1y8z64 -D_NX=1 -D_NY=8 -D_NZ=64"
  "x2y2z16 -D_NX=2 -D_NY=2 -D_NZ=16"
  "x2y2z32 -D_NX=2 -D_NY=2 -D_NZ=32"
  "x2y2z64 -D_NX=2 -D_NY=2 -D_NZ=64"
  "x2y4z16 -D_NX=2 -D_NY=4 -D_NZ=16"
  "x2y4z32 -D_NX=2 -D_NY=4 -D_NZ=32"
  "x2y4z64 -D_NX=2 -D_NY=4 -D_NZ=64"
  "x2y8z16 -D_NX=2 -D_NY=8 -D_NZ=16"
  "x2y8z32 -D_NX=2 -D_NY=8 -D_NZ=32"
  "x2y8z64 -D_NX=2 -D_NY=8 -D_NZ=64"
  "x4y8z64 -D_NX=4 -D_NY=8 -D_NZ=64"
)

module purge
module load nvidia/23.3
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02

for pair in "${pairs[@]}"; do
  read -r NAME NZ NY NX <<< "$pair"
  echo "$NAME, $NZ, $NY, $NX"

  BUILD_DIR=build_wisteria_a_gpu_${NAME}
  COMPILE_LOG="cmake_wisteria_a_gpu_${NAME}.log"
  cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT \
  -DCMAKE_CXX_FLAGS="-fast ${NX} ${NY} ${NZ}" &> ${COMPILE_LOG}
  cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}

  RUN_SCRIPT="run_wisteria_a_gpu.sh"
  cp -a run_wisteria_a_gpu.sh ./${BUILD_DIR}/${RUN_SCRIPT}
  pushd ${BUILD_DIR}
  
  pjsub ${RUN_SCRIPT}
  popd

done


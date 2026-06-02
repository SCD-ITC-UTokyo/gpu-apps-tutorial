#!/bin/sh
#------ pjsub option --------#
#PJM -L rscgrp=debug-o
#PJM -L node=1
#PJM --omp thread=48
#PJM -L elapse=00:30:00
#PJM -g xxx
#PJM -j
#PJM -S
#------- Program execution -------#

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
module load fj/1.2.42
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02
CMAKE=/xxx/cmake/3.29.6/bin/cmake

for pair in "${pairs[@]}"; do
  read -r NAME NZ NY NX <<< "$pair"
  echo "$NAME, $NZ, $NY, $NX"

  BUILD_DIR=build_wisteria_o_cpu_${NAME}
  COMPILE_LOG="cmake_wisteria_o_cpu_${NAME}.log"

${CMAKE} -S . -B ${BUILD_DIR}  \
  -DCMAKE_CXX_COMPILER=FCC \
  -DCMAKE_CXX_FLAGS="-Nclang -Kfast ${NX} ${NY} ${NZ}" \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT &> ${COMPILE_LOG}
${CMAKE} --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}


  RUN_SCRIPT="run_wisteria_o_cpu.sh"
  cp -a ${RUN_SCRIPT} ./${BUILD_DIR}/${RUN_SCRIPT}
done


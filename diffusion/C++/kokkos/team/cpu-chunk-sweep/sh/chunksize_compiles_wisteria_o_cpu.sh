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
module load fj/1.2.42
module list

export KOKKOS_ROOT=/xxx/kokkos/4.7.02
CMAKE=/xxx/cmake/3.29.6/bin/cmake

for pair in "${pairs[@]}"; do
  read -r NAME CHUNK <<< "$pair"
  echo "$NAME, $CHUNK"

  BUILD_DIR=build_wisteria_o_cpu_${NAME}
  COMPILE_LOG="cmake_wisteria_o_cpu_${NAME}.log"

${CMAKE} -S . -B ${BUILD_DIR}  \
  -DCMAKE_CXX_COMPILER=FCC \
  -DCMAKE_CXX_FLAGS="-Nclang -Kfast ${CHUNK}" \
  -DCMAKE_PREFIX_PATH=$KOKKOS_ROOT &> ${COMPILE_LOG}
${CMAKE} --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}


  RUN_SCRIPT="run_wisteria_o_cpu.sh"
  cp -a run_wisteria_o_cpu.sh ./${BUILD_DIR}/${RUN_SCRIPT}
done


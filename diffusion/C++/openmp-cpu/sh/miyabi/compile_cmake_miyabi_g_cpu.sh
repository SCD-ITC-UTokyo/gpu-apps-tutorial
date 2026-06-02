#!/bin/bash


module purge
module load nvidia/25.9
module list

COMPILE_LOG="cmake_miyabi_g_cpu.log"
BUILD_DIR=build_miyabi_g_cpu
cmake -S . -B ${BUILD_DIR}  &> ${COMPILE_LOG}
cmake --build ${BUILD_DIR} -j -v &>> ${COMPILE_LOG}


# compile direct N-body code (base implementation for multicore CPUs)
project(base CXX)
message(STATUS "Generate ${PROJECT_NAME} ...")

# define the executable
set(SOURCES ${PROJECT_SOURCE_DIR}/nbody.cpp)

add_executable(${PROJECT_NAME}
    ${SOURCES}
)

# apply common settings
include(${USER_MODULE_PATH}/apply_common_settings.cmake)

# add configuration for NVIDIA HPC SDK
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<BOOL:${OpenMP_FOUND}>,$<CXX_COMPILER_ID:NVHPC>>:${OpenMP_CXX_FLAGS}=multicore>
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    $<$<AND:$<BOOL:${OpenMP_FOUND}>,$<CXX_COMPILER_ID:NVHPC>>:${OpenMP_CXX_FLAGS}=multicore>
)

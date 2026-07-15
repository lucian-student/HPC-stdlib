
add_library(
    hpc_stdlib
    INTERFACE
)

target_include_directories(
    hpc_stdlib 
    INTERFACE 
    "${PROJECT_SOURCE_DIR}/include"
)


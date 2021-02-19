
cmake_minimum_required (VERSION 3.14.7)

project(vorbrodt_blog)

set(vorbrodt_target_name ${PROJECT_NAME})
set(vorbrodt_include_dir "${PROJECT_SOURCE_DIR}/include/")

add_library(${vorbrodt_target_name} INTERFACE)

target_compile_features(${vorbrodt_target_name} INTERFACE cxx_std_17)

target_include_directories( ${vorbrodt_target_name} INTERFACE
    ${vorbrodt_include_dir}
)
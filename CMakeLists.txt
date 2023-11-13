cmake_minimum_required(VERSION 3.17)

project(dcast C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(MSVC_DEFS "_CRT_SECURE_NO_DEPRECATE;_CRT_SILENCE_NONCONFORMING_TGMATH_H")

add_compile_definitions("$<$<BOOL:${MSVC}>:${MSVC_DEFS}>;OPT_USE_ALLOCA=1")

add_compile_options("$<$<BOOL:${MSVC}>:/Zc:__cplusplus;/openmp>")

add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/app)
add_subdirectory(${CMAKE_SOURCE_DIR}/spin)

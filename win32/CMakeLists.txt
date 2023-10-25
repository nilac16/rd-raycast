cmake_minimum_required(VERSION 3.15)

project(dcast C CXX)

find_package(DCMTK REQUIRED)
find_package(OpenMP REQUIRED)

set(OPT_FLAGS "/O2")
set(WARN_FLAGS "/W3")
set(ARCH_FLAGS "/arch:AVX2")
set(XTRA_FLAGS "/Zc:__cplusplus /openmp")

set(ALL_FLAGS "${OPT_FLAGS} ${WARN_FLAGS} ${ARCH_FLAGS} ${XTRA_FLAGS}")
set(DEFINES "/D_CRT_SECURE_NO_DEPRECATE /D_CRT_SILENCE_NONCONFORMING_TGMATH_H")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_FLAGS} ${DEFINES}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ALL_FLAGS} ${DEFINES}")

add_executable(${PROJECT_NAME}
                ../main.c
                app.c
                error.c
                ../cmap.c
                ../raycast.c
                ../dose.cc
                ../rcmath.c)

target_include_directories(${PROJECT_NAME}
                    PUBLIC ${CMAKE_SOURCE_DIR}/..)

target_link_libraries(${PROJECT_NAME}
               PUBLIC DCMTK::DCMTK
                      OpenMP::OpenMP_C)

set_property(TARGET ${PROJECT_NAME}
             PROPERTY MSVC_RUNTIME_LIBRARY
                      "MultiThreaded$<$<CONFIG:Debug>:Debug>")

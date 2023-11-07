cmake_minimum_required(VERSION 3.17)

project(dcast C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
find_package(DCMTK REQUIRED)
find_package(OpenMP REQUIRED)
find_package(WebP REQUIRED)

find_path(OPT_PATH opt.h REQUIRED HINTS ${OPT_ROOT})


set(OPT_FLAGS "-Og -g")
set(WARN_FLAGS "-Wall -Wextra -W -Werror -pedantic")
set(ARCH_FLAGS "-mavx2 -mfma -mbmi")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(ALL_FLAGS "${OPT_FLAGS} ${WARN_FLAGS} ${ARCH_FLAGS} ${XTRA_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ALL_FLAGS}")

add_executable(${PROJECT_NAME}
              main.c
              app.c
              cmap.c
              raycast.c
              dose.cc
              rcmath.c)

target_link_libraries(${PROJECT_NAME}
               PUBLIC ${DCMTK_LIBRARIES}
                      ${SDL2_LIBRARIES}
                      OpenMP::OpenMP_C)


add_executable(rdreport
              report.c
              stb.c
              cmap.c
              raycast.c
              dose.cc
              rcmath.c)

target_link_libraries(rdreport
               PUBLIC ${DCMTK_LIBRARIES}
                      OpenMP::OpenMP_C)


add_executable(spin
              spin/spin.c
              spin/params.c
              spin/anim.c
              cmap.c
              raycast.c
              dose.cc
              rcmath.c)

target_link_libraries(spin
               PUBLIC ${DCMTK_LIBRARIES}
                      ${WebP_LIBRARIES}
                      OpenMP::OpenMP_C)

target_include_directories(spin
                    PUBLIC ${OPT_PATH}
                           ${CMAKE_SOURCE_DIR})

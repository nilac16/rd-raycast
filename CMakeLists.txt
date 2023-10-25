cmake_minimum_required(VERSION 3.15)

project(dcast C CXX)

find_package(SDL2 REQUIRED)
find_package(DCMTK REQUIRED)
find_package(OpenMP REQUIRED)

set(OPT_FLAGS "-O2 -g0")
set(WARN_FLAGS "-Wall -Wextra -W -Werror -pedantic")
set(ARCH_FLAGS "-march=znver3")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(ALL_FLAGS "${OPT_FLAGS} ${WARN_FLAGS} ${ARCH_FLAGS} ${XTRA_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ALL_FLAGS}")

add_executable(${PROJECT_NAME} main.c app.c cmap.c raycast.c dose.cc rcmath.c)

target_link_libraries(${PROJECT_NAME}
               PUBLIC ${DCMTK_LIBRARIES}
                      ${SDL2_LIBRARIES}
                      OpenMP::OpenMP_C)

add_executable(rdreport report.c stb.c cmap.c raycast.c dose.cc rcmath.c)

target_link_libraries(rdreport
               PUBLIC ${DCMTK_LIBRARIES}
                      OpenMP::OpenMP_C)

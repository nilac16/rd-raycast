cmake_minimum_required(VERSION 3.15)

project(dcast C CXX)

find_package(SDL2 REQUIRED)
find_package(DCMTK REQUIRED)
find_package(OpenMP REQUIRED)

set(OPT_FLAGS "-O2 -g0")
set(STD_FLAGS "-std=c99 -pedantic")
set(WARN_FLAGS "-Wall -Wextra -W -Werror")
set(ARCH_FLAGS "-march=znver3")

set(ALL_FLAGS "${OPT_FLAGS} ${WARN_FLAGS} ${ARCH_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ALL_FLAGS}")

add_executable(${PROJECT_NAME} main.c app.c cmap.c raycast.c dose.cc math.c)

target_link_libraries(${PROJECT_NAME}
               PUBLIC ${DCMTK_LIBRARIES}
                      ${SDL2_LIBRARIES}
                      OpenMP::OpenMP_C)

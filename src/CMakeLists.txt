# TODO: Figure out how to make this an exported TARGET
set(RD_RAYCAST_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR} PARENT_SCOPE)
set(RD_RAYCAST_LIBRARIES rd-raycast PARENT_SCOPE)

find_package(DCMTK REQUIRED)
find_package(OpenMP REQUIRED)

find_library(CMATH_LIBRARY m)

if (NOT CMATH_LIBRARY)
    set(CMATH_LIBRARIES "")
else ()
    set(CMATH_LIBRARIES ${CMATH_LIBRARY})
endif ()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_library(rd-raycast
            rcmath.c
            raycast.c
            dose.cc
            cmap.c)

target_link_libraries(rd-raycast
               PUBLIC ${CMATH_LIBRARIES} OpenMP::OpenMP_C
              PRIVATE DCMTK::DCMTK)

# Add optimization flags
if(MSVC)
    list(APPEND CFLAGS $<IF:$<CONFIG:Debug>,,/O2>)
else()
    list(APPEND CFLAGS $<IF:$<CONFIG:Debug>,-Og;-g,-O2>)
endif()

# Add warning flags
list(APPEND CFLAGS $<IF:$<BOOL:${MSVC}>,/W3,-W;-Wall;-Wextra;-Werror>)

# Add architecture flags
list(APPEND CFLAGS $<IF:$<BOOL:${MSVC}>,/arch:AVX2,-mavx2;-mfma;-mbmi>)

target_compile_options(rd-raycast PRIVATE ${CFLAGS})

set_property(TARGET rd-raycast
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

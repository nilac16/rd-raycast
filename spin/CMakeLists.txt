find_package(WebP REQUIRED)

find_path(OPT_PATH opt.h REQUIRED HINTS ${OPT_ROOT})

set(EXE_NAME spin)

add_executable(${EXE_NAME}
               spin.c
               params.c
               anim.c)

target_link_libraries(${EXE_NAME}
               PUBLIC ${WebP_LIBRARIES} ${RD_RAYCAST_LIBRARIES})

target_include_directories(${EXE_NAME}
                    PUBLIC ${WEBP_INCLUDE_DIRS} ${OPT_PATH}
                           ${RD_RAYCAST_INCLUDE_DIRS})


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

target_compile_options(${EXE_NAME} PRIVATE ${CFLAGS})

set_property(TARGET ${EXE_NAME}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

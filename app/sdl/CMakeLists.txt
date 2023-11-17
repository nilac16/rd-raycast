find_package(SDL2 REQUIRED)

list(APPEND MAIN_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR})

set(EXE_NAME ${PROJECT_NAME})

add_executable(${EXE_NAME}
               ${MAIN_FILES}
               app.c)

target_link_libraries(${EXE_NAME}
               PUBLIC ${SDL2_LIBRARIES} ${RD_RAYCAST_LIBRARIES})

target_include_directories(${EXE_NAME}
                    PUBLIC ${MAIN_INCLUDE_DIRS}
                           ${RD_RAYCAST_INCLUDE_DIRS})

# Add optimization flags
list(APPEND CFLAGS $<IF:$<BOOL:${MSVC}>,/O2,$<IF:$<CONFIG:Debug>,-Og;-g,-O2>>)

# Add warning flags
list(APPEND CFLAGS $<IF:$<BOOL:${MSVC}>,/W3,-W;-Wall;-Wextra;-Werror>)

# Add architecture flags
list(APPEND CFLAGS $<IF:$<BOOL:${MSVC}>,/arch:AVX2,-mavx2;-mfma;-mbmi>)

list(APPEND CFLAGS $<$<BOOL:${MSVC}>:/Zc:__cplusplus>)

target_compile_options(${EXE_NAME} PRIVATE ${CFLAGS})

set_property(TARGET ${EXE_NAME}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

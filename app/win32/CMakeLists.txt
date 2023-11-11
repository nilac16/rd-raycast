list(APPEND MAIN_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR})

add_executable(${PROJECT_NAME}
               ${MAIN_FILE}
               app.c
               error.c)

target_link_libraries(${PROJECT_NAME}
               PUBLIC ${RD_RAYCAST_LIBRARIES})

target_include_directories(${PROJECT_NAME}
                    PUBLIC ${MAIN_INCLUDE_DIRS}
                           ${RD_RAYCAST_INCLUDE_DIRS})

target_compile_options(${PROJECT_NAME}
       PUBLIC $<IF:$<CONFIG:Debug>,,/O2> /W3 /arch:AVX2)

set_property(TARGET ${PROJECT_NAME}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

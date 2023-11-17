set(MAIN_FILES
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/params.c)

find_path(OPT_PATH opt.h REQUIRED HINTS ${OPT_ROOT})

list(APPEND MAIN_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR};${OPT_PATH})

if (WIN32)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/win32)
else ()
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/sdl)
endif ()

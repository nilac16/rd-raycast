set(MAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/main.c)

list(APPEND MAIN_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR})

if (WIN32)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/win32)
else ()
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/sdl)
endif ()

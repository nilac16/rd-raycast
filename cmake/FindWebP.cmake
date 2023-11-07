# Sets WebP_LIBRARIES to the relevant paths
# Maybe add support for individual components

message(CHECK_START "Searching for libwebp binaries")

foreach (lib webp webpmux webpdemux)
    find_library(WebP_${lib}_LIB ${lib})
    mark_as_advanced(WebP_${lib}_LIB)

    if (WebP_${lib}_LIB)
        list(APPEND WebP_LIBRARIES ${WebP_${lib}_LIB})
    endif ()
endforeach ()

if (NOT WebP_LIBRARIES)
    message(CHECK_FAIL "not found")
    set(WebP_LIBRARIES WebP_LIBRARIES-NOTFOUND)
else ()
    message(CHECK_PASS "found")
endif ()

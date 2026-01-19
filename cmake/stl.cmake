# Sets include directory for external STL lib.
# User can provide custom STL lib path if it is already installed by setting IT_STL_PATH variable.
set(IT_STL_PATH "" CACHE STRING "Path to STL common lib.")
if (IT_STL_PATH STREQUAL "")
    include(FetchContent)

    FetchContent_Declare(
        stl
        GIT_REPOSITORY https://github.com/alcolic-to/stl.git
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)

    FetchContent_MakeAvailable(stl)

    message("Using internal STL lib ${stl_SOURCE_DIR} for IT.")
    set(IT_STL_PATH ${stl_SOURCE_DIR} CACHE STRING "Path to STL common lib." FORCE)
    message("IT_STL_PATH value: ${IT_STL_PATH}")
else ()
    set(IT_STL_PATH ${IT_STL_PATH} CACHE STRING "Path to STL common lib." FORCE)
    message("Using external STL lib ${IT_STL_PATH} for IT.")
endif ()

target_include_directories(it SYSTEM PUBLIC ${IT_STL_PATH})

# Sets include directory for external STL lib.
# User can provide custom STL lib path if it is already installed by setting TT_STL_PATH variable.
set(TT_STL_PATH "" CACHE STRING "Path to STL common lib.")
if (TT_STL_PATH STREQUAL "")
    include(FetchContent)

    FetchContent_Declare(
        stl
        GIT_REPOSITORY https://github.com/alcolic-to/stl.git
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)

    FetchContent_MakeAvailable(stl)

    message("Using internal STL lib ${stl_SOURCE_DIR} for TT.")
    set(TT_STL_PATH ${stl_SOURCE_DIR} CACHE STRING "Path to STL common lib." FORCE)
    message("TT_STL_PATH value: ${TT_STL_PATH}")
else ()
    set(TT_STL_PATH ${TT_STL_PATH} CACHE STRING "Path to STL common lib." FORCE)
    message("Using external STL lib ${TT_STL_PATH} for TT.")
endif ()

target_include_directories(tt SYSTEM PUBLIC ${TT_STL_PATH})

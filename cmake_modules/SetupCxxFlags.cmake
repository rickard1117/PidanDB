set(CXX_COMMON_FLAGS "-Wall -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic")
set(CXX_ONLY_FLAGS "-std=c++17 -fPIC -mcx16 -march=native -fvisibility=hidden")

# if no build build type is specified, default to debug builds
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)

# Set compile flags based on the build type.
message("Configured for ${CMAKE_BUILD_TYPE} build (set with cmake -DCMAKE_BUILD_TYPE={release,debug,fastdebug,relwithdebinfo})")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
    set(CXX_OPTIMIZATION_FLAGS "-ggdb -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "FASTDEBUG")
    set(CXX_OPTIMIZATION_FLAGS "-ggdb -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
    set(CXX_OPTIMIZATION_FLAGS "-O3 -DNDEBUG")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RELWITHDEBINFO")
    set(CXX_OPTIMIZATION_FLAGS "-ggdb -O2 -DNDEBUG")
else ()
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif ()

set(CMAKE_CXX_FLAGS "${CXX_OPTIMIZATION_FLAGS}")
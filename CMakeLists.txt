cmake_minimum_required (VERSION 3.22)

set(ARPACK "ARPack")
project(${ARPACK} VERSION 0.0.3 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Default build type: Debug" FORCE)
endif()

# Generate and executable.
file(GLOB_RECURSE ARI_SRC "src/*.cpp" "external/utils/src/string_utils.cpp" "src/Arithmetic.rc")
file(GLOB_RECURSE CLI_SRC "external/cli/*.cpp")
file(GLOB_RECURSE LIBSAIS_SRC "external/libsais/src/libsais.c" "external/libsais/src/libsais64.c")

# Generate the test executable.
add_executable(${ARPACK} ${ARI_SRC} ${CLI_SRC} ${LIBSAIS_SRC})

# Set minimal C++ version to C++20
target_compile_features(${ARPACK} PRIVATE cxx_std_20)

set(CMAKE_C_COMPILER gcc-13)
set(CMAKE_CXX_COMPILER g++-13)

# Add libraries include path.
target_include_directories(${ARPACK} PRIVATE 
"${CMAKE_SOURCE_DIR}/include"
"${CMAKE_SOURCE_DIR}/external"
"${CMAKE_SOURCE_DIR}/external/libsais/include"
)

target_compile_definitions(${ARPACK} PRIVATE LOGENGINE_HEADER_ONLY)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add "_DEBUG" definition to the source code.
    target_compile_definitions(${ARPACK} PRIVATE _DEBUG)
else()
    # Add "NDEBUG" definition to the source code.
    target_compile_definitions(${ARPACK} PRIVATE NDEBUG)
endif()

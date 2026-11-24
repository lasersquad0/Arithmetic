cmake_minimum_required (VERSION 3.22)

set(ARPACK "ARPack")
set(ARPACK_VER "0.0.3")
project(${ARPACK} VERSION ${ARPACK_VER} LANGUAGES C CXX)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()

# define runtime library type - Threaded or Threaded DLL 
# use "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" for Threaded DLL runtime type 
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library")
endif()

# Generate and executable.
file(GLOB ARI_SRC CONFIGURE_DEPENDS "src/*.cpp" "external/utils/src/string_utils.cpp" "src/Arithmetic.rc")
file(GLOB CLI_SRC CONFIGURE_DEPENDS "external/cli/*.cpp")
file(GLOB LIBSAIS_SRC CONFIGURE_DEPENDS "external/libsais/src/libsais.c" "external/libsais/src/libsais64.c")

message(STATUS "CMAKE_MSVC_RUNTIME_LIBRARY: ${CMAKE_MSVC_RUNTIME_LIBRARY}")

# Generate the test executable.
add_executable(${ARPACK} ${ARI_SRC} ${CLI_SRC} ${LIBSAIS_SRC})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set minimal C++ version to C++20
target_compile_features(${ARPACK} PRIVATE cxx_std_20)

if(NOT MSVC)
    set(CMAKE_C_COMPILER gcc-13)
    set(CMAKE_CXX_COMPILER g++-13)
endif()

# Add libraries include path.
target_include_directories(${ARPACK} PRIVATE 
"${PROJECT_SOURCE_DIR}/include"
"${PROJECT_SOURCE_DIR}/external"
"${PROJECT_SOURCE_DIR}/external/libsais/include"
)

target_compile_definitions(${ARPACK} PRIVATE LOGENGINE_HEADER_ONLY)

target_compile_options(${ARPACK} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall
        -Wextra
        -Wpedantic
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /permissive-
        /EHsc
    >
)

#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add "_DEBUG" definition to the source code.
#    target_compile_definitions(${ARPACK} PRIVATE _DEBUG)
#else()
    # Add "NDEBUG" definition to the source code.
#    target_compile_definitions(${ARPACK} PRIVATE NDEBUG)
#endif()

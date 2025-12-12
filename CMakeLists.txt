cmake_minimum_required(VERSION 3.15)
project(rethinkdb VERSION 2.4.5 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(USE_CCACHE "Use ccache for compilation" ON)
option(BUILD_TESTING "Build tests" OFF)
option(USE_JEMALLOC "Use jemalloc allocator" ON)
option(STATIC_LINK "Statically link dependencies" OFF)

# Installation directories
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix")
set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc" CACHE PATH "Configuration directory")
set(LOCALSTATEDIR "${CMAKE_INSTALL_PREFIX}/var" CACHE PATH "Runtime data directory")

# Detect OS
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OS "Darwin")
    set(OSX_MIN_VERSION "10.9")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS "Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(OS "FreeBSD")
endif()

# Use ccache if available
if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    endif()
endif()

# Conan dependencies are handled via conanfile.py
# Run: conan install . --output-folder=build --build=missing
# before running cmake

# Find required packages
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

find_package(Boost REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(re2 REQUIRED)
find_package(quickjs REQUIRED)

if(USE_JEMALLOC)
    find_package(jemalloc REQUIRED)
endif()

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# Find protoc - prefer protobuf@21 from Homebrew for compatibility
find_program(PROTOC_EXECUTABLE
    NAMES protoc
    HINTS 
        /opt/homebrew/opt/protobuf@21/bin
        /usr/local/opt/protobuf@21/bin
        ${Protobuf_INCLUDE_DIRS}/../bin
    REQUIRED
)
message(STATUS "Found protoc: ${PROTOC_EXECUTABLE}")

# Platform-specific libraries
if(OS STREQUAL "Linux")
    set(PTHREAD_LIBS pthread)
    set(RT_LIBS rt)
    set(DL_LIBS dl)
elseif(OS STREQUAL "Darwin")
    set(PTHREAD_LIBS "")
    set(RT_LIBS "")
    set(DL_LIBS "")
elseif(OS STREQUAL "FreeBSD")
    set(PTHREAD_LIBS pthread)
    set(RT_LIBS "")
    set(DL_LIBS "")
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-deprecated-declarations
        -fno-strict-aliasing
    )
    
    # Define build machine info
    add_compile_definitions(BUILD_MACHINE="${CMAKE_SYSTEM_NAME}")
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${Protobuf_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
)

# Generate version.hpp
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.hpp.in
    ${CMAKE_BINARY_DIR}/src/rethinkdb_version.hpp
    @ONLY
)

# Add subdirectories
add_subdirectory(src)

# Print configuration summary
message(STATUS "")
message(STATUS "RethinkDB Configuration Summary:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  C++ Compiler:      ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Operating System:  ${OS}")
message(STATUS "  Install Prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Use ccache:        ${USE_CCACHE}")
message(STATUS "  Use jemalloc:      ${USE_JEMALLOC}")
message(STATUS "  Build Testing:     ${BUILD_TESTING}")
message(STATUS "  Static Link:       ${STATIC_LINK}")
message(STATUS "")

## @file src/CMakeLists.txt
## @brief CMake configuration for RethinkDB source compilation
##
## This CMakeLists.txt handles:
## - Protocol Buffer code generation from .proto files
## - Collection and compilation of all RethinkDB source files
## - Linking of external dependencies (Boost, OpenSSL, Protobuf, etc.)
## - Installation of the rethinkdb executable
##
## ## Key Components
##
## 1. **Protobuf Compilation**: Automatically compiles ql2.proto to C++ code
## 2. **Source Collection**: Recursively finds all .cc files in src/
## 3. **Executable Creation**: Links all sources into the main rethinkdb binary
## 4. **Dependency Linking**: Connects external libraries (Boost, OpenSSL, jemalloc, QuickJS)

cmake_minimum_required(VERSION 3.15)

## @defgroup ProtobufGeneration Protocol Buffer Code Generation
## Automatically generates C++ code from .proto files
## @{

## Define the protocol buffer files to compile
set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/rdb_protocol/ql2.proto
)

## Create output directory for generated protobuf files
set(PROTO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rdb_protocol")
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})

set(PROTO_SRCS)
set(PROTO_HDRS)

## Generate C++ files from each .proto file using protoc
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    set(PROTO_SRC "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.h")
    
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    
    ## Custom command to invoke protoc compiler
    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${PROTOC_EXECUTABLE}
            --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
            -I${CMAKE_CURRENT_SOURCE_DIR}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating ${PROTO_NAME} protobuf files with ${PROTOC_EXECUTABLE}"
        VERBATIM
    )
endforeach()

## @}

## @defgroup SourceCollection Source Code Collection and Compilation
## Finds and compiles all RethinkDB source files
## @{

## Recursively collect all C++ source files from the src directory
file(GLOB_RECURSE RETHINKDB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

## Remove unit test files unless explicitly requested
## Test files are identified by paths containing "unittest" or filenames ending in "test.cc"
if(NOT BUILD_TESTING)
    list(FILTER RETHINKDB_SOURCES EXCLUDE REGEX ".*unittest/.*")
    list(FILTER RETHINKDB_SOURCES EXCLUDE REGEX ".*test\\.cc$")
    message(STATUS "Excluded unit tests from build (set BUILD_TESTING=ON to include)")
endif()

message(STATUS "Collected ${CMAKE_CURRENT_SOURCE_DIR}: ${RETHINKDB_SOURCES}")

## Create the main RethinkDB executable
## Combines all source files and generated protobuf code
add_executable(rethinkdb
    ${RETHINKDB_SOURCES}
    ${PROTO_SRCS}
)

## Configure include directories for the executable
## Allows access to generated protobuf headers and local includes
target_include_directories(rethinkdb
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/src
)

## @}

## @defgroup DependencyLinking External Dependency Linking
## Configures linking of all required external libraries
## @{

## Link all required external libraries to the executable
target_link_libraries(rethinkdb
    PRIVATE
        Boost::boost
        protobuf::libprotobuf
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        CURL::libcurl
        re2::re2
        quickjs::quickjs
        ${PTHREAD_LIBS}
        ${RT_LIBS}
        ${DL_LIBS}
)

if(USE_JEMALLOC)
    target_link_libraries(rethinkdb PRIVATE jemalloc::jemalloc)
endif()

# Installation
install(TARGETS rethinkdb
    RUNTIME DESTINATION bin
)

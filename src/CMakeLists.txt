# Basic CMakeLists.txt for src directory
# This is a placeholder - needs to be expanded with actual source files

cmake_minimum_required(VERSION 3.15)

# Generate protobuf files
set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/rdb_protocol/ql2.proto
)

set(PROTO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rdb_protocol")
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})

set(PROTO_SRCS)
set(PROTO_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    set(PROTO_SRC "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.h")
    
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    
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

# Collect all source files
file(GLOB_RECURSE RETHINKDB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

# Remove test files if not building tests
if(NOT BUILD_TESTING)
    list(FILTER RETHINKDB_SOURCES EXCLUDE REGEX ".*unittest/.*")
    list(FILTER RETHINKDB_SOURCES EXCLUDE REGEX ".*test\\.cc$")
endif()


# Create the main executable
add_executable(rethinkdb
    ${RETHINKDB_SOURCES}
    ${PROTO_SRCS}
)

# Add include directories for generated protobuf headers and version.hpp
target_include_directories(rethinkdb
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/src
)

# Link libraries
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

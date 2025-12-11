# Basic CMakeLists.txt for src directory
# This is a placeholder - needs to be expanded with actual source files

cmake_minimum_required(VERSION 3.15)

# Collect all source files
file(GLOB_RECURSE RETHINKDB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

# Remove test files if not building tests
if(NOT BUILD_TESTING)
    list(FILTER RETHINKDB_SOURCES EXCLUDE REGEX ".*unittest/.*")
    list(FILTER RETHINKDB_SOURCES EXCLUDE REGEX ".*test\\.cc$")
endif()

# TODO: Generate protobuf files
# This needs to be implemented based on the actual .proto files in the project

# Create the main executable
add_executable(rethinkdb
    ${RETHINKDB_SOURCES}
)

# Link libraries
target_link_libraries(rethinkdb
    PRIVATE
        protobuf::libprotobuf
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        CURL::libcurl
        re2::re2
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

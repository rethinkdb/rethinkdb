# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build Protobuf.

include(FetchContent)

set(_rethinkdb_protobuf_version "36.2")
string(CONCAT _rethinkdb_protobuf_url
    "https://github.com/protocolbuffers/protobuf/releases/download/v${_rethinkdb_protobuf_version}"
    "/protobuf-${_rethinkdb_protobuf_version}.tar.gz")
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(utf8_range_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

set(_rethinkdb_saved_build_testing "${BUILD_TESTING}")
set(BUILD_TESTING OFF)
FetchContent_Declare(protobuf
    URL "${_rethinkdb_protobuf_url}"
    URL_HASH "SHA256=3d9642a662d10e68ebae5e53f14dcce5105684212d5078f8e0d47d1ab3ae6b64"
    DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=<SOURCE_DIR>"
        "-DPATCHES=${CMAKE_SOURCE_DIR}/cmake/deps/patches/protobuf_0-cmp0141-new.patch"
        -P "${CMAKE_SOURCE_DIR}/cmake/apply_patches.cmake"
)
FetchContent_MakeAvailable(protobuf)
set(BUILD_TESTING "${_rethinkdb_saved_build_testing}")

# Protobuf 36 still calls its own deprecated reflection APIs (GetEnum,
# GetRepeatedEnum). Those calls are removed upstream in Q1 2027. Silence them
# on the fetched targets so they do not hide deprecations in RethinkDB.
function(rethinkdb_silence_protobuf_deprecations directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        if(NOT TARGET "${target}")
            continue()
        endif()
        get_target_property(aliased "${target}" ALIASED_TARGET)
        if(aliased)
            continue()
        endif()
        get_target_property(target_type "${target}" TYPE)
        if(NOT target_type MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|OBJECT_LIBRARY|MODULE_LIBRARY|EXECUTABLE)$")
            continue()
        endif()
        target_compile_options("${target}" PRIVATE
            $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/wd4996>
            $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Wno-deprecated-declarations>
        )
    endforeach()
    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(subdirectory IN LISTS subdirectories)
        rethinkdb_silence_protobuf_deprecations("${subdirectory}")
    endforeach()
endfunction()

if(protobuf_SOURCE_DIR)
    rethinkdb_silence_protobuf_deprecations("${protobuf_SOURCE_DIR}")
endif()

add_library(RethinkDB_protobuf INTERFACE)
add_library(RethinkDB::protobuf ALIAS RethinkDB_protobuf)
if(TARGET protobuf::libprotobuf)
    target_link_libraries(RethinkDB_protobuf INTERFACE protobuf::libprotobuf)
elseif(TARGET libprotobuf)
    target_link_libraries(RethinkDB_protobuf INTERFACE libprotobuf)
else()
    message(FATAL_ERROR "The Protobuf build did not produce libprotobuf")
endif()

if(TARGET protobuf::protoc)
    set(RETHINKDB_PROTOC_TARGET protobuf::protoc)
elseif(TARGET protoc)
    set(RETHINKDB_PROTOC_TARGET protoc)
else()
    message(FATAL_ERROR "The Protobuf build did not produce protoc")
endif()

FetchContent_GetProperties(protobuf)
if(protobuf_SOURCE_DIR)
    rethinkdb_record_dist_source(protobuf "${protobuf_SOURCE_DIR}")
endif()
message(STATUS "Protobuf ${_rethinkdb_protobuf_version} will be built")

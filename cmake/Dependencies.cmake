# Copyright 2026 RethinkDB, all rights reserved.
#
# Resolve every library the server links. System packages are the default on
# Unix. Each RETHINKDB_FETCH_* option builds the pinned release instead.

include(FetchContent)
include("${CMAKE_CURRENT_LIST_DIR}/deps/FetchSource.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/External.cmake")

file(WRITE "${CMAKE_BINARY_DIR}/dist-support.txt" "")

if(APPLE)
    foreach(_rethinkdb_prefix
            /opt/homebrew/opt/openssl@3
            /usr/local/opt/openssl@3
            /opt/homebrew
            /usr/local)
        if(EXISTS "${_rethinkdb_prefix}")
            list(APPEND CMAKE_PREFIX_PATH "${_rethinkdb_prefix}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
endif()

if(WIN32)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" OR CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(RETHINKDB_WINDOWS_PLATFORM "Win32")
        set(RETHINKDB_OPENSSL_TARGET "VC-WIN32")
    else()
        set(RETHINKDB_WINDOWS_PLATFORM "x64")
        set(RETHINKDB_OPENSSL_TARGET "VC-WIN64A")
    endif()
else()
    set(RETHINKDB_WINDOWS_PLATFORM "")
    set(RETHINKDB_OPENSSL_TARGET "")
endif()

set(RETHINKDB_WINDOWS_TOOLSET "v141" CACHE STRING "MSVC toolset used to build fetched Windows dependencies")
set(RETHINKDB_WINDOWS_SDK "10.0.19041.0" CACHE STRING "Windows SDK version used to build fetched Windows dependencies")

find_package(Threads REQUIRED)

if(RETHINKDB_FETCH_OPENSSL)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/OpenSSL.cmake")
else()
    find_package(OpenSSL REQUIRED)
    add_library(RethinkDB::ssl ALIAS OpenSSL::SSL)
    add_library(RethinkDB::crypto ALIAS OpenSSL::Crypto)
    message(STATUS "Using system OpenSSL ${OPENSSL_VERSION}")
endif()

if(RETHINKDB_FETCH_ZLIB)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/Zlib.cmake")
else()
    find_package(ZLIB REQUIRED)
    add_library(RethinkDB::zlib ALIAS ZLIB::ZLIB)
    message(STATUS "Using system zlib ${ZLIB_VERSION_STRING}")
endif()

if(RETHINKDB_FETCH_CURL)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/Curl.cmake")
else()
    find_package(CURL REQUIRED)
    add_library(RethinkDB::curl ALIAS CURL::libcurl)
    message(STATUS "Using system libcurl ${CURL_VERSION_STRING}")
endif()

if(RETHINKDB_FETCH_PROTOBUF)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/Protobuf.cmake")
else()
    find_package(Protobuf CONFIG QUIET)
    if(NOT Protobuf_FOUND)
        find_package(Protobuf MODULE QUIET)
    endif()

    if(NOT Protobuf_FOUND OR NOT TARGET protobuf::libprotobuf)
        message(FATAL_ERROR
            "Protobuf was not found. Install the compiler and library, or reconfigure with -DRETHINKDB_FETCH_PROTOBUF=ON.")
    endif()

    add_library(RethinkDB::protobuf ALIAS protobuf::libprotobuf)
    if(TARGET protobuf::protoc)
        set(RETHINKDB_PROTOC_TARGET protobuf::protoc)
    elseif(Protobuf_PROTOC_EXECUTABLE)
        set(RETHINKDB_PROTOC_TARGET "")
        set(RETHINKDB_PROTOC "${Protobuf_PROTOC_EXECUTABLE}")
    else()
        find_program(RETHINKDB_PROTOC NAMES protoc REQUIRED)
        set(RETHINKDB_PROTOC_TARGET "")
    endif()

    message(STATUS "Using system Protobuf")
endif()

if(RETHINKDB_FETCH_BOOST)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/Boost.cmake")
else()
    find_package(Boost 1.84 REQUIRED)
    add_library(RethinkDB_boost INTERFACE)
    add_library(RethinkDB::boost ALIAS RethinkDB_boost)
    target_link_libraries(RethinkDB_boost INTERFACE Boost::boost)
    if(TARGET Boost::headers)
        target_link_libraries(RethinkDB_boost INTERFACE Boost::headers)
    endif()
    message(STATUS "Using system Boost ${Boost_VERSION}")
endif()

if(RETHINKDB_FETCH_RE2)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/Re2.cmake")
else()
    find_library(RETHINKDB_RE2_LIBRARY NAMES re2 REQUIRED)
    find_path(RETHINKDB_RE2_INCLUDE NAMES re2/re2.h REQUIRED)
    add_library(RethinkDB_re2 INTERFACE)
    add_library(RethinkDB::re2 ALIAS RethinkDB_re2)
    target_link_libraries(RethinkDB_re2 INTERFACE "${RETHINKDB_RE2_LIBRARY}")
    target_include_directories(RethinkDB_re2 SYSTEM INTERFACE "${RETHINKDB_RE2_INCLUDE}")
    message(STATUS "Using system RE2 ${RETHINKDB_RE2_LIBRARY}")
endif()

if(NOT RETHINKDB_FETCH_QUICKJS)
    message(FATAL_ERROR
        "QuickJS is the RethinkDB fork and has no system package. Leave RETHINKDB_FETCH_QUICKJS=ON.")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/deps/QuickJS.cmake")

include("${CMAKE_CURRENT_LIST_DIR}/deps/GTest.cmake")

add_library(RethinkDB_allocator INTERFACE)
add_library(RethinkDB::allocator ALIAS RethinkDB_allocator)
if(RETHINKDB_ALLOCATOR STREQUAL "jemalloc")
    if(RETHINKDB_FETCH_JEMALLOC)
        include("${CMAKE_CURRENT_LIST_DIR}/deps/Jemalloc.cmake")
        target_link_libraries(RethinkDB_allocator INTERFACE RethinkDB::jemalloc)
    else()
        find_library(RETHINKDB_JEMALLOC_LIBRARY NAMES jemalloc REQUIRED)
        target_link_libraries(RethinkDB_allocator INTERFACE "${RETHINKDB_JEMALLOC_LIBRARY}")
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_link_libraries(RethinkDB_allocator INTERFACE ${CMAKE_DL_LIBS})
        endif()
        message(STATUS "Using system jemalloc ${RETHINKDB_JEMALLOC_LIBRARY}")
    endif()
elseif(RETHINKDB_ALLOCATOR STREQUAL "tcmalloc")
    find_library(RETHINKDB_TCMALLOC_LIBRARY NAMES tcmalloc tcmalloc_minimal REQUIRED)
    target_link_libraries(RethinkDB_allocator INTERFACE "${RETHINKDB_TCMALLOC_LIBRARY}")
    message(STATUS "Using system tcmalloc ${RETHINKDB_TCMALLOC_LIBRARY}")
else()
    message(STATUS "Using the system allocator")
endif()

find_library(RETHINKDB_TERMCAP_LIBRARY NAMES termcap tinfo ncurses ncursesw)
add_library(RethinkDB_termcap INTERFACE)
add_library(RethinkDB::termcap ALIAS RethinkDB_termcap)
if(RETHINKDB_TERMCAP_LIBRARY)
    target_link_libraries(RethinkDB_termcap INTERFACE "${RETHINKDB_TERMCAP_LIBRARY}")
    target_compile_definitions(RethinkDB_termcap INTERFACE HAS_TERMCAP)
    message(STATUS "Using termcap ${RETHINKDB_TERMCAP_LIBRARY}")
else()
    message(STATUS "termcap/ncurses was not found; the server will link without it")
endif()

function(rethinkdb_link_runtime target)
    target_link_libraries("${target}" PRIVATE
        RethinkDB::boost
        RethinkDB::re2
        RethinkDB::termcap
        RethinkDB::quickjs
        RethinkDB::curl
        RethinkDB::ssl
        RethinkDB::crypto
        RethinkDB::zlib
        RethinkDB::protobuf
        RethinkDB::allocator
    )

    if(NOT WIN32)
        target_link_libraries("${target}" PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)
        if(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD")
            target_link_libraries("${target}" PRIVATE rt)
        endif()
    else()
        target_link_libraries("${target}" PRIVATE
            dbghelp
            iphlpapi
            ws2_32
            winmm
            crypt32
            wldap32
            normaliz
        )
    endif()

    get_property(_rethinkdb_exts GLOBAL PROPERTY RETHINKDB_EXTERNAL_DEPS)
    if(_rethinkdb_exts)
        add_dependencies("${target}" ${_rethinkdb_exts})
    endif()
endfunction()

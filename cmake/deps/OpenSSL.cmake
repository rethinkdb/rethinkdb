# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build OpenSSL.

include(ExternalProject)

set(_rethinkdb_openssl_version "3.0.7")
set(_rethinkdb_openssl_install "${CMAKE_BINARY_DIR}/external/openssl")
set(_rethinkdb_openssl_libdir "lib")

if(WIN32)
    set(_rethinkdb_openssl_config "${RETHINKDB_OPENSSL_TARGET}")
    set(_rethinkdb_openssl_lib_ssl "${_rethinkdb_openssl_install}/lib/libssl.lib")
    set(_rethinkdb_openssl_lib_crypto "${_rethinkdb_openssl_install}/lib/libcrypto.lib")

    ExternalProject_Add(openssl_ext
        URL "https://www.openssl.org/source/openssl-${_rethinkdb_openssl_version}.tar.gz"
        URL_HASH "SHA256=83049d042a260e696f62406ac5c08bf706fd84383f945cf21bd61e9ed95c396e"
        DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
        INSTALL_DIR "${_rethinkdb_openssl_install}"
        ${RETHINKDB_EXTERNAL_LOG_ARGS}
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_DIR=<SOURCE_DIR>"
            "-DCONFIG=$<CONFIG>"
            "-DTARGET=${_rethinkdb_openssl_config}"
            -P "${CMAKE_SOURCE_DIR}/cmake/deps/build_openssl_windows.cmake"
        INSTALL_COMMAND
            "${CMAKE_COMMAND}" -E make_directory "<INSTALL_DIR>/lib"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "<INSTALL_DIR>/include"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "<SOURCE_DIR>/libssl.lib" "<INSTALL_DIR>/lib/libssl.lib"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "<SOURCE_DIR>/libcrypto.lib" "<INSTALL_DIR>/lib/libcrypto.lib"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "<SOURCE_DIR>/include" "<INSTALL_DIR>/include"
        BUILD_BYPRODUCTS "${_rethinkdb_openssl_lib_ssl}" "${_rethinkdb_openssl_lib_crypto}"
    )
else()
    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_rethinkdb_openssl_target "darwin64-arm64-cc")
        else()
            set(_rethinkdb_openssl_target "darwin64-x86_64-cc")
        endif()
        set(_rethinkdb_openssl_configure
            "<SOURCE_DIR>/Configure" "${_rethinkdb_openssl_target}" shared
            "--prefix=${_rethinkdb_openssl_install}"
        )
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        set(_rethinkdb_openssl_configure
            "<SOURCE_DIR>/Configure" linux-armv4 shared
            "--prefix=${_rethinkdb_openssl_install}"
        )
    else()
        set(_rethinkdb_openssl_configure
            "<SOURCE_DIR>/config" shared
            "--prefix=${_rethinkdb_openssl_install}"
        )
    endif()

    if(APPLE)
        list(APPEND _rethinkdb_openssl_configure
            "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    set(_rethinkdb_openssl_lib_ssl "${_rethinkdb_openssl_install}/lib/libssl.a")
    set(_rethinkdb_openssl_lib_crypto "${_rethinkdb_openssl_install}/lib/libcrypto.a")

    ExternalProject_Add(openssl_ext
        URL "https://www.openssl.org/source/openssl-${_rethinkdb_openssl_version}.tar.gz"
        URL_HASH "SHA256=83049d042a260e696f62406ac5c08bf706fd84383f945cf21bd61e9ed95c396e"
        DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
        INSTALL_DIR "${_rethinkdb_openssl_install}"
        ${RETHINKDB_EXTERNAL_LOG_ARGS}
        CONFIGURE_COMMAND ${_rethinkdb_openssl_configure}
        BUILD_COMMAND "${RETHINKDB_GNU_MAKE}"
        INSTALL_COMMAND "${RETHINKDB_GNU_MAKE}" install_sw
        BUILD_BYPRODUCTS "${_rethinkdb_openssl_lib_ssl}" "${_rethinkdb_openssl_lib_crypto}"
    )
endif()

file(MAKE_DIRECTORY "${_rethinkdb_openssl_install}/include")
add_library(RethinkDB_crypto STATIC IMPORTED GLOBAL)
add_library(RethinkDB::crypto ALIAS RethinkDB_crypto)
add_library(RethinkDB_ssl STATIC IMPORTED GLOBAL)
add_library(RethinkDB::ssl ALIAS RethinkDB_ssl)
set_target_properties(RethinkDB_crypto PROPERTIES
    IMPORTED_LOCATION "${_rethinkdb_openssl_lib_crypto}"
)
set_target_properties(RethinkDB_ssl PROPERTIES
    IMPORTED_LOCATION "${_rethinkdb_openssl_lib_ssl}"
    INTERFACE_LINK_LIBRARIES RethinkDB::crypto
)
target_include_directories(RethinkDB_crypto SYSTEM INTERFACE
    "${_rethinkdb_openssl_install}/include")
target_include_directories(RethinkDB_ssl SYSTEM INTERFACE
    "${_rethinkdb_openssl_install}/include")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_property(TARGET RethinkDB_crypto APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS}")
    set_property(TARGET RethinkDB_ssl APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS}")
endif()
rethinkdb_external_dependency(openssl_ext)
message(STATUS "OpenSSL ${_rethinkdb_openssl_version} will be built")

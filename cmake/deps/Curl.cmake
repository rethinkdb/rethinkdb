# Copyright 2026 RethinkDB, all rights reserved.
#
# Fetch and build curl.

include(ExternalProject)

set(_rethinkdb_curl_version "7.82.0")
set(_rethinkdb_curl_install "${CMAKE_BINARY_DIR}/external/curl")
set(_rethinkdb_curl_depends "")

if(WIN32)
    set(_rethinkdb_curl_lib "${_rethinkdb_curl_install}/lib/curl.lib")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(_rethinkdb_curl_machine "x86")
    else()
        set(_rethinkdb_curl_machine "x64")
    endif()

    ExternalProject_Add(curl_ext
        URL "https://curl.se/download/curl-${_rethinkdb_curl_version}.tar.bz2"
        URL_HASH "SHA256=46d9a0400a33408fd992770b04a44a7434b3036f2e8089ac28b57573d59d371f"
        DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
        INSTALL_DIR "${_rethinkdb_curl_install}"
        ${RETHINKDB_EXTERNAL_LOG_ARGS}
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_DIR=<SOURCE_DIR>"
            "-DCONFIG=$<CONFIG>"
            "-DMACHINE=${_rethinkdb_curl_machine}"
            -P "${CMAKE_SOURCE_DIR}/cmake/deps/build_curl_windows.cmake"
        INSTALL_COMMAND
            "${CMAKE_COMMAND}" -E make_directory "<INSTALL_DIR>/lib"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "<INSTALL_DIR>/include"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "<SOURCE_DIR>/include/curl" "<INSTALL_DIR>/include/curl"
            COMMAND "${CMAKE_COMMAND}"
                "-DSOURCE_DIR=<SOURCE_DIR>"
                "-DINSTALL_DIR=<INSTALL_DIR>"
                "-DCONFIG=$<CONFIG>"
                "-DMACHINE=${_rethinkdb_curl_machine}"
                -P "${CMAKE_SOURCE_DIR}/cmake/deps/install_curl_windows.cmake"
        BUILD_BYPRODUCTS "${_rethinkdb_curl_lib}"
    )

    set(_rethinkdb_curl_extra Crypt32 ws2_32 wldap32 normaliz)
else()
    set(_rethinkdb_curl_lib "${_rethinkdb_curl_install}/lib/libcurl.a")
    set(_rethinkdb_curl_configure
        "<SOURCE_DIR>/configure"
        "--prefix=${_rethinkdb_curl_install}"
        "--without-gnutls"
        "--without-nghttp2"
        "--without-ngtcp2"
        "--without-nghttp3"
        "--without-librtmp"
        "--disable-ldap"
        "--disable-shared"
        "--enable-static"
    )

    if(RETHINKDB_FETCH_OPENSSL)
        list(APPEND _rethinkdb_curl_configure
            "--with-openssl=${CMAKE_BINARY_DIR}/external/openssl")
        list(APPEND _rethinkdb_curl_depends openssl_ext)
    else()
        list(APPEND _rethinkdb_curl_configure "--with-openssl")
    endif()

    if(RETHINKDB_FETCH_ZLIB)
        list(APPEND _rethinkdb_curl_configure
            "--with-zlib=${CMAKE_BINARY_DIR}/external/zlib")
        list(APPEND _rethinkdb_curl_depends zlib_ext)
    endif()

    if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_TARGET)
        list(APPEND _rethinkdb_curl_configure "--host=${CMAKE_CXX_COMPILER_TARGET}")
    endif()

    include("${CMAKE_CURRENT_LIST_DIR}/Libidn.cmake")
    list(APPEND _rethinkdb_curl_depends libidn_ext)

    ExternalProject_Add(curl_ext
        URL "https://curl.se/download/curl-${_rethinkdb_curl_version}.tar.bz2"
        URL_HASH "SHA256=46d9a0400a33408fd992770b04a44a7434b3036f2e8089ac28b57573d59d371f"
        DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
        INSTALL_DIR "${_rethinkdb_curl_install}"
        DEPENDS ${_rethinkdb_curl_depends}
        ${RETHINKDB_EXTERNAL_LOG_ARGS}
        CONFIGURE_COMMAND ${_rethinkdb_curl_configure}
        BUILD_COMMAND "${RETHINKDB_GNU_MAKE}" -C "<BINARY_DIR>/lib"
        INSTALL_COMMAND "${RETHINKDB_GNU_MAKE}" -C "<BINARY_DIR>/lib" install-libLTLIBRARIES
            COMMAND "${RETHINKDB_GNU_MAKE}" -C "<BINARY_DIR>" install-binSCRIPTS
            COMMAND "${RETHINKDB_GNU_MAKE}" -C "<BINARY_DIR>/include" install
        BUILD_BYPRODUCTS "${_rethinkdb_curl_lib}"
    )

    set(_rethinkdb_curl_extra RethinkDB::idn)
    if(RETHINKDB_FETCH_ZLIB)
        list(APPEND _rethinkdb_curl_extra RethinkDB::zlib)
    else()
        list(APPEND _rethinkdb_curl_extra ZLIB::ZLIB)
    endif()

    if(RETHINKDB_FETCH_OPENSSL)
        list(APPEND _rethinkdb_curl_extra RethinkDB::ssl RethinkDB::crypto)
    else()
        list(APPEND _rethinkdb_curl_extra OpenSSL::SSL OpenSSL::Crypto)
    endif()

    list(APPEND _rethinkdb_curl_extra ${CMAKE_DL_LIBS})
endif()

file(MAKE_DIRECTORY "${_rethinkdb_curl_install}/include")
add_library(RethinkDB_curl STATIC IMPORTED GLOBAL)
add_library(RethinkDB::curl ALIAS RethinkDB_curl)
set_target_properties(RethinkDB_curl PROPERTIES
    IMPORTED_LOCATION "${_rethinkdb_curl_lib}"
    INTERFACE_LINK_LIBRARIES "${_rethinkdb_curl_extra}"
)

target_include_directories(RethinkDB_curl SYSTEM INTERFACE
    "${_rethinkdb_curl_install}/include")

if(WIN32)
    set_property(TARGET RethinkDB_curl APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS CURL_STATICLIB)
endif()

rethinkdb_external_dependency(curl_ext)
message(STATUS "libcurl ${_rethinkdb_curl_version} will be built")

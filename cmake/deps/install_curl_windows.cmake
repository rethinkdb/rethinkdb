# Copyright 2026 RethinkDB, all rights reserved.
#
# Install the curl static library for Windows.

if(NOT SOURCE_DIR OR NOT INSTALL_DIR OR NOT MACHINE)
    message(FATAL_ERROR "SOURCE_DIR, INSTALL_DIR, and MACHINE are required")
endif()

if(NOT CONFIG)
    set(CONFIG Release)
endif()

if(CONFIG MATCHES "^[Dd]ebug")
    set(out_config "debug")
    set(out_suffix "_debug")
else()
    set(out_config "release")
    set(out_suffix "")
endif()

set(built
    "${SOURCE_DIR}/builds/libcurl-vc-${MACHINE}-${out_config}-static-ipv6-sspi-schannel/lib/libcurl_a${out_suffix}.lib")

if(NOT EXISTS "${built}")
    message(FATAL_ERROR "curl static library was not produced: ${built}")
endif()

file(MAKE_DIRECTORY "${INSTALL_DIR}/lib")
file(COPY "${built}" DESTINATION "${INSTALL_DIR}/lib")
file(RENAME "${INSTALL_DIR}/lib/libcurl_a${out_suffix}.lib" "${INSTALL_DIR}/lib/curl.lib")

# Copyright 2026 RethinkDB, all rights reserved.
#
# Cache options. Defaults to pinned QuickJS, RE2, Boost headers, and
# Google Test are fetched; OpenSSL, zlib, curl, and Protobuf come from the
# system except on Windows, where every third-party library is fetched. Linux
# links jemalloc. macOS and FreeBSD use the system allocator.

if(WIN32)
    set(_rethinkdb_fetch_system_default ON)
else()
    set(_rethinkdb_fetch_system_default OFF)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_rethinkdb_fetch_jemalloc_default ON)
    set(_rethinkdb_allocator_default jemalloc)
else()
    set(_rethinkdb_fetch_jemalloc_default OFF)
    set(_rethinkdb_allocator_default system)
endif()

option(BUILD_TESTING "Build rethinkdb-unittest" ON)
option(RETHINKDB_FETCH_QUICKJS "Build the pinned RethinkDB QuickJS fork" ON)
option(RETHINKDB_FETCH_RE2 "Build RE2 2015-11-01 instead of a system copy" ON)
option(RETHINKDB_FETCH_BOOST "Use the pinned Boost headers" ON)
option(RETHINKDB_FETCH_GTEST "Build Google Test 1.8.1 instead of a system copy" ON)
option(RETHINKDB_FETCH_JEMALLOC "Build jemalloc 5.2.1 instead of a system copy" ${_rethinkdb_fetch_jemalloc_default})
option(RETHINKDB_FETCH_PROTOBUF "Build Protobuf 36.2 instead of a system copy" ${_rethinkdb_fetch_system_default})
option(RETHINKDB_FETCH_OPENSSL "Build OpenSSL 3.0.7 instead of a system copy" ${_rethinkdb_fetch_system_default})
option(RETHINKDB_FETCH_ZLIB "Build zlib 1.2.11 instead of a system copy" ${_rethinkdb_fetch_system_default})
option(RETHINKDB_FETCH_CURL "Build libcurl 7.82.0 instead of a system copy" ${_rethinkdb_fetch_system_default})

set(RETHINKDB_ALLOCATOR "${_rethinkdb_allocator_default}" CACHE STRING
    "Allocator linked into the server: jemalloc, tcmalloc, or system")
set_property(CACHE RETHINKDB_ALLOCATOR PROPERTY STRINGS jemalloc tcmalloc system)

option(RETHINKDB_ALLOW_WARNINGS "Do not pass -Werror" ON)
option(RETHINKDB_SYMBOLS "Generate debug symbols" ON)
option(RETHINKDB_SPLIT_SYMBOLS "Strip the server binary and write a separate symbols file" OFF)
option(RETHINKDB_STRIP_ON_INSTALL "Strip the installed server binary" OFF)
option(RETHINKDB_NAMEVERSIONED "Install rethinkdb as rethinkdb-<major>.<minor>" OFF)
option(RETHINKDB_STATIC "Link the Linux executable as mostly static" OFF)
option(RETHINKDB_BUILD_PORTABLE "Link libgcc statically and fetch Protobuf for source packages" OFF)
option(RETHINKDB_COVERAGE "Pass --coverage (GCC only)" OFF)
option(RETHINKDB_LEGACY_GCC "Drop the newer GCC warning suppressions" OFF)
option(RETHINKDB_LEGACY_LINUX "Define LEGACY_LINUX and NO_EPOLL" OFF)
option(RETHINKDB_NO_EVENTFD "Define NO_EVENTFD" OFF)
option(RETHINKDB_NO_EPOLL "Define NO_EPOLL" OFF)
option(RETHINKDB_THREADED_COROUTINES "Define THREADED_COROUTINES" OFF)
option(RETHINKDB_VALGRIND "Define VALGRIND and require the system allocator" OFF)
option(RETHINKDB_FULL_PERFMON "Define FULL_PERFMON" OFF)
option(RETHINKDB_CORO_PROFILING "Define ENABLE_CORO_PROFILER" OFF)
option(RETHINKDB_RQL_ERROR_BT "Define RQL_ERROR_BT" OFF)
option(RETHINKDB_JSON_SHORTCUTS "Define JSON_SHORTCUTS" OFF)
option(RETHINKDB_DISABLE_BREAKPOINTS "Define DISABLE_BREAKPOINTS" OFF)
option(RETHINKDB_NO_OMIT_FRAME_POINTER "Keep frame pointers in non-debug builds" OFF)
option(RETHINKDB_KEEP_INLINE "Pass -fkeep-inline-functions in debug builds" OFF)
option(RETHINKDB_FORCE_NATIVE "Pass -march=native" OFF)
option(RETHINKDB_COPY_NATIVE "Pass -march=<host processor>" OFF)
option(RETHINKDB_REDUCE_NATIVE "Pass a reduced -march for older x86 hosts" OFF)

set(RETHINKDB_WEB_ASSETS_DIR "" CACHE PATH
    "Admin UI directory consumed by the web-assets target")

if(NOT RETHINKDB_ALLOCATOR MATCHES "^(jemalloc|tcmalloc|system)$")
    message(FATAL_ERROR
        "RETHINKDB_ALLOCATOR must be jemalloc, tcmalloc, or system, not '${RETHINKDB_ALLOCATOR}'")
endif()

if(RETHINKDB_VALGRIND AND NOT RETHINKDB_ALLOCATOR STREQUAL "system")
    message(FATAL_ERROR "RETHINKDB_VALGRIND requires RETHINKDB_ALLOCATOR=system")
endif()

if(RETHINKDB_SPLIT_SYMBOLS AND NOT RETHINKDB_SYMBOLS)
    message(FATAL_ERROR "RETHINKDB_SPLIT_SYMBOLS requires RETHINKDB_SYMBOLS")
endif()

if(RETHINKDB_COVERAGE AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "RETHINKDB_COVERAGE is supported only with GCC")
endif()

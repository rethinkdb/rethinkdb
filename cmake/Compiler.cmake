# Copyright 2026 RethinkDB, all rights reserved.
#
# Compiler and linker flags that used to live in src/build.mk. Warnings and
# defines are applied per target. Third-party targets keep their own flags.

function(rethinkdb_apply_warnings target)
    if(MSVC)
        target_compile_options("${target}" PRIVATE
            /W3
            /bigobj
            /wd4244
            /wd4267
            /wd4804
            /wd4091
            /wd4996
        )
        return()
    endif()

    target_compile_options("${target}" PRIVATE
        -Wall
        -Wextra
        -Wnon-virtual-dtor
        -Wno-deprecated-declarations
        -pthread
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options("${target}" PRIVATE
            -Wformat=2
            -Wswitch-enum
            -Wswitch-default
            -Wused-but-marked-unused
            -Wundef
            -Wvla
            -Wshadow
            -Wconditional-uninitialized
            -Wmissing-noreturn
        )
        if(CMAKE_CXX_COMPILER_LAUNCHER MATCHES "ccache")
            target_compile_options("${target}" PRIVATE -Qunused-arguments)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options("${target}" PRIVATE
            -Wformat=2
            -Wswitch-enum
            -Wswitch-default
        )
        if(RETHINKDB_LEGACY_GCC)
            target_compile_options("${target}" PRIVATE
                -Wno-switch-default
                -Wno-switch-enum
            )
        else()
            target_compile_options("${target}" PRIVATE
                -Wno-array-bounds
                -Wno-maybe-uninitialized
            )
        endif()
    endif()

    if(NOT RETHINKDB_ALLOW_WARNINGS)
        target_compile_options("${target}" PRIVATE -Werror)
    endif()

    if(RETHINKDB_LEGACY_LINUX)
        target_compile_options("${target}" PRIVATE -Wno-format)
    endif()

    if(RETHINKDB_COVERAGE)
        target_compile_options("${target}" PRIVATE --coverage)
    endif()

    if(RETHINKDB_FORCE_NATIVE)
        target_compile_options("${target}" PRIVATE -march=native)
    elseif(RETHINKDB_COPY_NATIVE OR RETHINKDB_REDUCE_NATIVE)
        set(_rethinkdb_arch "${CMAKE_SYSTEM_PROCESSOR}")
        if(RETHINKDB_REDUCE_NATIVE AND _rethinkdb_arch MATCHES "i[56]86")
            set(_rethinkdb_arch "i486")
        endif()
        target_compile_options("${target}" PRIVATE "-march=${_rethinkdb_arch}")
    endif()

    if(RETHINKDB_SYMBOLS)
        target_compile_options("${target}" PRIVATE
            $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-g>
        )
    endif()

    target_compile_options("${target}" PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:-fno-strict-aliasing>
    )

    if(RETHINKDB_NO_OMIT_FRAME_POINTER)
        target_compile_options("${target}" PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:-fno-omit-frame-pointer>
        )
    endif()

    if(RETHINKDB_KEEP_INLINE)
        target_compile_options("${target}" PRIVATE
            $<$<CONFIG:Debug>:-fkeep-inline-functions>
        )
    endif()
endfunction()

function(rethinkdb_apply_defines target)
    target_compile_definitions("${target}" PUBLIC
        "PRODUCT_NAME=\"RethinkDB\""
        "BUILD_MACHINE=\"${RETHINKDB_BUILD_MACHINE}\""
        __STDC_LIMIT_MACROS
        __STDC_FORMAT_MACROS
        ENABLE_TLS
        RAPIDJSON_HAS_STDSTRING
        RAPIDJSON_PARSE_DEFAULT_FLAGS=kParseFullPrecisionFlag
        "RETHINKDB_VERSION=\"${RETHINKDB_VERSION}\""
        "RETHINKDB_CODE_VERSION=\"${RETHINKDB_CODE_VERSION}\""
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_definitions("${target}" PUBLIC _FILE_OFFSET_BITS=64)
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64le|powerpc64le")
        target_compile_definitions("${target}" PUBLIC V8_NEEDS_BUFFER_ALLOCATOR)
    endif()

    if(RETHINKDB_LEGACY_LINUX)
        target_compile_definitions("${target}" PUBLIC LEGACY_LINUX NO_EPOLL)
    endif()

    if(RETHINKDB_NO_EVENTFD)
        target_compile_definitions("${target}" PUBLIC NO_EVENTFD)
    endif()

    if(RETHINKDB_NO_EPOLL)
        target_compile_definitions("${target}" PUBLIC NO_EPOLL)
    endif()

    if(RETHINKDB_THREADED_COROUTINES)
        target_compile_definitions("${target}" PUBLIC THREADED_COROUTINES)
    endif()

    if(RETHINKDB_VALGRIND)
        target_compile_definitions("${target}" PUBLIC VALGRIND)
    endif()

    if(RETHINKDB_FULL_PERFMON)
        target_compile_definitions("${target}" PUBLIC FULL_PERFMON)
    endif()

    if(RETHINKDB_CORO_PROFILING)
        target_compile_definitions("${target}" PUBLIC ENABLE_CORO_PROFILER)
    endif()

    if(RETHINKDB_RQL_ERROR_BT)
        target_compile_definitions("${target}" PUBLIC RQL_ERROR_BT)
    endif()

    if(RETHINKDB_JSON_SHORTCUTS)
        target_compile_definitions("${target}" PUBLIC JSON_SHORTCUTS)
    endif()

    if(RETHINKDB_DISABLE_BREAKPOINTS)
        target_compile_definitions("${target}" PUBLIC DISABLE_BREAKPOINTS)
    endif()

    if(WIN32)
        target_compile_definitions("${target}" PUBLIC
            WIN32
            WINVER=0x0A00
            _WIN32_WINNT=0x0A00
            _USE_MATH_DEFINES
            COMPILER_MSVC
            BOOST_DATE_TIME_NO_LIB
            JS_STRICT_NAN_BOXING
            CURL_STATICLIB
        )
    endif()
endfunction()

function(rethinkdb_apply_link_options target)
    if(MSVC)
        target_link_options("${target}" PRIVATE "/ENTRY:mainCRTStartup")
        if(RETHINKDB_SYMBOLS)
            target_link_options("${target}" PRIVATE "/DEBUG")
        endif()
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options("${target}" PRIVATE "-Wl,--no-as-needed")
    endif()

    if(RETHINKDB_SYMBOLS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options("${target}" PRIVATE "-rdynamic")
    endif()

    if(RETHINKDB_COVERAGE)
        target_link_options("${target}" PRIVATE --coverage)
    endif()

    if(RETHINKDB_STATIC AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options("${target}" PRIVATE
            "-static"
            "-Wl,-Bdynamic"
            "-static-libgcc"
            "-static-libstdc++"
        )
    endif()

    if(RETHINKDB_BUILD_PORTABLE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries("${target}" PRIVATE gcc_s gcc)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_directories("${target}" PRIVATE /usr/local/lib)
        target_link_libraries("${target}" PRIVATE stdc++ execinfo)
    endif()
endfunction()

function(rethinkdb_suppress_switch_default target)
    if(MSVC)
        return()
    endif()

    target_compile_options("${target}" PRIVATE -Wno-switch-default)
endfunction()

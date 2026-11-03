cmake_minimum_required(VERSION 3.24)

project(RethinkDB LANGUAGES C CXX)

# ISO C++17 for every compiler check this project runs, including the ones
# project() performs before the first target exists.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# macOS 11 reached end of support on 2026-02-02. 12.0 also satisfies the
# Abseil requirement for aligned operator new.
if(APPLE AND (NOT CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 12.0))
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version" FORCE)
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
endif()

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/Options.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/Version.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/Compiler.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(RETHINKDB_PROTO_DIR "${CMAKE_BINARY_DIR}/proto")
set(RETHINKDB_QL2_PB_CC "${RETHINKDB_PROTO_DIR}/rdb_protocol/ql2.pb.cc")
set(RETHINKDB_QL2_PB_H "${RETHINKDB_PROTO_DIR}/rdb_protocol/ql2.pb.h")
set(RETHINKDB_QL2_PROTO "${CMAKE_SOURCE_DIR}/src/rdb_protocol/ql2.proto")

if(RETHINKDB_PROTOC_TARGET)
    set(_rethinkdb_protoc "$<TARGET_FILE:${RETHINKDB_PROTOC_TARGET}>")
    set(_rethinkdb_protoc_dep "${RETHINKDB_PROTOC_TARGET}")
else()
    set(_rethinkdb_protoc "${RETHINKDB_PROTOC}")
    set(_rethinkdb_protoc_dep)
endif()

add_custom_command(
    OUTPUT "${RETHINKDB_QL2_PB_CC}" "${RETHINKDB_QL2_PB_H}"
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${RETHINKDB_QL2_PB_CC}" "${RETHINKDB_QL2_PB_H}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${RETHINKDB_PROTO_DIR}/rdb_protocol"
    COMMAND ${_rethinkdb_protoc}
        "--proto_path=${CMAKE_SOURCE_DIR}/src"
        "--cpp_out=${RETHINKDB_PROTO_DIR}"
        "${RETHINKDB_QL2_PROTO}"
    DEPENDS "${RETHINKDB_QL2_PROTO}" ${_rethinkdb_protoc_dep}
    VERBATIM
)
add_custom_target(ql2_proto DEPENDS "${RETHINKDB_QL2_PB_CC}" "${RETHINKDB_QL2_PB_H}")

if(MSVC)
    set_source_files_properties("${RETHINKDB_QL2_PB_CC}" PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties("${RETHINKDB_QL2_PB_CC}" PROPERTIES COMPILE_OPTIONS "-w")
endif()

file(GLOB_RECURSE RETHINKDB_ALL_CC CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cc")
set(RETHINKDB_CORE_SOURCES)
set(RETHINKDB_UNIT_SOURCES)
foreach(src IN LISTS RETHINKDB_ALL_CC)
    cmake_path(GET src FILENAME src_name)
    if(src_name MATCHES "^\\.")
        continue()
    endif()
    cmake_path(RELATIVE_PATH src BASE_DIRECTORY "${CMAKE_SOURCE_DIR}/src" OUTPUT_VARIABLE src_rel)
    if(src_rel MATCHES "^unittest/")
        if(NOT src_name STREQUAL "main.cc")
            list(APPEND RETHINKDB_UNIT_SOURCES "${src}")
        endif()
    elseif(NOT src_rel STREQUAL "main.cc")
        list(APPEND RETHINKDB_CORE_SOURCES "${src}")
    endif()
endforeach()
set_source_files_properties(
    ${RETHINKDB_CORE_SOURCES}
    ${RETHINKDB_UNIT_SOURCES}
    "${CMAKE_SOURCE_DIR}/src/main.cc"
    "${CMAKE_SOURCE_DIR}/src/unittest/main.cc"
    PROPERTIES OBJECT_DEPENDS "${RETHINKDB_QL2_PB_H}"
)

list(LENGTH RETHINKDB_CORE_SOURCES _rethinkdb_core_count)
if(_rethinkdb_core_count LESS 100)
    message(FATAL_ERROR "Expected the server sources under src/, found ${_rethinkdb_core_count}")
endif()
message(STATUS "RethinkDB core translation units: ${_rethinkdb_core_count}")

function(rethinkdb_configure_sources target)
    rethinkdb_apply_warnings("${target}")
    rethinkdb_apply_defines("${target}")
    target_include_directories("${target}" PUBLIC
        "${CMAKE_SOURCE_DIR}/src"
        "${RETHINKDB_PROTO_DIR}"
    )
    if(WIN32)
        target_include_directories("${target}" BEFORE PUBLIC
            "${CMAKE_SOURCE_DIR}/src/arch/windows_stub")
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_include_directories("${target}" PRIVATE /usr/local/include)
    endif()
    rethinkdb_link_runtime("${target}")
endfunction()

add_library(rethinkdb_core OBJECT ${RETHINKDB_CORE_SOURCES} "${RETHINKDB_QL2_PB_CC}")
rethinkdb_configure_sources(rethinkdb_core)
rethinkdb_suppress_switch_default(rethinkdb_core)
if(BUILD_TESTING AND RETHINKDB_GTEST_INCLUDE)
    target_include_directories(rethinkdb_core SYSTEM PRIVATE ${RETHINKDB_GTEST_INCLUDE})
endif()

add_executable(rethinkdb "${CMAKE_SOURCE_DIR}/src/main.cc")
target_link_libraries(rethinkdb PRIVATE rethinkdb_core)
rethinkdb_configure_sources(rethinkdb)
rethinkdb_apply_link_options(rethinkdb)
if(WIN32)
    target_sources(rethinkdb PRIVATE "${CMAKE_SOURCE_DIR}/packaging/windows/rethinkdb.exe.manifest")
endif()

add_custom_command(TARGET rethinkdb POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/scripts/rethinkdb-gdb.py"
        "$<TARGET_FILE_DIR:rethinkdb>/rethinkdb-gdb.py"
    VERBATIM
)

if(RETHINKDB_SPLIT_SYMBOLS)
    if(APPLE)
        add_custom_command(TARGET rethinkdb POST_BUILD
            COMMAND dsymutil "$<TARGET_FILE:rethinkdb>"
            COMMAND strip "$<TARGET_FILE:rethinkdb>"
            VERBATIM)
    else()
        add_custom_command(TARGET rethinkdb POST_BUILD
            COMMAND objcopy --only-keep-debug "$<TARGET_FILE:rethinkdb>" "$<TARGET_FILE:rethinkdb>.debug"
            COMMAND objcopy --strip-debug "$<TARGET_FILE:rethinkdb>"
            COMMAND objcopy "--add-gnu-debuglink=$<TARGET_FILE_NAME:rethinkdb>.debug" "$<TARGET_FILE:rethinkdb>"
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:rethinkdb>"
            VERBATIM)
    endif()
endif()

if(RETHINKDB_FETCH_JEMALLOC AND RETHINKDB_ALLOCATOR STREQUAL "jemalloc" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_custom_command(TARGET rethinkdb POST_BUILD
        COMMAND objdump -T "$<TARGET_FILE:rethinkdb>"
        COMMAND "${CMAKE_COMMAND}" -E echo "Checked jemalloc (mallctlnametomib must appear in the dynamic symbol table)"
        VERBATIM)
    # The echo above does not fail the build. The check itself does.
    add_custom_command(TARGET rethinkdb POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DBINARY=$<TARGET_FILE:rethinkdb>"
            -P "${CMAKE_SOURCE_DIR}/cmake/check_jemalloc.cmake"
        VERBATIM)
endif()

if(BUILD_TESTING)
    add_library(rethinkdb_unittest_objects OBJECT ${RETHINKDB_UNIT_SOURCES})
    rethinkdb_configure_sources(rethinkdb_unittest_objects)
    rethinkdb_suppress_switch_default(rethinkdb_unittest_objects)
    if(RETHINKDB_GTEST_INCLUDE)
        target_include_directories(rethinkdb_unittest_objects SYSTEM PRIVATE ${RETHINKDB_GTEST_INCLUDE})
    endif()

    add_executable(rethinkdb-unittest "${CMAKE_SOURCE_DIR}/src/unittest/main.cc")
    target_link_libraries(rethinkdb-unittest PRIVATE
        rethinkdb_unittest_objects
        rethinkdb_core
    )
    rethinkdb_configure_sources(rethinkdb-unittest)
    # Google Test is last, matching the Make link line.
    target_link_libraries(rethinkdb-unittest PRIVATE RethinkDB::gtest)
    rethinkdb_suppress_switch_default(rethinkdb-unittest)
    rethinkdb_apply_link_options(rethinkdb-unittest)
    if(WIN32)
        target_sources(rethinkdb-unittest PRIVATE
            "${CMAKE_SOURCE_DIR}/packaging/windows/rethinkdb.exe.manifest")
    endif()

    enable_testing()
    add_test(NAME rethinkdb-unittest COMMAND rethinkdb-unittest)
endif()

add_custom_target(generate-headers
    COMMAND "${CMAKE_COMMAND}"
        "-DPYTHON=${Python3_EXECUTABLE}"
        "-DROOT=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/generate_headers.cmake"
    VERBATIM
)
add_custom_target(web-assets
    COMMAND "${CMAKE_COMMAND}"
        "-DPYTHON=${Python3_EXECUTABLE}"
        "-DASSETS=${RETHINKDB_WEB_ASSETS_DIR}"
        "-DOUT=${CMAKE_SOURCE_DIR}/src/gen/web_assets.cc"
        -P "${CMAKE_SOURCE_DIR}/cmake/web_assets.cmake"
    VERBATIM
)

include("${CMAKE_SOURCE_DIR}/cmake/Packaging.cmake")

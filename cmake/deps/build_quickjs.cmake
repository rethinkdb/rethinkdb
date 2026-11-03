# Copyright 2026 RethinkDB, all rights reserved.
#
# Build and install the pinned QuickJS.

if(NOT SOURCE_DIR OR NOT INSTALL_DIR)
    message(FATAL_ERROR "SOURCE_DIR and INSTALL_DIR are required")
endif()

if(NOT CONFIG)
    set(CONFIG Release)
endif()

if(WIN32)
    if(NOT PREMAKE OR NOT MSBUILD)
        message(FATAL_ERROR "Premake5 and MSBuild are required to build QuickJS on Windows")
    endif()

    execute_process(
        COMMAND "${PREMAKE}" vs2017
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE premake_result
    )

    if(NOT premake_result EQUAL 0)
        message(FATAL_ERROR "premake5 vs2017 failed")
    endif()

    execute_process(
        COMMAND "${MSBUILD}" ".build/vs2017/quickjs.vcxproj"
            /nologo
            "/p:Configuration=${CONFIG}"
            "/p:Platform=${PLATFORM}"
            "/p:PlatformToolset=${TOOLSET}"
            "/p:WindowsTargetPlatformVersion=${SDK}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE build_result
    )

    if(NOT build_result EQUAL 0)
        message(FATAL_ERROR "QuickJS MSBuild failed")
    endif()

    set(built_lib "${SOURCE_DIR}/.bin/${CONFIG}/${PLATFORM}/quickjs.lib")
    set(installed_lib "${INSTALL_DIR}/lib/quickjs.lib")
else()
    set(make_args "CC=${CC}" "AR=${AR}")

    if(RETHINKDB_APPLE)
        list(APPEND make_args "CONFIG_DARWIN=y")
    endif()

    execute_process(
        COMMAND "${MAKE}" libquickjs.a ${make_args}
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE build_result
    )

    if(NOT build_result EQUAL 0)
        message(FATAL_ERROR "QuickJS make libquickjs.a failed")
    endif()

    set(built_lib "${SOURCE_DIR}/libquickjs.a")
    set(installed_lib "${INSTALL_DIR}/lib/quickjs/libquickjs.a")
endif()

file(MAKE_DIRECTORY "${INSTALL_DIR}/include/quickjs")
file(COPY "${SOURCE_DIR}/quickjs.h" "${SOURCE_DIR}/quickjs-version.h"
    DESTINATION "${INSTALL_DIR}/include/quickjs")

get_filename_component(installed_dir "${installed_lib}" DIRECTORY)
file(MAKE_DIRECTORY "${installed_dir}")
file(COPY "${built_lib}" DESTINATION "${installed_dir}")

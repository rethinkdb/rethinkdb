# Copyright 2026 RethinkDB, all rights reserved.
#
# Install layout from CMake and CPack targets.

include(GNUInstallDirs)

if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set(_rethinkdb_state_default "/var")
else()
    set(_rethinkdb_state_default "${CMAKE_INSTALL_PREFIX}/var")
endif()
set(RETHINKDB_LOCALSTATEDIR "${_rethinkdb_state_default}" CACHE PATH
    "Directory for variable state data (Make's LOCALSTATEDIR)")

if(RETHINKDB_NAMEVERSIONED)
    set(RETHINKDB_INSTALL_NAME "rethinkdb-${RETHINKDB_SHORT_VERSION}")
else()
    set(RETHINKDB_INSTALL_NAME "rethinkdb")
endif()

if(RETHINKDB_STRIP_ON_INSTALL)
    set(CMAKE_INSTALL_DO_STRIP TRUE)
endif()

set(RETHINKDB_MAN_PAGE "${CMAKE_BINARY_DIR}/assets/rethinkdb.1")
set(RETHINKDB_MAN_PAGE_GZ "${RETHINKDB_MAN_PAGE}.gz")
add_custom_command(
    OUTPUT "${RETHINKDB_MAN_PAGE}"
    COMMAND "${CMAKE_COMMAND}"
        "-DM4=${CMAKE_SOURCE_DIR}/packaging/assets/man/rethinkdb.1"
        "-DOUT=${RETHINKDB_MAN_PAGE}"
        "-DSHORT_VERSION=${RETHINKDB_SHORT_VERSION}"
        "-DDATE=${RETHINKDB_BUILD_DATE}"
        -P "${CMAKE_SOURCE_DIR}/cmake/render_man.cmake"
    DEPENDS "${CMAKE_SOURCE_DIR}/packaging/assets/man/rethinkdb.1"
            "${CMAKE_SOURCE_DIR}/cmake/render_man.cmake"
    VERBATIM
)
add_custom_command(
    OUTPUT "${RETHINKDB_MAN_PAGE_GZ}"
    COMMAND "${CMAKE_COMMAND}"
        "-DIN=${RETHINKDB_MAN_PAGE}"
        "-DOUT=${RETHINKDB_MAN_PAGE_GZ}"
        -P "${CMAKE_SOURCE_DIR}/cmake/gzip_file.cmake"
    DEPENDS "${RETHINKDB_MAN_PAGE}"
    VERBATIM
)
add_custom_target(rethinkdb_man ALL DEPENDS "${RETHINKDB_MAN_PAGE_GZ}")

install(TARGETS rethinkdb
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT runtime
)
if(RETHINKDB_NAMEVERSIONED)
    install(CODE "
        set(_bin \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/rethinkdb\")
        set(_versioned \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${RETHINKDB_INSTALL_NAME}\")
        if(EXISTS \"\${_bin}\" AND NOT \"\${_bin}\" STREQUAL \"\${_versioned}\")
            file(RENAME \"\${_bin}\" \"\${_versioned}\")
        endif()
    ")
endif()

install(FILES "${RETHINKDB_MAN_PAGE_GZ}"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    RENAME "${RETHINKDB_INSTALL_NAME}.1.gz"
    COMPONENT runtime
)
install(FILES "${CMAKE_SOURCE_DIR}/packaging/assets/docs/LICENSE"
    DESTINATION "share/doc/${RETHINKDB_INSTALL_NAME}"
    RENAME copyright
    COMPONENT runtime
)
install(PROGRAMS "${CMAKE_SOURCE_DIR}/packaging/assets/init/rethinkdb"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/init.d"
    COMPONENT runtime
)
install(FILES "${CMAKE_SOURCE_DIR}/packaging/assets/config/default.conf.sample"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/rethinkdb"
    COMPONENT runtime
)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/empty-install")
install(DIRECTORY "${CMAKE_BINARY_DIR}/empty-install/"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/rethinkdb/instances.d"
    COMPONENT runtime
)
install(DIRECTORY "${CMAKE_BINARY_DIR}/empty-install/"
    DESTINATION "${RETHINKDB_LOCALSTATEDIR}/lib/rethinkdb/instances.d"
    COMPONENT runtime
)

string(REPLACE "-" "+" RETHINKDB_DEBIAN_VERSION "${RETHINKDB_VERSION}")
string(REPLACE "-" "_" RETHINKDB_RPM_VERSION "${RETHINKDB_VERSION}")

execute_process(
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/gen-version.sh" -r
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE RETHINKDB_PACKAGE_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE _rethinkdb_release_result
)
string(STRIP "${RETHINKDB_PACKAGE_RELEASE}" RETHINKDB_PACKAGE_RELEASE)
if(NOT _rethinkdb_release_result EQUAL 0
        OR NOT RETHINKDB_PACKAGE_RELEASE MATCHES "^[1-9][0-9]*$")
    set(RETHINKDB_PACKAGE_RELEASE "1")
endif()

set(_rethinkdb_debug_packages FALSE)
if(RETHINKDB_SYMBOLS
        AND NOT RETHINKDB_SPLIT_SYMBOLS
        AND NOT RETHINKDB_STRIP_ON_INSTALL)
    set(_rethinkdb_debug_packages TRUE)
endif()

set(CPACK_PACKAGE_NAME "rethinkdb")
set(CPACK_PACKAGE_VENDOR "RethinkDB")
set(CPACK_PACKAGE_CONTACT "RethinkDB <packaging@rethinkdb.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The open-source database for the realtime web")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/packaging/description.txt")
set(CPACK_PACKAGE_VERSION "${RETHINKDB_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/packaging/assets/docs/LICENSE")
set(CPACK_SET_DESTDIR ON)
# State directories are absolute, so the packages are not relocatable.
set(CPACK_PACKAGE_RELOCATABLE OFF)
set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)
set(CPACK_GENERATOR "TGZ")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/packages/tgz")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "database")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.rethinkdb.com/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_VERSION "${RETHINKDB_DEBIAN_VERSION}")
set(CPACK_DEBIAN_PACKAGE_PREDEPENDS "adduser (>= 3.40), procps (>= 3.2)")
set(CPACK_DEBIAN_PACKAGE_BREAKS "rethinkdb (<< ${RETHINKDB_DEBIAN_VERSION})")
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "rethinkdb (<< ${RETHINKDB_DEBIAN_VERSION})")
set(CPACK_DEBIAN_PACKAGE_PROVIDES "rethinkdb")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/debian/preinst"
    "${CMAKE_SOURCE_DIR}/packaging/debian/postinst"
    "${CMAKE_SOURCE_DIR}/packaging/debian/prerm"
    "${CMAKE_SOURCE_DIR}/packaging/debian/postrm"
)
if(_rethinkdb_debug_packages)
    set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
endif()

set(CPACK_RPM_PACKAGE_LICENSE "ASL 2.0")
set(CPACK_RPM_PACKAGE_GROUP "Database")
set(CPACK_RPM_PACKAGE_URL "https://www.rethinkdb.com/")
set(CPACK_RPM_PACKAGE_VERSION "${RETHINKDB_RPM_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "${RETHINKDB_PACKAGE_RELEASE}")
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_PACKAGE_AUTOPROV ON)
set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE
    "${CMAKE_SOURCE_DIR}/packaging/rpm/preinstall.sh")
if(_rethinkdb_debug_packages)
    set(CPACK_RPM_DEBUGINFO_PACKAGE ON)
    # debugedit rewrites the build path in place, so <prefix>/src_0 must be
    # no longer than the source and build directories. Equal length is valid.
    set(_rethinkdb_rpm_debug_prefix
        "/usr/src/debug/${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
    string(LENGTH "${CMAKE_SOURCE_DIR}" _rethinkdb_src_len)
    string(LENGTH "${CMAKE_BINARY_DIR}" _rethinkdb_bin_len)
    set(_rethinkdb_shortest "${_rethinkdb_src_len}")
    if(_rethinkdb_bin_len LESS _rethinkdb_shortest)
        set(_rethinkdb_shortest "${_rethinkdb_bin_len}")
    endif()
    string(LENGTH "${_rethinkdb_rpm_debug_prefix}/src_0" _rethinkdb_debug_len)
    if(_rethinkdb_shortest LESS _rethinkdb_debug_len)
        set(_rethinkdb_rpm_debug_prefix "/usr")
    endif()
    set(CPACK_RPM_BUILD_SOURCE_DIRS_PREFIX "${_rethinkdb_rpm_debug_prefix}")
endif()

include(CPack)

add_custom_target(dist
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
        "-DRETHINKDB_VERSION=${RETHINKDB_VERSION}"
        "-DRETHINKDB_CODE_VERSION=${RETHINKDB_CODE_VERSION}"
        "-DPROTO_CC=${RETHINKDB_QL2_PB_CC}"
        "-DPROTO_H=${RETHINKDB_QL2_PB_H}"
        "-DBUILD_PORTABLE=$<IF:$<BOOL:${RETHINKDB_BUILD_PORTABLE}>,ON,OFF>"
        -P "${CMAKE_SOURCE_DIR}/cmake/dist.cmake"
    DEPENDS ql2_proto
    VERBATIM
    COMMENT "Building the source tarball"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_custom_target(package-deb
        COMMAND "${CMAKE_COMMAND}" -E env
            "MODE=binary"
            "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            "BINARY_DIR=${CMAKE_BINARY_DIR}"
            "CONFIG=$<CONFIG>"
            "CPACK=${CMAKE_CPACK_COMMAND}"
            bash "${CMAKE_SOURCE_DIR}/cmake/package-deb.sh"
        DEPENDS rethinkdb rethinkdb_man
        VERBATIM
        COMMENT "Packaging the DEB from the existing build"
    )
    add_custom_target(package-rpm
        COMMAND "${CMAKE_COMMAND}" -E env
            "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            "BINARY_DIR=${CMAKE_BINARY_DIR}"
            "CONFIG=$<CONFIG>"
            "CPACK=${CMAKE_CPACK_COMMAND}"
            bash "${CMAKE_SOURCE_DIR}/cmake/package-rpm.sh"
        DEPENDS rethinkdb rethinkdb_man
        VERBATIM
        COMMENT "Packaging the RPM from the existing build"
    )
    add_custom_target(package-deb-src
        COMMAND "${CMAKE_COMMAND}" -E env
            "MODE=src"
            "RETHINKDB_VERSION=${RETHINKDB_VERSION}"
            "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            "BINARY_DIR=${CMAKE_BINARY_DIR}"
            "BUILD_PORTABLE=$<IF:$<BOOL:${RETHINKDB_BUILD_PORTABLE}>,1,0>"
            bash "${CMAKE_SOURCE_DIR}/cmake/package-deb.sh"
        DEPENDS dist
        VERBATIM
        COMMENT "Packaging the Debian source package"
    )
endif()

if(APPLE)
    add_custom_target(package-osx
        COMMAND "${CMAKE_COMMAND}" -E env
            "CMAKE=${CMAKE_COMMAND}"
            bash "${CMAKE_SOURCE_DIR}/cmake/package-osx.sh"
            "${CMAKE_BINARY_DIR}"
            "${CMAKE_INSTALL_PREFIX}"
            "${CMAKE_INSTALL_BINDIR}"
            "$<CONFIG>"
        DEPENDS rethinkdb rethinkdb_man
        VERBATIM
        COMMENT "Packaging the macOS disk image from the existing build"
    )
endif()

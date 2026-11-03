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

set(CPACK_PACKAGE_NAME "rethinkdb")
set(CPACK_PACKAGE_VENDOR "RethinkDB")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The open-source database for the realtime web")
set(CPACK_PACKAGE_VERSION "${RETHINKDB_RPM_VERSION}")
set(CPACK_PACKAGE_CONTACT "packaging@rethinkdb.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/packaging/assets/docs/LICENSE")
set(CPACK_DEBIAN_PACKAGE_SECTION "database")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_VERSION "${RETHINKDB_DEBIAN_VERSION}")
set(CPACK_RPM_PACKAGE_LICENSE "ASL 2.0")
set(CPACK_RPM_PACKAGE_GROUP "Database")
set(CPACK_RPM_PACKAGE_URL "https://www.rethinkdb.com/")
set(CPACK_GENERATOR "TGZ")
if(APPLE)
    list(APPEND CPACK_GENERATOR "DragNDrop")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND CPACK_GENERATOR "DEB" "RPM")
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

add_custom_target(build-deb-src
    COMMAND "${CMAKE_COMMAND}" -E env
        "RETHINKDB_VERSION=${RETHINKDB_VERSION}"
        "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "BINARY_DIR=${CMAKE_BINARY_DIR}"
        "MODE=src"
        bash "${CMAKE_SOURCE_DIR}/cmake/package-deb.sh"
    DEPENDS dist
    VERBATIM
)
add_custom_target(build-deb
    COMMAND "${CMAKE_COMMAND}" -E env
        "RETHINKDB_VERSION=${RETHINKDB_VERSION}"
        "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "BINARY_DIR=${CMAKE_BINARY_DIR}"
        "MODE=binary"
        bash "${CMAKE_SOURCE_DIR}/cmake/package-deb.sh"
    DEPENDS dist
    VERBATIM
)

if(APPLE)
    add_custom_target(build-osx
        COMMAND bash "${CMAKE_SOURCE_DIR}/cmake/package-osx.sh"
            "${CMAKE_BINARY_DIR}"
            "${CMAKE_INSTALL_PREFIX}"
            "${CMAKE_INSTALL_BINDIR}"
        DEPENDS rethinkdb
        VERBATIM
    )
endif()

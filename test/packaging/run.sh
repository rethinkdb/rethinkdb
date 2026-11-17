#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.
#
# The package scripts must select CPack or the macOS installer tools and must
# not configure or compile. Stubbed tools record every invocation.
#
#   bash test/packaging/run.sh

set -euo pipefail

root=$(
    unset CDPATH
    cd -- "$(dirname -- "$0")/../.." && pwd
)
work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT

bin="${work}/bin"
log="${work}/invocations.log"
linux_build="${work}/linux-build"
osx_build="${work}/osx-build"
mkdir -p "${bin}" "${linux_build}/Release" "${osx_build}"

fail() {
    printf 'packaging test failed: %s\n' "$1" >&2
    if [[ -f "${log}" ]]; then
        printf 'invocations:\n' >&2
        cat "${log}" >&2
    fi
    exit 1
}

assert_log() {
    local pattern=$1
    if ! grep -F -x -e "${pattern}" "${log}" >/dev/null; then
        fail "missing invocation: ${pattern}"
    fi
}

assert_absent() {
    local pattern=$1
    if grep -F -e "${pattern}" "${log}" >/dev/null; then
        fail "unexpected invocation: ${pattern}"
    fi
}

write_platform_build() {
    local dest=$1
    local system_name=$2
    mkdir -p "${dest}/CMakeFiles/3.28.3" "${dest}/Release"
    cat > "${dest}/CMakeCache.txt" <<'EOF'
CMAKE_CACHE_MAJOR_VERSION:INTERNAL=3
CMAKE_CACHE_MINOR_VERSION:INTERNAL=28
CMAKE_CACHE_PATCH_VERSION:INTERNAL=3
EOF
    printf 'set(CMAKE_SYSTEM_NAME "%s")\n' "${system_name}" \
        > "${dest}/CMakeFiles/3.28.3/CMakeSystem.cmake"
    printf '%s\n' '# cpack config' > "${dest}/CPackConfig.cmake"
    cat > "${dest}/rethinkdb" <<'EOF'
#!/bin/sh
exit 0
EOF
    chmod +x "${dest}/rethinkdb"
    cp "${dest}/rethinkdb" "${dest}/Release/rethinkdb"
}

cat > "${linux_build}/CMakeCache.txt" <<'EOF'
CMAKE_SYSTEM_NAME:INTERNAL=Linux
EOF
printf '%s\n' '# cpack config' > "${linux_build}/CPackConfig.cmake"
cat > "${linux_build}/rethinkdb" <<'EOF'
#!/bin/sh
exit 0
EOF
chmod +x "${linux_build}/rethinkdb"
cp "${linux_build}/rethinkdb" "${linux_build}/Release/rethinkdb"

cat > "${osx_build}/CMakeCache.txt" <<'EOF'
CMAKE_SYSTEM_NAME:INTERNAL=Darwin
EOF

cat > "${bin}/cpack" <<EOF
#!/bin/sh
printf 'cpack %s\\n' "\$*" >> "${log}"
for arg in "\$@"; do
    case "\$arg" in
        -S|--build)
            printf 'cpack saw forbidden %s\\n' "\$arg" >&2
            exit 3
            ;;
    esac
done
exit 0
EOF

cat > "${bin}/cmake" <<EOF
#!/bin/sh
printf 'cmake %s\\n' "\$*" >> "${log}"
install=0
dest=\${DESTDIR:-}
for arg in "\$@"; do
    case "\$arg" in
        -S|--build|-B)
            printf 'cmake saw forbidden %s\\n' "\$arg" >&2
            exit 3
            ;;
        --install)
            install=1
            ;;
    esac
done
if [ "\${install}" -eq 1 ]; then
    if [ -z "\${dest}" ]; then
        printf 'cmake --install missing DESTDIR\\n' >&2
        exit 4
    fi
    mkdir -p "\${dest}/usr/local/bin"
    cp "${linux_build}/rethinkdb" "\${dest}/usr/local/bin/rethinkdb"
    chmod +x "\${dest}/usr/local/bin/rethinkdb"
fi
exit 0
EOF

cat > "${bin}/otool" <<EOF
#!/bin/sh
printf 'otool %s\\n' "\$*" >> "${log}"
printf '\\t/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1.0.0)\\n'
exit 0
EOF

cat > "${bin}/python3" <<EOF
#!/bin/sh
printf 'python3 %s\\n' "\$*" >> "${log}"
exit 0
EOF

cat > "${bin}/m4" <<EOF
#!/bin/sh
printf 'm4 %s\\n' "\$*" >> "${log}"
exit 0
EOF

cat > "${bin}/debuild" <<EOF
#!/bin/sh
printf 'debuild %s\\n' "\$*" >> "${log}"
exit 0
EOF

cat > "${bin}/dpkg-buildpackage" <<EOF
#!/bin/sh
printf 'dpkg-buildpackage %s\\n' "\$*" >> "${log}"
exit 9
EOF

chmod +x "${bin}/cpack" "${bin}/cmake" "${bin}/otool" "${bin}/python3" \
    "${bin}/m4" "${bin}/debuild" "${bin}/dpkg-buildpackage"

PATH="${bin}:${PATH}"
export PATH

env -u CMAKE -u CPACK \
    MODE=binary \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${linux_build}" \
    CONFIG=Release \
    bash "${root}/cmake/package-deb.sh"

env -u CMAKE -u CPACK \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${linux_build}" \
    CONFIG=Release \
    bash "${root}/cmake/package-rpm.sh"

env -u CMAKE -u CPACK \
    bash "${root}/cmake/package-osx.sh" \
    "${osx_build}" /usr/local bin Release

mkdir -p "${linux_build}/packages/rethinkdb-2.4.4"
env -u CMAKE -u CPACK \
    MODE=src \
    RETHINKDB_VERSION=2.4.4 \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${linux_build}" \
    BUILD_PORTABLE=0 \
    bash "${root}/cmake/package-deb.sh"

assert_log "cpack -G DEB -B ${linux_build}/packages/deb --config ${linux_build}/CPackConfig.cmake -C Release"
assert_log "cpack -G RPM -B ${linux_build}/packages/rpm --config ${linux_build}/CPackConfig.cmake -C Release"
assert_log "cmake --install ${osx_build} --config Release"
assert_log "debuild -S -sa -us -uc"
assert_absent "dpkg-buildpackage"
if grep -E '^(cmake|cpack) .*--build( |$)' "${log}" >/dev/null; then
    fail "cmake or cpack received --build"
fi
if grep -E '^(cmake|cpack) (.* )?-S( |$)' "${log}" >/dev/null; then
    fail "cmake or cpack received -S"
fi
if ! grep -F "create_dmg.py" "${log}" >/dev/null; then
    fail "macOS packaging did not invoke create_dmg.py"
fi

missing="${work}/missing-build"
mkdir -p "${missing}"
cat > "${missing}/CMakeCache.txt" <<'EOF'
CMAKE_SYSTEM_NAME:INTERNAL=Linux
EOF
printf '%s\n' '# cpack config' > "${missing}/CPackConfig.cmake"
if env -u CMAKE -u CPACK \
    MODE=binary \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${missing}" \
    bash "${root}/cmake/package-deb.sh" >"${work}/missing.out" 2>"${work}/missing.err"
then
    fail "package-deb succeeded without a server binary"
fi
if ! grep -F -e "Built server is missing" "${work}/missing.err" >/dev/null; then
    fail "missing-server error did not name the binary"
fi

linux_platform="${work}/linux-platform"
darwin_platform="${work}/darwin-platform"
write_platform_build "${linux_platform}" Linux
write_platform_build "${darwin_platform}" Darwin

env -u CMAKE -u CPACK \
    MODE=binary \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${linux_platform}" \
    CONFIG=Release \
    bash "${root}/cmake/package-deb.sh"

env -u CMAKE -u CPACK \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${linux_platform}" \
    CONFIG=Release \
    bash "${root}/cmake/package-rpm.sh"

env -u CMAKE -u CPACK \
    bash "${root}/cmake/package-osx.sh" \
    "${darwin_platform}" /usr/local bin Release

assert_log "cpack -G DEB -B ${linux_platform}/packages/deb --config ${linux_platform}/CPackConfig.cmake -C Release"
assert_log "cpack -G RPM -B ${linux_platform}/packages/rpm --config ${linux_platform}/CPackConfig.cmake -C Release"
assert_log "cmake --install ${darwin_platform} --config Release"

if env -u CMAKE -u CPACK \
    MODE=binary \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${darwin_platform}" \
    bash "${root}/cmake/package-deb.sh" \
    >"${work}/darwin.out" 2>"${work}/darwin.err"
then
    fail "package-deb accepted a Darwin build"
fi
if ! grep -F -e "DEB packages require a Linux CMake build: ${darwin_platform}" \
    "${work}/darwin.err" >/dev/null
then
    fail "Darwin build was not rejected as a non-Linux package"
fi

windows_platform="${work}/windows-platform"
mkdir -p "${windows_platform}/CMakeFiles/3.31.8"
printf 'CMAKE_CACHE_MAJOR_VERSION:INTERNAL=3\r\nCMAKE_CACHE_MINOR_VERSION:INTERNAL=31\r\nCMAKE_CACHE_PATCH_VERSION:INTERNAL=8\r\n' \
    > "${windows_platform}/CMakeCache.txt"
printf 'set(CMAKE_HOST_SYSTEM_NAME "Linux")\r\nset(CMAKE_SYSTEM_NAME "Windows")\r\n' \
    > "${windows_platform}/CMakeFiles/3.31.8/CMakeSystem.cmake"
printf '%s\n' '# cpack config' > "${windows_platform}/CPackConfig.cmake"

# shellcheck source=../../cmake/cmake-system-name.sh
source "${root}/cmake/cmake-system-name.sh"
windows_name=$(cmake_system_name "${windows_platform}" || true)
if [[ "${windows_name}" != "Windows" ]]; then
    fail "Windows platform file was read as '${windows_name}'"
fi

if env -u CMAKE -u CPACK \
    MODE=binary \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${windows_platform}" \
    bash "${root}/cmake/package-deb.sh" \
    >"${work}/windows-deb.out" 2>"${work}/windows-deb.err"
then
    fail "package-deb accepted a Windows build"
fi
if ! grep -F -e "DEB packages require a Linux CMake build: ${windows_platform}" \
    "${work}/windows-deb.err" >/dev/null
then
    fail "Windows build was not rejected by package-deb"
fi

if env -u CMAKE -u CPACK \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${windows_platform}" \
    bash "${root}/cmake/package-rpm.sh" \
    >"${work}/windows-rpm.out" 2>"${work}/windows-rpm.err"
then
    fail "package-rpm accepted a Windows build"
fi
if ! grep -F -e "RPM packages require a Linux CMake build: ${windows_platform}" \
    "${work}/windows-rpm.err" >/dev/null
then
    fail "Windows build was not rejected by package-rpm"
fi

if env -u CMAKE -u CPACK \
    bash "${root}/cmake/package-osx.sh" \
    "${windows_platform}" /usr/local bin Release \
    >"${work}/windows-osx.out" 2>"${work}/windows-osx.err"
then
    fail "package-osx accepted a Windows build"
fi
if ! grep -F -e "macOS packages require a Darwin CMake build: ${windows_platform}" \
    "${work}/windows-osx.err" >/dev/null
then
    fail "Windows build was not rejected by package-osx"
fi

log_lines=$(wc -l < "${log}")
ln -s "$(command -v dirname)" "${bin}/dirname"
bash_bin=$(command -v bash)
if env -u CMAKE -u CPACK PATH="${bin}" \
    MODE=binary \
    SOURCE_DIR="${root}" \
    BINARY_DIR="${linux_build}" \
    "${bash_bin}" "${root}/cmake/package-deb.sh" \
    >"${work}/nofile.out" 2>"${work}/nofile.err"
then
    fail "package-deb succeeded without the file utility"
fi
if ! grep -F -x -e "DEB packages require the file utility" \
    "${work}/nofile.err" >/dev/null
then
    fail "missing file utility was not reported"
fi
if [[ "$(wc -l < "${log}")" -ne "${log_lines}" ]]; then
    fail "package-deb invoked a tool without the file utility"
fi

printf 'ok\n'

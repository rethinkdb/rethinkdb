#!/usr/bin/env bash

# Hermes JavaScript engine package for RethinkDB
# Facebook's JavaScript engine optimized for React Native

version=0.12.0  # Stable Hermes version

# Hermes requires building from source with CMake
src_url=https://github.com/facebook/hermes/archive/refs/tags/v${version}.tar.gz
src_url_sha256=bd8fd158381813483123eb1ab553ed08db68e4949f314c99ee8fa79fa8f3e7ed

pkg_install-include () {
    mkdir -p "$install_dir/include"
    # Copy Hermes public headers
    if [ -d "$src_dir/include/hermes" ]; then
        cp -r "$src_dir/include/hermes"/* "$install_dir/include/" 2>/dev/null || true
    fi
    if [ -d "$src_dir/public/hermes" ]; then
        cp -r "$src_dir/public/hermes"/* "$install_dir/include/" 2>/dev/null || true
    fi
    # Copy JSI headers if present
    if [ -d "$src_dir/API/jsi" ]; then
        mkdir -p "$install_dir/include/jsi"
        cp "$src_dir"/API/jsi/*.h "$install_dir/include/jsi/" 2>/dev/null || true
    fi
}

pkg_build () {
    pkg_copy_src_to_build
    
    # Create build directory
    mkdir -p "$build_dir/build"
    cd "$build_dir/build"
    
    # Configure with CMake
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$(niceabspath "$install_dir")" \
        -DHERMES_ENABLE_DEBUGGER=OFF \
        -DHERMES_ENABLE_INTL=OFF \
        -DHERMES_ENABLE_FUZZING=OFF \
        -DHERMES_BUILD_APPLE_FRAMEWORK=OFF \
        -DBUILD_SHARED_LIBS=OFF
    
    # Build
    $EXTERN_MAKE hermes hermesvm
}

pkg_install () {
    pkg_build
    
    # Install headers
    pkg_install-include
    
    # Install libraries
    mkdir -p "$install_dir/lib"
    if [ -d "$build_dir/build/lib" ]; then
        cp "$build_dir/build/lib"/*.a "$install_dir/lib/" 2>/dev/null || true
    fi
    if [ -d "$build_dir/build/API/hermes" ]; then
        cp "$build_dir/build/API/hermes"/*.a "$install_dir/lib/" 2>/dev/null || true
    fi
    if [ -d "$build_dir/build/API/jsi" ]; then
        cp "$build_dir/build/API/jsi"/*.a "$install_dir/lib/" 2>/dev/null || true
    fi
}

pkg_link-flags () {
    local flags=""
    # Hermes libraries in order of dependency
    if [ -f "$install_dir/lib/libhermesvm.a" ]; then
        flags="$flags $install_dir/lib/libhermesvm.a"
    fi
    if [ -f "$install_dir/lib/libhermes.a" ]; then
        flags="$flags $install_dir/lib/libhermes.a"
    fi
    if [ -f "$install_dir/lib/libjsi.a" ]; then
        flags="$flags $install_dir/lib/libjsi.a"
    fi
    echo "$flags"
}

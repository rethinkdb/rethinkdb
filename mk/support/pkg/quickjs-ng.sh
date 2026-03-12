#!/usr/bin/env bash

# QuickJS-NG JavaScript engine package for RethinkDB
# Next-generation fork of QuickJS with improvements

# Use a recent stable commit
version=0.5.0

src_url=https://github.com/quickjs-ng/quickjs/archive/refs/tags/v${version}.tar.gz
src_url_sha256=41212a6fb84a6410c02eb4209e2942b2ca1e8f9d6b0c47c50d99e47f1f77a9d1

pkg_install-include () {
    mkdir -p "$install_dir/include/quickjs-ng"
    cp "$src_dir/quickjs.h" "$src_dir/quickjs-version.h" "$install_dir/include/quickjs-ng/"
}

pkg_build () {
    pkg_copy_src_to_build
    
    cd "$build_dir"
    
    # QuickJS-NG uses CMake
    mkdir -p build
    cd build
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$(niceabspath "$install_dir")" \
        -DCONFIG_LTO=ON \
        -DCONFIG_M32=OFF \
        -DBUILD_SHARED_LIBS=OFF
    
    # Build static library
    cmake --build . --target qjs
}

pkg_install () {
    pkg_build
    
    cd "$build_dir/build"
    
    # Install library
    mkdir -p "$install_dir/lib"
    cp libqjs.a "$install_dir/lib/" 2>/dev/null || cp *.a "$install_dir/lib/"
    
    # Install headers
    pkg_install-include
}

pkg_link-flags () {
    echo "$install_dir/lib/libqjs.a"
}

#!/usr/bin/env bash

# Duktape JavaScript engine package for RethinkDB
# Small, embeddable ES2015+ JavaScript engine

version=2.7.0  # Stable Duktape version

src_url=https://github.com/svaarala/duktape/releases/download/v${version}/duktape-${version}.tar.xz
src_url_sha256=90f8d2fa8b5567c6899830ddef2c03f3c27960b11aca222fa17aa7ac613c2890

pkg_install-include () {
    mkdir -p "$install_dir/include"
    cp "$src_dir/src/duktape.h" "$src_dir/src/duk_config.h" "$install_dir/include/"
}

pkg_build () {
    pkg_copy_src_to_build
    
    # Duktape provides a single-file library
    # We need to compile it as a static library
    cd "$build_dir"
    
    # Create a minimal wrapper to compile as library
    cat > duktape_lib.c << 'EOF'
#include "duktape.h"
// Additional Duktape API implementations can be added here
EOF
    
    # Compile static library
    ${CC:-gcc} -c -O2 -fPIC -I"$src_dir/src" "$src_dir/src/duktape.c" -o duktape.o
    ${AR:-ar} rcs libduktape.a duktape.o
}

pkg_install () {
    pkg_build
    
    mkdir -p "$install_dir/lib"
    cp "$build_dir/libduktape.a" "$install_dir/lib/"
}

pkg_link-flags () {
    echo "$install_dir/lib/libduktape.a"
}

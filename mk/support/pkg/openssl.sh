
version=1.0.1i

src_url="https://www.openssl.org/source/openssl-$version.tar.gz"


pkg_configure () {
    # use shared instead of no-shared because curl's configure script
    # fails on some platforms if it can't find -lssl
    in_dir "$build_dir" ./config shared --prefix="$(niceabspath "$install_dir")"
}

pkg_install () {
    pkg_copy_src_to_build

    pkg_configure

    # Compiling without -j1 causes a lot of "undefined reference" errors
    pkg_make -j1

    pkg_make install
}

pkg_link-flags () {
    local dl_libs=''
    if [[ "$OS" = "Linux" ]]; then
        dl_libs=-ldl
    fi
    echo "$install_dir/lib/lib$(lc $1).a" $dl_libs
}


version=2.3.1

src_url=http://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-$version.tar.gz

pkg_configure () {
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags="--host=$($CXX -dumpmachine)"
    fi
    in_dir "$build_dir" ./configure --prefix="$(niceabspath "$install_dir")" ${configure_flags:-}"$@"
}

pkg_link-flags () {
    local lib="$install_dir/lib/lib$(lc $1).a"
    if [[ ! -e "$lib" ]]; then
        echo "pkg.sh: error: static library was not built: $lib" >&2
        exit 1
    fi

    echo "$lib"
}

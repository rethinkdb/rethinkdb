
version=5.2.1

src_url=https://github.com/jemalloc/jemalloc/releases/download/$version/jemalloc-$version.tar.bz2
src_url_sha256=34330e5ce276099e2e8950d9335db5a875689a4c6a56751ef3b1d8c537f887f6

pkg_install () {
    configure_flags="--libdir=${install_dir}/lib"
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags+=" --host=$($CXX -dumpmachine)"
    fi

    pkg_copy_src_to_build
    pkg_configure ${configure_flags:-}
    pkg_make install
}


version=5.2.1

src_url=https://github.com/jemalloc/jemalloc/releases/download/$version/jemalloc-$version.tar.bz2
src_url_sha1=9e06b5cc57fd185379d007696da153893cf73e30

pkg_install () {
    configure_flags="--libdir=${install_dir}/lib"
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags+=" --host=$($CXX -dumpmachine)"
    fi

    pkg_copy_src_to_build
    pkg_configure ${configure_flags:-}
    pkg_make install
}

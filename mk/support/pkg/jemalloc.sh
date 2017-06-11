
version=4.5.0

src_url=https://github.com/jemalloc/jemalloc/releases/download/4.5.0/jemalloc-4.5.0.tar.bz2
src_url_sha1=e7714d070c623bff9acf682e9d52c930e491acd8

pkg_install () {
    configure_flags="--libdir=${install_dir}/lib"
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags+=" --host=$($CXX -dumpmachine)"
    fi

    pkg_copy_src_to_build
    pkg_configure ${configure_flags:-}
    pkg_make install
}

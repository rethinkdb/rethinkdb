
version=4.5.0

src_url=http://www.canonware.com/download/jemalloc/jemalloc-$version.tar.bz2
src_url_sha1=450a2f0331cd4544140bb3be5d6e11cc82383cdf

pkg_install () {
    configure_flags="--libdir=${install_dir}/lib"
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags+=" --host=$($CXX -dumpmachine)"
    fi

    pkg_copy_src_to_build
    pkg_configure ${configure_flags:-}
    pkg_make install
}

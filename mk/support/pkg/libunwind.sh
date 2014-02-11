
version=1.1

src_url=http://gnu.mirrors.pair.com/savannah/savannah/libunwind/libunwind-$version.tar.gz

pkg_install () {
    pkg_copy_src_to_build
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags="--host=$(${CXX:-c++} -dumpmachine)"
    fi
    pkg_configure $configure_flags
    pkg_make install
}


version=1.1

src_url=http://gnu.mirrors.pair.com/savannah/savannah/libunwind/libunwind-$version.tar.gz

pkg_install () {
    pkg_copy_src_to_build
    pkg_configure --host="$($CXX -dumpmachine)"
    pkg_make install
}

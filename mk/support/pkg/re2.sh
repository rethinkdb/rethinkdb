
version=20140111

src_url=http://re2.googlecode.com/files/re2-$version.tgz

pkg_install-include () {
    mkdir -p "$install_dir/include/re2"
    cp "$src_dir/re2/re2.h" "$install_dir/include/re2"
}

pkg_install () {
    pkg_copy_src_to_build
    pkg_make install prefix="$install_dir" CXXFLAGS="-O3" LDFLAGS="$PTHREAD_LIBS"
}

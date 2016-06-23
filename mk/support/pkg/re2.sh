
version=20140111

src_url=http://re2.googlecode.com/files/re2-$version.tgz
src_url_sha1=d51b3c2e870291070a1bcad8e5b471ae83e1f8df

pkg_install-include () {
    mkdir -p "$install_dir/include/re2"
    cp "$src_dir/re2"/*.h "$install_dir/include/re2"
}

pkg_install () {
    pkg_copy_src_to_build
    if [[ "$OS" = Darwin ]]; then
        CXXFLAGS="${CXXFLAGS:-} -stdlib=libc++"
        LDFLAGS="${LDFLAGS:-} -lc++"
    fi
    pkg_make install prefix="$install_dir" CXXFLAGS="${CXXFLAGS:-} -O3" LDFLAGS="${LDFLAGS:-} $PTHREAD_LIBS"
}

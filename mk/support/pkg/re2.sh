
version=20140111

src_url=http://re2.googlecode.com/files/re2-$version.tgz

pkg_install-include () {
    mkdir -p "$install_dir/include/re2"
    cp "$src_dir/re2"/*.h "$install_dir/include/re2"
}

pkg_install () {
    pkg_copy_src_to_build
    if [[ "$OS" = Darwin ]]; then
        # The symbol list is broken on OS X 10.7.5 with XCode 3.2.6
        sed -i '' 's/-exported_symbols_list libre2.symbols.darwin//' "$build_dir/Makefile"
    fi
    pkg_make install prefix="$install_dir" CXXFLAGS="${CXXFLAGS:-} -O3" LDFLAGS="${LDFLAGS:-} $PTHREAD_LIBS"
}

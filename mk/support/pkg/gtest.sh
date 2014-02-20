
version=1.6.0

pkg_fetch () {
    error "cannot fetch: gtest is already part of the source tree"
}

pkg_install () {
    pkg_copy_src_to_build
    make -C "$build_dir/make" gtest.a
    mkdir -p "$install_dir/lib"
    cp "$build_dir/make/gtest.a" "$install_dir/lib/libgtest.a"
}

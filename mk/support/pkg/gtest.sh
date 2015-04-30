
version=1.7.0
src_url=https://googletest.googlecode.com/files/gtest-$version.zip

pkg_install () {
    pkg_copy_src_to_build
    make -C "$build_dir/make" gtest.a
    mkdir -p "$install_dir/lib"
    cp "$build_dir/make/gtest.a" "$install_dir/lib/libgtest.a"
}

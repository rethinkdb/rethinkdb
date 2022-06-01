version=1.8.1
src_url=https://github.com/google/googletest/archive/release-$version.tar.gz
src_url_sha1=152b849610d91a9dfa1401293f43230c2e0c33f8

pkg_install-include () {
    test -e "$install_dir/include" && rm -rf "$install_dir/include"
    if [[ -e "$src_dir/googletest/include" ]]; then
        mkdir -p "$install_dir/include"
        cp -vRL "$src_dir/googletest/include/." "$install_dir/include"
    fi
}

pkg_install () {
    pkg_copy_src_to_build
    make -C "$build_dir/googletest/make" gtest.a
    mkdir -p "$install_dir/lib"
    cp "$build_dir/googletest/make/gtest.a" "$install_dir/lib/libgtest.a"
}

pkg_install-windows () {
    pkg_copy_src_to_build

    local out
    out=gtest.lib
    if [[ "$DEBUG" = 1 ]]; then
        out=gtestd.lib
    fi

    rm -rf "$build_dir/googletest"/{CMakeCache.txt,CMakeFiles}
    in_dir "$build_dir/googletest" cmake -G "Visual Studio 17 2022" -T "v141" -A "$PLATFORM"
    in_dir "$build_dir/googletest" cmake --build . --config "$CONFIGURATION"

    cp "$build_dir/googletest/$CONFIGURATION/$out" "$windows_deps_libs/gtest.lib"
}

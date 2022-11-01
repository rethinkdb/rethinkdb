version=1.8.1
src_url=https://github.com/google/googletest/archive/release-$version.tar.gz
src_url_sha256=9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c

pkg_install-include () {
    test -e "$install_dir/include" && rm -rf "$install_dir/include"
    if [[ -e "$src_dir/googletest/include" ]]; then
        mkdir -p "$install_dir/include"
        cp -vRL "$src_dir/googletest/include/." "$install_dir/include"
    fi
}

pkg_install () {
    pkg_copy_src_to_build
    $EXTERN_MAKE -C "$build_dir/googletest/make" gtest.a
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
    in_dir "$build_dir/googletest" "$CMAKE" -G "Visual Studio 17 2022" -T "v141" -A "$PLATFORM"
    in_dir "$build_dir/googletest" "$CMAKE" --build . --config "$CONFIGURATION"

    cp "$build_dir/googletest/$CONFIGURATION/$out" "$windows_deps_libs/gtest.lib"
}

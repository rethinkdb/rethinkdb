
version=3.19.4

src_url=https://github.com/google/protobuf/releases/download/v$version/protobuf-cpp-$version.tar.gz
src_url_sha256=89ac31a93832e204db6d73b1e80f39f142d5747b290f17340adce5be5b122f94

pkg_install-include () {
    in_dir "$src_dir/src" find . \( -name "*.h" -o -name "*.inc" \) | while read -r file; do
        mkdir -p "$install_dir/include/$(dirname "$file")"
        cp -af "$src_dir/src/$file" "$install_dir/include/$file"
    done
}

pkg_install () (
    pkg_copy_src_to_build

    configure_flags="--libdir=${install_dir}/lib"

    if [[ "$CROSS_COMPILING" = 1 ]]; then
        cross_build_dir=$build_dir/cross_build
        configure_flags="--with-protoc=$cross_build_dir/src/protoc --host=$($CXX -dumpmachine)"
        if ! test -e $cross_build_dir/src/protoc; then
            cp -a "$src_dir/." "$cross_build_dir"
            (
                cross_build_env
                in_dir "$cross_build_dir" ./configure --enable-static --disable-shared
                in_dir "$cross_build_dir" $EXTERN_MAKE
            )
        fi
    fi

    pkg_configure --prefix="$(niceabspath "$install_dir")" $configure_flags --enable-static --disable-shared
    pkg_make ${protobuf_install_target:-install}

    if [[ "$CROSS_COMPILING" = 1 ]]; then
        cp -f $cross_build_dir/src/protoc $install_dir/bin/protoc
    fi
)

pkg_install-windows () {
    pkg_copy_src_to_build

    rm -rf "$build_dir/cmake"/{CMakeCache.txt,CMakeFiles}
    in_dir "$build_dir/cmake" "$CMAKE" -G "Visual Studio 17 2022" -T "v141" -A "$PLATFORM" -DCMAKE_INSTALL_PREFIX=../out -DCMAKE_BUILD_TYPE="$CONFIGURATION" -Dprotobuf_BUILD_TESTS=OFF
    in_dir "$build_dir/cmake" "$CMAKE" --build . --config "$CONFIGURATION"
    in_dir "$build_dir/cmake" "$CMAKE" --build . --config "$CONFIGURATION" --target install

    local out
    out=libprotobuf.lib
    if [[ "$DEBUG" = 1 ]]; then
        out=libprotobufd.lib
    fi

    cp "$build_dir/out/lib/$out" "$windows_deps_libs/libprotobuf.lib"
    mkdir -p "$install_dir/bin"
    cp "$build_dir/out/bin/protoc.exe" "$install_dir/bin/"
}

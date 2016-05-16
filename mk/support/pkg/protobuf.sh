
version=2.5.0

src_url=http://protobuf.googlecode.com/files/protobuf-$version.tar.bz2
src_url_sha1=62c10dcdac4b69cc8c6bb19f73db40c264cb2726

pkg_install-include () {
    in_dir "$src_dir/src" find . -name \*.h | while read -r file; do
        mkdir -p "$install_dir/include/$(dirname "$file")"
        cp -af "$src_dir/src/$file" "$install_dir/include/$file"
    done
}

pkg_install () (
    pkg_copy_src_to_build

    configure_flags=

    if [[ "$CROSS_COMPILING" = 1 ]]; then
        cross_build_dir=$build_dir/cross_build
        configure_flags="--with-protoc=$cross_build_dir/src/protoc --host=$($CXX -dumpmachine)"
        if ! test -e $cross_build_dir/src/protoc; then
            cp -a "$src_dir/." "$cross_build_dir"
            (
                cross_build_env
                in_dir "$cross_build_dir" ./configure --enable-static --disable-shared
                in_dir "$cross_build_dir" make
            )
        fi
    fi

    if [[ "$OS" = "Darwin" ]]; then
        # TODO: is this necessary?
        export CXX=clang++
        export CXXFLAGS='-std=c++11 -stdlib=libc++'
        export LDFLAGS=-lc++
    fi

    pkg_configure --prefix="$(niceabspath "$install_dir")" $configure_flags --enable-static --disable-shared
    pkg_make ${protobuf_install_target:-install}

    if [[ "$CROSS_COMPILING" = 1 ]]; then
        cp -f $cross_build_dir/src/protoc $install_dir/bin/protoc
    fi
)

pkg_install-windows () {
    pkg_copy_src_to_build

    for project in libprotobuf libprotoc protoc; do
        in_dir "$build_dir" "$MSBUILD" /nologo /maxcpucount /p:Configuration=$CONFIGURATION /p:Platform=$PLATFORM vsprojects\\$project.vcxproj
    done

    cp "$build_dir/vsprojects/$VS_OUTPUT_DIR/libprotobuf.lib" "$windows_deps_libs/"
    mkdir -p "$install_dir/bin"
    cp "$build_dir/vsprojects/$VS_OUTPUT_DIR/protoc.exe" "$install_dir/bin/"
}

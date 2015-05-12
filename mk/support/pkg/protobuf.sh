
version=2.5.0

src_url=http://protobuf.googlecode.com/files/protobuf-$version.tar.bz2

pkg_install-include () {
    pkg_copy_src_to_build
    find "$build_dir/src" -name \*.h | while read -r file; do
        relative=${file#$build_dir/src/}
        dir=$install_dir/include/${file%/*}
        mkdir -p "$dir"
        cp -avf "$file" "$dir"
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
                in_dir "$cross_build_dir" $MAKE_EXECUTABLE
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

    # TODO: is there a platform that needs the library path to be adjusted like this?
    # local protoc="$install_dir/bin/protoc"
    # if test -e "$protoc"; then
    #     mv "$protoc" "$protoc-orig"
    #     echo '#!/bin/sh' > "$protoc"
    #     echo "export LD_LIBRARY_PATH='$install_dir/lib':\$LD_LIBRARY_PATH" >> "$protoc"
    #     echo "export PATH='$install_dir/bin':\$PATH" >> "$protoc"
    #     echo 'exec protoc-orig "$@"' >> "$protoc"
    #     chmod +x "$protoc"
    # fi
)

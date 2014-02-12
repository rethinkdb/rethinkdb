
version=2.5.0

src_url=http://protobuf.googlecode.com/files/protobuf-$version.tar.bz2

pkg_install-include () {
    protobuf_install_target=install-data pkg_install
}

pkg_install () (
    pkg_copy_src_to_build

    if [[ "$OS" = "Darwin" ]]; then
        export CXX=clang++
        export CXXFLAGS='-std=c++11 -stdlib=libc++'
        export LDFLAGS=-lc++
    fi

    pkg_configure --prefix="$(niceabspath "$install_dir")"
    pkg_make ${protobuf_install_target:-install}

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

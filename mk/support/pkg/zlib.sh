
version=1.2.11

src_url=https://zlib.net/fossils/zlib-$version.tar.gz
src_url_sha256=c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1

pkg_install-include () {
    mkdir -p "$install_dir/include"
    cp "$src_dir/zconf.h" "$src_dir/zlib.h" "$install_dir/include"
}

pkg_install-windows () {
    pkg_copy_src_to_build

    local flags=
    if [[ "$DEBUG" = 1 ]]; then
        flags=RUNTIME=-MTd
    fi

    in_dir "$build_dir" with_vs_env nmake -f 'win32\Makefile.msc' clean all $flags

    cp "$build_dir/zlib.lib" "$windows_deps_libs/"
}

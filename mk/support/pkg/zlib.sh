
version=1.3.1

src_url=https://zlib.net/fossils/zlib-$version.tar.gz
src_url_sha256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23

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

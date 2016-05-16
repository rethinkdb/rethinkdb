
version=1.2.8

src_url=http://zlib.net/zlib-$version.tar.gz
src_url_sha1=a4d316c404ff54ca545ea71a27af7dbc29817088

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

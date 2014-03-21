
version=1.2.8

src_url=http://zlib.net/zlib-$version.tar.gz

pkg_install-include () {
    mkdir -p "$install_dir/include"
    cp "$src_dir/zconf.h" "$src_dir/zlib.h" "$install_dir/include"
}

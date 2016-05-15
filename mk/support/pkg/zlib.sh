
version=1.2.8

src_url=http://zlib.net/zlib-$version.tar.gz
src_url_sha1=a4d316c404ff54ca545ea71a27af7dbc29817088

pkg_install-include () {
    mkdir -p "$install_dir/include"
    cp "$src_dir/zconf.h" "$src_dir/zlib.h" "$install_dir/include"
}

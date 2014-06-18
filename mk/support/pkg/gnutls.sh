
version=3.3.2

src_url=ftp://ftp.gnutls.org/gcrypt/gnutls/v${version%.*}/gnutls-$version.tar.xz

pkg_configure () {
    set
    in_dir "$build_dir" ./configure --prefix="$(niceabspath "$install_dir")" --enable-static --disable-shared
}

pkg_depends () {
    echo nettle gmp
}

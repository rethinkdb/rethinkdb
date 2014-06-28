
version=7.36.0

src_url=http://curl.haxx.se/download/curl-$version.tar.bz2

pkg_configure () {
    local prefix
    prefix="$(niceabspath "$install_dir")"
    in_dir "$build_dir" ./configure --prefix="$prefix" --with-gnutls --without-ssl
}

pkg_install-include () {
    pkg_copy_src_to_build
    pkg_configure
    make -C "$build_dir/include" install
}

pkg_depends () {
    echo libidn zlib gnutls
}


pkg_link-flags () {
    local flags
    flags="`"$install_dir/bin/curl-config" --static-libs`"
    flags=$(echo " $flags " | sed 's/ -lz \| -lidn \| -lgnutls / /')
    echo  $flags `pkg link-flags zlib z` `pkg link-flags libidn idn` `pkg link-flags gnutls gnutls`
}

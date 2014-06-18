
version=2.7

src_url=http://www.lysator.liu.se/~nisse/archive/nettle-$version.tar.gz

pkg_configure () {
    in_dir "$build_dir" ./configure --prefix="$(niceabspath "$install_dir")" --libdir="$(niceabspath "$install_dir/lib")"
}

pkg_depends () {
    echo gmp
}


version=2.1

src_url=http://gperftools.googlecode.com/files/gperftools-$version.tar.gz

pkg_depends () {
    echo libunwind
}

pkg_link-flags () {
    echo "$install_dir/lib/libtcmalloc_minimal.a"
}

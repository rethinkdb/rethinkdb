
version=2.1

src_url=http://gperftools.googlecode.com/files/gperftools-$version.tar.gz

pkg_install () {
    pkg_copy_src_to_build

    pkg_configure LDFLAGS="$LDFLAGS -lunwind"
    pkg_make install
}

pkg_depends () {
    echo libunwind
}

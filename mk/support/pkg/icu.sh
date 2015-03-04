
version=54.1

src_url=http://download.icu-project.org/files/icu4c/$version/icu4c-${version/\./_}-src.tgz

pkg_configure () {
    in_dir "$build_dir/source" ./configure --prefix="$(niceabspath "$install_dir")" --enable-static "$@"
}

pkg_make () {
    in_dir "$build_dir/source" make "$@"
}

pkg_install-include () {
    pkg_copy_src_to_build
    pkg_configure
    # The install-headers-recursive target is missing. Let's patch it.
    sed -i.bak $'s/distclean-recursive/install-headers-recursive/g;$a\\\ninstall-headers-local:' "$build_dir/source/Makefile"
    for file in "$build_dir"/source/*/Makefile; do
        sed -i.bak $'$a\\\ninstall-headers:' "$file"
    done
    pkg_make install-headers-recursive
}

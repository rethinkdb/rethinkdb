
version=54.1

src_url=http://download.icu-project.org/files/icu4c/$version/icu4c-${version/\./_}-src.tgz

pkg_configure () {
    in_dir "$build_dir/source" ./configure --prefix="$(niceabspath "$install_dir")" --enable-static ${configure_flags:-} "$@"
}

pkg_make () {
    in_dir "$build_dir/source" make "$@"
}

pkg_install-include () {
    pkg_copy_src_to_build
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        ( cross_build_env; pkg_configure )
    else
        pkg_configure
    fi
    # The install-headers-recursive target is missing. Let's patch it.
    sed -i.bak $'s/distclean-recursive/install-headers-recursive/g;$a\\\ninstall-headers-local:' "$build_dir/source/Makefile"
    for file in "$build_dir"/source/*/Makefile; do
        sed -i.bak $'$a\\\ninstall-headers:' "$file"
    done
    pkg_make install-headers-recursive
}

pkg_install () {
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        cross_build_dir=$build_dir/cross_build/source
        configure_flags="--with-cross-build=$cross_build_dir --host=$($CXX -dumpmachine)"
        if ! test -e $cross_build_dir/src/protoc; then
            rm -rf "$build_dir/cross_build"
            cp -a "$src_dir/." "$build_dir/cross_build"
            (
                cross_build_env
                in_dir "$cross_build_dir" ./configure
                in_dir "$cross_build_dir" make
            )
        fi
    fi

    pkg_copy_src_to_build
    sed -i.bak '/LDFLAGSICUDT=-nodefaultlibs -nostdlib/d' "$build_dir/source/config/mh-linux"
    pkg_configure ${configure_flags:-}
    pkg_make install
}

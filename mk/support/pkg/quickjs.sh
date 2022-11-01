github_user=rethinkdb
version=95d6dcf6358a74e9cbe04eade7c383f43ba306cb

src_url=https://github.com/${github_user}/quickjspp/archive/${version}.tar.gz
src_url_sha256=ce498cdd2879ac2ad1e3755da8642867f9363e27d41e9aea4aad6692fde8f138

pkg_configure () {
    ( cd "$build_dir" && sed "s!^prefix=/usr/local\$!prefix=$(niceabspath "$install_dir")!" < Makefile > Makefile.tmp && mv Makefile.tmp Makefile )
}

pkg_link-flags () {
    local lib="$install_dir/lib/quickjs/libquickjs.a"
    if [[ ! -e "$lib" ]]; then
        echo "quickjs.sh: error: static library was not built: $lib" >&2
        exit 1
    fi
    echo "$lib"
}

pkg_install-include () {
    test -e "$install_dir/include" && rm -rf "$install_dir/include"
    mkdir -p "$install_dir/include/quickjs"
    cp "$src_dir"/quickjs.h "$install_dir/include/quickjs"
    cp "$src_dir"/quickjs-version.h "$install_dir/include/quickjs"
}

pkg_install () {
    if ! fetched; then
        error "cannot install package, it has not been fetched"
    fi
    pkg_copy_src_to_build
    pkg_configure ${configure_flags:-}
    # The pkg.sh pkg_install would work on newer systems (invoking
    # "pkg_make install").  But instead, we (a) avoid building quickjs
    # executables, and (b) we avoid linking problems that occur on
    # older platforms with those executables.
    pkg_make libquickjs.a
    mkdir -p "$install_dir/lib/quickjs"
    install -m644 "$build_dir/libquickjs.a" "$install_dir/lib/quickjs/libquickjs.a"
}

pkg_install-windows () {
    if ! fetched; then
        error "cannot install package, it has not been fetched"
    fi
    pkg_copy_src_to_build

    in_dir "$build_dir" premake5 vs2017
    in_dir "$build_dir" "$MSBUILD" /nologo /p:Configuration=$CONFIGURATION /p:Platform=$PLATFORM /p:PlatformToolset=v141 /p:WindowsTargetPlatformVersion=10.0.19041.0 .build/vs2017/quickjs.vcxproj
    cp "$build_dir/.bin/$CONFIGURATION/$PLATFORM/quickjs.lib" "$windows_deps_libs"
}

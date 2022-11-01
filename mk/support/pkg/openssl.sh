version=3.0.7

src_url="https://www.openssl.org/source/openssl-$version.tar.gz"
src_url_backup="ftp://ftp.openssl.org/source/openssl-$version.tar.gz"
src_url_sha256="83049d042a260e696f62406ac5c08bf706fd84383f945cf21bd61e9ed95c396e"

pkg_configure () {
    case $($CXX -dumpmachine) in
        arm*) arch=arm ;;
        *)    arch=
    esac

    # use shared instead of no-shared because curl's configure script
    # fails on some platforms if it can't find -lssl
    if [[ "$OS" = "Darwin" ]]; then
        if [[ "$arch" = arm ]]; then
            in_dir "$build_dir" ./Configure darwin64-arm64-cc -shared --prefix="$(niceabspath "$install_dir")" $CFLAGS
        else
            in_dir "$build_dir" ./Configure darwin64-x86_64-cc -shared --prefix="$(niceabspath "$install_dir")" $CFLAGS
        fi
    else
        if [[ "$arch" = arm ]]; then
            in_dir "$build_dir" ./Configure linux-armv4 -shared --prefix="$(niceabspath "$install_dir")" $CFLAGS
        else
            in_dir "$build_dir" ./config shared --prefix="$(niceabspath "$install_dir")" $CFLAGS
        fi
    fi
}

separate_install_include=false

pkg_install () {
    pkg_copy_src_to_build

    pkg_configure

    pkg_make

    pkg_make install
}

pkg_install-windows () {
    rm -rf "$build_dir"
    pkg_copy_src_to_build

    local config script out
    case "$PLATFORM" in
        Win32) config=VC-WIN32 ;
               script=ms ;;
        x64) config=VC-WIN64A ;
             script=win64a ;;
    esac

    out=out32
    if [[ "$DEBUG" = 1 ]]; then
        config=debug-$config
        out=$out.dbg
    fi

    in_dir "$build_dir" with_vs_env perl Configure $config no-asm no-shared
    in_dir "$build_dir" with_vs_env nmake

    cp "$build_dir"/{libssl,libcrypto}.lib "$windows_deps_libs/"

    mkdir -p "$windows_deps/include/"
    cp -R "$build_dir"/include/* "$windows_deps/include/"
}

pkg_link-flags () {
    local dl_libs
    dl_libs=''
    if [[ "$OS" = "Linux" ]]; then
        dl_libs=-ldl
    fi
    local libdir
    libdir="$(test -d "$install_dir/lib64" && echo lib64 || echo lib)"
    echo "$install_dir/${libdir}/lib$(lc $1).a" $dl_libs
}

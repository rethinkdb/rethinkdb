
version=1.0.1t

src_url="https://www.openssl.org/source/openssl-$version.tar.gz"
src_url_backup="ftp://ftp.openssl.org/source/old/1.0.1/openssl-$version.tar.gz"
src_url_sha1="a684ba59d6721a90f354b1953e19611646be7e7d"

pkg_configure () {
    case $($CXX -dumpmachine) in
        arm*) arch=arm ;;
        *)    arch=
    esac

    # use shared instead of no-shared because curl's configure script
    # fails on some platforms if it can't find -lssl
    if [[ "$OS" = "Darwin" ]]; then
        in_dir "$build_dir" ./Configure darwin64-x86_64-cc -shared --prefix="$(niceabspath "$install_dir")"
    elif [[ "$arch" = arm ]]; then
        in_dir "$build_dir" ./Configure linux-armv4 -shared --prefix="$(niceabspath "$install_dir")"
    else
        in_dir "$build_dir" ./config shared --prefix="$(niceabspath "$install_dir")"
    fi
}

pkg_install () {
    pkg_copy_src_to_build

    pkg_configure

    # Compiling without -j1 causes a lot of "undefined reference" errors
    pkg_make -j1

    pkg_make install
}

pkg_link-flags () {
    local dl_libs=''
    if [[ "$OS" = "Linux" ]]; then
        dl_libs=-ldl
    fi
    echo "$install_dir/lib/lib$(lc $1).a" $dl_libs
}

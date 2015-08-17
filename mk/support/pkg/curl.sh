
version=7.40.0

src_url=http://curl.haxx.se/download/curl-$version.tar.bz2

pkg_configure () {
    local prefix
    prefix="$(niceabspath "$install_dir")"
    local ssl_command
    ssl_command="--with-ssl"
    if [[ "$OS" = "Darwin" ]]; then
        sslCommand="--with-darwinssl --without-ssl"
    fi
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags="--host=$($CXX -dumpmachine)"
    fi
    in_dir "$build_dir" ./configure --prefix="$prefix" --without-gnutls $ssl_command --without-librtmp --disable-ldap --disable-shared ${configure_flags:-}
}

pkg_install-include () {
    pkg_copy_src_to_build
    pkg_configure
    make -C "$build_dir/include" install
}

pkg_install () {
    pkg_copy_src_to_build

    pkg_configure

    # install the libraries
    make -C "$build_dir/lib" install-libLTLIBRARIES

    # install the curl-config script
    make -C "$build_dir" install-binSCRIPTS
}

pkg_depends () {
    local deps='libidn zlib'
    if will_fetch openssl; then
        echo $deps openssl
    else
        echo $deps
    fi
}

pkg_link-flags () {
    local ret=''
    out () { ret="$ret$@ " ; }
    out_openssl () {
        if will_fetch openssl; then
            out `pkg link-flags openssl ssl`
        else
            out -l$1
        fi
    }
    local flags
    local dl_libs=''
    flags="`"$install_dir/bin/curl-config" --static-libs`"
    for flag in $flags; do
        case "$flag" in
            -lz)      out `pkg link-flags zlib z` ;;
            -lidn)    out `pkg link-flags libidn idn` ;;
            -lssl)    out_openssl ssl ;;
            -lcrypto) out_openssl crypto ;;
            -ldl)     dl_libs=-ldl;; # Linking may fail if -ldl isn't last
            -lrt)     out "$flag" ;;
            -l*)      echo "Warning: '$pkg' links with '$flag'" >&2
                      out "$flag" ;;
            *)        out "$flag" ;;
        esac
    done
    echo "$ret" $dl_libs
}

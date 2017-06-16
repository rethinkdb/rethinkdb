
version=7.54.1

src_url=http://curl.haxx.se/download/curl-$version.tar.bz2
src_url_sha1=f5193316e4b5ff23505cb09bc946763d35d02cd6

pkg_configure () {
    local prefix
    prefix="$(niceabspath "$install_dir")"
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags="--host=$($CXX -dumpmachine)"
    fi
    in_dir "$build_dir" ./configure --prefix="$prefix" --without-gnutls --with-ssl --without-nghttp2 --without-librtmp --disable-ldap --disable-shared ${configure_flags:-}
}

pkg_install-include () {
    pkg_copy_src_to_build
    pkg_configure
    make -C "$build_dir/include" install
}

pkg_install-include-windows () {
    mkdir -p "$windows_deps/include/curl"
    cp -vL "$src_dir/include/curl"/*.h  "$windows_deps/include/curl/"
    mkdir -p "$install_dir/include"
}

pkg_install-windows () {
    pkg_copy_src_to_build

    local flags out_config out_suffix machine
    if [[ "$DEBUG" = 1 ]]; then
        flags=DEBUG=yes
        out_config=debug
        out_suffix=_debug
    else
        flags=
        out_config=release
        out_suffix=
    fi
    case "$PLATFORM" in
        Win32) machine=x86 ;;
        x64) machine=x64 ;;
    esac

    in_dir "$build_dir/winbuild" with_vs_env \
      nmake /f Makefile.vc mode=static MACHINE=$machine RTLIBCFG=static $flags

    cp "$build_dir/builds/libcurl-vc-$machine-$out_config-static-ipv6-sspi-winssl/lib/libcurl_a${out_suffix}.lib" "$windows_deps_libs/curl.lib"
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
    if [[ "$OS" != Windows ]]; then
        local deps='libidn zlib'
        if will_fetch openssl; then
            echo $deps openssl
        else
            echo $deps
        fi
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
            -l*)      if ! [[ $LDFLAGS =~ (^| )"$flag"($| ) ]]; then
                          echo "$LDFLAGS ||||| $flag" >&2
                          echo "Warning: '$pkg' links with '$flag'" >&2
                      fi
                      out "$flag" ;;
            *)        out "$flag" ;;
        esac
    done
    echo "$ret" $dl_libs
}

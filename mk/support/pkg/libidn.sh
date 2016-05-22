
version=1.28

src_url=http://ftp.gnu.org/gnu/libidn/libidn-$version.tar.gz
src_url_sha1=725587211b229c156e29fa2ad116b0ef71a7ca17

pkg_configure () {
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        configure_flags="--host=$($CXX -dumpmachine)"
    fi
    in_dir "$build_dir" ./configure --prefix="$(niceabspath "$install_dir")" ${configure_flags:-}"$@"
}

pkg_link-flags () {
    local lib="$install_dir/lib/lib$(lc $1).a"
    if [[ ! -e "$lib" ]]; then
        echo "pkg.sh: error: static library was not built: $lib" >&2
        exit 1
    fi

    TEMPFOLDER="$BUILD_ROOT_DIR"/tmp
    if [[ ! -e "$TEMPFOLDER" ]]; then
        mkdir "$TEMPFOLDER"
    fi
    INFILE=$TEMPFOLDER/libiconv_check.c
    OUTFILE=$TEMPFOLDER/libiconv_check.out
    printf "#include <iconv.h>\n int main(void) { iconv_t sample; sample = iconv_open(\"UTF-8\", \"ASCII\"); return 0; }" >$INFILE

    if $CXX "$INFILE" -o "$OUTFILE" 2>/dev/null 1>&2; then
        echo "$lib"
    elif $CXX -liconv "$INFILE" -o "$OUTFILE" 2>/dev/null 1>&2; then
        echo "-liconv $lib"
    else
        rm -rf "$TEMPFOLDER"
        echo "Unable to figure out how to link libiconv" >&2
        exit 1
    fi
}

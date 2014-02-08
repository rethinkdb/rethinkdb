
version=3.22.24.17

# Other versions of v8 to test against:
# version=3.7.12.22 # Ubuntu Lucid
# version=3.17.4.1  # A pre-3.19 version
# version=3.19.18.4 # A post-3.19 version
# version=3.24.30.1 # A future version, with incompatible API changes
# See http://omahaproxy.appspot.com/ for the current stable/beta/dev versions of v8

src_url=https://commondatastorage.googleapis.com/chromium-browser-official/v8-$version.tar.bz2

pkg_install () {
    pkg_copy_src_to_build
    if ! python --version 2>&1  | grep --quiet 'Python 2'; then
        pybin=$build_dir/pybin
        mkdir -p "$pybin"
        rm -f "$pybin/python"
        ln -s "$(which python2)" "$pybin/python"
        export PATH=$pybin:$PATH
    fi
    mkdir -p "$install_dir/lib"
    pkg_make native CXXFLAGS="${CXXFLAGS:-} -Wno-error"
    find "$build_dir" -iname "*.o" | grep -v '\/preparser_lib\/' | xargs ar cqs "$install_dir/lib/libv8.a"
}

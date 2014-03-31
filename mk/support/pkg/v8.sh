
version=3.22.24.17

# Other versions of v8 to test against:
# version=3.7.12.22 # Ubuntu Lucid
# version=3.17.4.1  # A pre-3.19 version
# version=3.19.18.4 # A post-3.19 version
# version=3.24.30.1 # A future version, with incompatible API changes
# See http://omahaproxy.appspot.com/ for the current stable/beta/dev versions of v8

src_url=http://commondatastorage.googleapis.com/chromium-browser-official/v8-$version.tar.bz2

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
    CXX=${CXX:-g++} # as defined by the v8 Makefile
    host=$($CXX -dumpmachine)
    makeflags=
    case ${host%%-*} in
        i?86)   arch=ia32 ;;
        x86_64) arch=x64 ;;
        arm*)   arch=arm ;;
        *)      arch=native ;;
    esac
    case "$($CXX -dumpversion)" in
        4.4*)
            # Some versions of GCC 4.4 crash with "pure virtual method called" unless these flag are passed
            CXXFLAGS="${CXXFLAGS:-} -fno-function-sections -fno-inline-functions"
    esac
    pkg_make $arch.release CXX=$CXX LINK=$CXX LINK.target=$CXX werror=no $makeflags CXXFLAGS="${CXXFLAGS:-} -Wno-error"
    find "$build_dir/out/$arch.release/obj.target" -iname "*.o" | grep -v '\/preparser_lib\/' | xargs ${AR:-ar} cqs "$install_dir/lib/libv8.a"
}

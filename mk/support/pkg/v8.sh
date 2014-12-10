
version=3.30.33.16
# See http://omahaproxy.appspot.com/ for the current stable/beta/dev versions of v8

src_url=http://commondatastorage.googleapis.com/chromium-browser-official/v8-$version.tar.bz2

pkg_install-include () {
    rm -rf "$install_dir/include"
    mkdir -p "$install_dir/include"
    cp -RL "$src_dir/include/." "$install_dir/include"
    sed -i 's/include\///' "$install_dir/include/libplatform/libplatform.h"
}

pkg_install () {
    pkg_copy_src_to_build
    if ! python --version 2>&1  | grep --quiet 'Python 2'; then
        pybin=$build_dir/pybin
        mkdir -p "$pybin"
        rm -f "$pybin/python"
        ln -s "$(command -v python2)" "$pybin/python"
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
    mode=release
    pkg_make $arch.$mode CXX=$CXX LINK=$CXX LINK.target=$CXX werror=no $makeflags CXXFLAGS="${CXXFLAGS:-} -Wno-error"
    for lib in `find "$build_dir/out/$arch.$mode" -maxdepth 1 -name \*.a` `find "$build_dir/out/$arch.$mode/obj.target" -name \*.a`; do
        name=`basename $lib`
        cp $lib "$install_dir/lib/${name/.$arch/}"
    done
    touch "$install_dir/lib/libv8.a" # Create a dummy libv8.a because the makefile looks for it
}

pkg_link-flags () {
    # These are the necessary libraries recommended by the docs:
    # https://developers.google.com/v8/get_started#hello
    for lib in libv8_{base,libbase,snapshot,libplatform} libicu{i18n,uc,data}; do
        echo "$install_dir/lib/$lib.a"
    done
}


version=3.30.33.16
# See http://omahaproxy.appspot.com/ for the current stable/beta/dev versions of v8

src_url=http://commondatastorage.googleapis.com/chromium-browser-official/v8-$version.tar.bz2

pkg_fetch () {
    pkg_fetch_archive
    find "$src_dir"/third_party/icu/* -maxdepth 0 -not -name 'icu.gyp*' -print0 | xargs -0 rm -rf
}

pkg_install-include () {
    rm -rf "$install_dir/include"
    mkdir -p "$install_dir/include"
    cp -RL "$src_dir/include/." "$install_dir/include"
    sed -i.bak 's/include\///' "$install_dir/include/libplatform/libplatform.h"
}

pkg_install () {
    pkg_copy_src_to_build
    sed -i.bak '/unittests/d;/cctest/d' "$build_dir/build/all.gyp" # don't build the tests
    mkdir -p "$install_dir/lib"
    if [[ "$OS" = Darwin ]]; then
        export CXXFLAGS="-stdlib=libc++ ${CXXFLAGS:-}"
        export LDFLAGS="-stdlib=libc++ -lc++ ${LDFLAGS:-}"
        export GYP_DEFINES='clang=1 mac_deployment_target=10.7'
    fi
    host=$($CXX -dumpmachine)
    case ${host%%-*} in
        i?86)   arch=ia32 ;;
        x86_64) arch=x64 ;;
        arm*)   arch=arm ;;
        *)      arch=native ;;
    esac
    mode=release
    pkg_make $arch.$mode CXX=$CXX LINK=$CXX LINK.target=$CXX GYPFLAGS='-Duse_system_icu=1 -Dwerror=' V=1
    for lib in `find "$build_dir/out/$arch.$mode" -maxdepth 1 -name \*.a` `find "$build_dir/out/$arch.$mode/obj.target" -name \*.a`; do
        name=`basename $lib`
        cp $lib "$install_dir/lib/${name/.$arch/}"
    done
    touch "$install_dir/lib/libv8.a" # Create a dummy libv8.a because the makefile looks for it
}

pkg_link-flags () {
    # These are the necessary libraries recommended by the docs:
    # https://developers.google.com/v8/get_started#hello
    # ICU is linked separately
    for lib in libv8_{base,libbase,snapshot,libplatform}; do
        echo "$install_dir/lib/$lib.a"
    done
}

pkg_depends () {
    if will_fetch icu; then
        echo icu
    fi
}

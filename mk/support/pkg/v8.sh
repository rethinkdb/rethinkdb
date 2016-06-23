
version=3.30.33.16
# See http://omahaproxy.appspot.com/ for the current stable/beta/dev versions of v8

src_url=http://commondatastorage.googleapis.com/chromium-browser-official/v8-$version.tar.bz2
src_url_sha1=e753b6671eecf565d96c1e5a83563535ee2fe24b

pkg_install-include () {
    pkg_copy_src_to_build

    rm -rf "$install_dir/include"
    mkdir -p "$install_dir/include"
    cp -RL "$src_dir/include/." "$install_dir/include"
    sed -i.bak 's/include\///' "$install_dir/include/libplatform/libplatform.h"

    # -- assemble the icu headers
    if [[ "$CROSS_COMPILING" = 1 ]]; then
        ( cross_build_env; in_dir "$build_dir/third_party/icu" ./configure --prefix="$(niceabspath "$install_dir")" --enable-static "$@" )
    else
        in_dir "$build_dir/third_party/icu/source" ./configure --prefix="$(niceabspath "$install_dir")" --enable-static --disable-layout "$@"
    fi

    in_dir "$build_dir/third_party/icu/source" make install-headers-recursive
}

pkg_install () {
    pkg_copy_src_to_build

    mkdir -p "$install_dir/lib"
    if [[ "$OS" = Darwin ]]; then
        export CXXFLAGS="-stdlib=libc++ ${CXXFLAGS:-}"
        export LDFLAGS="-stdlib=libc++ -lc++ ${LDFLAGS:-}"
        export GYP_DEFINES='clang=1 mac_deployment_target=10.7'
    fi
    arch_gypflags=
    raspberry_pi_gypflags='-Darm_version=6 -Darm_fpu=vfpv2'
    host=$($CXX -dumpmachine)
    case ${host%%-*} in
        i?86)   arch=ia32 ;;
        x86_64) arch=x64 ;;
        arm*)   arch=arm; arch_gypflags=$raspberry_pi_gypflags ;;
        *)      arch=native ;;
    esac
    mode=release
    pkg_make $arch.$mode CXX=$CXX LINK=$CXX LINK.target=$CXX GYPFLAGS="-Dwerror= $arch_gypflags" V=1
    for lib in `find "$build_dir/out/$arch.$mode" -maxdepth 1 -name \*.a` `find "$build_dir/out/$arch.$mode/obj.target" -name \*.a`; do
        name=`basename $lib`
        cp $lib "$install_dir/lib/${name/.$arch/}"
    done
    touch "$install_dir/lib/libv8.a" # Create a dummy libv8.a because the makefile looks for it
}

pkg_link-flags () {
    # These are the necessary libraries recommended by the docs:
    # https://developers.google.com/v8/get_started#hello
    for lib in libv8_{base,libbase,snapshot,libplatform}; do
        echo "$install_dir/lib/$lib.a"
    done
    for lib in libicu{i18n,uc,data}; do
        echo "$install_dir/lib/$lib.a"
    done
}

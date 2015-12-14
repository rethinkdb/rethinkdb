
version=4.7.80.23
# See http://omahaproxy.appspot.com/ for the current stable/beta/dev versions of v8

pkg_fetch () {
    pkg_make_tmp_fetch_dir

    # These steps are inspired by the official docs:
    # https://github.com/v8/v8/wiki/Using%20Git
    # with a few additional steps to get an exact version instead of HEAD

    git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git "$tmp_dir/depot_tools"
    PATH="$tmp_dir/depot_tools:$PATH"

    in_dir "$tmp_dir" gclient config --unmanaged https://chromium.googlesource.com/v8/v8.git
    mkdir "$tmp_dir/v8"
    in_dir "$tmp_dir" git clone https://chromium.googlesource.com/v8/v8.git
    v8_commit=`in_dir "$tmp_dir/v8" git rev-parse refs/tags/$version`
    in_dir "$tmp_dir" gclient sync --revision v8@$v8_commit

    find "$tmp_dir" -depth -name .git -exec rm -rf {} \;
    rm -rf "$src_dir"
    mv "$tmp_dir/v8" "$src_dir"
    rm -rf "$tmp_dir"
}

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
    # The install-headers-recursive target is missing. Let's patch it.
    sed -i.bak $'s/distclean-recursive/install-headers-recursive/g;$a\\\ninstall-headers-local:' "$build_dir/third_party/icu/source/Makefile"
    for file in "$build_dir"/third_party/icu/source/*/Makefile; do
        sed -i.bak $'$a\\\ninstall-headers:' "$file"
    done
    in_dir "$build_dir/third_party/icu/source" make install-headers-recursive
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
    pkg_make $arch.$mode CXX=$CXX LINK=$CXX LINK.target=$CXX GYPFLAGS="-Dwerror= $arch_gypflags -Dv8_use_external_startup_data=0" V=1
    for lib in `find "$build_dir/out/$arch.$mode" -maxdepth 1 -name \*.a` `find "$build_dir/out/$arch.$mode/obj.target" -name \*.a`; do
        name=`basename $lib`
        cp $lib "$install_dir/lib/${name/.$arch/}"
    done
    touch "$install_dir/lib/libv8.a" # Create a dummy libv8.a because the makefile looks for it
}

pkg_link-flags () {
    # These are the necessary libraries recommended by the docs:
    # https://developers.google.com/v8/get_started#hello
    for lib in libv8_{base,snapshot,libplatform,libbase}; do
        echo "$install_dir/lib/$lib.a"
    done
    for lib in libicu{i18n,uc,data}; do
        echo "$install_dir/lib/$lib.a"
    done
}

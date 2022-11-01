
version=2015-11-01

src_url=https://github.com/google/re2/archive/$version.tar.gz
src_url_sha256=4aeca04541b63af1e38682b20440fcde2207a4a8a04466359dd113c8a7d4657e

pkg_install-include () {
    mkdir -p "$install_dir/include/re2"
    cp "$src_dir/re2"/*.h "$install_dir/include/re2"
}

pkg_install () {
    pkg_copy_src_to_build
    pkg_make install prefix="$install_dir" CXXFLAGS="${CXXFLAGS:-} -O3" LDFLAGS="${LDFLAGS:-} $PTHREAD_LIBS"
}

pkg_install-windows () {
    pkg_copy_src_to_build

    in_dir "$build_dir" "$MSBUILD" /nologo /maxcpucount /p:Configuration=$CONFIGURATION /p:Platform=$PLATFORM /p:PlatformToolset=v141 /p:WindowsTargetPlatformVersion=10.0.19041.0 re2.vcxproj 

    cp "$build_dir/$PLATFORM/$CONFIGURATION/re2.lib" "$windows_deps_libs/"
}

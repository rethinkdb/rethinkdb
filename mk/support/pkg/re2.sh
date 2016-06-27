
version=2015-11-01

src_url=https://github.com/google/re2/archive/$version.tar.gz
src_url_sha1=52b24d0994e08004a2c81acb2fb4f255fe1a7378

pkg_install-include () {
    mkdir -p "$install_dir/include/re2"
    cp "$src_dir/re2"/*.h "$install_dir/include/re2"
}

pkg_install () {
    pkg_copy_src_to_build
    if [[ "$OS" = Darwin ]]; then
        CXXFLAGS="${CXXFLAGS:-} -stdlib=libc++"
        LDFLAGS="${LDFLAGS:-} -lc++"
    fi
    pkg_make install prefix="$install_dir" CXXFLAGS="${CXXFLAGS:-} -O3" LDFLAGS="${LDFLAGS:-} $PTHREAD_LIBS"
}

pkg_install-windows () {
    pkg_copy_src_to_build

    in_dir "$build_dir" "$MSBUILD" /nologo /maxcpucount /p:Configuration=$CONFIGURATION /p:Platform=$PLATFORM re2.vcxproj

    cp "$build_dir/$VS_OUTPUT_DIR/re2.lib" "$windows_deps_libs/"
}

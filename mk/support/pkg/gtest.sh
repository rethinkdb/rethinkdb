version=1.7.0
src_url=https://googletest.googlecode.com/files/gtest-$version.zip
src_url_sha1=f85f6d2481e2c6c4a18539e391aa4ea8ab0394af

pkg_install () {
    pkg_copy_src_to_build
    make -C "$build_dir/make" gtest.a
    mkdir -p "$install_dir/lib"
    cp "$build_dir/make/gtest.a" "$install_dir/lib/libgtest.a"
}

pkg_install-windows () {
    pkg_copy_src_to_build

    local generator
    case "$PLATFORM" in
        x64) generator="Visual Studio 14 Win64" ;;
        Win32) generator="Visual Studio 14 2015" ;;
    esac

    rm -rf "$build_dir"/{CMakeCache.txt,CMakeFiles}
    in_dir "$build_dir" "$CMAKE" -G"$generator"
    in_dir "$build_dir" "$MSBUILD" /nologo /maxcpucount /p:Configuration=$CONFIGURATION /p:Platform=$PLATFORM gtest.vcxproj

    cp "$build_dir/$CONFIGURATION/gtest.lib" "$windows_deps_libs/"
}

version=1.1.0
src_url=https://github.com/google/benchmark/archive/v$version.tar.gz
src_url_sha1=8c539bbe2a212618fa87b6c38fba087100b6e4ae

pkg_install () {
    pkg_copy_src_to_build
    make -C "$build_dir/make" benchmark.a
    mkdir -p "$install_dir/lib"
    cp "$build_dir/make/benchmark.a" "$install_dir/lib/benchmark.a"
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
    in_dir "$build_dir" "$MSBUILD" /nologo /maxcpucount /p:Configuration=$CONFIGURATION /p:Platform=$PLATFORM benchmark.vcxproj

    cp "$build_dir/$CONFIGURATION/benchmark.lib" "$windows_deps_libs/"
}

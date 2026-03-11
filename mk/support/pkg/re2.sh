
version=2021-11-01

src_url=https://github.com/google/re2/archive/$version.tar.gz
src_url_sha256=8c45f7fba029ab41f2a7e6545058d9eec94eef97ce70df58e92d85cfc08b4669

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

    # Allow toolset override via VCTOOLS_VERSION environment variable
    local toolset="${VCTOOLS_VERSION:-v141}"
    local windows_sdk="${WINDOWSSDKVERSION:-10.0.19041.0}"
    
    in_dir "$build_dir" "$MSBUILD" /nologo /maxcpucount /p:Configuration=$CONFIGURATION /p:Platform=$PLATFORM /p:PlatformToolset=$toolset /p:WindowsTargetPlatformVersion=$windows_sdk re2.vcxproj 

    cp "$build_dir/$PLATFORM/$CONFIGURATION/re2.lib" "$windows_deps_libs/"
}


version=1.60.0

src_url=http://sourceforge.net/projects/boost/files/boost/${version}/boost_${version//./_}.tar.bz2
src_url_sha1=7f56ab507d3258610391b47fef6b11635861175a

pkg_install-include () {
    mkdir -p "$install_dir/include/boost"
    cp -a "$src_dir/boost/." "$install_dir/include/boost"
}

pkg_install () {
    pkg_copy_src_to_build
    in_dir "$build_dir" ./bootstrap.sh --with-libraries=system
    in_dir "$build_dir" ./b2
    mkdir -p "$install_dir/lib"
    in_dir "$build_dir" cp "stage/lib/libboost_system.a" "$install_dir/lib"
}

pkg_install-windows () {
    : # Include files only
}

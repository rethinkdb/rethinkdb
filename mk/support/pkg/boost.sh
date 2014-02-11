
version=1.55.0

src_url=http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_${version//./_}.tar.bz2

pkg_install-include () {
    mkdir -p "$install_dir/include/boost"
    cp -a "$src_dir/boost/." "$install_dir/include/boost"
}

pkg_install () {
    # RethinkDB only requires the boost include files
    true
}

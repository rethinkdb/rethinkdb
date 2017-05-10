version=6.10.0

src_url=http://nodejs.org/dist/v$version/node-v$version.tar.gz
src_url_sha1=3bb2629ed623f38b8c3011cf422333862d3653a3

pkg_install () {
    pkg_copy_src_to_build
    pkg_configure
    pkg_make install

    mv "$install_dir"/bin/npm "$install_dir"/bin/npm.orig
    { echo "#!/bin/sh"
      echo "exec $install_dir/bin/node $install_dir/bin/npm.orig "'"$@"'
    } > "$install_dir"/bin/npm
    chmod +x "$install_dir"/bin/npm
}

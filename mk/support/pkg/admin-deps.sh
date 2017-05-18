full_npm_package=rethinkdb-webui
version=2.5.0

npm_conf=$(niceabspath "$conf_dir/npm.conf")

include npm-pkg.inc

pkg_shrinkwrap () {
    in_dir "$root_dir/admin" npm shrinkwrap
}

pkg_fetch () {
    pkg_make_tmp_fetch_dir
    if [[ "$OS" != Windows ]]; then
        # With this file, `npm install' fails on Windows for me with a lot of ECONNRESET errors
        # Not using the shrinkwrap file may break things
        cp "$root_dir/admin/npm-shrinkwrap.json" "$tmp_dir"
    fi
    cp "$root_dir/admin/package.json" "$tmp_dir"
    mkdir "$tmp_dir/node_modules"
    in_dir "$tmp_dir" npm install -dd
    sed -i.bak '$d' "$tmp_dir/package.json" # remove the last '}'
    {   echo '  ,"bundleDependencies": ['
        comma=
        for d in $(cd "$tmp_dir/node_modules"; ls -d *); do
            echo -n "$comma    \"$d\""
            comma=$',\n'
        done
        echo $'\n  ]'
        echo '}'
    } >> "$tmp_dir/package.json"
    rm -rf "$src_dir"
    mkdir -p "$src_dir"
    mv "$tmp_dir/"{node_modules,*.json} "$src_dir"
}

#!/usr/bin/env bash

# Fetch and build an npm package

# Expects npm_package to be set

npm_conf=$(niceabspath "$conf_dir/npm.conf")

npm () {
    "$NPM" --userconfig "$npm_conf" "$@"
}

package_json=$(cat <<EOF
{ "name": "packed-$npm_package",
  "version": "$version",
  "dependencies": { "$npm_package": "$version" },
  "bundleDependencies": [ "$npm_package" ] }
EOF
)

pkg_fetch () {
    pkg_make_tmp_fetch_dir
    cp "$pkg_dir/$npm_package.shrinkwrap" "$tmp_dir/npm-shrinkwrap.json"
    echo "$package_json" > "$tmp_dir/package.json"
    mkdir "$tmp_dir/node_modules"
    in_dir "$tmp_dir" npm --cache "$tmp_dir/npm-cache" install
    in_dir "$tmp_dir" npm pack
    in_dir "$tmp_dir" rm -rf npm-shrinkwrap.json package.json node_modules npm-cache
    pkg_move_tmp_to_src
}

pkg_shrinkwrap () {
    pkg_make_tmp_fetch_dir
    echo "$package_json" > "$tmp_dir/package.json"
    mkdir "$tmp_dir/node_modules"
    in_dir "$tmp_dir" npm --cache "$tmp_dir/npm-cache" install
    in_dir "$tmp_dir" npm shrinkwrap
    cp "$tmp_dir/npm-shrinkwrap.json" "$pkg_dir/$npm_package.shrinkwrap"
    pkg_remove_tmp_fetch_dir
}

pkg_install () {
    rm -rf "$install_dir" || : 2>/dev/null
    mkdir -p "$install_dir/node_modules"
    local tgz
    tgz=$(niceabspath "$src_dir/packed-$npm_package-$version.tgz")
    in_dir "$install_dir" npm --no-registry install "$tgz"

    local bin_dir="$install_dir/node_modules/packed-$npm_package/node_modules/.bin/"
    mkdir "$install_dir/bin"
    for bin in "$bin_dir"*; do
        local name
        name=$(basename "$bin")
        local script="$install_dir/bin/$name"
        echo '#!/bin/sh' > "$script"
        echo "exec $(niceabspath "$bin")"' "$@"' >> "$script"
        chmod +x "$script"
    done
}

pkg_depends () {
    if will_fetch node; then
        echo node
    fi
}

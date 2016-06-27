#!/usr/bin/env bash

# Fetch and build an npm package

# Expects npm_package to be set

npm_conf=$(niceabspath "$conf_dir/npm.conf")

npm () {
    cache_dir=$(mktemp -d "$external_dir/npm_cache-XXXXXXXX")
    if [[ "$OS" = Windows ]]; then
        # Simplify the call to npm, because we are using the Windows npm from inside cygwin
        nocygpath "$NPM" "$@"
    else
        "$NPM" --userconfig "$npm_conf" --cache "$cache_dir" "$@"
    fi
    rm -rf "$cache_dir"
}

package_json () {
    cat <<EOF
{ "name": "packed-$npm_package",
  "version": "$version",
  "dependencies": { "$npm_package": "$version" },
  "bundleDependencies": [ "$npm_package" ] }
EOF
}

pkg_fetch () {
    pkg_make_tmp_fetch_dir
    cp "$pkg_dir/$npm_package.shrinkwrap" "$tmp_dir/npm-shrinkwrap.json"
    package_json > "$tmp_dir/package.json"
    mkdir "$tmp_dir/node_modules"
    in_dir "$tmp_dir" npm install
    in_dir "$tmp_dir" npm pack
    in_dir "$tmp_dir" tar xf packed-$npm_package-$version.tgz
    mv "$tmp_dir/package" "$src_dir"
    pkg_remove_tmp_fetch_dir
}

pkg_shrinkwrap () {
    pkg_make_tmp_fetch_dir
    package_json > "$tmp_dir/package.json"
    mkdir "$tmp_dir/node_modules"
    in_dir "$tmp_dir" npm install
    in_dir "$tmp_dir" npm shrinkwrap
    cp "$tmp_dir/npm-shrinkwrap.json" "$pkg_dir/$npm_package.shrinkwrap"
    pkg_remove_tmp_fetch_dir
}

pkg_install () {
    full_npm_package=${full_npm_package:-packed-$npm_package}
    rm -rf "$install_dir" || : 2>/dev/null
    mkdir -p "$install_dir/node_modules"
    local pkg
    pkg=$(niceabspath "$src_dir")
    if [[ "$OS" = Windows ]]; then
        in_dir "$install_dir" npm install "$(cpdir "$pkg")"
    else
        in_dir "$install_dir" npm install "$pkg"
    fi

    local bin_dir="$install_dir/node_modules/$full_npm_package/node_modules/.bin/"
    mkdir -p "$install_dir/bin"
    for bin in "$bin_dir"*; do
        local name
        name=$(basename "$bin")
        local script="$install_dir/bin/$name"
        echo '#!/bin/sh' > "$script"
        echo "export PATH='$(dirname "$NPM")':"'$PATH' >> "$script"
        echo "export NODE_PATH='$(cpdir "$install_dir/node_modules/$full_npm_package/node_modules")'" >> "$script"
        echo "exec $(niceabspath "$bin")"' "$@"' >> "$script"
        chmod +x "$script"
    done
}

pkg_install-windows () {
    pkg_install "$@"
}

pkg_depends () {
    if will_fetch node; then
        echo node
    fi
}

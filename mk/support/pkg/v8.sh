#!/usr/bin/env bash

# V8 JavaScript engine package for RethinkDB
# This script builds V8 in jitless mode for use as the default JS engine

version=11.9.169.7  # Stable V8 version with good jitless support

# Use depot_tools to fetch V8 (Google's build system)
depot_tools_url="https://chromium.googlesource.com/chromium/tools/depot_tools.git"

pkg_install-include () {
    mkdir -p "$install_dir/include"
    if [ -d "$src_dir/include" ]; then
        cp -r "$src_dir/include"/* "$install_dir/include/"
    fi
}

pkg_fetch () {
    # Fetch depot_tools
    local depot_tools_dir="$build_dir/depot_tools"
    if [ ! -d "$depot_tools_dir" ]; then
        git clone --depth 1 "$depot_tools_url" "$depot_tools_dir"
    fi
    
    export PATH="$depot_tools_dir:$PATH"
    
    # Fetch V8 source
    mkdir -p "$build_dir/v8"
    cd "$build_dir/v8"
    
    if [ ! -d "v8" ]; then
        fetch v8
    fi
    
    cd v8
    git checkout "$version"
    
    # Sync dependencies
    gclient sync -D
}

pkg_build () {
    local depot_tools_dir="$build_dir/depot_tools"
    cd "$build_dir/v8/v8"
    export PATH="$depot_tools_dir:$PATH"
    
    # Generate build configuration
    local gn_args=(
        "is_debug=false"
        "is_official_build=true"
        "v8_static_library=true"
        "use_custom_libcxx=false"
        "treat_warnings_as_errors=false"
        "v8_enable_i18n_support=false"
        "v8_use_external_startup_data=false"
        "v8_enable_pointer_compression=false"
    )
    
    # Enable jitless mode by default for security
    if [ "${V8_JITLESS:-1}" = "1" ]; then
        gn_args+=("v8_jitless=true")
    fi
    
    # Platform-specific settings
    case $(uname -m) in
        x86_64)
            gn_args+=("target_cpu=\"x64\"")
            ;;
        aarch64|arm64)
            gn_args+=("target_cpu=\"arm64\"")
            ;;
        arm*)
            gn_args+=("target_cpu=\"arm\"")
            ;;
    esac
    
    # Generate build files
    gn gen out.gn/release --args="${gn_args[*]}"
    
    # Build V8
    ninja -C out.gn/release v8_monolith
}

pkg_install () {
    pkg_build
    
    local include_dir="$install_dir/include"
    local lib_dir="$install_dir/lib"
    
    mkdir -p "$include_dir" "$lib_dir"
    
    # Copy built files
    cd "$build_dir/v8/v8"
    
    # Copy headers
    cp -r include/* "$include_dir/"
    
    # Copy static library
    cp out.gn/release/obj/libv8_monolith.a "$lib_dir/"
    
    # Copy additional required libraries
    if [ -f out.gn/release/obj/third_party/icu/libicuuc.a ]; then
        cp out.gn/release/obj/third_party/icu/libicuuc.a "$lib_dir/"
    fi
}

pkg_link-flags () {
    local flags="$install_dir/lib/libv8_monolith.a"
    if [ -f "$install_dir/lib/libicuuc.a" ]; then
        flags="$flags -licuuc"
    fi
    # V8 needs these system libraries
    flags="$flags -lpthread -ldl"
    echo "$flags"
}

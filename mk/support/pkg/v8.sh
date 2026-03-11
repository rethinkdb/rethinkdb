#!/usr/bin/env bash

# V8 JavaScript engine package for RethinkDB
# This script builds V8 in jitless mode for use as the default JS engine

version=11.9.169.7  # Stable V8 version with good jitless support

# Use depot_tools to fetch V8 (Google's build system)
depot_tools_url="https://chromium.googlesource.com/chromium/tools/depot_tools.git"
depot_tools_dir="$build_dir/depot_tools"

# Alternative: Use pre-built V8 static libraries from GitHub releases
# For production builds, fetching and building V8 is time-consuming
# This script supports both approaches

fetch_v8_source () {
    # Check if we should use pre-built libraries
    if [ "${V8_USE_PREBUILT:-0}" = "1" ]; then
        fetch_prebuilt_v8
        return
    fi
    
    # Fetch depot_tools
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

fetch_prebuilt_v8 () {
    # For CI/CD and faster builds, use pre-built V8 libraries
    # These are platform-specific
    local platform=$(uname -m)
    local os=$(uname -s | tr '[:upper:]' '[:lower:]')
    
    local prebuilt_url="https://github.com/rethinkdb/v8-prebuilt/releases/download/v${version}/v8-${os}-${platform}.tar.gz"
    
    curl -L "$prebuilt_url" | tar -xzf - -C "$build_dir"
}

build_v8 () {
    if [ "${V8_USE_PREBUILT:-0}" = "1" ]; then
        # Pre-built libraries already extracted
        return
    fi
    
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

install_v8 () {
    local include_dir="$install_dir/include"
    local lib_dir="$install_dir/lib"
    
    mkdir -p "$include_dir" "$lib_dir"
    
    if [ "${V8_USE_PREBUILT:-0}" = "1" ]; then
        # Copy pre-built files
        cp -r "$build_dir/include"/* "$include_dir/"
        cp -r "$build_dir/lib"/* "$lib_dir/"
    else
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
    fi
}

# Main package interface
case "${1:-}" in
    fetch)
        fetch_v8_source
        ;;
    build)
        build_v8
        ;;
    install)
        install_v8
        ;;
    *)
        echo "Usage: $0 {fetch|build|install}"
        exit 1
        ;;
esac

#!/bin/bash

# Call this from the parent directory with ./admin/generate.sh
if [ ! -f admin/generate.sh ]; then
    echo "Error: Call me from the rethinkdb project root directory"
    exit 1
fi

mkdir -p build
make -j7 build/web-assets
./scripts/compile-web-assets.py build/web_assets > src/gen/web_assets.cc

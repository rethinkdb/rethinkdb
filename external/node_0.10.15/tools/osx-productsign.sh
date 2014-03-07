#!/bin/bash

set -x
set -e

if ! [ -n "$SIGN" ]; then
  echo "No SIGN environment var.  Skipping codesign." >&2
  exit 0
fi

productsign --sign "$SIGN" "$PKG" "$PKG"-SIGNED
mv "$PKG"-SIGNED "$PKG"

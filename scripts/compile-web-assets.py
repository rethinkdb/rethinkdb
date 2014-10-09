#!/usr/bin/env python

from __future__ import print_function

import os
import sys

def main():
    try:
        assets = sys.argv[1]
        assert os.path.isdir(assets)
    except:
        print("First argument must be a directory", file=sys.stderr)

    files = [
        os.path.join(root, path)
        for path in paths
        for root, __, paths in os.walk(assets)
    ]

    

    if os.isatty(sys.stdout):
        print("Cutting output short because stdout is a terminal. Please redirect to a file.", file=stderr)

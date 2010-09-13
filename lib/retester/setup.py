#!/usr/bin/env python

from distutils.core import setup

setup(
    name = "retester",
    version = "1.0",
    description = "Automated test driver",
    py_modules=["retester"],
    scripts = ["git-test"]
    )

# Copyright 2009 Vladimir Prus 
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import imp
import sys

def bootstrap(root_path):
    """Performs python-side bootstrapping of Boost.Build/Python.

    This function arranges for 'b2.whatever' package names to work, while also
    allowing to put python files alongside corresponding jam modules.
    """

    m = imp.new_module("b2")
    # Note that:
    # 1. If __path__ is not list of strings, nothing will work
    # 2. root_path is already list of strings.
    m.__path__ = root_path
    sys.modules["b2"] = m

    import b2.build_system
    return b2.build_system.main()


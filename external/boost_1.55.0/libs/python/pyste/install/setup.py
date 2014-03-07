# Copyright Prabhu Ramachandran 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

from distutils.core import setup
import sys

setup (name              = "Pyste",
       version           = "0.9.10",
       description       = "Pyste - Python Semi-Automatic Exporter",
       maintainer        = "Bruno da Silva de Oliveira",
       maintainer_email  = "nicodemus@globalite.com.br",
       licence           = "Boost License",
       long_description  = "Pyste is a Boost.Python code generator",
       url               = "http://www.boost.org/libs/python/pyste/index.html",
       platforms         = ['Any'],
       packages          = ['Pyste'],
       scripts           = ['pyste.py'],
       package_dir       = {'Pyste': '../src/Pyste'},
       )

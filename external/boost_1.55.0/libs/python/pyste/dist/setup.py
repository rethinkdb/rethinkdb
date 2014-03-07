# Copyright Bruno da Silva de Oliveira 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

from distutils.core import setup
import py2exe
import sys

sys.path.append('../src')
setup(name='pyste', scripts=['../src/pyste.py'])

# Copyright 2010-2013 RethinkDB, all rights reserved.

from setuptools import setup, Extension
from distutils.command.build_ext import build_ext
import os
from subprocess import check_call

class build_ext_genproto(build_ext):
    # This class replaces the build_ext command with one that
    # first generates the ql2.pb.{cpp,h} files if the correct
    # environment variable is set.

    def build_extension(self, ext):
        if os.environ.get('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION') == 'cpp':
            print "Calling protoc to generate ql2.pb.cc and ql2.pb.h"
            check_call(['protoc', 'ql2.proto', '--cpp_out=.'])
            ext.sources = ['./ql2.pb.cc'] + ext.sources
            build_ext.build_extension(self, ext)
        else:
            print "* * * * * * * * * * * * * * * * *"
            print "* WARNING: The faster C++ protobuf backend is not enabled."
            print "* WARNING: To enable it, run `export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp' and reinstall the rethinkdb package."
            print "* WARNING: See http://rethinkdb.com/docs/driver-performance/ for more information."
            print "* * * * * * * * * * * * * * * * *"

setup(name="rethinkdb"
      ,version = "1.10.0-999"
      ,description = "This package provides the Python driver library for the RethinkDB database server."
      ,url = "http://rethinkdb.com"
      ,maintainer = "RethinkDB Inc."
      ,maintainer_email = "bugs@rethinkdb.com"
      ,packages = ['rethinkdb']
      ,install_requires = ['protobuf']
      ,entry_points = {'console_scripts': [
          'rethinkdb-import = rethinkdb._import:main',
          'rethinkdb-dump = rethinkdb._dump:main',
          'rethinkdb-export = rethinkdb._export:main',
          'rethinkdb-restore = rethinkdb._restore:main']}
      ,cmdclass = {"build_ext":build_ext_genproto}
      ,ext_modules = [Extension(
          'rethinkdb/_pbcpp',
          sources=['./rethinkdb/_pbcpp.cpp'],
          include_dirs=['./'],
          libraries=['protobuf'])])

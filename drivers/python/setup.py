# Copyright 2010-2012 RethinkDB, all rights reserved.

from setuptools import setup, Extension
from distutils.command.build_ext import build_ext
from distutils.errors import DistutilsPlatformError, CCompilerError, DistutilsExecError
import sys

class build_ext_nofail(build_ext):
    # This class can replace the build_ext command with one that does not fail
    # when the extension fails to build.

    def run(self):
        try:
            build_ext.run(self)
        except DistutilsPlatformError as e:
            self._failed(e)

    def build_extension(self, ext):
        try:
            build_ext.build_extension(self, ext)
        except (CCompilerError, DistutilsExecError) as e:
            self._failed(e)
        else:
            try:
                import google.protobuf.internal.cpp_message
            except ImportError:
                print("*** WARNING: The installed protobuf library does not seem to include the C++ extension", file=sys.stderr)
                print("*** WARNING: The RethinkDB driver will fallback to using the pure python implementation", file=sys.stderr) 

    def _failed(self, e):
            print("*** WARNING: Unable to compile the C++ extension", file=sys.stderr)
            print(e, file=sys.stderr)
            print("*** WARNING: Defaulting to the python implementation", file=sys.stderr)

setup(name="rethinkdb"
     ,version="1.8.0-0"
     ,description="This package provides the Python driver library for the RethinkDB database server."
     ,url="http://rethinkdb.com"
     ,maintainer="RethinkDB Inc."
     ,maintainer_email="bugs@rethinkdb.com"
     ,packages=['rethinkdb']
     ,install_requires=['protobuf']
     ,ext_modules=[Extension('rethinkdb_pbcpp', sources=['./rethinkdb/pbcpp.cpp', './rethinkdb/ql2.pb.cc'],
                             include_dirs=['./rethinkdb'], libraries=['protobuf'])]
     ,cmdclass={"build_ext":build_ext_nofail}
)

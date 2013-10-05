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
                sys.stderr.write("*** WARNING: The installed protobuf library does not seem to include the C++ extension\n")
                sys.stderr.write("*** WARNING: The RethinkDB driver will fallback to using the pure python implementation\n")

    def _failed(self, e):
            sys.stderr.write("*** WARNING: Unable to compile the C++ extension\n")
            sys.stderr.write(str(e) + "\n")
            sys.stderr.write("*** WARNING: Defaulting to the python implementation\n")

setup(name="rethinkdb"
     ,version="1.10.0-0"
     ,description="This package provides the Python driver library for the RethinkDB database server."
     ,url="http://rethinkdb.com"
     ,maintainer="RethinkDB Inc."
     ,maintainer_email="bugs@rethinkdb.com"
     ,packages=['rethinkdb']
     ,install_requires=['protobuf']
     ,ext_modules=[Extension('rethinkdb_pbcpp', sources=['./rethinkdb/pbcpp.cpp', './rethinkdb/ql2.pb.cc'],
                             include_dirs=['./rethinkdb'], libraries=['protobuf'])]
     ,cmdclass={"build_ext":build_ext_nofail}
     ,entry_points={'console_scripts': [
            'rethinkdb-import = rethinkdb._import:main',
            'rethinkdb-dump = rethinkdb._dump:main',
            'rethinkdb-export = rethinkdb._export:main',
            'rethinkdb-restore = rethinkdb._restore:main']}
)

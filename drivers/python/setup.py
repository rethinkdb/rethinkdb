# Copyright 2010-2012 RethinkDB, all rights reserved.
from setuptools import setup, Extension

from os import environ
environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'

setup(name="rethinkdb"
     ,version="1.6.0-0"
     ,description="This package provides the Python driver library for the RethinkDB database server."
     ,url="http://rethinkdb.com"
     ,maintainer="RethinkDB Inc."
     ,maintainer_email="bugs@rethinkdb.com"
     ,packages=['rethinkdb']
     ,ext_modules=[Extension('pbcpp', sources=['./rethinkdb/pbcpp.cpp', './rethinkdb/ql2.pb.cc'], include_dirs=['./rethinkdb'], libraries=['protobuf'])]
)

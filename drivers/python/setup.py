# Copyright 2010-2012 RethinkDB, all rights reserved.
from setuptools import setup

setup(name="rethinkdb"
     ,version="1.4.0-2"
     ,description="This package provides the Python driver library for the RethinkDB database server."
     ,url="http://rethinkdb.com"
     ,maintainer="RethinkDB Inc."
     ,maintainer_email="bugs@rethinkdb.com"
     ,packages=['rethinkdb']
     ,install_requires=['protobuf']
)

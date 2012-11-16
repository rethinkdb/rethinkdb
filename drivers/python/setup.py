# Copyright 2010-2012 RethinkDB, all rights reserved.
from distutils.core import setup

setup(name="rethinkdb"
     ,version="1.2.0-0"
     ,description="This package provides the Python driver library for the RethinkDB database server."
     ,url="http://rethinkdb.com"
     ,maintainer="RethinkDB Inc."
     ,maintainer_email="bugs@rethinkdb.com"
     ,packages=['rethinkdb']
     ,requires=['protobuf']
     )

#!/bin/bash
# Copyright 2010-2012 RethinkDB, all rights reserved.
pushd rethinkdb
rdoc1.9.1 "$@" net.rb query.rb rql.rb \
    sequence.rb writes.rb streams.rb jsons.rb \
    base_classes.rb tables.rb data_collectors.rb
mkdir -p /var/www/mlucy/ruby_docs
rm -rf /var/www/mlucy/ruby_docs/*
mv doc/* /var/www/mlucy/ruby_docs/
rmdir doc


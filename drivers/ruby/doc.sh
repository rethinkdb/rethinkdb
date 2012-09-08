#!/bin/bash
pushd rethinkdb
rdoc1.9.1 net.rb query.rb rql.rb \
    sequence.rb writes.rb streams.rb jsons.rb \
    base_classes.rb tables.rb
mkdir -p /var/www/mlucy/ruby_docs
rm -rf /var/www/mlucy/ruby_docs/*
mv doc/* /var/www/mlucy/ruby_docs/
rmdir doc


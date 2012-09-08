#!/bin/bash
rdoc1.9.1 net.rb query_user.rb rql_mixin.rb \
    reads.rb writes.rb streams.rb jsons.rb \
    rethinkdb_shortcuts.rb
mkdir -p /var/www/mlucy/ruby_docs
rm -rf /var/www/mlucy/ruby_docs/*
mv doc/* /var/www/mlucy/ruby_docs/
rmdir doc


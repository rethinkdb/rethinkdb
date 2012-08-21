#!/bin/bash
rdoc1.9.1 connection.rb query_user.rb rql_mixin.rb \
    reads.rb writes.rb streams.rb jsons.rb \
    rethinkdb_shortcuts.rb
rm -rf /var/www/code.rethinkdb.com/htdocs/ruby_docs/*
mv doc/* /var/www/code.rethinkdb.com/htdocs/ruby_docs
rmdir doc


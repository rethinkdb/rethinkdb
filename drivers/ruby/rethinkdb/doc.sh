#!/bin/bash
rdoc1.9.1 user_interface.rb rethinkdb_shortcuts.rb
rm -rf /var/www/code.rethinkdb.com/htdocs/ruby_docs/*
mv doc/* /var/www/code.rethinkdb.com/htdocs/ruby_docs
rmdir doc


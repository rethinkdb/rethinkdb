#!/bin/sh
# This fetches current versions of the included dependencies .
curl http://closure-compiler.googlecode.com/files/compiler-latest.tar.gz > google-closure-compiler-latest.tar.gz ;
curl https://nodeload.github.com/rethinkdb/google-closure-library/tarball/rethinkdb > google-closure-library-rethinkdb.tar.gz ;
curl https://nodeload.github.com/rethinkdb/protobuf-plugin-closure/tarball/rethinkdb > protobuf-plugin-closure-rethinkdb.tar.gz ;
mkdir google-closure-compiler ;
tar -xC google-closure-compiler -zf google-closure-compiler-latest.tar.gz ;
tar -xzf google-closure-library-rethinkdb.tar.gz ; mv rethinkdb-google-closure-library-[0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z] google-closure-library ;
tar -xzf protobuf-plugin-closure-rethinkdb.tar.gz ; mv rethinkdb-protobuf-plugin-closure-[0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z] protobuf-plugin-closure ;


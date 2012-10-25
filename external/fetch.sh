#!/bin/sh
# This fetches current versions of the included dependencies .
curl http://closure-compiler.googlecode.com/files/compiler-latest.tar.gz > google-closure-compiler-latest.tar.gz ;
curl https://nodeload.github.com/rethinkdb/google-closure-library/tarball/rethinkdb > google-closure-library-rethinkdb.tar.gz ;
curl https://nodeload.github.com/rethinkdb/protobuf-plugin-closure/tarball/rethinkdb > protobuf-plugin-closure-rethinkdb.tar.gz ;
if [ -e google-closure-compiler ] ; then echo "google-closure-compiler already exists. Run clean in order to remove it." ; else mkdir google-closure-compiler ; tar -xC google-closure-compiler -zf google-closure-compiler-latest.tar.gz ; fi ;
if [ -e google-closure-library ] ; then echo "google-closure-library already exists. Run clean in order to remove it." ; else tar -xzf google-closure-library-rethinkdb.tar.gz ; mv rethinkdb-google-closure-library-[0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z] google-closure-library ; fi ;
if [ -e protobuf-plugin-closure ] ; then echo "protobuf-plugin-closure already exists. Run clean in order to remove it." ; else tar -xzf protobuf-plugin-closure-rethinkdb.tar.gz ; mv rethinkdb-protobuf-plugin-closure-[0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z] protobuf-plugin-closure ; fi ;


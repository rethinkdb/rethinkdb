#!/usr/bin/env node
goog.provide('rethinkdb.RQLJS')

goog.require('rethinkdb.TCPServer')

port = 28015

# Start the server
new RDBTcpServer(port)
console.log "Listening for RDB client connections on #{port}"

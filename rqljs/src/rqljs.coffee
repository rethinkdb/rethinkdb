#!/usr/bin/env node
goog.provide('rethinkdb.RQLJS')

goog.require('rethinkdb.TCPServer')
goog.require('rethinkdb.HttpServer')

port = 28016

# Start the server
new RDBTcpServer(port)
new RDBHttpServer(port+1)
console.log "Listening for RDB client connections on #{port}"
console.log "Listening for RDB Http connections on #{port+1}"

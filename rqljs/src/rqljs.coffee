#!/usr/bin/env node
goog.provide('rethinkdb.RQLJS')

goog.require('rethinkdb.TCPServer')
goog.require('rethinkdb.HttpServer')

args = process.argv[2..]
portOffset = parseInt(args[0] || '0')

port = 28016+portOffset
httpPort = 8080+portOffset

# Start the server
new RDBTcpServer(port)
new RDBHttpServer(httpPort)
console.log "Listening for RDB client connections on #{port}"
console.log "Listening for RDB Http connections on #{httpPort}"

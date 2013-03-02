#!/usr/bin/env node
goog.provide('rethinkdb.RQLJS')

goog.require('rethinkdb.TCPServer')
goog.require('rethinkdb.HttpServer')

args = process.argv[2..]
port = parseInt(args[0] || '28016')
httpPort = parseInt(args[1] || '8080')

# Start the server
new RDBTcpServer(port)
new RDBHttpServer(httpPort)
console.log "Listening for RDB client connections on #{port}"
console.log "Listening for RDB Http connections on #{httpPort}"

# Top level namespace and base for dependency generator
goog.provide("rethinkdb.root")

# Require all other modules here to ensure they are included
goog.require("rethinkdb.base")
goog.require("rethinkdb.errors")
goog.require("rethinkdb.query")
goog.require("rethinkdb.net")
goog.require("rethinkdb.protobuf")

# Export RethinDB namespace to commonjs module
if typeof module isnt 'undefined' and module.exports
    module.exports = rethinkdb

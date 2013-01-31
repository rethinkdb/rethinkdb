# Module function shortcut for wrapping values
# must be defined before the 'rethinkdb' namespace
rethinkdb = (val) -> rethinkdb.expr(val)

goog.provide("rethinkdb.base")

print = (args...) -> console.log(args...)

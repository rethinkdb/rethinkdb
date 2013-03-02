goog.provide('rethinkdb.Utils')

# Class of static helper functions
class Utils
    @checkName: (name) ->
        if not /^[A-Za-z0-9_\-]+$/.test name
            throw new BadQuery "Invalid name '#{name}'.  (Use A-Za-z0-9_ only.)."

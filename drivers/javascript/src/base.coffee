# Module function shortcut for wrapping values
# must be defined before the 'rethinkdb' namespace
rethinkdb = (args...) -> rethinkdb.expr(args...)

goog.provide("rethinkdb.base")

print = (args...) -> console.log(args...)

# Function wrapper that enforces that the function is
# called with the correct number of arguments
ar = (name, fun) -> (args...) ->
    if args.length isnt fun.length
        throw new RqlDriverError "Expected #{fun.length} argument(s) but found #{args.length} in #{name}."
    fun.apply(@, args)

# Like ar but for functions that take an optional options dict as the last argument
aropt = (name, fun) -> (args...) ->
    expectedPosArgs = fun.length - 1
    perhapsOptDict = args[expectedPosArgs]
    
    if perhapsOptDict and ((not (perhapsOptDict instanceof Object)) or (perhapsOptDict instanceof TermBase))
        perhapsOptDict = null

    numPosArgs = args.length - (if perhapsOptDict? then 1 else 0)

    if expectedPosArgs isnt numPosArgs
        throw new RqlDriverError "Expected #{expectedPosArgs} argument(s) but found #{numPosArgs} in #{name}."
    fun.apply(@, args)

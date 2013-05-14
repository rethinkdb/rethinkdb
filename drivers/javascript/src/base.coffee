# Module function shortcut for wrapping values
# must be defined before the 'rethinkdb' namespace
rethinkdb = (args...) -> rethinkdb.expr(args...)

goog.provide("rethinkdb.base")

print = (args...) -> console.log(args...)

# Function wrapper that enforces that the function is
# called with the correct number of arguments
ar = (fun) -> (args...) ->
    if args.length isnt fun.length
        throw new RqlDriverError "Expected #{fun.length} argument(s) but found #{args.length}."
    fun.apply(@, args)

# Like ar for varaible argument functions. Takes minimum
# and maximum argument parameters.
varar = (min, max, fun) -> (args...) ->
    if (min? and args.length < min) or (max? and args.length > max)
        if min? and not max?
            throw new RqlDriverError "Expected #{min} or more argument(s) but found #{args.length}."
        if max? and not min?
            throw new RqlDriverError "Expected #{max} or fewer argument(s) but found #{args.length}."
        throw new RqlDriverError "Expected between #{min} and #{max} argument(s) but found #{args.length}."
    fun.apply(@, args)

# Like ar but for functions that take an optional options dict as the last argument
aropt = (fun) -> (args...) ->
    expectedPosArgs = fun.length - 1
    perhapsOptDict = args[expectedPosArgs]

    if perhapsOptDict and ((not (perhapsOptDict instanceof Object)) or (perhapsOptDict instanceof TermBase))
        perhapsOptDict = null

    numPosArgs = args.length - (if perhapsOptDict? then 1 else 0)

    if expectedPosArgs isnt numPosArgs
         throw new RqlDriverError "Expected #{expectedPosArgs} argument(s) but found #{numPosArgs}."
    fun.apply(@, args)

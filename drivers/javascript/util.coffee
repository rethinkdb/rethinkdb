err = require('./errors')

# Function wrapper that enforces that the function is
# called with the correct number of arguments
module.exports.ar = (fun) -> (args...) ->
    if args.length isnt fun.length
        throw new err.RqlDriverError "Expected #{fun.length} argument(s) but found #{args.length}."
    fun.apply(@, args)

# Like ar for variable argument functions. Takes minimum
# and maximum argument parameters.
module.exports.varar = (min, max, fun) -> (args...) ->
    if (min? and args.length < min) or (max? and args.length > max)
        if min? and not max?
            throw new err.RqlDriverError "Expected #{min} or more argument(s) but found #{args.length}."
        if max? and not min?
            throw new err.RqlDriverError "Expected #{max} or fewer argument(s) but found #{args.length}."
        throw new err.RqlDriverError "Expected between #{min} and #{max} argument(s) but found #{args.length}."
    fun.apply(@, args)

# Like ar but for functions that take an optional options dict as the last argument
module.exports.aropt = (fun) -> (args...) ->
    expectedPosArgs = fun.length - 1
    perhapsOptDict = args[expectedPosArgs]

    if perhapsOptDict? and (Object::toString.call(perhapsOptDict) isnt '[object Object]')
        perhapsOptDict = null

    numPosArgs = args.length - (if perhapsOptDict? then 1 else 0)

    if expectedPosArgs isnt numPosArgs
         throw new err.RqlDriverError "Expected #{expectedPosArgs} argument(s) but found #{numPosArgs}."
    fun.apply(@, args)

module.exports.toArrayBuffer = (node_buffer) ->
    # Convert from node buffer to array buffer
    arr = new Uint8Array new ArrayBuffer node_buffer.length
    for byte,i in node_buffer
        arr[i] = byte
    return arr.buffer

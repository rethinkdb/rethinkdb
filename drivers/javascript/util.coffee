err = require('./errors')

plural = (number) -> if number == 1 then "s" else ""

# Function wrapper that enforces that the function is
# called with the correct number of arguments
module.exports.ar = (fun) -> (args...) ->
    if args.length isnt fun.length
        throw new err.RqlDriverError "Expected #{fun.length} argument#{plural(fun.length)} but found #{args.length}."
    fun.apply(@, args)

# Like ar for variable argument functions. Takes minimum
# and maximum argument parameters.
module.exports.varar = (min, max, fun) -> (args...) ->
    if (min? and args.length < min) or (max? and args.length > max)
        if min? and not max?
            throw new err.RqlDriverError "Expected #{min} or more arguments but found #{args.length}."
        if max? and not min?
            throw new err.RqlDriverError "Expected #{max} or fewer arguments but found #{args.length}."
        throw new err.RqlDriverError "Expected between #{min} and #{max} arguments but found #{args.length}."
    fun.apply(@, args)

# Like ar but for functions that take an optional options dict as the last argument
module.exports.aropt = (fun) -> (args...) ->
    expectedPosArgs = fun.length - 1
    perhapsOptDict = args[expectedPosArgs]

    if perhapsOptDict? and (Object::toString.call(perhapsOptDict) isnt '[object Object]')
        perhapsOptDict = null

    numPosArgs = args.length - (if perhapsOptDict? then 1 else 0)

    if expectedPosArgs isnt numPosArgs
        if expectedPosArgs isnt 1
            throw new err.RqlDriverError "Expected #{expectedPosArgs} arguments (not including options) but found #{numPosArgs}."
        else
            throw new err.RqlDriverError "Expected #{expectedPosArgs} argument (not including options) but found #{numPosArgs}."
    fun.apply(@, args)

module.exports.toArrayBuffer = (node_buffer) ->
    # Convert from node buffer to array buffer
    arr = new Uint8Array new ArrayBuffer node_buffer.length
    for value,i in node_buffer
        arr[i] = value
    return arr.buffer

convertPseudotype = (obj, opts) ->
    # An R_OBJECT may be a regular object or a "pseudo-type" so we need a
    # second layer of type switching here on the obfuscated field "$reql_type$"
    switch obj['$reql_type$']
        when 'TIME'
            switch opts.timeFormat
                # Default is native
                when 'native', undefined
                    if not obj['epoch_time']?
                        throw new err.RqlDriverError "pseudo-type TIME #{obj} object missing expected field 'epoch_time'."

                    # We ignore the timezone field of the pseudo-type TIME object. JS dates do not support timezones.
                    # By converting to a native date object we are intentionally throwing out timezone information.

                    # field "epoch_time" is in seconds but the Date constructor expects milliseconds
                    (new Date(obj['epoch_time']*1000))
                when 'raw'
                    # Just return the raw (`{'$reql_type$'...}`) object
                    obj
                else
                    throw new err.RqlDriverError "Unknown timeFormat run option #{opts.timeFormat}."
        when 'GROUPED_DATA'
            switch opts.groupFormat
                when 'native', undefined
                    # Don't convert the data into a map, because the keys could be objects which doesn't work in JS
                    # Instead, we have the following format:
                    # [ { 'group': <group>, 'reduction': <value(s)> } }, ... ]
                    { group: i[0], reduction: i[1] } for i in obj['data']
                when 'raw'
                    obj
                else
                    throw new err.RqlDriverError "Unknown groupFormat run option #{opts.groupFormat}."
        else
            # Regular object or unknown pseudo type
            obj

recursivelyConvertPseudotype = (obj, opts) ->
    if obj instanceof Array
        for value, i in obj
            obj[i] = recursivelyConvertPseudotype(value, opts)
    else if obj instanceof Object
        for key, value of obj
            obj[key] = recursivelyConvertPseudotype(value, opts)
        obj = convertPseudotype(obj, opts)
    obj

mkAtom = (response, opts) -> recursivelyConvertPseudotype(response.r[0], opts)

mkSeq = (response, opts) -> recursivelyConvertPseudotype(response.r, opts)

mkErr = (ErrClass, response, root) ->
    new ErrClass(mkAtom(response), root, response.b)

module.exports.recursivelyConvertPseudotype = recursivelyConvertPseudotype
module.exports.mkAtom = mkAtom
module.exports.mkSeq = mkSeq
module.exports.mkErr = mkErr

err = require('./errors')
pb = require('./protobuf')

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

deconstructDatum = (datum, opts) ->
    pb.DatumTypeSwitch(datum, {
        "R_JSON": =>
            obj = JSON.parse(datum.r_str)
            recursivelyConvertPseudotype(obj, opts)
       ,"R_NULL": =>
            null
       ,"R_BOOL": =>
            datum.r_bool
       ,"R_NUM": =>
            datum.r_num
       ,"R_STR": =>
            datum.r_str
       ,"R_ARRAY": =>
            deconstructDatum(dt, opts) for dt in datum.r_array
       ,"R_OBJECT": =>
            obj = {}
            for pair in datum.r_object
                obj[pair.key] = deconstructDatum(pair.val, opts)

            convertPseudotype(obj, opts)
        },
            => throw new err.RqlDriverError "Unknown Datum type"
        )

mkAtom = (response, opts) -> deconstructDatum(response.response[0], opts)

mkSeq = (response, opts) -> (deconstructDatum(res, opts) for res in response.response)

mkErr = (ErrClass, response, root) ->
    msg = mkAtom response

    bt = for frame in response.backtrace.frames
        pb.convertFrame frame

    new ErrClass msg, root, bt

module.exports.deconstructDatum = deconstructDatum
module.exports.mkAtom = mkAtom
module.exports.mkSeq = mkSeq
module.exports.mkErr = mkErr

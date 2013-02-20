goog.provide('rethinkdb.TypeChecker')

print = console.log

tp = (typestr) -> new TypeChecker typestr
class TypeChecker
    constructor: (typestr) ->
        @parseTypeStr typestr

    # Someday perhaps this will be done with a proper parsing algorithm
    # for now though the language is regular enough we can hack it as
    # a first pass.
    parseTypeStr: (typestr) ->
        @typesigs = typestr.split('|').map (sig) ->
            [args, result] = sig.split('->')
            result = TypeName::parse result

            args = args.split(', ')

            match = args[-1..-1][0].match /{(.*)}/
            if match?
                args = args[...-1]
                optargs = match[1]
                optargs = optargs.split '; '
                optobj = {}
                for pair in optargs
                    [key, typ] = pair.split ':'
                    optobj[key] = TypeName::parse typ

            args = args.filter((a)->a.length>0).map TypeName::parse

            {
                result: result
                args: args
                optargs: optobj
            }

    checkType: (op, args, optargs, context) ->
        firstError = null
        for sig in @typesigs

            # Check args
            argError = null
            for arg,i in args
                type = sig.args[i]
                unless type?
                    last = sig.args[-1..-1][0]
                    if last? and last.repeated
                        type = last
                    else
                        argError = new RqlCompileError "Too many arguments provided. Expected #{sig.args.length}."
                        argError.backtrace.unshift i
                        break
                unless type.check(arg)
                    argError = new RqlCompileError "Expected argument type #{type} but found #{TypeName::typeOf(arg)}"
                    argError.backtrace.unshift i
                    break

            unless argError?

                # Eval
                ret = op(args, optargs, context)

                # Check return val
                unless sig.result.check(ret)
                    throw new RqlCompileError "Expected return type #{sig.result} buf found #{TypeName::typeOf(ret)}"

                return ret
            else
                unless firstError? then firstError = argError

        throw firstError

class TypeName
    parse: (typestr) ->
        strtype = typestr.match(/\s*!?(\w+)(?:\(\d+\))?(?:\.\.\.)?\s*/)[1]
        if typestr.match(/^!/)?
            literal = true
        if typestr.match(/\.\.\.\s*$/)?
            repeated = true
        airity = typestr.match(/\((\d+)\)\s*$/)
        if airity? then airity = parseInt airity[1]

        new (switch strtype
            when "Top" then TopType
            when "DATUM" then DatumType
            when "NULL" then NullType
            when "BOOL" then BoolType
            when "NUMBER" then NumberType
            when "STRING" then StringType
            when "OBJECT" then ObjectType
            when "SingleSelection" then SingleSelectionType
            when "ARRAY" then ArrayType
            when "Sequence" then SequenceType
            when "Stream" then StreamType
            when "StreamSelection" then StreamSelectionType
            when "Table" then TableType
            when "Database" then DatabaseType
            when "Function" then FunctionType
            when "Error" then ErrorType
            else print "Unknown type: #{strtype}"
        )(literal, repeated, airity)

    constructor: (literal=false, repeated=false, airity=-1) ->
        @literal = literal
        @repeated = repeated
        @airity = airity

    typeOf: (val) ->
        new (if val instanceof RDBDatabase then DatabaseType
        else if val instanceof RDBTable then TableType
        else if val instanceof Function then FunctionType
        else if val instanceof RDBArray then ArrayType
        else if val instanceof RDBObject
            if val.getPK? then SingleSelectionType else ObjectType
        else if val instanceof RDBPrimitive
            switch val.typeOf()
                when RDBType.NULL then NullType
                when RDBType.BOOLEAN then BoolType
                when RDBType.NUMBER then NumberType
                when RDBType.STRING then StringType
                else
                    Object
        else
            Object
        )

    check: (val) ->
        TypeName::typeOf(val) instanceof @constructor

    toString: -> @st

class TopType extends TypeName
    st: "Top"

class DatumType extends TopType
    st: "DATUM"

class NullType extends DatumType
    st: "NULL"

class BoolType extends DatumType
    st: "BOOL"

class NumberType extends DatumType
    st: "NUMBER"

class StringType extends DatumType
    st: "STRING"

class ObjectType extends DatumType
    st: "OBJECT"

class SingleSelectionType extends ObjectType
    st: "SingleSelection"

class ArrayType extends DatumType
    st: "ARRAY"

class SequenceType extends TopType
    st: "Sequence"

    check: (val) ->
        t = TypeName::typeOf(val)
        (t instanceof ArrayType or t instanceof @constructor)

class StreamType extends SequenceType
    st: "Stream"

class StreamSelectionType extends StreamType
    st: "StreamSelection"

class TableType extends StreamSelectionType
    st: "Table"

class DatabaseType extends TopType
    st: "Database"

class FunctionType extends TopType
    toString: -> "Function(#{@airity})"

class ErrorType extends TypeName
    toString: -> "Error"

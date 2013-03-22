goog.provide('rethinkdb.Context')

goog.require('rethinkdb.Universe')

# Defines the execution context in which a query is evaluated
class RDBContext
    constructor: (universe) ->
        # The universe of all databases available in this context
        @universe = universe

        @scopeStack = null
        @implicitVarStack = []
        @defaultDb = null

    setDefaultDb: (db) ->
        @defaultDb = db

    getDefaultDb: -> @defaultDb

    pushScope: (binds) ->
        binds.parent = @scopeStack
        @scopeStack = binds

    popScope: ->
        @scopeStack = @scopeStack.parent

    lookupVar: (varId) ->
        cur = @scopeStack
        while cur?
            val = cur[varId]
            if val?
                return val
            else
                cur = cur.parent
        throw new RqlRuntimeError "Var #{varId} is not in scope"

    bindIvar: (fun) ->
        (row) =>
            @pushImplicitVar(row)
            row_res = fun(row)
            @popImplicitVar()
            return row_res

    pushImplicitVar: (implicitVar) -> @implicitVarStack.push implicitVar
    popImplicitVar: -> @implicitVarStack.pop()

    getImplicitVar: ->
        if @implicitVarStack.length > 1
            throw new RqlCompileError "Cannot use r.row in nested queries. Use functions instead."
        else if @implicitVarStack.length < 1
            throw new RqlCompileError "No implicit variable is bound."
        else @implicitVarStack[0]

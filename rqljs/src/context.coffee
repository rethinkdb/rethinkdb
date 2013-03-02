goog.provide('rethinkdb.Context')

goog.require('rethinkdb.Universe')

# Defines the execution context in which a query is evaluated
class RDBContext
    constructor: (universe) ->
        # The universe of all databases available in this context
        @universe = universe

        @scopeStack = null
        @implicitVarStack = []

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
    popImplicitVar: (implicitVar) -> @implicitVarStack.pop implicitVar

    getImplicitVar: ->
        if @implicitVarStack.length > 1 then throw new RqlRuntimeError "ambiguious implicit var"
        else if @implicitVarStack.length < 1 then throw new RqlRuntimeError "no implicit var"
        else @implicitVarStack[0]

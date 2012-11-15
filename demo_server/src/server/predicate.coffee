class Predicate
    constructor: (data) ->
        @data = data

    evaluate: (server, context) ->
        #TODO
        #args = @data.getArg()
        term = new Term @data.getBody()
        return term.evaluate server, context

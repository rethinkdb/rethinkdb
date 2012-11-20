class Predicate
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        context = args.context

        # var_args is a string, but it has a count method, why?
        var_args = {}
        var_args[@data.getArg()] = context
        term = new Term @data.getBody()
        return term.evaluate
            server: server
            context: context
            var_args: var_args

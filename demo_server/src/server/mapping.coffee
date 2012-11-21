class Mapping
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        context = args.context
        var_args = args.var_args

        new_var_args = @data.getArg()
        if not var_args?
            var_args = {}
        var_args[new_var_args] = context

        term = new Term @data.getBody()
        return term.evaluate
            server: server
            var_args: var_args
            context: context


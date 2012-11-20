class Mapping
    constructor: (data) ->
        @data = data
    evaluate: (args) ->
        server = args.server
        context = args.context

        term = new Term @data.getBody()
        return term.evaluate
            server: server
            context: context

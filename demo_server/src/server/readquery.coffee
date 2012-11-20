class ReadQuery
    constructor: (data) ->
        @data = data

    evaluate: (server) =>
        term = new Term @data.getTerm()
        return term.evaluate
            server: server

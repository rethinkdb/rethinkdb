class Mapping
    constructor: (data) ->
        @data = data
    evaluate: (server) ->
        term = new Term @data.getBody()
        return term.evaluate server

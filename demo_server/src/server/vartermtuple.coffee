class VarTermTuple
    constructor: (data) ->
        @data = data

    get_key: =>
        return @data.getVar()

    evaluate: (server) =>
        term = new Term @data.getTerm()
        return term.evaluate server

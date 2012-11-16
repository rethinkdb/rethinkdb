class Term
    constructor: (data) ->
        @data = data


    delete: (server) ->
        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            #when 3 # CALL
            #when 4 # IF
            #when 5 # ERROR
            #when 6 # NUMBER
            #when 7 # STRING
            # when '8' # JSON
            #when 9 # BOOL
            #when 10 # ARRAY
            #when 11 # OBJECT
            #when 12 # GETBYKEY
            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.delete server
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR

    evaluate: (server, context) ->
        type = @data.getType()
        switch type
            when 0 # JSON_NULL
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify null
                return response
            #when 1 # VAR
            #when 2 # LET
            when 3 # CALL
                term_call = new TermCall @data.getCall()
                return term_call.evaluate server, context
            #when 4 # IF
            #when 5 # ERROR
            when 6 # NUMBER
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify @data.getNumber()
                return response
            when 7 # STRING
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify @data.getValuestring()
                return response
            # Not yet used
            # when '8' # JSON
            #    response = new Response
            #    response.setStatusCode 1
            #    response.addResponse JSON.stringify @data.getJsonstring()
            #    return response
            when 9 # BOOL
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify @data.getValuebool()
                return response
            when 10 # ARRAY
                response = new Response
                response.setStatusCode 1
                result = []
                for term_data in @data.arrayArray()
                    new_term = new Term term_data
                    result.push JSON.parse new_term.evaluate().getResponse()
                response.addResponse JSON.stringify result
                return response
            when 11 # OBJECT
                response = new Response
                response.setStatusCode 1
                result = {}
                for term_data in @data.objectArray()
                    new_var_term_tuple = new VarTermTuple term_data
                    result[new_var_term_tuple.get_key()] = JSON.parse(new_var_term_tuple.evaluate().getResponse())
                response.addResponse JSON.stringify result
                return response
            when 12 # GETBYKEY
                term_get_by_key = new TermGetByKey @data.getGetByKey()
                return term_get_by_key.evaluate server

            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.evaluate server

            when 14 # JAVASCRIPT
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify eval '(function(){'+@data.getJavascript()+';})()'
                return response
            when 15 # IMPLICIT_VAR
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify context
                return response
            
    filter: (server, predicate) ->
        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            #when 3 # CALL
            #when 4 # IF
            #when 5 # ERROR
            #when 6 # NUMBER
            #when 7 # STRING
            # when '8' # JSON
            #when 9 # BOOL
            when 10 # ARRAY
                #TODO push it to array?
                response = new Response
                response.setStatusCode 1
                result = []
                for term_data in @data.arrayArray()
                    new_term = new Term term_data
                    if JSON.parse(predicate.evaluate(server, new_term).getResponse()) is true
                        result.push JSON.parse new_term.evaluate().getResponse()
                response.addResponse JSON.stringify result
                return response
            #when 11 # OBJECT
            #when 12 # GETBYKEY
            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.filter server, predicate
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR

    mutate: (server, mapping) ->
        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            #when 3 # CALL
            #when 4 # IF
            #when 5 # ERROR
            #when 6 # NUMBER
            #when 7 # STRING
            # when '8' # JSON
            #when 9 # BOOL
            #when 10 # ARRAY
            #when 11 # OBJECT
            #when 12 # GETBYKEY
            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.mutate server, mapping
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR




    range: (server, attr_name, lower_bound, upper_bound) ->
        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            #when 3 # CALL
            #when 4 # IF
            #when 5 # ERROR
            #when 6 # NUMBER
            #when 7 # STRING
            # when '8' # JSON
            #when 9 # BOOL
            #when 10 # ARRAY
            #when 11 # OBJECT
            #when 12 # GETBYKEY
            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.range server, attr_name, lower_bound, upper_bound
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR
            
    update: (args) ->
        server = args.server
        mapping = args.mapping
        context = args.context
        predicate = args.predicate
        builtin = args.builtin
        builtin_args = args.builtin_args


        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            when 3 # CALL
                term_call = new TermCall @data.getCall()
                return term_call.update
                    server: server
                    mapping: mapping
                    context: context
                    builtin: builtin
                    builtin_args: builtin_args
            #when 4 # IF
            #when 5 # ERROR
            #when 6 # NUMBER
            #when 7 # STRING
            # when '8' # JSON
            #when 9 # BOOL
            #when 10 # ARRAY
            #when 11 # OBJECT
            #when 12 # GETBYKEY
            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.update
                    server: server
                    mapping: mapping
                    predicate: predicate
                    builtin: builtin
                    builtin_args: builtin_args
                    context: context
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR


class TermCall
    constructor: (data) ->
        @data = data
    
    evaluate: (server, context) ->
        builtin = new Builtin @data.getBuiltin()
        args = @data.argsArray() # is the table (or something close to that)
        return builtin.evaluate server, args, context

    update: (args) ->
        server = args.server
        mapping = args.mapping
        context = args.context

        builtin = new Builtin @data.getBuiltin()
        builtin_args = @data.argsArray() # is the table (or something close to that)
        return builtin.update
            server: server
            mapping: mapping
            builtin_args: builtin_args
            context: context


class TermGetByKey
    constructor: (data) ->
        @data = data

    evaluate: (server) ->
        table_ref = new TableRef @data.getTableRef()
        # We don't need the primary key
        # key = @data.getKey()
        term = new Term @data.getKey()
        return table_ref.get_term server, term
        

class TermTable
    constructor: (data) ->
        @data = data

    evaluate: (server) ->
        table_ref = new TableRef @data.getTableRef()
        return table_ref.evaluate server

    filter: (server, predicate) ->
        table_ref = new TableRef @data.getTableRef()
        return table_ref.filter server, predicate

    mutate: (server, mapping) ->
        table_ref = new TableRef @data.getTableRef()
        return table_ref.mutate server, mapping

    delete: (server) ->
        table_ref = new TableRef @data.getTableRef()
        return table_ref.delete server

    range: (server, attr_name, lower_bound, upper_bound) ->
        table_ref = new TableRef @data.getTableRef()
        return table_ref.range server, attr_name, lower_bound, upper_bound

    update: (args) ->
        server = args.server
        mapping = args.mapping
        predicate = args.predicate
        context = args.context
        table_ref = new TableRef @data.getTableRef()
        return table_ref.update
            server: server
            mapping: mapping
            predicate: predicate
            context: context

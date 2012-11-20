class Term
    constructor: (data) ->
        @data = data

    delete : (args) ->
        server = args.server
        context = args.context
        predicate = args.predicate
        builtin = args.builtin
        builtin_args = args.builtin_args
        var_args = args.var_args

        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            when 3 # CALL
                term_call = new TermCall @data.getCall()
                return term_call.delete
                    server: server
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
                return term_table.delete
                    server: server
                    predicate: predicate
                    builtin: builtin
                    builtin_args: builtin_args
                    context: context
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR


    evaluate: (args) ->
        server = args.server
        context = args.context
        var_args = args.var_args

        type = @data.getType()
        switch type
            when 0 # JSON_NULL
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify null
                return response
            when 1 # VAR
                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify var_args[@data.getVar()]
                return response
            #when 2 # LET
            when 3 # CALL
                term_call = new TermCall @data.getCall()
                return term_call.evaluate
                    server: server
                    context: context
                    var_args: var_args
            when 4 # IF
                term_if = new TermIf @data.getIf()
                return term_if.evaluate
                    server: server
                    context: context
                    var_args: var_args
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
                    evaluation = new_term.evaluate
                        server: server
                        context: context
                        var_args: var_args
                    result.push JSON.parse evaluation.getResponse()
                response.addResponse JSON.stringify result
                return response
            when 11 # OBJECT
                response = new Response
                response.setStatusCode 1
                result = {}
                for term_data in @data.objectArray()
                    new_var_term_tuple = new VarTermTuple term_data
                    evaluation = new_var_term_tuple.evaluate
                        server: server
                        context: context
                        var_args: var_args
                    result[new_var_term_tuple.get_key()] = JSON.parse(evaluation.getResponse())
                response.addResponse JSON.stringify result
                return response
            when 12 # GETBYKEY
                term_get_by_key = new TermGetByKey @data.getGetByKey()
                return term_get_by_key.evaluate
                    server: server
                    context: context
                    var_args: var_args

            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.evaluate
                    server: server
                    context: context
                    var_args: var_args

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
            
    filter: (args) ->
        server = args.server
        predicate = args.predicate
        attr_name = args.attr_name
        lower_bound = args.lower_bound
        upper_bound = args.upper_bound
        context = args.context
        var_args = args.var_args

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
                    evaluation = predicate.evaluate
                        server: server
                        new_term: new_term
                        var_args: var_args
                    if JSON.parse(evaluation.getResponse()) is true
                        term_evaluation = new_term.evaluate
                            server: server
                            context: context
                            var_args: var_args
                        result.push JSON.parse term_evaluation.getResponse()
                response.addResponse JSON.stringify result
                return response
            #when 11 # OBJECT
            #when 12 # GETBYKEY
            when 13 # TABLE
                term_table = new TermTable @data.getTable()
                return term_table.filter
                    server: server
                    predicate: predicate
                    attr_name: attr_name
                    lower_bound: lower_bound
                    upper_bound: upper_bound
                    var_args: var_args
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR

    mutate: (args) ->
        server = args.server
        mapping = args.mapping
        context = args.context
        predicate = args.predicate
        builtin = args.builtin
        builtin_args = args.builtin_args
        var_args = args.var_args

        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            when 3 # CALL
                term_call = new TermCall @data.getCall()
                return term_call.mutate
                    server: server
                    mapping: mapping
                    context: context
                    builtin: builtin
                    builtin_args: builtin_args
                    var_args: var_args
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
                return term_table.mutate
                    server: server
                    mapping: mapping
                    predicate: predicate
                    builtin: builtin
                    builtin_args: builtin_args
                    context: context
                    var_args: var_args
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR




    range: (args) ->
        server = args.server
        attr_name = args.attr_name
        lower_bound = args.lower_bound
        upper_bound = args.upper_bound
        predicate = args.predicate
        context = args.context
        var_args = args.var_args

        type = @data.getType()
        switch type
            #when 0 # JSON_NULL
            #when 1 # VAR
            #when 2 # LET
            when 3 # CALL
                term_call = new TermCall @data.getCall()
                return term_call.range
                    server: server
                    attr_name: attr_name
                    lower_bound: lower_bound
                    upper_bound: upper_bound
                    predicate: predicate
                    context: context
                    var_args: var_args

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
                return term_table.range
                    server: server
                    attr_name: attr_name
                    lower_bound: lower_bound
                    upper_bound: upper_bound
                    predicate: predicate
                    context: context
                    var_args: var_args
            
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR
            
    update: (args) ->
        server = args.server
        mapping = args.mapping
        context = args.context
        predicate = args.predicate
        builtin = args.builtin
        builtin_args = args.builtin_args
        var_args = args.var_args


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
                    predicate: predicate
                    var_args: var_args

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
                    builtin: builtin
                    builtin_args: builtin_args
                    context: context
                    predicate: predicate
                    var_args: var_args
            #when 14 # JAVASCRIPT
            #when 15 # IMPLICIT_VAR


class TermCall
    constructor: (data) ->
        @data = data
    
    evaluate: (args) ->
        server = args.server
        context = args.context
        var_args = args.var_args

        builtin = new Builtin @data.getBuiltin()
        builtin_args = @data.argsArray() # is the table (or something close to that)
        return builtin.evaluate
            server: server
            builtin_args: builtin_args
            context: context
            var_args: var_args

    range: (args) ->
        server = args.server
        attr_name = args.attr_name
        lower_bound = args.lower_bound
        upper_bound = args.upper_bound
        context = args.context
        var_args = args.var_args

        builtin = new Builtin @data.getBuiltin()
        builtin_args = @data.argsArray() # is the table (or something close to that)
        return builtin.range
            server: server
            attr_name: attr_name
            lower_bound: lower_bound
            upper_bound: upper_bound
            builtin_args: builtin_args
            context: context
            var_args: var_args

    delete: (args) ->
        server = args.server
        mapping = args.mapping
        context = args.context
        var_args = args.var_args

        builtin = new Builtin @data.getBuiltin()
        builtin_args = @data.argsArray() # is the table (or something close to that)
        return builtin.delete
            server: server
            mapping: mapping
            builtin_args: builtin_args
            context: context
            var_args: var_args

    mutate: (args) ->
        server = args.server
        mapping = args.mapping
        context = args.context
        var_args = args.var_args

        builtin = new Builtin @data.getBuiltin()
        builtin_args = @data.argsArray() # is the table (or something close to that)
        return builtin.mutate
            server: server
            mapping: mapping
            builtin_args: builtin_args
            context: context
            var_args: var_args

    update: (args) ->
        server = args.server
        mapping = args.mapping
        context = args.context
        var_args = args.var_args

        builtin = new Builtin @data.getBuiltin()
        builtin_args = @data.argsArray() # is the table (or something close to that)
        return builtin.update
            server: server
            mapping: mapping
            builtin_args: builtin_args
            context: context
            var_args: var_args

class TermIf
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        context = args.context
        var_args = args.var_args

        condition = new Term @data.getTest()
        true_branch = new Term @data.getTrueBranch()
        false_branch = new Term @data.getFalseBranch()

        evaluation = condition.evaluate
            server: server
            context: context
            var_args: var_args
        if JSON.parse(evaluation.getResponse()) is true
            return true_branch.evaluate
                server: server
                context: context
                var_args: var_args
        else
            return false_branch.evaluate
                server: server
                context: context
                var_args: var_args
        

class TermGetByKey
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        context = args.context

        table_ref = new TableRef @data.getTableRef()
        # We don't need the primary key
        # key = @data.getKey()
        term = new Term @data.getKey()
        return table_ref.get_term
            server: server
            term: term
        

class TermTable
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server

        table_ref = new TableRef @data.getTableRef()
        return table_ref.evaluate
            server: server

    filter: (args) ->
        server = args.server
        predicate = args.predicate
        context = args.context

        table_ref = new TableRef @data.getTableRef()
        return table_ref.filter
            server: server
            predicate: predicate
            context: context

    mutate: (args) ->
        server = args.server
        mapping = args.mapping
        predicate = args.predicate
        context = args.context

        table_ref = new TableRef @data.getTableRef()
        return table_ref.mutate
            server: server
            mapping: mapping
            predicate: predicate
            context: context

    delete: (args) ->
        server = args.server
        predicate = args.predicate
        context = args.context

        table_ref = new TableRef @data.getTableRef()
        return table_ref.delete
            server: server
            predicate: predicate
            context: context

    range: (args) ->
        server = args.server
        attr_name = args.attr_name
        lower_bound = args.lower_bound
        upper_bound = args.upper_bound
        predicate = args.predicate
        context = args.context

        table_ref = new TableRef @data.getTableRef()
        return table_ref.range
            server: server
            attr_name: attr_name
            lower_bound: lower_bound
            upper_bound: upper_bound
            predicate: predicate
            context: context

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

# Helpers
class Helpers
    generate_uuid: =>
        # That should be good enough for the client server
        result = (Math.random()+'').slice(2)+(Math.random()+'').slice(2)
        result = CryptoJS.SHA1(result)+''
        result = result.slice(0, 8)+'-'+result.slice(8,12)+'-'+result.slice(12, 16)+'-'+result.slice(16, 20)+'-'+result.slice(20, 32)
        return result

    are_equal: (a, b) =>
        if typeof a isnt typeof b
            return false
        else
            if typeof a is 'object' and typeof b is 'object'
                if Object.prototype.toString.call(a) is '[object Array]' and Object.prototype.toString.call(b) is '[object Array]'
                    if a.length isnt b.length
                        return false
                    else
                        for a_value, i in a
                            if @are_equal(a[i], b[i]) is false
                                return false
                        return true
                else if a is null and b isnt null
                    return false
                else if b is null and a isnt null
                    return false
                else
                    for key of a
                        if @are_equal(a[key], b[key]) is false
                            return false
                    for key of b
                        if @are_equal(a[key], b[key]) is false
                            return false
                    return true
            else
                return a is b

    compare: (a, b) =>
        value =
            array: 1
            boolean: 2
            null: 3
            number: 4
            object: 5
            string: 6
        if typeof a is typeof b
            switch typeof a
                when 'array'
                    for value, i in a
                        result = @compare(a[i], b[i])
                        if result isnt 0
                            return result
                    return len(a)-len(b)
                when 'boolean' # false < true
                    if a is b
                        return 0
                    else if a is false
                        return -1
                    else
                        return 1
                when 'object'
                    if a is null and b is null
                        return 0
                    else
                        return { error: true }
                when 'number'
                    return a-b
                when 'string'
                    if a > b
                        return 1
                    else if a < b
                        return -1
                    else
                        return 0
        else
            type_a = typeof a
            if type_a is 'object' and a isnt null
                return {error: true}
            type_b = typeof b
            if type_b is 'object' and b isnt null
                return {error: true}

            return value[type_a] - value[type_b]

class Builtin
    constructor: (data) ->
        @data = data

    evaluate: (server, args, context) ->
        type = @data.getType()
        switch type
            when 1 # Not
                response = new Response
                if args[0].getType() isnt 9
                    response.setStatusCode 103
                    response.setErrorMessage 'Not can only be called on a boolean'
                    #TODO Backtrace
                    return response
                
                response.setStatusCode 1
                response.addResponse not args[0].getValuebool()
                return response

            when 2 # Getattr
                term = new Term args[0]
                original_object = JSON.parse term.evaluate().getResponse()

                attr = @data.getAttr()

                response = new Response
                if typeof attr isnt 'string'
                    response.setStatusCode 103
                    response.addResponse JSON.stringify 'TypeError: Function argument '+JSON.strinfigy yattr+' must be of the type string'
                    # That's a poor answer, especially if attr is an object. But I'm doing the same thing as our actual server has for now.
                else if not original_object[attr]?
                    response.setStatusCode 103
                    response.addResponse JSON.stringify 'Runtime Error: Object:'+ JSON.stringify(original_object, undefined, 4)+'is missing attribute '+JSON.stringify attr
                    #TODO backtrace
                else
                    response.setStatusCode 1
                    response.addResponse JSON.stringify original_object[attr]
                return response
            #when 3 # Implicit getattr
            when 4 # hasattr
                term = new Term args[0]
                original_object = JSON.parse term.evaluate().getResponse()

                attr = @data.getAttr()

                response = new Response
                if typeof attr isnt 'string'
                    response.setStatusCode 103
                    response.addResponse JSON.stringify 'TypeError: Function argument '+JSON.strinfigy yattr+' must be of the type string'
                    # That's a poor answer, especially if attr is an object. But I'm doing the same thing as our actual server has for now.
                else if original_object[attr] is undefined # Coffeescript ternary opertor check for null too.
                    response.setStatusCode 1
                    response.addResponse JSON.stringify false
                else
                    response.setStatusCode 1
                    response.addResponse JSON.stringify true
                return response
            #when 5 # implicit hasattr
            when 6 # pickattrs
                new_term = new Term args[0]
                original_object = JSON.parse new_term.evaluate().getResponse()
                attrs = @data.attrsArray()
                new_object = {}
                for attr in attrs
                    new_object[attr] = original_object[attr]

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify new_object
                return response
            #when 7 # implicit pickattrs
            when 8 # map merge
                #TODO Handle errors?
                result_term = new Term args[0]
                result_object = JSON.parse result_term.evaluate().getResponse()

                extra_term = new Term args[1]
                extra_object = JSON.parse extra_term.evaluate().getResponse()
               
                for key, value of extra_object
                    result_object[key] = value

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify result_object
                return response
            when 9 # array append
                #TODO Handle errors?
                result_term = new Term args[0]
                result_array = JSON.parse result_term.evaluate().getResponse()

                new_term = new Term args[1]
                new_term_value = JSON.parse new_term.evaluate().getResponse()
                
                result_array.push new_term_value

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify result_array
                return response
            #when 11 # slice
            when 14 # Add
                response = new Response
                if (args[0].getType() isnt 6 and args[0].getType() isnt 10) or (args[1].getType() isnt 6 and args[1].getType() isnt 10) or (args[0].getType() isnt args[1].getType())
                    response.setStatusCode 103
                    response.setErrorMessage 'Can only ADD numbers with number and arrays with arrays'
                    #TODO Backtrace
                    return response

                if args[0].getType() is 6
                    result = args[0].getNumber() + args[1].getNumber()
                else if args[0].getType() is 10
                    result = []
                    for term_raw in args[0].arrayArray()
                        term = new Term term_raw
                        result.push JSON.parse term.evaluate(server).getResponse()
                    for term_raw in args[1].arrayArray()
                        term = new Term term_raw
                        result.push JSON.parse term.evaluate(server).getResponse()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 15 # Substract
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to SUBSTRACT must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber() - args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 16 # Multiply
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to MULTIPLY must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber() * args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 17 # Divide
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to DIVIDE  must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber() / args[1].getNumber()
                result = parseFloat result.toFixed 6
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 18 # Modulo
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to DIVIDE  must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber()%args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response
            when 19 # Compare
                term1 = new Term args[0]
                term1_value = JSON.parse term1.evaluate(server, context).getResponse()
                term2 = new Term args[1]
                term2_value = JSON.parse term2.evaluate(server, context).getResponse()


                response = new Response()
                comparison = @data.getComparison()
                switch comparison
                    when 1 # eq
                        result = Helpers.prototype.are_equal term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result is true)
                    when 2 # ne
                        result = Helpers.prototype.are_equal term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result is false)
                    when 3 # lt
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result < 0)
                    when 4 #le
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result <= 0)
                    when 5 #gt
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result > 0)
                    when 6 #ge
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result >= 0)
                return response
                    
            when 20 # Filter
                ###
                terms = []
                for term_data in args
                    terms.push new Term term_data
                ###
                term = new Term args[0]
                builtin_filter = new BuiltinFilter @data.getFilter()
                return builtin_filter.evaluate server, term
            #when 21 # Map
            #when 22 # Concatmap
            #when 23 # Orderby
            #when 24 # Distinct
            #when 26 # Length
            #when 27 # Union
            #when 28 # Nth
            #when 29 # Stream to array
            #when 30 # Array to strean
            #when 31 # Reduce
            #when 32 # Group mapreduce
            when 35 # Any
                term1 = new Term args[0]
                term1_value = JSON.parse term1.evaluate(server).getResponse()
                term2 = new Term args[1]
                term2_value = JSON.parse term2.evaluate(server).getResponse()

                response = new Response()
                response.setStatusCode 1
                response.addResponse JSON.stringify(term1_value or term2_value)
                return response

            when 36 # All
                term1 = new Term args[0]
                term1_value = JSON.parse term1.evaluate(server).getResponse()
                term2 = new Term args[1]
                term2_value = JSON.parse term2.evaluate(server).getResponse()

                response = new Response()
                response.setStatusCode 1
                response.addResponse JSON.stringify(term1_value and term2_value)
                return response
            #when 37 # Range
            #when 38 # Implicit without
            when 39 # Without
                new_term = new Term args[0]
                original_object = JSON.parse new_term.evaluate().getResponse()
                new_object = {}
                for key, value of original_object
                    new_object[key] = value
                attrs = @data.attrsArray()
                for attr in attrs
                    delete new_object[attr]

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify new_object
                return response

class BuiltinFilter
    constructor: (data) ->
        @data = data
    evaluate: (server, term) ->
        predicate = new Predicate @data.getPredicate()
        # Can we have more than one term? Let's suppose not for the time being
        return term.filter server, predicate



# Classes for the protocol
class CreateTable
    constructor: (data) ->
        @data = data

    evaluate: (server) =>
        datacenter = @data.getDatacenter()
        table_ref = new TableRef(@data.getTableRef())
        primary_key = @data.getPrimaryKey()
        cache_size = @data.getCacheSize()

        options =
            datacenter: (datacenter if datacenter?)
            primary_key: (primary_key if primary_key?)
            cache_size: (cache_size if cache_size?)
        return table_ref.create server, options

 
class Mapping
    constructor: (data) ->
        @data = data
    evaluate: (server) ->
        term = new Term @data.getBody()
        return term.evaluate()

class MetaQuery
    constructor: (data) ->
        @data = data

    evaluate: (server) ->
        type = @data.getType()
        switch type
            when 1 # .dbCreate()
                db_name = @data.getDbName()
                # Check for bad characters
                if /[a-zA-Z0-9_]+/.test(db_name) is false
                    response = new Response
                    response.setStatusCode 102
                    response.setErrorMessage "Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.)."
                    return response

                # If the database already exist, we send back an error
                if db_name of server
                    response = new Response
                    response.setStatusCode 103
                    response.setErrorMessage "Error: Error during operation `CREATE_DB #{db_name}`: Entry already exists."
                    return response

                # All green, we can create the database
                server[db_name] = {}
                response = new Response
                response.setStatusCode 0 # SUCCESS_STREAM
                return response

            when 2 # .dropDB()
                db_name = @data.getDbName()
                # Check for bad characters
                if /[a-zA-Z0-9_]+/.test(db_name) is false
                    response = new Response
                    response.setStatusCode 102
                    response.setErrorMessage "Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.)."
                    return response

                # Make sure that the database does exist
                if not (db_name of server)
                    response = new Response
                    response.setStatusCode 103
                    response.setErrorMessage "Error: Error during operation `DROP_DB #{db_name}`: No entry with that name"
                    return response
                
                # All green, we can delete the database
                delete server[db_name]
                response = new Response
                response.setStatusCode 0 # SUCCESS_STREAM
                return response

            when 3 # .dbList()
                response = new Response
                result = []
                for database of server
                    response.addResponse JSON.stringify database
                response.setStatusCode 3 # SUCCESS_STREAM
                return response

            when 4 # db('...').create('...')
                create_table = new CreateTable @data.getCreateTable()
                return create_table.evaluate server

            when 5 # db('...').drop('...')
                table_to_drop = new TableRef @data.getDropTable()
                return table_to_drop.drop server

            when 6 # db('test').list()
                response = new Response
                result = []
                for table of server[@data.getDbName()]
                    response.addResponse JSON.stringify table
                response.setStatusCode 3 # SUCCESS_STREAM
                return response


class Predicate
    constructor: (data) ->
        @data = data

    evaluate: (server, context) ->
        #TODO
        #args = @data.getArg()
        term = new Term @data.getBody()
        return term.evaluate server, context
        


class ReadQuery
    constructor: (data) ->
        @data = data

    evaluate: (server) =>
        term = new Term @data.getTerm()
        return term.evaluate server


class Query
    constructor: (data) ->
        @data = data

    # Probably need an extra arg (this)
    evaluate: (server) =>
        type = @data.getType()
        switch type
            when 1 # Read query
                read_query = new ReadQuery @data.getReadQuery()
                return read_query.evaluate server
            when 2 # Write query
                write_query = new WriteQuery @data.getWriteQuery()
                return write_query.write server
            #when 3 # Continue
            #when 4 # Stop
            when 5 # Meta query
                meta_query = new MetaQuery @data.getMetaQuery()
                return meta_query.evaluate server

class TableRef
    constructor: (data) ->
        @data = data

    get_db_name: =>
        return @data.getDbName()
    get_table_name: =>
        return @data.getTableName()
    get_use_outdated: =>
        return @data.getUseOutdated()
    get_primary_key: (server) =>
        db_name = @data.getDbName()
        table_name = @data.getTableName()
        return server[db_name][table_name]['options']['primary_key']

    create: (server, options) =>
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        if /[a-zA-Z0-9_]+/.test(db_name) is false or /[a-zA-Z0-9_]+/.test(table_name) is false
            response = new Response
            response.setStatusCode 102
            response.setErrorMessage "Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.)."
            return response

        if not db_name of server
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_DATABASE #{db_name}`: No entry with that name"
            return response
        else if table_name of server[db_name]
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `CREATE_TABLE #{table_name}`: Entry already exists"
            return response
        else
            if not options.datacenter?
                options.datacenter = null
            if not options.primary_key?
                options.primary_key = 'id'
            if not options.cache_size?
                options.cache_size = 1024*1024*1024 # 1GB
            server[db_name][table_name] =
                options: options
                data: {}
            response = new Response
            response.setStatusCode 0 # SUCCESS_STREAM
            return response

    drop: (server) =>
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        if not db_name of server
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_DATABASE #{db_name}`: No entry with that name"
            return response
        else if not table_name of server[db_name]
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_TABLE #{table_name}`: No entry with that name"
            return response
        else
            delete server[db_name][table_name]
            response = new Response
            response.setStatusCode 0 # SUCCESS_STREAM
            return response

    evaluate: (server) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        # Check format
        if /[a-zA-Z0-9_]+/.test(db_name) is false or /[a-zA-Z0-9_]+/.test(table_name) is false
            response = new Response
            response.setStatusCode 102
            response.setErrorMessage "Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.)."
            return response
        
        # Make sure that the table exists
        if not server[db_name]?
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `EVAL_DB #{db_name}`: No entry with that name"
            return response
        if not server[db_name][table_name]?
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `EVAL_TABLE #{db_name}`: No entry with that name"
            return response
        
        # All green, we can get the data
        response = new Response
        for id, document of server[db_name][table_name]['data']
            response.addResponse JSON.stringify document
        response.setStatusCode 3
        return response

    get_term: (server, term) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        key = JSON.parse term.evaluate().getResponse()
        if typeof key is 'number'
            internal_key = 'N'+key
        else if typeof key is 'string'
            internal_key = 'S'+key
        else
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Runtime Error: Primary key must be a number or a string, not #{key}"
            return response

        response = new Response
        if server[db_name][table_name]['data'][internal_key]?
            response.addResponse JSON.stringify server[db_name][table_name]['data'][internal_key]
        response.setStatusCode 3
        return response

    find_table: (server) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()
        if not db_name of server
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_DATABASE #{db_name}`: No entry with that name"
            return response
        else if not table_name of server[db_name]
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_TABLE #{table_name}`: No entry with that name"
            return response
        return null

    point_delete: (server, attr_name, attr_value) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        response_find_table = @find_table(server)
        if response_find_table?
            return response_find_table
        else
            if typeof attr_value is 'number'
                internal_key = 'N'+attr_value
            else if typeof attr_value is 'string'
                internal_key = 'S'+attr_value
            response = new Response
            if server[db_name][table_name]['data'][internal_key]?
                delete server[db_name][table_name]['data'][internal_key]
                response.addResponse JSON.stringify
                    deleted: 1
                response.setStatusCode 1
            else
                response.addResponse JSON.stringify
                    deleted: 1
                response.setStatusCode 1
            return response

    point_update: (server, attr_name, attr_value, mapping_value) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        response_find_table = @find_table(server)
        if response_find_table?
            return response_find_table
        else
            if typeof attr_value is 'number'
                internal_key = 'N'+attr_value
            else if typeof attr_value is 'string'
                internal_key = 'S'+attr_value
            response = new Response
            if server[db_name][table_name]['data'][internal_key]?
                for key, value of mapping_value
                    server[db_name][table_name]['data'][internal_key][key] = value
                response.addResponse JSON.stringify
                    updated: 1
                    skipped: 0
                    errors: 0
                response.setStatusCode 1
            else
                response.addResponse JSON.stringify
                    updated: 1
                    skipped: 0
                    errors: 0
                response.setStatusCode 1
            return response

    filter: (server, predicate) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        # Check format
        if /[a-zA-Z0-9_]+/.test(db_name) is false or /[a-zA-Z0-9_]+/.test(table_name) is false
            response = new Response
            response.setStatusCode 102
            response.setErrorMessage "Bad Query: Invalid name 'f-f'.  (Use A-Za-z0-9_ only.)."
            return response
        
        # Make sure that the table exists
        if not server[db_name]?
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `EVAL_DB #{db_name}`: No entry with that name"
            return response
        if not server[db_name][table_name]?
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `EVAL_TABLE #{db_name}`: No entry with that name"
            return response
        
        # All green, we can get the data
        response = new Response
        results = []
        for id, document of server[db_name][table_name]['data']
            #TODO Type of document is not the good one...
            if JSON.parse(predicate.evaluate(server, document).getResponse()) is true
                response.addResponse JSON.stringify document
        response.setStatusCode 3
        return response

      

class Term
    constructor: (data) ->
        @data = data

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

class TermCall
    constructor: (data) ->
        @data = data
    
    evaluate: (server, context) ->
        builtin = new Builtin @data.getBuiltin()
        args = @data.argsArray() # is the table (or something close to that)
        return builtin.evaluate server, args, context


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

class VarTermTuple
    constructor: (data) ->
        @data = data

    get_key: =>
        return @data.getVar()

    evaluate: (server) =>
        term = new Term @data.getTerm()
        return term.evaluate server

class WriteQuery
    constructor: (data) ->
        @data = data
    write: (server) ->
        type = @data.getType()
        switch type
            when 4
                insert_query = new WriteQueryInsert @data.getInsert()
                return insert_query.insert server
            when 7
                point_update_query = new WriteQueryPointUpdate @data.getPointUpdate()
                return point_update_query.update server
            when 8
                point_delete_query = new WriteQueryPointDelete @data.getPointDelete()
                return point_delete_query.delete server

class WriteQueryInsert
    constructor: (data) ->
        @data = data

    insert: (server) ->
        table_ref = new TableRef @data.getTableRef()
        data_to_insert = @data.termsArray()
        response_data =
            inserted: 0
            errors: 0
        generated_keys = []
        for term_to_insert_raw in data_to_insert # For all documents we want to insert
            term_to_insert = new Term term_to_insert_raw
            json_object = JSON.parse term_to_insert.evaluate(server).getResponse()
            if table_ref.get_primary_key(server) of json_object
                key = json_object[table_ref.get_primary_key(server)]
                if typeof key is 'number'
                    internal_key = 'N'+key
                else if typeof key is 'string'
                    internal_key = 'S'+key
                else
                    if not ('first_error' of response_data)
                        response_data.first_error = "Cannot insert row #{JSON.stringify(json_object, undefined, 1)} with primary key #{JSON.stringify(key, undefined, 1)} of non-string, non-number type."
                    response_data.errors++
                    continue

                # Checking if the key is already used
                if internal_key of server[table_ref.get_db_name()][table_ref.get_table_name()]['data']
                    response_data.errors++
                    if not ('first_error' of response_data)
                        response_data.first_error = "Duplicate primary key #{table_ref.get_primary_key(server)} in #{JSON.stringify(json_object, undefined, 1)}"
                else
                    server[table_ref.get_db_name()][table_ref.get_table_name()]['data'][internal_key] = json_object
                    response_data.inserted++
            else
                new_id = Helpers.prototype.generate_uuid()
                internal_key = 'S' + new_id
                if internal_key of server[table_ref.get_db_name()][table_ref.get_table_name()]['data']
                    response_data.errors++
                    if not ('first_error' of response_data)
                        response_data.first_error = "Generated key was a duplicate either you've won the uuid lottery or you've intentionnaly tried to predict the keys rdb would generate... in which case well done."
                else
                    json_object[table_ref.get_primary_key(server)] = new_id
                    server[table_ref.get_db_name()][table_ref.get_table_name()]['data'][internal_key] = json_object
                    response_data.inserted++
                    generated_keys.push new_id
        if generated_keys.length > 0
            response_data.generated_keys = generated_keys
        response = new Response
        response.setStatusCode 1 # SUCCESS EMPTY
        response.addResponse JSON.stringify response_data
        return response




class WriteQueryPointDelete
    constructor: (data) ->
        @data = data

    delete: (server) ->
        table_ref = new TableRef @data.getTableRef()
        attr_name = @data.getAttrname()
        term = new Term @data.getKey()
        attr_value = JSON.parse term.evaluate().getResponse()

        return table_ref.point_delete server, attr_name, attr_value


class WriteQueryPointUpdate
    constructor: (data) ->
        @data = data

    update: (server) ->
        table_ref = new TableRef @data.getTableRef()
        attr_name = @data.getAttrname()
        term = new Term @data.getKey()
        attr_value = JSON.parse term.evaluate().getResponse()
        mapping = new Mapping @data.getMapping()
        mapping_value = JSON.parse mapping.evaluate().getResponse()
        return table_ref.point_update server, attr_name, attr_value, mapping_value





###
response = new Response
response.setStatusCode 103
response.setErrorMessage 
return response
###
# Local JavaScript server
class DemoServer
    constructor: ->
        @local_server = {}
        @set_descriptor()
        @set_serializer()

    print_all: =>
        console.log @local_server

    set_descriptor: =>
        @descriptor = window.Query.getDescriptor()
    set_serializer: =>
        @serializer = new goog.proto2.WireFormatSerializer()

    buffer2pb: (buffer) ->
        expanded_array = new Uint8Array buffer
        return expanded_array.subarray 4

    pb2query: (intarray) =>
        @serializer.deserialize @descriptor, intarray

    execute: (data) =>
        data_query = @pb2query @buffer2pb data
        console.log 'Server: deserialized query'
        console.log data_query
        query = new Query data_query
        response = query.evaluate @local_server
        if response?
            response.setToken data_query.getToken()
        console.log 'Server: response'
        console.log response

        serialized_response = @serializer.serialize response
        length = serialized_response.byteLength
        final_response = new Uint8Array(length + 4)
        final_response[0] = length%256
        final_response[1] = Math.floor(length/256)
        final_response[2] = Math.floor(length/(256*256))
        final_response[3] = Math.floor(length/(256*256*256))
        final_response.set serialized_response, 4
        console.log 'Server: serialized response'
        console.log final_response
        return final_response


window.DemoServer = DemoServer

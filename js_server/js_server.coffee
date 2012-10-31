# Helpers
class Helpers
    generate_uuid: =>
        # That should be good enough for the client server
        result = (Math.random()+'').slice(2)+(Math.random()+'').slice(2)
        result = CryptoJS.SHA1(result)+''
        result = result.slice(0, 8)+'-'+result.slice(8,12)+'-'+result.slice(12, 16)+'-'+result.slice(16, 20)+'-'+result.slice(20, 32)
        return result

class Builtin
    constructor: (data) ->
        @data = data

    evaluate: (server, args) ->
        console.log args

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

            #when 2 # Getattr
            #when 3 # Implicit getattr
            #when 4 # hasattr
            #when 5 # implicit hasattr
            #when 6 # pickattrs
            #when 7 # implicit pickattrs
            #when 8 # map merge
            #when 9 # array append
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
            #when 19 # Compare
            #That's a little tricky to do
            
            #when 20 # Filter
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
            #when 35 # Any
            #when 36 # All
            #when 37 # Range
            #when 38 # Implicit without
            #when 39 # Without

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
       

class Term
    constructor: (data) ->
        @data = data

    evaluate: (server) ->
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
                return term_call.evaluate server
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
            #when 15 # IMPLICIT_VAR

class TermCall
    constructor: (data) ->
        @data = data
    
    evaluate: (server) ->
        builtin = new Builtin @data.getBuiltin()
        args = @data.argsArray()
        return builtin.evaluate server, args


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



###
response = new Response
response.setStatusCode 103
response.setErrorMessage 
return response
###
# Local JavaScript server
class JavascriptServer
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


$(document).ready ->
    ###
    window.r = rethinkdb
    window.R = r.R

    window.js_server = new JavascriptServer
    r.fake_connect(window.js_server)

    window.q = $.extend true, {}, r
    window.Q = q.R

    server = {
        host: window.location.hostname,
        port: parseInt(window.location.port)-888+1000
    }
    

    q.connect server

    setTimeout test_all, 500

    ###


test_all = ->

    queries = [
        {
        local: "r.dbList().run()",
        server:"q.dbList().run()"
        }
    ]
    test(queries, 0)


    
test = (queries, i) ->
    local_cursor = eval(queries[i].local)
    server_cursor = eval(queries[i].server)
    if _.isEqual(local_cursor, server_cursor) is false
        console.log '~~~~~~~~~~~~~~~~~~~~~~~~~~~'
        console.log 'Local cursor'
        console.log local_cursor
        console.log 'Server cursor'
        console.log server_cursor
        display_result(i, false)
    else
        display_result(i, true)

display_result = (i, is_success) ->
    if is_success is true
        li_template = '<li>Query '+i+': Success</li>'
    else
        li_template = '<li>Query '+i+': Fail</li>'
    $('#result').append li_template

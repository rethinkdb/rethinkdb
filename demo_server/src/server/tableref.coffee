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


class TableRef
    #TODO Refactor errors
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

    delete: (server) =>
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
 
        deleted = 0
        for internal_key of server[db_name][table_name]['data']
            delete server[db_name][table_name]['data'][internal_key]
            deleted++
        response = new Response
        response.addResponse JSON.stringify
            deleted: deleted
        response.setStatusCode 1 # SUCCESS_STREAM
        return response


    drop: (server) =>
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        if not server[db_name]?
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_DATABASE #{db_name}`: No entry with that name"
            return response
        else if not server[db_name][table_name]?
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
        if not server[db_name]?
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_DATABASE #{db_name}`: No entry with that name"
            return response
        else if not server[db_name][table_name]
            response = new Response
            response.setStatusCode 103
            response.setErrorMessage "Error: Error during operation `FIND_TABLE #{table_name}`: No entry with that name"
            return response
        return null

    mutate: (server, mapping) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        mapping_value = JSON.parse mapping.evaluate().getResponse()

        response_find_table = @find_table(server)
        if response_find_table?
            return response_find_table
        else
            response = new Response
            result =
                modified: 0
                inserted: 0
                deleted: 0
                errors: 0

            primary_key = server[db_name][table_name]['options']['primary_key']

            updated_at_least_one = false
            for internal_key of server[db_name][table_name]['data']
                if mapping_value[primary_key] isnt server[db_name][table_name]['data'][internal_key][primary_key]
                    #TODO backtrace
                    if not result.first_error?
                        result.first_error = 'mutate cannot change primary key '+primary_key+' (got objects '+JSON.stringify(server[db_name][table_name]['data'][internal_key], undefined, 4)+', '+JSON.stringify(mapping_value, undefined, 4)+')'
                        #TODO backtrace
                    result.errors++
                else
                    server[db_name][table_name]['data'][internal_key] = mapping_value
                    result.modified++
                    updated_at_least_one = true

            if updated_at_least_one is true
                response.setStatusCode 1
            else
                response.setStatusCode 103
            response.addResponse JSON.stringify result

            return response

    update: (server, mapping) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()
        mapping_value = JSON.parse mapping.evaluate(server).getResponse()

        response_find_table = @find_table(server)
        if response_find_table?
            return response_find_table
        else
            response = new Response
            result =
                updated: 0
                skipped: 0
                errors: 0

            primary_key = server[db_name][table_name]['options']['primary_key']

            updated_at_least_one = false
            for internal_key of server[db_name][table_name]['data']
                if mapping_value[primary_key]? and mapping_value[primary_key] isnt server[db_name][table_name]['data'][internal_key][primary_key]
                    #TODO backtrace
                    if not result.first_error?
                        result.first_error = 'update cannot change primary key '+primary_key+' (got objects '+JSON.stringify(server[db_name][table_name]['data'][internal_key], undefined, 4)+', '+JSON.stringify(mapping_value, undefined, 4)+')'
                        #TODO backtrace
                    result.errors++
                else
                    result.updated++
                    for key, value of mapping_value
                        server[db_name][table_name]['data'][internal_key][key] = value
                        updated_at_least_one = true

            if updated_at_least_one is true
                response.setStatusCode 1
            else
                response.setStatusCode 103
            response.addResponse JSON.stringify result

            return response



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

    point_mutate: (server, attr_name, attr_value, mapping) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        mapping_value = JSON.parse mapping.evaluate().getResponse()

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
                if mapping_value[attr_name] isnt attr_value
                    response.addResponse JSON.stringify 'Runtime Error: mutate cannot change primary key id (got objects) '+JSON.strinfigy(server[db_name][table_name]['data'][internal_key], undefined, 4)+', '+JSON.stringify(mapping_value, undefined, 4)+')'
                    #TODO backtrace
                    response.setStatusCode 103
                else
                    server[db_name][table_name]['data'][internal_key] = mapping_value
                    response.addResponse JSON.stringify
                        modified: 1
                        inserted: 0
                        deleted: 0
                        errors: 0
                    response.setStatusCode 1
            else
                response.addResponse JSON.stringify
                    modified: 1
                    inserted: 0
                    deleted: 0
                    errors: 0
                response.setStatusCode 1
            return response




    point_update: (server, attr_name, attr_value, mapping) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()

        mapping_value = JSON.parse mapping.evaluate().getResponse()

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
                    updated: 0
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

    range: (server, attr_name, lower_bound, upper_bound) ->
        db_name = @data.getDbName()
        table_name = @data.getTableName()
        lower_bound = JSON.parse lower_bound.evaluate().getResponse()
        upper_bound = JSON.parse upper_bound.evaluate().getResponse()

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

        response = new Response
        results = []
        for id, document of server[db_name][table_name]['data']
            if not document[attr_name]?
                response.setStatusCode 103
                response.addResponse JSON.strinfigy 'Object '+JSON.strinfigy(document, undefined, 4)+' has no attribute '+attr_name
                #TODO backtrace
            else
                console.log '~~'
                console.log lower_bound
                console.log upper_bound
                if Helpers.prototype.compare(document[attr_name], lower_bound) >= 0 and Helpers.prototype.compare(document[attr_name], upper_bound) <= 0
                    results.push document
        response.addResponse JSON.stringify results
        response.setStatusCode 1
        return response

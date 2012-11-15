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

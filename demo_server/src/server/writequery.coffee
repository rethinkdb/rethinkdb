class WriteQuery
    constructor: (data) ->
        @data = data
    write: (server) ->
        type = @data.getType()
        switch type
            when 1 # Update
                update_query = new WriteQueryUpdate @data.getUpdate()
                return update_query.update server
            when 2 # Delete
                delete_query = new WriteQueryDelete @data.getDelete()
                return delete_query.delete server
            when 3 # Mutate
                mutate_query = new WriteQueryMutate @data.getMutate()
                return mutate_query.mutate server
            when 4
                insert_query = new WriteQueryInsert @data.getInsert()
                return insert_query.insert server
            #when 6 # For each
            when 7
                point_update_query = new WriteQueryPointUpdate @data.getPointUpdate()
                return point_update_query.update server
            when 8
                point_delete_query = new WriteQueryPointDelete @data.getPointDelete()
                return point_delete_query.delete server
            when 9
                point_mutate_query = new WriteQueryPointMutate @data.getPointMutate()
                return point_mutate_query.mutate server

class WriteQueryDelete
    constructor: (data) ->
        @data = data

    delete: (server) ->
        term = new Term @data.getView()
        return term.delete
            server: server


 
class WriteQueryMutate
    constructor: (data) ->
        @data = data

    mutate: (server) ->
        term = new Term @data.getView()
        mapping = new Mapping @data.getMapping()
        return term.mutate
            server: server
            mapping: mapping

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


class WriteQueryPointMutate
    constructor: (data) ->
        @data = data

    mutate: (server) ->
        table_ref = new TableRef @data.getTableRef()
        attr_name = @data.getAttrname()
        term = new Term @data.getKey()
        attr_value = JSON.parse term.evaluate().getResponse()

        mapping = new Mapping @data.getMapping()

        return table_ref.point_mutate server, attr_name, attr_value, mapping

class WriteQueryPointUpdate
    constructor: (data) ->
        @data = data

    update: (server) ->
        table_ref = new TableRef @data.getTableRef()
        attr_name = @data.getAttrname()
        term = new Term @data.getKey()
        attr_value = JSON.parse term.evaluate().getResponse()
        mapping = new Mapping @data.getMapping()
        return table_ref.point_update server, attr_name, attr_value, mapping

class WriteQueryUpdate
    constructor: (data) ->
        @data = data

    update: (server) ->
        term = new Term @data.getView()
        mapping = new Mapping @data.getMapping()
        return term.update
            server: server
            mapping: mapping

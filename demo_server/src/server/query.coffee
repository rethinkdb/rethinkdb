#BC: I don't this this class is necessary. All this does is wrap the
#    logic of distinguishing the query type

###
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
###

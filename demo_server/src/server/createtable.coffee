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



# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'TableView', ->
    class @ShardAssignments extends Backbone.View
        template: Handlebars.templates['shard_assignments-template']

        initialize: (data) =>
            @model = data.model
            @collection = data.collection

            #TODO Create one view per model instead of blowing up everything
            @collection.on 'update', @render

        render: =>
            @$el.html @template
                shards: @collection.map (shard) -> shard.toJSON()
            @

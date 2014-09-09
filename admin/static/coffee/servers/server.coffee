# Copyright 2010-2012 RethinkDB, all rights reserved.
# Machine view
module 'ServerView', ->
    class @ServerContainer extends Backbone.View
        not_found_template: Handlebars.templates['element_view-not_found-template']
        template: Handlebars.templates['full_server-template']

        events:
            'click .operations .rename': 'rename_server'

        rename_server: (event) =>
            #TODO Update when table_rename will be implemented
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'server'
            rename_modal.render()

        initialize: (id) =>
            @id = id
            @loading = true #TODO render
            @server = null

            @fetch_server()
            @interval = setInterval @fetch_server, 5000

        fetch_server: =>
            query = r.db(system_db).table('server_config').get(@id).do( (server) ->
                r.branch(
                    server.eq(null),
                    null,
                    server.merge( (server) ->
                        responsabilities: r.db('rethinkdb').table('table_status').map( (table) ->
                            table.merge( (table) ->
                                shards: table("shards").indexesOf( () -> true ).map( (index) ->
                                    table("shards").nth(index).merge({num_keys: "TODO", index: index.add(1), num_shards: table("shards").count()}).filter( (replica) ->
                                        replica('server').eq(server("name"))
                                    )
                                ).filter( (shard) ->
                                    shard.isEmpty().not()
                                ).map( (role) ->
                                    role.nth(0)
                                )
                            )
                        ).filter( (table) ->
                            table("shards").isEmpty().not()
                        ).coerceTo("ARRAY")
                    )
                )
            )

            driver.run query, (error, result) =>
                # We should call render only once to avoid blowing all the sub views
                if @loading is true
                    @loading = false
                    if result is null
                        @render()
                    else
                        @server = new Server result
                        @render()
                else
                    if @server is null
                        if result isnt null
                            @server = new Server result
                            @render()
                    else
                        if result is null
                            @server = null
                            @render()
                        else
                            @server.set result


        render: =>
            if @loading
                @$el.html @template
                    loading: @loading
                    id: @id
            else
                if @server is null
                    @$el.html @not_found_template
                else
                    #TODO Handle ghost?
                    @$el.html @template @server

                    @title = new ServerView.Title
                        model: @server
                    @$('.main_title').html @title.render().$el

                    @profile = new ServerView.Profile
                        model: @server
                    @$('.profile').html @profile.render().$el

                    ###
                    # TODO: Implement when stats will be available
                    @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance,
                        width:  564             # width in pixels
                        height: 210             # height in pixels
                        seconds: 73             # num seconds to track
                        type: 'server'
                    )
                    @.$('.performance-graph').html @performance_graph.render().$el
                    ###

                    @data = new ServerView.Data
                        model: @server
                    @$('.server-data').html @data.render().$el

                    ###
                    # TODO: Implement when logs will be available
                    @logs = new LogView.Container
                        route: "ajax/log/"+@model.get('id')+"?"
                        type: 'machine'
                    @$('.recent-log-entries').html @logs.render().$el
                    ###
            @

        destroy: =>
            clearInterval @interval
            @title.remove()
            @profile.remove()
            @data.remove()

    class @Title extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.templates['machine_view_title-template']
        initialize: ->
            @name = @model.get('name')
            @listenTo @model, 'change:name', @update

        update: =>
            if @name isnt @model.get('name')
                @name = @model.get('name')
                @render()

        render: =>
            @.$el.html @template
                name: @name
            return @

        destroy: =>
            @stopListeningobject()

    class @Profile extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.templates['machine_view_profile-template']
        initialize: =>
            @model.on 'change', @render
            @data = {}

        render: =>
            #TODO Replace with real data when server_status is available
            data =
                main_ip: '127.0.0.1'
                uptime: $.timeago(new Date())
                assigned_to_datacenter: false
                reachability:
                    reachable: true
                    last_seen: $.timeago(new Date()) #TODO make sure this fit in the html block

            if not _.isEqual @data, data
                @data = data
                @.$el.html @template @data

            return @

        destroy: =>
            directory.off 'all', @render
            @model.off 'all', @render


    #TODO Fix when server_status will be available
    # We shouldn't need @namespaces_with_listeners at that time, all the data should be
    # made available with a single query
    class @Data extends Backbone.View
        template: Handlebars.templates['server_data-template']

        initialize: =>
            @data = {}
            @listenTo @model, 'change:responsabilities', @render

        render: =>
            @.$el.html @template
                has_data: @model.get('responsabilities').length > 0
                tables: @model.get('responsabilities')

            return @

        remove: =>
            @stopListening()

# Copyright 2010-2012 RethinkDB, all rights reserved.
# Machine view
module 'ServerView', ->
    class @ServerContainer extends Backbone.View
        not_found_template: Handlebars.templates['element_view-not_found-template']
        template:
            main: Handlebars.templates['full_server-template']
            loading: Handlebars.templates['loading-template']

        events:
            'click .operations .rename': 'rename_server'

        rename_server: (event) =>
            event.preventDefault()

            if @rename_modal?
                @rename_modal.remove()
            @rename_modal = new UIComponents.RenameItemModal
                model: @server
            @rename_modal.render()

        initialize: (id) =>
            @id = id
            @loading = true #TODO render
            @server = null

            @fetch_server()
            @interval = setInterval @fetch_server, 5000

        fetch_server: =>
            query = r.db(system_db).table('server_status').get(@id).do( (server) ->
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
                                ).concatMap( (roles) -> roles )
                            )
                        ).filter( (table) ->
                            table("shards").isEmpty().not()
                        ).coerceTo("ARRAY")
                    )
                )
            ).merge
                id: r.row 'uuid'

            driver.run query, (error, result) =>
                console.log JSON.stringify(result, null, 2)
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
                @$el.html @template.loading
                    page: "server"
            else
                if @server is null
                    @$el.html @not_found_template
                else
                    #TODO Handle ghost?
                    @$el.html @template.main @server

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
            if @rename_modal?
                @rename_modal.remove()


    class @Title extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.templates['machine_view_title-template']
        initialize: =>
            @listenTo @model, 'change:name', @render

        render: =>
            @$el.html @template
                name: @model.get('name')
            @

        remove: =>
            @stopListening()

    class @Profile extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.templates['machine_view_profile-template']
        initialize: =>
            @listenTo @model, 'change', @render

        render: =>
            @$el.html @template
                main_ip: @model.get 'host'
                uptime: $.timeago(@model.get('time_started')).slice(0, -4)
                num_shards: @model.get('responsabilities').length
                reachability:
                    reachable: @model.get('status') is 'available'
                    last_seen: $.timeago(@model.get('time_disconnected')).slice(0, -4) if @model.get('status') isnt 'available'
            @

        remove: =>
            @stopListening()


    class @Data extends Backbone.View
        template: Handlebars.templates['server_data-template']

        initialize: =>
            @data = {}
            @listenTo @model, 'change:responsabilities', @render

        render: =>
            @$el.html @template
                has_data: @model.get('responsabilities').length > 0
                tables: @model.get('responsabilities')

            return @

        remove: =>
            @stopListening()

# Copyright 2010-2012 RethinkDB, all rights reserved.
# Machine view
module 'ServerView', ->
    class @ServerContainer extends Backbone.View
        template:
            loading: Handlebars.templates['loading-template']
            error: Handlebars.templates['error-query-template']
            not_found: Handlebars.templates['element_view-not_found-template']

        initialize: (id) =>
            @id = id
            @loading = true #TODO render
            @server = null

            @fetch_server()

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
                        ).merge( (table) ->
                            id: table("uuid")
                        ).coerceTo("ARRAY")
                    )
                )
            ).merge
                id: r.row 'uuid'

            @timer = driver.run query, 5000, (error, result) =>
                # We should call render only once to avoid blowing all the sub views
                if error?
                    @error = error
                    @render()
                else
                    @error = null
                    if result is null
                        if @loading is true
                            @loading = false
                            @render()
                        else if @model isnt null
                            #TODO Test
                            @server = null
                            @render()
                    else
                        @loading = false
                        if not @server?
                            @server = new Server result
                            @server_view = new ServerView.ServerMainView
                                model: @server
                            @render()
                        else
                            @server.set result

        render: =>
            if @error?
                @$el.html @template.error
                    error: @error?.message
                    url: '#servers/'+@id
            else if @loading is true
                @$el.html @template.loading
                    page: "server"
            else
                if @server_view?
                    @$el.html @server_view.render().$el
                else # In this case, the query returned null, so the server
                    @$el.html @template.not_found
                        id: @id
                        type: 'server'
                        type_url: 'servers'
                        type_all_url: 'servers'
            @

        remove: =>
            driver.stop_timer @timer
            @server_view?.remove()
            super()

    class @ServerMainView extends Backbone.View
        template:
            main: Handlebars.templates['full_server-template']

        events:
            'click .close': 'close_alert'
            'click .operations .rename': 'rename_server'

        rename_server: (event) =>
            event.preventDefault()

            if @rename_modal?
                @rename_modal.remove()
            @rename_modal = new UIComponents.RenameItemModal
                model: @model
            @rename_modal.render()

        # Method to close an alert/warning/arror
        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        initialize: =>
            @title = new ServerView.Title
                model: @model

            @profile = new ServerView.Profile
                model: @model

            @stats = new Stats
            @stats_timer = driver.run r.expr(
                keys_read: r.random(2000, 3000)
                keys_set: r.random(1500, 2500)
            ), 1000, @stats.on_result

            @performance_graph = new Vis.OpsPlot(@stats.get_stats,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'server'
            )

            @data = new ServerView.Data
                model: @model

        render: =>
            console.log 'render'
            #TODO Handle ghost?
            @$el.html @template.main()

            @$('.main_title').html @title.render().$el
            @$('.profile').html @profile.render().$el
            @$('.performance-graph').html @performance_graph.render().$el
            @$('.server-data').html @data.render().$el

            # TODO: Implement when logs will be available
            #@logs = new LogView.Container
            #    route: "ajax/log/"+@model.get('id')+"?"
            #    type: 'machine'
            #@$('.recent-log-entries').html @logs.render().$el
            @

        remove: =>
            driver.stop_timer @stats_timer
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
            super()

    class @Profile extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.templates['machine_view_profile-template']
        initialize: =>
            @listenTo @model, 'change', @render

        render: =>
            # TODO Try with a release/clean version
            version = @model.get('version').split(' ')[1].split('-')[0]
            @$el.html @template
                main_ip: @model.get 'hostname'
                uptime: $.timeago(@model.get('time_started')).slice(0, -4)
                version: version
                num_shards: @model.get('responsabilities').length
                reachability:
                    reachable: @model.get('status') is 'available'
                    last_seen: $.timeago(@model.get('time_disconnected')).slice(0, -4) if @model.get('status') isnt 'available'
            @

        remove: =>
            @stopListening()
            super()


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
            super()

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

                        responsabilities = []
                        for table in result.responsabilities
                            responsabilities.push new Responsability
                                type: "table"
                                is_table: true
                                db: table.db
                                table: table.name
                                id: table.db+"."+table.name

                            for shard in table.shards
                                responsabilities.push new Responsability
                                    is_shard: true
                                    db: table.db
                                    table: table.name
                                    index: shard.index
                                    num_shards: shard.num_shards
                                    role: shard.role
                                    num_keys: shard.num_keys
                                    id: table.db+"."+table.name+"."+shard.index

                        if not @responsabilities?
                            @responsabilities = new Responsabilities responsabilities
                        else
                            @responsabilities.set responsabilities
                        delete result.responsabilities

                        if not @server?
                            @server = new Server result
                            @server_view = new ServerView.ServerMainView
                                model: @server
                                collection: @responsabilities

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
                collection: @collection

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

            @responsabilities = new ServerView.ResponsabilitiesList
                collection: @collection

        render: =>
            #TODO Handle ghost?
            @$el.html @template.main()

            @$('.main_title').html @title.render().$el
            @$('.profile').html @profile.render().$el
            @$('.performance-graph').html @performance_graph.render().$el
            @$('.responsabilities').html @responsabilities.render().$el

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
            @responsabilities.remove()
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
            @listenTo @collection, 'add', @render
            @listenTo @collection, 'remove', @render

        render: =>
            # TODO Try with a release/clean version
            version = @model.get('version').split(' ')[1].split('-')[0]
            @$el.html @template
                main_ip: @model.get 'hostname'
                uptime: $.timeago(@model.get('time_started')).slice(0, -4)
                version: version
                num_shards: @collection.length
                reachability:
                    reachable: @model.get('status') is 'available'
                    last_seen: $.timeago(@model.get('time_disconnected')).slice(0, -4) if @model.get('status') isnt 'available'
            @

        remove: =>
            @stopListening()
            super()


    class @ResponsabilitiesList extends Backbone.View
        template: Handlebars.templates['responsabilities-template']

        initialize: =>
            @responsabilities_view = []

            @$el.html @template

            @collection.each (responsability) =>
                view = new ServerView.ResponsabilityView
                    model: responsability
                    container: @
                # The first time, the collection is sorted
                @responsabilities_view.push view
                @$('.responsabilities_list').append view.render().$el

            if @responsabilities_view.length > 0
                @$('.no_element').hide()

            @listenTo @collection, 'add', (responsability) =>
                new_view = new ServerView.ResponsabilityView
                    model: responsability
                    container: @

                if @responsabilities_view.length is 0
                    @responsabilities_view.push new_view
                    @$('.responsabilities_list').html new_view.render().$el
                else
                    added = false
                    for view, position in @responsabilities_view
                        if Responsabilities.prototype.comparator(view.model, responsability) > 0
                            added = true
                            @responsabilities_view.splice position, 0, new_view
                            if position is 0
                                @$('.responsabilities_list').prepend new_view.render().$el
                            else
                                @$('.responsability_container').eq(position-1).after new_view.render().$el
                            break
                    if added is false
                        @responsabilities_view.push new_view
                        @$('.responsabilities_list').append new_view.render().$el

                if @responsabilities_view.length > 0
                    @$('.no_element').hide()

            @listenTo @collection, 'remove', (responsability) =>
                for view, position in @responsabilities_view
                    if view.model is responsability
                        responsability.destroy()
                        view.remove()
                        @responsabilities_view.splice position, 1
                        break

                if @responsabilities_view.length is 0
                    @$('.no_element').show()



        render: =>
            @

        remove: =>
            @stopListening()
            for view in @responsabilities_view
                view.model.destroy()
                view.remove()
            super()


    class @ResponsabilityView extends Backbone.View
        template: Handlebars.templates['responsability-template']

        initialize: =>
            @listenTo @model, 'change', @render

        render: =>
            @$el.html @template @model.toJSON()
            @

        remove: =>
            @stopListening()
            super()

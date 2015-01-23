# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'ServersView', ->
    class @ServersContainer extends Backbone.View
        id: 'servers_container'
        template:
            main: Handlebars.templates['servers_container-template']

        initialize: =>
            if not window.view_data_backup.servers_view_servers?
                window.view_data_backup.servers_view_servers = new Servers
                @loading = true
            else
                @loading = false
            @servers = window.view_data_backup.servers_view_servers
        
            @servers_list = new ServersView.ServersListView
                collection: @servers

            @fetch_servers()

        render: =>
            @$el.html @template.main({})
            @$('.servers_list').html @servers_list.render().$el
            @


        fetch_servers: =>
            query = r.db(system_db).table('server_config').coerceTo('array').do(
                r.db(system_db).table('table_config').coerceTo('array'),
                (server_config, table_config) ->
                    r.db(system_db).table('server_status').merge( (server) ->
                        id: server("id")
                        tags: server_config.filter(id: server('id'))(0)('tags')
                        primary_count:
                            table_config.concatMap( (table) -> table("shards") )
                            .count((shard) ->
                                shard("primary_replica").eq(server("name")))
                        secondary_count:
                            table_config.concatMap((table) -> table("shards"))
                            .filter((shard) ->
                                shard("primary_replica").ne(server("name")))
                            .concatMap((shard) -> shard("replicas"))
                            .count((replica) -> replica.eq(server("name")))
                )
            )
            @timer = driver.run query, 5000, (error, result) =>
                if error?
                    console.log error
                    return
                ids = {}
                for server, index in result
                    @servers.add new Server(server)
                    ids[server.id] = true

                # Clean  removed servers
                toDestroy = []
                for server in @servers.models
                    if ids[server.get('id')] isnt true
                        toDestroy.push server
                for server in toDestroy
                    server.destroy()

                @render()

        remove: =>
            driver.stop_timer @timer
            @servers_list.remove()
            super()

    class @ServersListView extends Backbone.View
        className: 'servers_view'
        tagName: 'tbody'
        template:
            loading_servers: Handlebars.templates['loading_servers-template']
        initialize: =>
            @servers_view = []

            @collection.each (server) =>
                view = new ServersView.ServerView
                    model: server
                @servers_view.push view
                @$el.append view.render().$el


            @listenTo @collection, 'add', (server) =>
                new_view = new ServersView.ServerView
                    model: server

                if @servers_view.length is 0
                    @servers_view.push new_view
                    @$el.html new_view.render().$el
                else
                    added = false
                    for view, position in @servers_view
                        if view.model.get('name') > server.get('name')
                            added = true
                            @servers_view.splice position, 0, new_view
                            if position is 0
                                @$el.prepend new_view.render().$el
                            else
                                @$('.server_container').eq(position-1).after new_view.render().$el
                            break
                    if added is false
                        @servers_view.push new_view
                        @$el.append new_view.render().$el

            #TODO Test when we can remove a server from the API
            @listenTo @collection, 'remove', (server) =>
                for view in @servers_view
                    if view.model is server
                        server.destroy()
                        view.remove()
                        break

        render: =>
            if @servers_view.length is 0
                # No servers means we are probably loading
                @$el.append @template.loading_servers()
            else
                for server_view in @servers_view
                    @$el.append server_view.render().$el
            @

        remove: =>
            @stopListening()
            for view in @servers_view
                view.remove()
            super()

    class @ServerView extends Backbone.View
        className: 'server_container'
        tagName: 'tr'
        template: Handlebars.templates['server-template']
        initialize: =>
            @listenTo @model, 'change', @render

        render: ->
            @$el.html @template @model.toJSON()
            @

        remove: =>
            @stopListening()

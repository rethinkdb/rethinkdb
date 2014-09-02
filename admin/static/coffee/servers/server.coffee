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
            query = r.db(system_db).table('server_config').get(@id)

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

                    @title = new ServerView.Title model: @server
                    @$('.main_title').html @title.render().$el

                    @profile = new ServerView.Profile model: @server
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

                    @data = new ServerView.Data model: @server
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
        template: Handlebars.templates['machine_view_data-template']

        initialize: =>
            @namespaces_with_listeners = {}

            namespaces.on 'change:blueprint', @render
            namespaces.on 'change:key_distr', @render
            namespaces.each (namespace) -> namespace.load_key_distr()
            @data = {}

        render: =>
            data_by_namespace = []
                    
            # Examine each namespace and collect info on its shards / attach listeners to count the number of keys
            namespaces.each (namespace) =>
                ns =
                    name: namespace.get('name')
                    id: namespace.get('id')
                    shards: []
                # Examine each machine's role for the namespace-- only consider namespaces that actually use this machine
                for machine_id, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_id is @model.get('id')
                        # Build up info on each shard present on this machine for this namespace
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                keys = namespace.compute_shard_rows_approximation shard
                                json_shard = $.parseJSON(shard)
                                ns.shards.push
                                    name: human_readable_shard shard
                                    shard: human_readable_shard_obj shard
                                    num_keys: keys
                                    role: role
                                    secondary: role is 'role_secondary'
                                    primary: role is 'role_primary'

                # Finished building, add it to the list (only if it has shards on this server)
                data_by_namespace.push ns if ns.shards.length > 0

            data =
                has_data: data_by_namespace.length > 0
                # Sort the tables alphabetically by name
                tables: _.sortBy(data_by_namespace, (namespace) -> namespace.name)

            if not _.isEqual data, @data
                @data = data
                @.$el.html @template @data

            return @

        destroy: =>
            namespaces.off 'change:blueprint', @render
            namespaces.off 'change:key_distr', @render

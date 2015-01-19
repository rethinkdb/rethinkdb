# Copyright 2010-2012 RethinkDB, all rights reserved.

module 'DashboardView', ->
    # DashboardContainer is responsible to retrieve all the data displayed
    # on the dashboard.
    class @DashboardContainer extends Backbone.View
        id: 'dashboard_container'
        template:
            error: Handlebars.templates['error-query-template']

        initialize: =>
            if not window.view_data_backup.dashboard_view_dashboard?
                window.view_data_backup.dashboard_view_dashboard = new Dashboard
            @dashboard = window.view_data_backup.dashboard_view_dashboard

            @dashboard_view = new DashboardView.DashboardMainView
                model: @dashboard

            @fetch_data()

        fetch_data: =>
            # We need to flatten a few sequences with identity
            # So we store the function here to make the code a bit shorter
            identity = (a) -> a

            system_table = (name) -> r.db(system_db).table(name)
            table_status = system_table('table_status')
            table_config_id = r.db(system_db).table(
                'table_config', identifierFormat: 'uuid')
            server_config = system_table('server_config')
            server_status = system_table('server_status')

            tables_with_primaries_not_ready = table_status.map(
                table_config_id, (status, config) ->
                    id: status('id')
                    name: status('name')
                    db: status('db')
                    shards: status('shards').map(
                        r.range(), config('shards'), (shard, pos, conf_shard) ->
                            primary_id = conf_shard('primary_replica')
                            primary_name = server_config.get(primary_id)('name')
                            position: pos.add(1)
                            num_shards: status('shards').count()
                            primary_id: primary_id
                            primary_name: primary_name
                            primary_state: shard('replicas').filter(
                                server: primary_name
                            )('state')(0)
                    ).filter((shard) ->
                        r.expr(['ready', 'looking_for_primary_replica'])
                            .contains(shard('primary_state')).not()
                    ).coerceTo('array')
                ).filter((table) -> table('shards').isEmpty().not())
                .coerceTo('array')
            tables_with_replicas_not_ready = table_status.map(
                table_config_id, (status, config) ->
                    id: status('id')
                    name: status('name')
                    db: status('db')
                    shards: status('shards').map(
                        r.range(), config('shards'), (shard, pos, conf_shard) ->
                            position: pos.add(1)
                            num_shards: status('shards').count(),
                            replicas: shard('replicas')
                                .filter((replica) ->
                                    r.expr(['ready',
                                        'looking_for_primary_replica',
                                        'offloading_data'])
                                        .contains(replica('state')).not()
                                ).map(conf_shard('replicas'), (replica, replica_id) ->
                                    replica_id: replica_id
                                    replica_name: replica('server')
                                ).coerceTo('array')
                    ).coerceTo('array')
            ).filter((table) -> table('shards')(0)('replicas').isEmpty().not())
            .coerceTo('array')
            num_primaries = table_config_id('shards')
                .concatMap(identity)('primary_replica').count()
            num_connected_primaries = table_status.concatMap((table) ->
                table('shards')('primary_replica')
            ).count((primary) -> primary.ne(null))
            num_replicas = table_config_id('shards')
                .concatMap((shard) -> shard("replicas"))
                .concatMap(identity).count()
            num_connected_replicas = table_status('shards')
                .concatMap((shard) ->
                    shard('replicas').concatMap((replica) -> replica('state')))
                .count((replica) ->
                    r.expr(['ready', 'looking_for_primary_replica']).contains(replica))
            disconnected_servers = server_status.filter((server) ->
                    server("status").ne("connected")
            ).map((server) ->
                time_disconnected: server('connection')('time_disconnected')
                name: server('name')
            ).coerceTo('array')
            num_disconnected_tables = table_status.count((table) ->
                shard_is_down = (shard) -> shard('primary_replica').eq(null)
                table('shards').map(shard_is_down).contains(true)
            )
            num_tables_w_missing_replicas = table_status.count((table) ->
                table('status')('all_replicas_ready').not()
            )
            num_connected_servers = server_status.count((server) ->
                server('status').eq("connected")
            )


            query = r.expr(
                num_primaries: num_primaries
                num_connected_primaries: num_connected_primaries
                num_replicas: num_replicas
                num_connected_replicas: num_connected_replicas
                tables_with_primaries_not_ready: tables_with_primaries_not_ready
                tables_with_replicas_not_ready: tables_with_replicas_not_ready
                num_tables: table_config_id.count()
                num_servers: server_status.count()
                num_connected_servers: num_connected_servers
                disconnected_servers: disconnected_servers
                num_disconnected_tables: num_disconnected_tables
                num_tables_w_missing_replicas: num_tables_w_missing_replicas
            )

            @timer = driver.run query, 5000, (error, result) =>
                if error?
                    #TODO
                    console.log error
                    @error = error
                    @render()
                else
                    rerender = @error?
                    @error = null
                    @dashboard.set result
                    if rerender
                        @render()

        render: =>
            if @error?
                @$el.html @template.error
                    error: @error?.message
                    url: '#'
            else
                @$el.html @dashboard_view.render().$el
            @

        remove: =>
            driver.stop_timer @timer
            if @dashboard_view
                @dashboard_view.remove()
            super()

    class @DashboardMainView extends Backbone.View
        template: Handlebars.templates['dashboard_view-template']
        id: 'dashboard_main_view'

        events:
            'click .view-logs': 'show_all_logs'

        initialize: =>
            @cluster_status_availability = new DashboardView.ClusterStatusAvailability
                model: @model
            @cluster_status_redundancy = new DashboardView.ClusterStatusRedundancy
                model: @model
            @cluster_status_connectivity = new DashboardView.ClusterStatusConnectivity
                model: @model

            @stats = new Stats
            @stats_timer = driver.run(
                r.db(system_db)
                .table('stats').get(['cluster'])
                .do((stat) ->
                    keys_read: stat('query_engine')('read_docs_per_sec')
                    keys_set: stat('query_engine')('written_docs_per_sec')
                ), 1000, @stats.on_result)

            @cluster_performance = new Vis.OpsPlot(@stats.get_stats,
                width:  833             # width in pixels
                height: 300             # height in pixels
                seconds: 119            # num seconds to track
                type: 'cluster'
            )

            @logs = new LogView.LogsContainer
                limit: 5
                query: driver.queries.all_logs


        show_all_logs: ->
            main_view.router.navigate '#logs',
                trigger: true

        render: =>
            @$el.html @template({})
            @$('.availability').html @cluster_status_availability.render().$el
            @$('.redundancy').html @cluster_status_redundancy.render().$el
            @$('.connectivity').html @cluster_status_connectivity.render().$el

            @$('#cluster_performance_container').html @cluster_performance.render().$el
            @$('.recent-log-entries-container').html @logs.render().$el

            return @

        remove: =>
            driver.stop_timer @stats_timer

            @cluster_status_availability.remove()
            @cluster_status_redundancy.remove()
            @cluster_status_connectivity.remove()
            @cluster_performance.remove()
            @logs.remove()
            super()

    class @ClusterStatusAvailability extends Backbone.View
        className: 'cluster-status-availability '

        template: Handlebars.templates['dashboard_availability-template']

        events:
            'click .show_details': 'show_popup'
            'click .close': 'hide_popup'

        initialize: =>
            # We could eventually properly create a collection from @model.get('shards')
            # But this is probably not worth the effort for now.

            @listenTo @model, 'change:num_primaries', @render
            @listenTo @model, 'change:num_connected_primaries', @render

            $(window).on 'mouseup', @hide_popup
            @$el.on 'click', @stop_propagation


            @display_popup = false
            @margin = {}

        stop_propagation: (event) ->
            event.stopPropagation()

        show_popup: (event) =>
            if event?
                event.preventDefault()

                @margin.top = event.pageY-60-13
                @margin.left = event.pageX+12

            @$('.popup_container').show()
            @$('.popup_container').css 'margin', @margin.top+'px 0px 0px '+@margin.left+'px'
            @display_popup = true

        hide_popup: (event) =>
            event.preventDefault()
            @display_popup = false
            @$('.popup_container').hide()

        render: =>
            template_model =
                status_is_ok: @model.get('num_connected_primaries') is @model.get('num_primaries')
                num_primaries: @model.get 'num_primaries'
                num_connected_primaries: @model.get 'num_connected_primaries'
                num_disconnected_primaries: @model.get('num_primaries')-@model.get('num_connected_primaries')
                num_disconnected_tables: @model.get 'num_disconnected_tables'
                num_tables: @model.get 'num_tables'
                tables_with_primaries_not_ready: @model.get('tables_with_primaries_not_ready')
            @$el.html @template template_model

            if @display_popup is true and @model.get('num_connected_primaries') isnt @model.get('num_primaries')
                # We re-display the pop up only if there are still issues
                @show_popup()

            @

        remove: =>
            @stopListening()
            $(window).off 'mouseup', @remove_popup
            super()

    class @ClusterStatusRedundancy extends Backbone.View
        className: 'cluster-status-redundancy'

        template: Handlebars.templates['dashboard_redundancy-template']

        events:
            'click .show_details': 'show_popup'
            'click .close': 'hide_popup'

        initialize: =>
            # We could eventually properly create a collection from @model.get('shards')
            # But this is probably not worth the effort for now.

            @listenTo @model, 'change:num_replicas', @render
            @listenTo @model, 'change:num_connected_replicas', @render

            $(window).on 'mouseup', @hide_popup
            @$el.on 'click', @stop_propagation


            @display_popup = false
            @margin = {}

        stop_propagation: (event) ->
            event.stopPropagation()

        show_popup: (event) =>
            if event?
                event.preventDefault()

                @margin.top = event.pageY-60-13
                @margin.left = event.pageX+12

            @$('.popup_container').show()
            @$('.popup_container').css 'margin', @margin.top+'px 0px 0px '+@margin.left+'px'
            @display_popup = true

        hide_popup: (event) =>
            event.preventDefault()
            @display_popup = false
            @$('.popup_container').hide()

        render: =>
            #TODO: Do we have to handle unsatisfiable goals here?
            @$el.html @template
                status_is_ok: @model.get('num_connected_replicas') is @model.get('num_replicas')
                num_replicas: @model.get 'num_replicas'
                num_connected_replicas: @model.get 'num_available_replicas'
                num_disconnected_replicas: @model.get('num_replicas')-@model.get('num_connected_replicas')
                num_disconnected_tables: @model.get 'num_disconnected_tables'
                num_tables: @model.get 'num_tables'
                tables_with_replicas_not_ready: @model.get('tables_with_replicas_not_ready')

            if @display_popup is true and @model.get('num_connected_replicas') isnt @model.get('num_replicas')
                # We re-display the pop up only if there are still issues
                @show_popup()

            @

        remove: =>
            @stopListening()
            $(window).off 'mouseup', @remove_popup
            super()

    class @ClusterStatusConnectivity extends Backbone.View
        className: 'cluster-status-connectivity '

        template: Handlebars.templates['dashboard_connectivity-template']

        events:
            'click .show_details': 'show_popup'
            'click .close': 'hide_popup'

        initialize: =>
            # We could eventually properly create a collection from @model.get('shards')
            # But this is probably not worth the effort for now.

            @listenTo @model, 'change:num_servers', @render
            @listenTo @model, 'change:num_connected_servers', @render

            $(window).on 'mouseup', @hide_popup
            @$el.on 'click', @stop_propagation


            @display_popup = false
            @margin = {}

        stop_propagation: (event) ->
            event.stopPropagation()

        show_popup: (event) =>
            if event?
                event.preventDefault()

                @margin.top = event.pageY-60-13
                @margin.left = event.pageX+12

            @$('.popup_container').show()
            @$('.popup_container').css 'margin', @margin.top+'px 0px 0px '+@margin.left+'px'
            @display_popup = true

        hide_popup: (event) =>
            event.preventDefault()
            @display_popup = false
            @$('.popup_container').hide()

        render: =>
            template_model =
                status_is_ok: @model.get('num_connected_servers') is @model.get('num_servers')
                num_servers: @model.get 'num_servers'
                num_disconnected_servers: @model.get('num_servers')-@model.get('num_connected_servers')
                num_connected_servers: @model.get 'num_connected_servers'
                disconnected_servers: @model.get 'disconnected_servers'
            @$el.html @template template_model

            if @display_popup is true and @model.get('num_connected_servers') isnt @model.get('num_servers')
                # We re-display the pop up only if there are still issues
                @show_popup()

            @

        remove: =>
            @stopListening()
            $(window).off 'mouseup', @remove_popup
            super()

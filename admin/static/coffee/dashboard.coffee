# Copyright 2010-2012 RethinkDB, all rights reserved.

module 'DashboardView', ->
    # DashboardContainer is responsaible to retrieve all the data displayed
    # on the dashboard.
    class @DashboardContainer extends Backbone.View
        id: 'dashboard_container'
        template:
            loading: Handlebars.templates['loading-template']
            error: Handlebars.templates['error-query-template']

        initialize: =>
            @loading = true

            @fetch_data()

        fetch_data: =>
            identity = (a) -> a

            query = r.do(
                r.db(system_db).table('table_config').coerceTo("ARRAY"),
                r.db(system_db).table('table_status').coerceTo("ARRAY"),
                r.db(system_db).table('server_status').coerceTo("ARRAY"),
                (table_config, table_status, server_status) ->
                    num_directors: table_config('shards').concatMap(identity)("director").count()
                    num_available_directors: table_status('shards')
                        .concatMap(identity)
                        .concatMap(identity)
                        .filter({role: "director", state: "ready"})
                        .count()
                    num_replicas: table_config('shards').concatMap(identity)("replicas").concatMap(identity).count()
                    num_available_replicas: table_status('shards')
                        .concatMap(identity)
                        .concatMap(identity)
                        .filter( (assignment) ->
                            assignment("state").eq("ready").and(
                                assignment("role").eq("replica").or(assignment("role").eq("director"))
                            )
                        ).count()
                    tables_with_directors_not_ready: table_status.merge( (table) ->
                        shards: table("shards").indexesOf( () -> true ).map( (position) ->
                            table("shards").nth(position).merge
                                id: r.add(
                                    table("db"),
                                    ".",
                                    table("name"),
                                    ".",
                                    position.add(1).coerceTo("STRING")
                                )
                                position: position.add(1)
                                num_shards: table("shards").count()
                        ).map( (shard) ->
                            shard.filter (assignment) ->
                                assignment("role").eq("director").and(assignment("state").ne("ready"))
                        ).concatMap identity
                    ).filter (table) ->
                        table("shards").isEmpty().not()
                    tables_with_replicas_not_ready: table_status.merge( (table) ->
                        shards: table("shards").indexesOf( () -> true ).map( (position) ->
                            table("shards").nth(position).merge
                                id: r.add(
                                    table("db"),
                                    ".",
                                    table("name"),
                                    ".",
                                    position.add(1).coerceTo("STRING")
                                )
                                position: position.add(1)
                                num_shards: table("shards").count()
                        ).map( (shard) ->
                            shard.filter (assignment) ->
                                assignment("role").eq("replica").and(assignment("state").ne("ready"))
                        ).concatMap identity
                    ).filter (table) ->
                        table("shards").isEmpty().not()

                    num_tables: table_config.count()
                    num_servers: server_status.count()
                    num_available_servers: server_status.filter({status: "available"}).count()
                    servers_non_available: server_status.filter (server) ->
                        server("status").ne("available")
            ).merge
                num_non_available_tables: r.row("tables_with_directors_not_ready").count()

            @timer = driver.run query, 5000, (error, result) =>
                if error?
                    #TODO
                    console.log error
                    @error = error
                    @render()
                else
                    @error = null
                    @loading = false
                    if @dashboard?
                        @dashboard.set result
                    else
                        @dashboard = new Dashboard result
                        @dashboard_view = new DashboardView.DashboardMainView
                            model: @dashboard
                        @render()

        render: =>
            if @error?
                @$el.html @template.error
                    error: @error?.message
                    url: '#'
            else if @loading is true
                @$el.html @template.loading
                    page: "dashboard"
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
            @cluster_status_reachability = new DashboardView.ClusterStatusReachability
                model: @model
            @cluster_status_consistency = new DashboardView.ClusterStatusConsistency
                model: @model

            @stats = new Stats(r.expr(
                keys_read: r.random(2000, 3000)
                keys_set: r.random(1500, 2500)
            ))
            @cluster_performance = new Vis.OpsPlot(@stats.get_stats,
                width:  833             # width in pixels
                height: 300             # height in pixels
                seconds: 119            # num seconds to track
                type: 'cluster'
            )

            ###
            @logs = new DashboardView.Logs()
            ###

        show_all_logs: ->
            window.router.navigate '#logs',
                trigger: true

        render: =>
            @$el.html @template({})
            @$('.availability').html @cluster_status_availability.render().$el
            @$('.redundancy').html @cluster_status_redundancy.render().$el
            @$('.reachability').html @cluster_status_reachability.render().$el
            @$('.consistency').html @cluster_status_consistency.render().$el

            @$('#cluster_performance_container').html @cluster_performance.render().$el
            #@$('.recent-log-entries-container').html @logs.render().$el

            return @

        remove: =>
            @cluster_status_availability.remove()
            @cluster_status_redundancy.remove()
            @cluster_status_reachability.remove()
            @cluster_status_consistency.remove()
            @cluster_performance.remove()
            #@logs.remove()
            @stats.destroy()
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

            @listenTo @model, 'change:num_directors', @render
            @listenTo @model, 'change:num_available_directors', @render

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
            @display_popup = false
            @$('.popup_container').hide()

        render: =>
            @$el.html @template
                status_is_ok: @model.get('num_available_directors') is @model.get('num_directors')
                num_directors: @model.get 'num_directors'
                num_available_directors: @model.get 'num_available_directors'
                num_non_available_directors: @model.get('num_directors')-@model.get('num_available_directors')
                num_non_available_tables: @model.get 'num_non_available_tables'
                num_tables: @model.get 'num_tables'
                tables_with_directors_not_ready: @model.get('tables_with_directors_not_ready')

            if @display_popup is true and @model.get('num_available_directors') isnt @model.get('num_directors')
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
            @listenTo @model, 'change:num_available_replicas', @render

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
            @display_popup = false
            @$('.popup_container').hide()

        render: =>
            #TODO: Do we have to handle unsatisfiable goals here?
            @$el.html @template
                status_is_ok: @model.get('num_available_replicas') is @model.get('num_replicas')
                num_replicas: @model.get 'num_replicas'
                num_available_replicas: @model.get 'num_available_replicas'
                num_non_available_replicas: @model.get('num_replicas')-@model.get('num_available_replicas')
                num_non_available_tables: @model.get 'num_non_available_tables'
                num_tables: @model.get 'num_tables'
                tables_with_replicas_not_ready: @model.get('tables_with_replicas_not_ready')

            if @display_popup is true and @model.get('num_available_replicas') isnt @model.get('num_replicas')
                # We re-display the pop up only if there are still issues
                @show_popup()

            @

        remove: =>
            @stopListening()
            $(window).off 'mouseup', @remove_popup
            super()

    class @ClusterStatusReachability extends Backbone.View
        className: 'cluster-status-reachability '

        template: Handlebars.templates['dashboard_reachability-template']

        events:
            'click .show_details': 'show_popup'
            'click .close': 'hide_popup'

        initialize: =>
            # We could eventually properly create a collection from @model.get('shards')
            # But this is probably not worth the effort for now.

            @listenTo @model, 'change:num_servers', @render
            @listenTo @model, 'change:num_available_servers', @render

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
            @display_popup = false
            @$('.popup_container').hide()

        render: =>
            @$el.html @template
                status_is_ok: @model.get('num_available_servers') is @model.get('num_servers')
                num_servers: @model.get 'num_servers'
                num_servers_non_available: @model.get('num_servers')-@model.get('num_available_servers')
                num_available_servers: @model.get 'num_available_servers'
                servers_non_available: @model.get 'servers_non_available'

            if @display_popup is true and @model.get('num_available_servers') isnt @model.get('num_servers')
                # We re-display the pop up only if there are still issues
                @show_popup()

            @

        remove: =>
            @stopListening()
            $(window).off 'mouseup', @remove_popup
            super()

    class @ClusterStatusConsistency extends Backbone.View
        className: 'cluster-status-consistency'
        template: Handlebars.templates['cluster_status-consistency_status-template']

        events:
            'click .show_details': 'show_popup'
            'click .close': 'hide_popup'

        initialize: =>
            # We could eventually properly create a collection from @model.get('shards')
            # But this is probably not worth the effort for now.

            @listenTo @model, 'change:num_servers', @render
            @listenTo @model, 'change:num_available_servers', @render

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
            @display_popup = false
            @$('.popup_container').hide()

        render: =>
            @$el.html @template
                has_conflicts: false
                num_tables: 0

            if @display_popup is true and @model.get('num_issues') > 0
                # We re-display the pop up only if there are still issues
                @show_popup()

            @


        remove: =>
            @stopListening()
            $(window).off 'mouseup', @remove_popup
            super()

    class @Logs extends Backbone.View
        className: 'log-entries'
        tagName: 'ul'
        min_timestamp: 0
        max_entry_logs: 5
        interval_update_log: 10000
        compact_entries: true

        initialize: ->
            @fetch_log()
            @interval = setInterval @fetch_log, @interval_update_log
            @log_entries = []

        fetch_log: =>
            $.ajax({
                contentType: 'application/json'
                url: 'ajax/log/_?max_length='+@max_entry_logs+'&min_timestamp='+@min_timestamp
                dataType: 'json'
                success: @set_log_entries
            })

        set_log_entries: (response) =>
            need_render = false
            for machine_id, data of response
                for new_log_entry in data
                    for old_log_entry, i in @log_entries
                        if parseFloat(new_log_entry.timestamp) > parseFloat(old_log_entry.get('timestamp'))
                            entry = new LogEntry new_log_entry
                            entry.set('machine_uuid', machine_id)
                            @log_entries.splice i, 0, entry
                            need_render = true
                            break

                    if @log_entries.length < @max_entry_logs
                        entry = new LogEntry new_log_entry
                        entry.set('machine_uuid', machine_id)
                        @log_entries.push entry
                        need_render = true
                    else if @log_entries.length > @max_entry_logs
                        @log_entries.pop()

            if need_render
                @render()

            if @log_entries[0]? and _.isNaN(parseFloat(@log_entries[0].get('timestamp'))) is false
                @min_timestamp = parseFloat(@log_entries[0].get('timestamp'))+1

        render: =>
            @$el.html ''
            for log in @log_entries
                view = new LogView.LogEntry model: log
                @$el.append view.render(@compact_entries).$el
            return @

        remove: =>
            clearInterval @interval
            super()

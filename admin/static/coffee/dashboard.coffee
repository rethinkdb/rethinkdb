# Copyright 2010-2012 RethinkDB, all rights reserved.
# Dashboard: provides an overview and visualizations of the cluster
# Dashboard View
module 'DashboardView', ->
    # Cluster.Container
    class @DashboardContainer extends Backbone.View
        template:
            loading: Handlebars.templates['loading-template']
            error: Handlebars.templates['error-query-template']

        initialize: =>
            @loading = true

            @fetch_data()
            @interval = setInterval @fetch_data, 5000

        fetch_data: =>
            identity = (a) -> a

            query = r.do(
                r.db(system_db).table('table_config').coerceTo("ARRAY"),
                r.db(system_db).table('table_status').coerceTo("ARRAY"),
                r.db(system_db).table('server_status').coerceTo("ARRAY"),
                (table_config, table_status, server_status) ->
                    num_directors: table_config('shards').concatMap(identity)("directors").count()
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
                        ).filter( (shard) ->
                            shard.contains (assignment) ->
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
                        ).filter( (shard) ->
                            shard.contains (assignment) ->
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

            driver.run query, (error, result) =>
                ###
                console.log '----- err, result ------'
                console.log error
                console.log result
                ###
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
                @$el.html @template.loading().$el
            else
                @$el.html @dashboard_view.render().$el
            @

        remove: =>
            clearInterval @interval

    class @DashboardMainView extends Backbone.View
        template: Handlebars.templates['dashboard_view-template']
        id: 'dashboard_container'

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

            ###
            @cluster_performance = new Vis.OpsPlot(computed_cluster.get_stats,
                width:  833             # width in pixels
                height: 300             # height in pixels
                seconds: 119            # num seconds to track
                type: 'cluster'
            )

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

            #@.$('#cluster_performance_container').html @cluster_performance.render().$el
            #@.$('.recent-log-entries-container').html @logs.render().$el

            return @

        destroy: =>
            @cluster_status_availability.destroy()
            @cluster_status_redundancy.destroy()
            @cluster_status_reachability.destroy()
            @cluster_status_consistency.destroy()
            @cluster_performance.destroy()
            @logs.destroy()

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
            @stopListeningTo()
            $(window).off 'mouseup', @remove_popup()

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
            @stopListeningTo()
            $(window).off 'mouseup', @remove_popup()

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
            @stopListeningTo()
            $(window).off 'mouseup', @remove_popup()

    class @ClusterStatusConsistency extends Backbone.View
        className: 'cluster-status-consistency'

        template: Handlebars.templates['cluster_status-container-template']
        status_template: Handlebars.templates['cluster_status-consistency_status-template']
        popup_template: Handlebars.templates['cluster_status-consistency-popup-template']

        events:
            'click .show_details': 'show_details'
            'click .close': 'hide_details'

        initialize: =>
            @data = ''
            issues.on 'all', @render_status

        compute_data: =>
            # Looking for vclock conflict
            conflicts = []
            for issue in issues.models
                if issue.get('type') is 'VCLOCK_CONFLICT'
                    type = issue.get('object_type')
                    switch type
                        when 'namespace'
                            type = 'table'
                            if namespaces.get(issue.get('object_id'))?
                                name = namespaces.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found table'
                        when 'database'
                            type = 'database'
                            if databases.get(issue.get('object_id'))
                                name = databases.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found database'
                        when 'datacenter'
                            type = 'datacenter'
                            if issue.get('object_id') is universe_datacenter.get('id')
                                name = universe_datacenter.get('name')
                            else if datacenters.get(issue.get('object_id'))?
                                name = datacenters.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found datacenter'
                        when 'machine'
                            type = 'server'
                            if machines.get(issue.get('object_id'))
                                name = machines.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found server'

                    conflicts.push
                        id: issue.get('object_id')
                        type: type # Use this to generate url
                        name: name
                        field: issue.get('field')


            types = {}
            num_types_conflicts = 0
            for conflict in conflicts
                if not types[conflict.type]?
                    types[conflict.type] = 1
                    num_types_conflicts++
                else
                    types[conflict.type]++
                
            has_conflicts: conflicts.length > 0
            conflicts: conflicts
            has_multiple_types: num_types_conflicts > 1
            num_types_conflicts:num_types_conflicts
            types: (type if num_types_conflicts is 1)
            type: (type if type?)
            num_conflicts: conflicts.length
            num_namespaces_conflicting: conflicts.length
            num_namespaces: namespaces.length

        render: =>
            @.$el.html @template()
            @render_status()

        render_status: =>
            data = @compute_data()
            if _.isEqual(@data, data) is false
                @.$('.status').html @status_template data
                if data.has_conflicts is false
                    @.$('.status').addClass 'no-problems-detected'
                    @.$('.status').removeClass 'problems-detected'
                else
                    @.$('.status').addClass 'problems-detected'
                    @.$('.status').removeClass 'no-problems-detected'

                @.$('.popup_container').html @popup_template data
            return @

        clean_dom_listeners: =>
            if @link_clicked?
                @link_clicked.off 'mouseup', @stop_propagation
            @.$('.popup_container').off 'mouseup', @stop_propagation
            $(window).off 'mouseup', @hide_details

        show_details: (event) =>
            event.preventDefault()
            @clean_dom_listeners()
            @.$('.popup_container').show()
            margin_top = event.pageY-60-13
            margin_left= event.pageX-12-470
            @.$('.popup_container').css 'margin', margin_top+'px 0px 0px '+margin_left+'px'


            @.$('.popup_container').on 'mouseup', @stop_propagation
            @link_clicked = @.$(event.target)
            @link_clicked.on 'mouseup', @stop_propagation
            $(window).on 'mouseup', @hide_details

        stop_propagation: (event) ->
            event.stopPropagation()

        hide_details: (event) =>
            @.$('.popup_container').hide()
            @clean_dom_listeners()

        destroy: =>
            issues.off 'all', @render_status
            @clean_dom_listeners()


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
            @.$el.html ''
            for log in @log_entries
                view = new LogView.LogEntry model: log
                @.$el.append view.render(@compact_entries).$el
            return @

        destroy: =>
            clearInterval @interval

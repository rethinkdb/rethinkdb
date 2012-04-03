# Dashboard: provides an overview and visualizations of the cluster
class DashboardView extends Backbone.View
    className: 'dashboard-view'
    template: Handlebars.compile $('#dashboard_view-template').html()

    update_data_streams: (datastreams) ->
        timestamp = new Date(Date.now())

        for stream in datastreams
           stream.update(timestamp)

    create_fake_data: ->
        data_points = new DataPoints()
        cached_data = {}
        for collection in [namespaces, datacenters, machines]
            collection.map (model, i) ->
                d = new DataPoint
                    collection: collection
                    value: (i+1) * 100
                    id: model.get('id')
                    # Assumption: every datapoint across datastreams at the time of sampling will have the same datetime stamp
                    time: new Date(Date.now())
                data_points.add d
                cached_data[model.get('id')] = [d]
        return data_points

    initialize: ->
        log_initial '(initializing) dashboard view'

        mem_usage_data = new DataStream
            name: 'mem_usage_data'
            pretty_print_name: 'memory usage'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        disk_usage_data = new DataStream
            name: 'disk_usage_data'
            pretty_print_name: 'disk usage'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        cluster_performance_total = new DataStream
            name:  'cluster_performance_total'
            pretty_print_name: 'total ops/sec'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        cluster_performance_reads = new DataStream
            name:  'cluster_performance_reads'
            pretty_print_name: 'reads/sec'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        cluster_performance_writes = new DataStream
            name:  'cluster_performance_writes'
            pretty_print_name: 'writes/sec'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        @data_streams = [mem_usage_data, disk_usage_data, cluster_performance_total, cluster_performance_reads, cluster_performance_writes]

        setInterval (=> @update_data_streams @data_streams), 1500

        @data_picker = new Vis.DataPicker @data_streams
        color_map = @data_picker.get_color_map()

        @disk_usage = new Vis.ResourcePieChart disk_usage_data, color_map
        @mem_usage = new Vis.ResourcePieChart mem_usage_data, color_map
        @cluster_performance = new Vis.ClusterPerformanceGraph [cluster_performance_total, cluster_performance_reads, cluster_performance_writes], color_map

    render: ->
        # Updates elements tracked by our fake data streams | Should be removed when DataStreams are live from the server TODO
        for stream in @data_streams
            stream.set('data', @create_fake_data())

        log_render '(rendering) dashboard view'
        @.$el.html @template({})

        @.$('.data-picker').html @data_picker.render().el
        @.$('.disk-usage').html @disk_usage.render().el
        @.$('.mem-usage').html @mem_usage.render().el
        @.$('.chart.cluster-performance').html @cluster_performance.render().el

        return @

# Machine view
module 'MachineView', ->
    # Container
    class @Container extends Backbone.View
        className: 'machine-view'
        template: Handlebars.compile $('#machine_view-container-template').html()

        initialize: ->
            log_initial '(initializing) machine view: container'

            # Sparkline for CPU
            @cpu_sparkline =
                data: []
                total_points: 30
                update_interval: 750
            @cpu_sparkline.data[i] = 0 for i in [0...@cpu_sparkline.total_points]

            # Performance graph (flot)
            @performance_graph =
                data: []
                total_points: 75
                update_interval: 750
                options:
                    series:
                        shadowSize: 0
                    yaxis:
                        min: 0
                        max: 10000
                    xaxis:
                        show: true
                        min: 0
                        max: 75
                plot: null
            @performance_graph.data[i] = 0 for i in [0..@performance_graph.total_points]

            @model.on 'change', @update_meters

            setInterval @update_sparklines, @cpu_sparkline.update_interval
            setInterval @update_graphs, @performance_graph.update_interval

        render: =>
            log_render '(rendering) machine view: container'

            @.$el.html @template @model.toJSON()

            return @

        # Update the sparkline data and render a new sparkline for the view
        update_sparklines: =>
            @cpu_sparkline.data = @cpu_sparkline.data.slice(1)
            @cpu_sparkline.data.push @model.get 'cpu'
            @.$('.cpu-graph').sparkline(@cpu_sparkline.data)

        # Update the performance data and render a graph for the view
        update_graphs: =>
            @performance_graph.data = @performance_graph.data.slice(1)
            @performance_graph.data.push @model.get 'iops'
            # TODO: consider making this a utility function: enumerate
            # Maps the graph data y-values to x-values (zips them up) and sets the data
            data = _.map @performance_graph.data, (val, i) -> [i, val]


            # If the plot isn't in the DOM yet, create the initial plot
            if not @performance_graph.plot?
                @performance_graph.plot = $.plot($('#performance-graph'), [data], @performance_graph.options)
                window.graph = @performance_graph
            # Otherwise, set the updated data and draw the plot again
            else
                @performance_graph.plot.setData [data]
                @performance_graph.plot.draw() if not pause_live_data

        # Update the meters
        update_meters: =>
            $('.meter > span').each ->
                $(this)
                    .data('origWidth', $(this).width())
                    .width(0)
                    .animate width: $(this).data('origWidth'), 1200

# Datacenter view
module 'DatacenterView', ->
    # Container
    class @Container extends Backbone.View
        className: 'datacenter-view'
        template: Handlebars.compile $('#datacenter_view-container-template').html()

        initialize: ->
            log_initial '(initializing) datacenter view: container'

        render: =>
            log_render('(rendering) datacenter view: container')
            # Add a list of machines that belong to this datacenter to the json
            json = @model.toJSON()
            # Filter all the machines for those belonging to this datacenter, then return an array of their JSON representations
            json.machines = _.map _.filter(machines.models, (machine) => return machine.get('datacenter_uuid') == @model.id),
                (machine) -> machine.toJSON()
            @.$el.html @template json

            return @

# Sidebar view
module 'Sidebar', ->
    # Sidebar.Container
    class @Container extends Backbone.View
        className: 'sidebar-container'
        template: Handlebars.compile $('#sidebar-container-template').html()
        max_recent_events: 5
        resolve_issues_route: '#resolve_issues'

        initialize: =>
            log_initial '(initializing) sidebar view: container'

            @client_connectivity_status = new Sidebar.ClientConnectionStatus()
            @connectivity_status = new Sidebar.ConnectivityStatus()
            @issues = new Sidebar.Issues()

            @reset_event_views()
            events.on 'add', (model, collection) =>
                @reset_event_views()
                @render()

            # Set up bindings for the router
            $(window.app_events).on 'on_ready', =>
                window.app.on 'all', => @render()

        render: (route) =>
            @.$el.html @template
                show_resolve_issues: window.location.hash isnt @resolve_issues_route

            # Render connectivity status
            @.$('.client-connection-status').html @client_connectivity_status.render().el
            @.$('.connectivity-status').html @connectivity_status.render().el

            # Render issue summary
            @.$('.issues').html @issues.render().el

            # Render each event view and add it to the list of recent events
            for view in @event_views
                @.$('.recent-events').append view.render().el

            return @

        reset_event_views: =>
            # Wipe out any existing event views
            @event_views = []

            # Add an event view for the most recent events.
            for event in events.models[0...@max_recent_events]
                @event_views.push new Sidebar.Event model: event

    # Sidebar.ClientConnectionStatus
    class @ClientConnectionStatus extends Backbone.View
        className: 'client-connection-status'
        tagName: 'div'
        template: Handlebars.compile $('#sidebar-client_connection_status-template').html()

        initialize: ->
            log_initial '(initializing) client connection status view'
            connection_status.on 'all', => @render()
            machines.on 'all', => @render()

        render: ->
            log_render '(rendering) status panel view'
            connected_machine = machines.get(connection_status.get('contact_machine_id'))
            json =
                disconnected: connection_status.get('client_disconnected')

            # If we're connected to a machine, get its machine name
            if connected_machine?
                json['machine_name'] = connected_machine.get('name')
                # If the machine is assigned to a datacenter, include it
                assigned_datacenter = datacenters.get(connected_machine.get('datacenter_uuid'))
                console.log 'dc',assigned_datacenter
                json['datacenter_name'] = if assigned_datacenter? then assigned_datacenter.get('name') else 'Unassigned'
                    
            @.$el.html @template json

            return @

    # Sidebar.ConnectivityStatus
    class @ConnectivityStatus extends Backbone.View
        className: 'connectivity-status'
        template: Handlebars.compile $('#sidebar-connectivity_status-template').html()

        initialize: =>
            # Rerender every time some relevant info changes
            directory.on 'all', (model, collection) => @render()
            machines.on 'all', (model, collection) => @render()
            datacenters.on 'all', (model, collection) => @render()

        compute_connectivity: =>
            # data centers with machines
            dc_have_machines = []
            for m in machines.models
                if m.get('datacenter_uuid')
                    dc_have_machines[dc_have_machines.length] = m.get('datacenter_uuid')
            dc_have_machines = _.uniq(dc_have_machines)
            # data centers visible
            dc_visible = []
            for m in directory.models
                _m = machines.get(m.get('id'))
                if _m and _m.get('datacenter_uuid')
                    dc_visible[dc_visible.length] = _m.get('datacenter_uuid')
            dc_visible = _.uniq(dc_visible)
            conn =
                machines_active: directory.length
                machines_total: machines.length
                datacenters_active: dc_visible.length
                datacenters_total: dc_have_machines.length
            return conn

        render: =>
            @.$el.html @template @compute_connectivity()
            return @

    # Sidebar.Issues
    class @Issues extends Backbone.View
        className: 'issues'
        template: Handlebars.compile $('#sidebar-issues-template').html()

        initialize: =>
            log_initial '(initializing) sidebar view: issues'
            issues.on 'all', => @render()

        render: =>
            # Group critical issues by type
            critical_issues = issues.filter (issue) -> issue.get('critical')
            critical_issues = _.groupBy critical_issues, (issue) -> issue.get('type')

            # Get a list of all other issues (non-critical) 
            other_issues = issues.filter (issue) -> not issue.get('critical')

            @.$el.html @template
                critical_issues:
                    exist: _.keys(critical_issues).length > 0
                    types: _.map(critical_issues, (issues, type) ->
                        json = {}
                        json[type] = true
                        json['num'] = issues.length
                        return json
                    )
                other_issues:
                    exist: other_issues.length > 0
                    num: other_issues.length
                no_issues: _.keys(critical_issues).length is 0 and other_issues.length is 0 
            return @

    # Sidebar.Event
    class @Event extends Backbone.View
        className: 'event'
        template: Handlebars.compile $('#sidebar-event-template').html()

        initialize: =>
            log_initial '(initializing) sidebar view: event'

        render: =>
            @.$el.html @template @model.toJSON()
            @.$('abbr.timeago').timeago()
            return @

# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Container
    class @Container extends Backbone.View
        className: 'resolve-issues'
        template: Handlebars.compile $('#resolve_issues-container-template').html()

        initialize: =>
            log_initial '(initializing) resolve issues view: container'
            issues.on 'all', (model, collection) => @render()

        render: ->
            @.$el.html @template({})

            issue_views = []
            issues.each (issue) ->
                issue_views.push new ResolveIssuesView.Issue
                    model: issue

            $issues = @.$('.issues').empty()
            $issues.append view.render().el for view in issue_views

            return @

    # ResolveIssuesView.Issue
    class @Issue extends Backbone.View
        className: 'issue-container'
        templates:
            'MACHINE_DOWN': Handlebars.compile $('#resolve_issues-machine_down-template').html()
            'NAME_CONFLICT_ISSUE': Handlebars.compile $('#resolve_issues-name_conflict-template').html()
            'PERSISTENCE_ISSUE': Handlebars.compile $('#resolve_issues-persistence-template').html()
            'VCLOCK_CONFLICT': Handlebars.compile $('#resolve_issues-vclock_conflict-template').html()
            'PINNINGS_SHARDS_MISMATCH': Handlebars.compile $('#resolve_issues-pinnings_shards_mismatch-template').html()

        unknown_issue_template: Handlebars.compile $('#resolve_issues-unknown-template').html()

        initialize: ->
            log_initial '(initializing) resolve issues view: issue'

        render: ->
            _template = @templates[@model.get('type')]
            switch @model.get('type')
                when 'MACHINE_DOWN'
                    machine = machines.get(@model.get('victim'))

                    masters = []
                    replicas = []
                    
                    # Look at all namespaces in the cluster and determine whether this machine had a master or replicas for them
                    namespaces.each (namespace) ->
                        for machine_uuid, role_summary of namespace.get('blueprint').peers_roles
                            if machine_uuid is machine.get('id')
                                for shard, role of role_summary
                                    console.log role
                                    if role is 'role_primary'
                                        masters.push
                                            name: namespace.get('name')
                                            uuid: namespace.get('id')
                                            shard: shard
                                    if role is 'role_secondary'
                                        replicas.push
                                            name: namespace.get('name')
                                            uuid: namespace.get('id')
                                            shard: shard

                    json =
                        name: machine.get('name')
                        masters: masters
                        replicas: replicas
                        datetime: ISODateString new Date # faked TODO -- the time field should be ISO 8601

                when 'NAME_CONFLICT_ISSUE'
                   json =
                        name: @model.get('contested_name')
                        type: @model.get('contested_type')
                        num_contestants: @model.get('contestants').length
                        contestants: _.map(@model.get('contestants'), (uuid) ->
                            uuid: uuid
                        )
                        datetime: iso_date_from_unix_time @model.get('time')
                when 'PERSISTENCE_ISSUE'
                    json =
                        datetime: ISODateString new Date() # faked TODO -- the time field should be ISO 8601
                when 'VCLOCK_CONFLICT'
                    json =
                        datetime: ISODateString new Date() # faked TODO -- the time field should be ISO 8601
                when 'PINNINGS_SHARDS_MISMATCH'
                    json =
                        datetime: ISODateString new Date() # faked TODO -- the time field should be ISO 8601
                else
                    _template = @unknown_issue_template
                    json =
                        issue_type: @model.get('type')

            @.$el.html _template(json)
            @.$('abbr.timeago').timeago()

            return @

# Events view
module 'EventsView', ->
    # EventsView.Container
    class @Container extends Backbone.View
        className: 'events-view'
        template: Handlebars.compile $('#events-container-template').html()

        initialize: ->
            log_initial '(initializing) events view: container'

            @reset_event_views()
            events.on 'add', (model, collection) =>
                @reset_event_views()
                @render()

        render: ->
            @.$el.html @template({})

            # Render each event view and add it to the list of events
            for view in @event_views
                @.$('.events').append view.render().el
            return @

        reset_event_views: =>
            # Wipe out any existing event views
            @event_views = []

            # Add an event view for the most recent events.
            events.forEach (event) =>
                @event_views.push new EventsView.Event model: event

    # EventsView.Event
    class @Event extends Backbone.View
        className: 'event'
        template: Handlebars.compile $('#events-event-template').html()

        initialize: ->
            log_initial '(initializing) events view: event'

        render: =>
            @.$el.html @template @model.toJSON()

            return @


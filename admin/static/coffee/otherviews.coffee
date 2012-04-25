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

        events: ->
            'click a.set-datacenter': 'set_datacenter'
            'click a.rename-machine': 'rename_machine'

        initialize: (id) =>
            log_initial '(initializing) machine view: container'
            @machine_uuid = id

        rename_machine: (event) ->
            event.preventDefault()
            rename_modal = new ClusterView.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

        wait_for_model_noop: =>
            return true

        wait_for_model: =>
            @model = machines.get(@machine_uuid)
            if not @model
                machines.off 'all', @render
                machines.on 'all', @render
                return false

            # Model is finally ready, bind necessary handlers
            machines.off 'all', @render
            @model.on 'all', @render
            directory.on 'all', @render

            # Everything has been set up, we don't need this logic any
            # more
            @wait_for_model = @wait_for_model_noop

            return true

        render_empty: =>
            @.$el.text 'Machine ' + @machine_uuid + ' is not available.'
            return @

        render: =>
            log_render '(rendering) machine view: container'

            if @wait_for_model() is false
                return @render_empty()

            datacenter_uuid = @model.get('datacenter_uuid')
            json =
                name: @model.get('name')
                ip: "192.168.1.#{Math.round(Math.random() * 255)}" # Fake IP, replace with real data TODO
                datacenter_uuid: datacenter_uuid

            # If the machine is assigned to a datacenter, add relevant json
            if datacenter_uuid?
                json = _.extend json,
                    assigned_to_datacenter: datacenter_uuid
                    datacenter_name: datacenters.get(datacenter_uuid).get('name')

            # If the machine is reachable, add relevant json
            _namespaces = []
            for namespace in namespaces.models
                _shards = []
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                _shards[_shards.length] =
                                    role: role
                                    shard: shard
                                    name: human_readable_shard shard
                if _shards.length > 0
                    _namespaces[_namespaces.length] =
                        shards: _shards
                        name: namespace.get('name')
                        uuid: namespace.id

            json = _.extend json,
                data:
                    namespaces: _namespaces

            # Reachability
            _.extend json,
                status: DataUtils.get_machine_reachability(@model.get('id'))

            @.$el.html @template json

            if @model.get('log_entries')?
                @model.get('log_entries').each (log_entry) =>
                    view = new MachineView.RecentLogEntry
                        model: log_entry
                    @.$('.recent-log-entries').append view.render().el

            return @

        set_datacenter: (event) =>
            event.preventDefault()
            set_datacenter_modal = new ClusterView.SetDatacenterModal
            set_datacenter_modal.render [@model]

    # MachineView.RecentLogEntry
    class @RecentLogEntry extends Backbone.View
        className: 'machine-logs'
        template: Handlebars.compile $('#machine_view-recent_log_entry-template').html()

        render: =>
            @.$el.html @template @model.toJSON()
            @.$('abbr.timeago').timeago()
            return @

# Datacenter view
module 'DatacenterView', ->
    # Container
    class @Container extends Backbone.View
        className: 'datacenter-view'
        template: Handlebars.compile $('#datacenter_view-container-template').html()
        events: ->
            'click a.rename-datacenter': 'rename_datacenter'

        initialize: (id) =>
            log_initial '(initializing) datacenter view: container'
            @datacenter_uuid = id

        rename_datacenter: (event) ->
            event.preventDefault()
            rename_modal = new ClusterView.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render()

        wait_for_model_noop: ->
            return true

        wait_for_model: =>
            @model = datacenters.get(@datacenter_uuid)
            if not @model
                datacenters.off 'all', @render
                datacenters.on 'all', @render
                return false

            # Model is finally ready, bind necessary handlers
            datacenters.off 'all', @render
            @model.on 'all', @render
            machines.on 'all', @render
            directory.on 'all', @render

            # Everything has been set up, we don't need this logic any
            # more
            @wait_for_model = @wait_for_model_noop

            return true

        render_empty: =>
            @.$el.text 'Datacenter ' + @datacenter_uuid + ' is not available.'
            return @

        render: =>
            log_render('(rendering) datacenter view: container')

            if @wait_for_model() is false
                return @render_empty()

            # Filter all the machines for those belonging to this datacenter
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')
            # Machines we can actually reach in this datacenter
            reachable_machines = directory.filter (m) => machines.get(m.get('id')).get('datacenter_uuid') is @model.get('id')

            # Data residing on this lovely datacenter
            _namespaces = []
            for namespace in namespaces.models
                _shards = []
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machines.get(machine_uuid).get('datacenter_uuid') is @model.get('id')
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                _shards[_shards.length] =
                                    role: role
                                    shard: shard
                                    name: human_readable_shard shard
                if _shards.length > 0
                    # Compute number of primaries and secondaries for each shard
                    __shards = {}
                    for shard in _shards
                        shard_repr = shard.shard.toString()
                        if not __shards[shard_repr]?
                            __shards[shard_repr] =
                                shard: shard.shard
                                name: human_readable_shard shard.shard
                                nprimaries: if shard.role is 'role_primary' then 1 else 0
                                nsecondaries: if shard.role is 'role_secondary' then 1 else 0
                        else
                            if shard.role is 'role_primary'
                                __shards[shard_repr].nprimaries += 1
                            if shard.role is 'role_secondary'
                                __shards[shard_repr].nsecondaries += 1

                    # Append the final data
                    _namespaces[_namespaces.length] =
                        shards: _.map(__shards, (shard, shard_repr) -> shard)
                        name: namespace.get('name')
                        uuid: namespace.id

            # Generate json and render
            @.$el.html @template
                name: @model.get('name')
                machines: _.map(machines_in_datacenter, (machine) ->
                    name: machine.get('name')
                    id: machine.get('id')
                    status: DataUtils.get_machine_reachability(machine.get('id'))
                )
                status: DataUtils.get_datacenter_reachability(@model.get('id'))
                data:
                    namespaces: _namespaces

            dc_log_entries = new LogEntries
            for machine in machines_in_datacenter
                if machine.get('log_entries')?
                    dc_log_entries.add machine.get('log_entries').models

            dc_log_entries.each (log_entry) =>
                view = new DatacenterView.RecentLogEntry
                    model: log_entry
                @.$('.recent-log-entries').append view.render().el

            return @

    # DatacenterView.RecentLogEntry
    class @RecentLogEntry extends Backbone.View
        className: 'datacenter-logs'
        template: Handlebars.compile $('#datacenter_view-recent_log_entry-template').html()

        render: =>
            @.$el.html @template @model.toJSON()
            @.$('abbr.timeago').timeago()
            return @

# Sidebar view
module 'Sidebar', ->
    # Sidebar.Container
    class @Container extends Backbone.View
        className: 'sidebar-container'
        template: Handlebars.compile $('#sidebar-container-template').html()
        max_recent_log_entries: 5

        initialize: =>
            log_initial '(initializing) sidebar view: container'

            @client_connectivity_status = new Sidebar.ClientConnectionStatus()
            @connectivity_status = new Sidebar.ConnectivityStatus()
            @issues = new Sidebar.Issues()

            @reset_log_entries()
            recent_log_entries.on 'reset', =>
                @reset_log_entries()
                @render()

            # Set up bindings for the router
            $(window.app_events).on 'on_ready', =>
                window.app.on 'all', => @render()

        render: (route) =>
            @.$el.html @template({})

            # Render connectivity status
            @.$('.client-connection-status').html @client_connectivity_status.render().el
            @.$('.connectivity-status').html @connectivity_status.render().el

            # Render issue summary
            @.$('.issues').html @issues.render().el

            # Render each event view and add it to the list of recent events
            for view in @recent_log_entries
                @.$('.recent-log-entries').append view.render().el

            return @

        reset_log_entries: =>
            # Wipe out any existing recent log entry views
            @recent_log_entries = []

            # Add an log view for the most recent log entries
            for log_entry in recent_log_entries.models[0...@max_recent_log_entries]
                @recent_log_entries.push new Sidebar.RecentLogEntry model: log_entry

    # Sidebar.ClientConnectionStatus
    class @ClientConnectionStatus extends Backbone.View
        className: 'client-connection-status'
        tagName: 'div'
        template: Handlebars.compile $('#sidebar-client_connection_status-template').html()

        initialize: ->
            log_initial '(initializing) client connection status view'
            connection_status.on 'all', => @render()
            datacenters.on 'all', => @render()
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
            # data centers visible
            dc_visible = []
            directory.each (m) =>
                _m = machines.get(m.get('id'))
                if _m and _m.get('datacenter_uuid')
                    dc_visible[dc_visible.length] = _m.get('datacenter_uuid')
            dc_visible = _.uniq(dc_visible)
            conn =
                machines_active: directory.length
                machines_total: machines.length
                datacenters_active: dc_visible.length
                datacenters_total: datacenters.models.length
            return conn

        render: =>
            @.$el.html @template @compute_connectivity()
            return @

    # Sidebar.Issues
    class @Issues extends Backbone.View
        className: 'issues'
        template: Handlebars.compile $('#sidebar-issues-template').html()
        resolve_issues_route: '#resolve_issues'

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
                show_resolve_issues: window.location.hash isnt @resolve_issues_route
            return @

    # Sidebar.RecentLogEntry
    class @RecentLogEntry extends Backbone.View
        className: 'recent-log-entry'
        template: Handlebars.compile $('#sidebar-recent_log_entry-template').html()

        render: =>
            @.$el.html @template @model.toJSON()
            @.$('abbr.timeago').timeago()
            return @

# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Container
    class @Container extends Backbone.View
        className: 'resolve-issues'
        template_outer: Handlebars.compile $('#resolve_issues-container-outer-template').html()
        template_inner: Handlebars.compile $('#resolve_issues-container-inner-template').html()

        initialize: =>
            log_initial '(initializing) resolve issues view: container'
            issues.on 'all', (model, collection) => @render_issues()

        render: ->
            @.$el.html @template_outer
            @render_issues()

            return @

        # we're adding an inner render function to avoid rerendering
        # everything (for example we need to not render the alert,
        # otherwise it disappears)
        render_issues: ->
            @.$('#resolve_issues-container-inner-placeholder').html @template_inner
                issues_exist: if issues.length > 0 then true else false

            issue_views = []
            issues.each (issue) ->
                issue_views.push new ResolveIssuesView.Issue
                    model: issue

            $issues = @.$('.issues').empty()
            $issues.append view.render().el for view in issue_views

            return @

    class @DeclareMachineDeadModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#declare_machine_dead-modal-template').html()
        alert_tmpl: Handlebars.compile $('#declared_machine_dead-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: declare machine dead'
            super @template

        render: (machine_to_kill) ->
            log_render '(rendering) declare machine dead dialog'
            validator_options =
                submitHandler: =>
                    $.ajax
                        url: "/ajax/machines/#{machine_to_kill.id}"
                        type: 'DELETE'
                        contentType: 'application/json'

                        success: (response) =>
                            clear_modals()

                            if (response)
                                throw "Received a non null response to a delete... this is incorrect"

                            # Grab the new set of issues (so we don't have to wait)
                            $.ajax
                                url: '/ajax/issues'
                                success: set_issues
                                async: false

                            # rerender issue view (just the issues, not the whole thing)
                            window.app.resolve_issues_view.render_issues()

                            # notify the user that we succeeded
                            $('#user-alert-space').append @alert_tmpl
                                machine_name: machine_to_kill.get("name")

                            # remove the dead machine from the models (this must be last)
                            machines.remove(machine_to_kill.id)

            super validator_options, { 'machine_name': machine_to_kill.get("name") }

    class @ResolveVClockModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#resolve_vclock-modal-template').html()
        alert_tmpl: Handlebars.compile $('#resolved_vclock-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: resolve vclock'
            super @template

        render: (final_value, resolution_url) ->
            log_render '(rendering) resolve vclock'
            validator_options =
                submitHandler: =>
                    $.ajax
                        url: "/ajax/" + resolution_url
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify(final_value)
                        success: (response) =>
                            clear_modals()

                            # Grab the new set of issues (so we don't have to wait)
                            $.ajax
                                url: '/ajax/issues'
                                success: set_issues
                                async: false

                            # rerender issue view (just the issues, not the whole thing)
                            window.app.resolve_issues_view.render_issues()

                            # notify the user that we succeeded
                            $('#user-alert-space').append @alert_tmpl
                                final_value: final_value

            super validator_options, { 'final_value': final_value }

    # ResolveIssuesView.Issue
    class @Issue extends Backbone.View
        className: 'issue-container'
        templates:
            'MACHINE_DOWN': Handlebars.compile $('#resolve_issues-machine_down-template').html()
            'NAME_CONFLICT_ISSUE': Handlebars.compile $('#resolve_issues-name_conflict-template').html()
            'PERSISTENCE_ISSUE': Handlebars.compile $('#resolve_issues-persistence-template').html()
            'VCLOCK_CONFLICT': Handlebars.compile $('#resolve_issues-vclock_conflict-template').html()

        unknown_issue_template: Handlebars.compile $('#resolve_issues-unknown-template').html()

        initialize: ->
            log_initial '(initializing) resolve issues view: issue'

        render_machine_down: (_template) ->
            machine = machines.get(@model.get('victim'))

            masters = []
            replicas = []

            # Look at all namespaces in the cluster and determine whether this machine had a master or replicas for them
            namespaces.each (namespace) ->
                for machine_uuid, role_summary of namespace.get('blueprint').peers_roles
                    if machine_uuid is machine.get('id')
                        for shard, role of role_summary
                            if role is 'role_primary'
                                masters.push
                                    name: namespace.get('name')
                                    uuid: namespace.get('id')
                                    shard: human_readable_shard shard
                            if role is 'role_secondary'
                                replicas.push
                                    name: namespace.get('name')
                                    uuid: namespace.get('id')
                                    shard: human_readable_shard shard

            json =
                name: machine.get('name')
                masters: if _.isEmpty(masters) then null else masters
                replicas: if _.isEmpty(replicas) then null else replicas
                no_responsibilities: if (_.isEmpty(replicas) and _.isEmpty(masters)) then true else false
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')

            @.$el.html _template(json)

            # Declare machine dead handler
            @.$('p a.btn').off "click"
            @.$('p a.btn').click =>
                declare_dead_modal = new ResolveIssuesView.DeclareMachineDeadModal
                declare_dead_modal.render machine

        render_name_conflict_issue: (_template) ->
            json =
                name: @model.get('contested_name')
                type: @model.get('contested_type')
                num_contestants: @model.get('contestants').length
                contestants: _.map(@model.get('contestants'), (uuid) =>
                   uuid: uuid
                   type: @model.get('contested_type')
                )
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')

            @.$el.html _template(json)

            # bind rename handlers
            _.each(@model.get('contestants'), (uuid) =>
                @.$("a#rename_" + uuid).click (e) =>
                    e.preventDefault()
                    rename_modal = new ClusterView.RenameItemModal(uuid, @model.get('contested_type'), (response) =>
                        # Grab the new set of issues (so we don't have to wait)
                        $.ajax
                            url: '/ajax/issues'
                            success: set_issues
                            async: false

                        # rerender issue view (just the issues, not the whole thing)
                        window.app.resolve_issues_view.render_issues()
                    )
                    rename_modal.render()
            )

        render_persistence_issue: (_template) ->
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                machine_name: machines.get(@model.get('location')).get('name')
                machine_uuid: @model.get('location')
            @.$el.html _template(json)
            # Declare machine dead handler
            @.$('p a.btn').off "click"
            @.$('p a.btn').click =>
                declare_dead_modal = new ResolveIssuesView.DeclareMachineDeadModal
                declare_dead_modal.render machines.get(@model.get('location'))

        render_vclock_conflict: (_template) ->
            get_resolution_url = =>
                return @model.get('object_type') + 's/' + @model.get('object_id') + '/' + @model.get('field') + '/resolve'

            # grab possible conflicting values
            $.ajax
                url: '/ajax/' + get_resolution_url()
                type: 'GET'
                contentType: 'application/json'
                async: false
                success: (response) =>
                    @contestants = _.map response, (x, index) -> { value: x[1], contestant_id: index }

            # renderevsky
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                object_type: @model.get('object_type')
                object_id: @model.get('object_id')
                object_name: datacenters.get(@model.get('object_id')).get('name')
                field: @model.get('field')
                name_contest: @model.get('field') is 'name'
                contestants: @contestants
            @.$el.html _template(json)

            # bind resolution events
            _.each @contestants, (contestant) =>
                @.$('#resolve_' + contestant.contestant_id).off 'click'
                @.$('#resolve_' + contestant.contestant_id).click (event) =>
                    event.preventDefault()
                    resolve_modal = new ResolveIssuesView.ResolveVClockModal
                    resolve_modal.render contestant.value, get_resolution_url()

        render_unknown_issue: (_template) ->
            json =
                issue_type: @model.get('type')
                critical: @model.get('critical')
            @.$el.html _template(json)

        render: ->
            _template = @templates[@model.get('type')]
            switch @model.get('type')
                when 'MACHINE_DOWN'
                    @render_machine_down _template
                when 'NAME_CONFLICT_ISSUE'
                    @render_name_conflict_issue _template
                when 'PERSISTENCE_ISSUE'
                    @render_persistence_issue _template
                when 'VCLOCK_CONFLICT'
                    @render_vclock_conflict _template
                else
                    @render_unknown_issue @unknown_issue_template

            @.$('abbr.timeago').timeago()

            return @

# Log view
module 'LogView', ->
    # LogView.Container
    class @Container extends Backbone.View
        className: 'log-view'
        template: Handlebars.compile $('#log-container-template').html()

        initialize: ->
            log_initial '(initializing) events view: container'

        render: ->
            @.$el.html @template({})
            return @

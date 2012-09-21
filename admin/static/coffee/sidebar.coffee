# Sidebar view
module 'Sidebar', ->
    # Sidebar.Container
    class @Container extends Backbone.View
        className: 'sidebar-container'
        template: Handlebars.compile $('#sidebar-container-template').html()
        template_dataexplorer: Handlebars.compile $('#sidebar-dataexplorer_container-template').html()

        type_view: 'default'
        previous_queries: []
        events:
            'click .namespace_query': 'write_query_namespace'
            'click .old_query': 'write_query_old'

        initialize: =>
            log_initial '(initializing) sidebar view: container'

            @client_connectivity_status = new Sidebar.ClientConnectionStatus()
            @connectivity_status = new Sidebar.ConnectivityStatus()
            @issues = new Sidebar.Issues()
            @logs = new Sidebar.Logs()

            window.app.on 'all', @render

        set_type_view: (type = 'default') =>
            if type isnt @type_view
                @type_view = type
                @render()

        add_query: (query) =>
            if query.length > 17
                query_summary = query.slice(0, 5) + '...' + query.slice(query.length-10)
            else
                query_summary = query

            @previous_queries.unshift
                query: query
                query_summary: query_summary

            @render()

        write_query_namespace: (event) ->
            window.router.current_view.write_query_namespace(event)

        write_query_old: (event) ->
            window.router.current_view.write_query_old(event)

        compute_data: =>
            data_temp = {}
            for database in databases.models
                data_temp[database.get('id')] = []
            for namespace in namespaces.models
                data_temp[namespace.get('database')].push
                    name: namespace.get('name')
                    database: databases.get(namespace.get('database')).get 'name'

            data = {}
            data['databases'] = []
            for database_id of data_temp
                if data_temp[database_id].length > 0
                    data['databases'].push
                        name: databases.get(database_id).get 'name'
                        namespaces: data_temp[database_id] 

            return data

        render: =>
            if @type_view is 'default'
                @.$('.recent-log-entries').html ''
                @.$el.html @template({})
    
                # Render connectivity status
                @.$('.client-connection-status').html @client_connectivity_status.render().el
                @.$('.connectivity-status').html @connectivity_status.render().el
    
                # Render issue summary
                @.$('.issues').html @issues.render().el

                # Render log
                @.$('.recent-log-entries-container').html @logs.render().el

                return @
            else
                data = @compute_data()
                data['previous_queries'] = @previous_queries
                data['has_namespaces'] = data['databases'].length > 0
                data['has_previous_queries'] = @previous_queries.length > 0
                @.$el.html @template_dataexplorer data

                # Render issue summary
                @.$('.issues').html @issues.render().el

                return @

        destroy: =>
            window.app.off 'all', @render

    class @Logs extends Backbone.View
        className: 'recent-log-entries'
        tagName: 'ul'
        min_timestamp: 0
        max_entry_logs: 5
        interval_update_log: 10000

        initialize: ->
            @fetch_log()
            @set_interval = setInterval @fetch_log, @interval_update_log
            @log_entries = []

        fetch_log: =>
            $.ajax({
                contentType: 'application/json'
                url: '/ajax/log/_?max_length='+@max_entry_logs+'&min_timestamp='+@min_timestamp
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

            @min_timestamp = parseFloat(@log_entries[0].get('timestamp'))+1

        render: =>
            @.$el.html ''
            for log in @log_entries
                view = new LogView.LogEntry model: log
                @.$el.append view.render_small().$el
            return @

    # Sidebar.ClientConnectionStatus
    class @ClientConnectionStatus extends Backbone.View
        className: 'client-connection-status'
        tagName: 'div'
        template: Handlebars.compile $('#sidebar-client_connection_status-template').html()

        initialize: =>
            connection_status.on 'all', @render
            datacenters.on 'all', @render
            machines.on 'all', @render

        render: =>
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

        destroy: =>
            connection_status.off 'all', @render
            datacenters.off 'all', @render
            machines.off 'all', @render

    # Sidebar.ConnectivityStatus
    class @ConnectivityStatus extends Backbone.View
        className: 'connectivity-status'
        template: Handlebars.compile $('#sidebar-connectivity_status-template').html()

        initialize: =>
            # Rerender every time some relevant info changes
            directory.on 'all', @render
            machines.on 'all', @render
            datacenters.on 'all', @render

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

        destroy: =>
            # Rerender every time some relevant info changes
            directory.off 'all', @render
            machines.off 'all', @render
            datacenters.off 'all', @render

    # Sidebar.Issues
    class @Issues extends Backbone.View
        className: 'issues'
        template: Handlebars.compile $('#sidebar-issues-template').html()
        resolve_issues_route: '#resolve_issues'

        initialize: =>
            log_initial '(initializing) sidebar view: issues'
            issues.on 'all', @render

        render: =>
            # Group critical issues by type
            critical_issues = issues.filter (issue) -> issue.get('critical')
            critical_issues = _.groupBy critical_issues, (issue) -> issue.get('type')

            # Get a list of all other issues (non-critical)
            other_issues = issues.filter (issue) -> not issue.get('critical')
            other_issues = _.groupBy other_issues, (issue) -> issue.get('type')

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
                    exist: _.keys(other_issues).length > 0
                    types: _.map(other_issues, (issues, type) ->
                        json = {}
                        json[type] = true
                        json['num'] = issues.length
                        return json
                    )
                no_issues: _.keys(critical_issues).length is 0 and _.keys(other_issues).length is 0
            return @

        destroy: =>
            issues.off 'all', @render

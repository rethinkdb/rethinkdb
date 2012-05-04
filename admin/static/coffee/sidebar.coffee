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
            app_events.on 'router_ready', =>
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

        events: ->
            'click a[rel=popover]': 'do_nothing'

        do_nothing: (event) -> event.preventDefault()

        render: =>
            json = _.extend @model.toJSON(), @model.get_formatted_message()
            @.$el.html @template _.extend json,
                machine_name: machines.get(@model.get('machine_uuid')).get('name')
                timeago_timestamp: @model.get_iso_8601_timestamp()

            @.$('abbr.timeago').timeago()
            @.$('a[rel=popover]').popover
                html: true
            return @


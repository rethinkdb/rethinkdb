# Sidebar view
module 'Sidebar', ->
    # Sidebar.Container
    class @Container extends Backbone.View
        className: 'sidebar-container'
        template: Handlebars.compile $('#sidebar-container-template').html()
        template_dataexplorer: Handlebars.compile $('#sidebar-dataexplorer_container-template').html()

        initialize: =>
            @client_connectivity_status = new Sidebar.ClientConnectionStatus()
            @servers_connected = new Sidebar.ServersConnected()
            @datacenters_connected = new Sidebar.DatacentersConnected()
            @issues = new Sidebar.Issues()
            @issues_banner = new Sidebar.IssuesBanner()

            window.app.on 'all', @render

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
            @.$el.html @template({})

            # Render connectivity status
            @.$('.client-connection-status').html @client_connectivity_status.render().el
            @.$('.servers-connected').html @servers_connected.render().el
            @.$('.datacenters-connected').html @datacenters_connected.render().el

            # Render issue summary and issue banner
            @.$('.issues').html @issues.render().el
            @.$('.issues-banner').html @issues_banner.render().el

        destroy: =>
            window.app.off 'all', @render

    # Sidebar.ClientConnectionStatus
    class @ClientConnectionStatus extends Backbone.View
        className: 'client-connection-status'
        template: Handlebars.compile $('#sidebar-client_connection_status-template').html()

        initialize: =>
            connection_status.on 'all', @render
            datacenters.on 'all', @render
            machines.on 'all', @render

        render: =>
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

    # Sidebar.ServersConnected
    class @ServersConnected extends Backbone.View
        template: Handlebars.compile $('#sidebar-servers_connected-template').html()

        initialize: =>
            # Rerender every time some relevant info changes
            directory.on 'all', @render
            machines.on 'all', @render

        render: =>
            @.$el.html @template
                servers_active: directory.length
                servers_total: machines.length
            return @

        destroy: =>
            # Rerender every time some relevant info changes
            directory.off 'all', @render
            machines.off 'all', @render

    # Sidebar.DatacentersConnected
    class @DatacentersConnected extends Backbone.View
        template: Handlebars.compile $('#sidebar-datacenters_connected-template').html()

        initialize: =>
            # Rerender every time some relevant info changes
            directory.on 'all', @render
            machines.on 'all', @render
            datacenters.on 'all', @render

        compute_connectivity: =>
            if datacenters.length > 0
                # data centers visible
                dc_visible = []
                directory.each (m) =>
                    _m = machines.get(m.get('id'))
                    if _m and _m.get('datacenter_uuid') and _m.get('datacenter_uuid') isnt universe_datacenter.get('id')
                        dc_visible.push _m.get('datacenter_uuid')
                dc_visible = _.uniq(dc_visible)
                conn =
                    datacenters_exist: true
                    datacenters_active: dc_visible.length
                    datacenters_total: datacenters.models.length
            else
                conn =
                    datacenters_exist: false
                    servers_active: directory.length
                    servers_total: machines.length

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

        initialize: =>
            issues.on 'all', @render

        render: =>
            @.$el.html @template
                num_issues: issues.length

            return @

        destroy: =>
            issues.off 'all', @render

    # Sidebar.IssuesBanner
    class @IssuesBanner extends Backbone.View
        template: Handlebars.compile $('#sidebar-issues_banner-template').html()
        resolve_issues_route: '#resolve_issues'

        initialize: =>
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


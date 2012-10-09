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

        render: =>
            @.$el.html @template({})

            # Render connectivity status
            @.$('.client-connection-status').html @client_connectivity_status.render().el
            @.$('.servers-connected').html @servers_connected.render().el
            @.$('.datacenters-connected').html @datacenters_connected.render().el

            # Render issue summary and issue banner
            @.$('.issues').html @issues.render().el
            @.$('.issues-banner').html @issues_banner.render().el
            return @

        destroy: =>
            @client_connectivity_status.destroy()
            @servers_connected.destroy()
            @datacenters_connected.destroy()
            @issues.destroy()
            @issues_banner.destroy()


    # Sidebar.ClientConnectionStatus
    class @ClientConnectionStatus extends Backbone.View
        className: 'client-connection-status'
        template: Handlebars.compile $('#sidebar-client_connection_status-template').html()

        initialize: =>
            connection_status.on 'all', @render
            machines.on 'all', @render
            @data = ''

        render: =>
            data =
                disconnected: connection_status.get('client_disconnected')
                machine_name: machines.get(connection_status.get('contact_machine_id')).get 'name' if connection_status.get('contact_machine_id')? and machines.get(connection_status.get('contact_machine_id'))?

            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @.$el.html @template data
                @data = data_in_json

            return @

        destroy: =>
            connection_status.off 'all', @render
            machines.off 'all', @render

    # Sidebar.ServersConnected
    class @ServersConnected extends Backbone.View
        template: Handlebars.compile $('#sidebar-servers_connected-template').html()

        initialize: =>
            # Rerender every time some relevant info changes
            directory.on 'all', @render
            machines.on 'all', @render
            @data = ''

        render: =>
            servers_active = 0
            for machine_id in directory.models
                if directory.get(machine.get('id'))? # Clean ghost
                    servers_active++

            data =
                servers_active: servers_active
                servers_total: machines.length

            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @.$el.html @template data
                @data = data_in_json
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

            @data = ''

        compute_connectivity: =>
            if datacenters.length > 0
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
                servers_active = 0
                for machine_id in directory.models
                    if directory.get(machine.get('id'))? # Clean ghost
                        servers_active++
                conn =
                    datacenters_exist: false
                    servers_active: servers_active
                    servers_total: machines.length

            return conn

        render: =>
            data = @compute_connectivity()
            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @.$el.html @template data
                @data = data_in_json
            return @

        destroy: =>
            directory.off 'all', @render
            machines.off 'all', @render
            datacenters.off 'all', @render

    # Sidebar.Issues
    class @Issues extends Backbone.View
        className: 'issues'
        template: Handlebars.compile $('#sidebar-issues-template').html()

        initialize: =>
            issues.on 'all', @render
            @issues_length = -1

        render: =>
            if issues.length isnt @issues_length
                @.$el.html @template
                    num_issues: issues.length
                @issues_length = issues.length

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
    

            reduced_issues = _.map(critical_issues, (issues, type) ->
                json = {}
                json[type] = true
                json['num'] = issues.length
                return json
            )
            if reduced_issues.length > 0
                reduced_issues[0].is_first = true

            
            reduced_other_issues = _.map(other_issues, (issues, type) ->
                json = {}
                json[type] = true
                json['num'] = issues.length
                return json
            )
            if reduced_other_issues.length > 0
                reduced_other_issues[0].is_first = true

            @.$el.html @template
                critical_issues:
                    exist: _.keys(critical_issues).length > 0
                    num: _.keys(critical_issues).length
                    data: reduced_issues
                other_issues:
                    exist: _.keys(other_issues).length > 0
                    num: _.keys(other_issues).length
                    data: reduced_other_issues
                no_issues: _.keys(critical_issues).length is 0 and _.keys(other_issues).length is 0

            return @

        destroy: =>
            issues.off 'all', @render


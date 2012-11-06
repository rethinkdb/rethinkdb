# Copyright 2010-2012 RethinkDB, all rights reserved.
# Sidebar view
module 'Sidebar', ->
    # Sidebar.Container
    class @Container extends Backbone.View
        className: 'sidebar-container'
        template: Handlebars.compile $('#sidebar-container-template').html()
        template_dataexplorer: Handlebars.compile $('#sidebar-dataexplorer_container-template').html()

        events: ->
            'click .show-issues': 'toggle_showing_issues'
            'click .hide-issues': 'toggle_showing_issues'
            'click a.change-route': 'toggle_showing_issues'
            'click #issue-alerts .alert .close': 'remove_parent_alert'

        initialize: =>
            @client_connectivity_status = new Sidebar.ClientConnectionStatus()
            @servers_connected = new Sidebar.ServersConnected()
            @datacenters_connected = new Sidebar.DatacentersConnected()
            @issues = new Sidebar.Issues()
            @issues_banner = new Sidebar.IssuesBanner()
            @all_issues = new ResolveIssuesView.Container

            # whether we're currently showing the issue list expanded (@all_issues)
            @showing_all_issues = false

            # Watch as issues get removed / reset. If the size is zero, let's figure out what to show
            issues.on 'remove', @issues_being_resolved
            issues.on 'reset', @issues_being_resolved

        render: =>
            @.$el.html @template({})

            # Render connectivity status
            @.$('.client-connection-status').html @client_connectivity_status.render().el
            @.$('.servers-connected').html @servers_connected.render().el
            @.$('.datacenters-connected').html @datacenters_connected.render().el

            # Render issue summary and issue banner
            @.$('.issues').html @issues.render().el
            @.$('.issues-banner').html @issues_banner.render().el

            @.$('.all-issues').html @all_issues.render().$el
            return @

        # Change the state of the issue banner and show / hide the issue list based on state
        toggle_showing_issues: =>
            @showing_all_issues = not @showing_all_issues
            if @showing_all_issues
                @.$('.all-issues').show()
            else
                @.$('.all-issues').hide()

            @issues_banner.set_showing_issues @showing_all_issues

        remove_parent_alert: (event) =>
            element = $(event.target).parent()
            element.slideUp 'fast', =>
                element.remove()
                @issues_being_resolved()
                @issues_banner.render()

        # As issues get resolved, we need to make sure that we're showing the right elements
        issues_being_resolved: =>
            if issues.length is 0 and @.$('#issue-alerts').children().length is 0
                @.$('.all-issues').hide()
                @showing_all_issues = false
                @issues_banner.set_showing_issues @showing_all_issues

        destroy: =>
            issues.off 'remove', @issues_being_resolved
            issues.off 'reset', @issues_being_resolved
            @client_connectivity_status.destroy()
            @servers_connected.destroy()
            @datacenters_connected.destroy()
            @issues.destroy()
            @issues_banner.destroy()
            @all_issues.destroy()

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
                machine_name: (machines.get(connection_status.get('contact_machine_id')).get 'name' if connection_status.get('contact_machine_id')? and machines.get(connection_status.get('contact_machine_id'))?)

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
            for machine in directory.models
                if directory.get(machine.get('id'))? # Clean ghost
                    servers_active++

            data =
                servers_active: servers_active
                servers_total: machines.length
                servers_not_reachable: servers_active < machines.length

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
            dc_visible = []
            directory.each (m) =>
                _m = machines.get(m.get('id'))
                if _m and _m.get('datacenter_uuid') and _m.get('datacenter_uuid') isnt universe_datacenter.get('id')
                    dc_visible.push _m.get('datacenter_uuid')
            # Also add empty datacenters to visible -- we define them
            # as reachable.
            datacenters.each (dc) =>
                if DataUtils.get_datacenter_machines(dc.get('id')).length is 0
                    dc_visible.push dc.get('id')
            dc_visible = _.uniq(dc_visible)
            conn =
                datacenters_active: dc_visible.length
                datacenters_total: datacenters.models.length
                datacenters_not_reachable: dc_visible.length < datacenters.length

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
    # Issue count panel at the top
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
            @showing_issues = false
            @data = {}

        render: =>
            data =
                num_issues: issues.length
                no_issues: issues.length is 0
                show_banner: issues.length > 0 or $('#issue-alerts').children().length > 0

            if _.isEqual(data, @data) is false
                @data = data
                @.$el.html @template @data

                # Preserve the state of the button (show or hide issues) between renders
                @set_showing_issues(@showing_issues) if @showing_issues

            return @

        set_showing_issues: (showing) =>
            @showing_issues = showing
            if showing
                @.$('.show-issues').hide()
                @.$('.hide-issues').show()
            else
                @.$('.show-issues').show()
                @.$('.hide-issues').hide()

        destroy: =>
            issues.off 'all', @render

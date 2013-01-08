# Copyright 2010-2012 RethinkDB, all rights reserved.
# Machine view
module 'MachineView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.templates['element_view-not_found-template']
        ghost_template: Handlebars.templates['machine_view-ghost-template']
        initialize: (id) =>
            @id = id
        render: =>
            if directory.get(@id)?
                for issue in issues.models
                    if issue.get('type') is 'MACHINE_GHOST' and issue.get('ghost') is @id
                        @.$el.html @ghost_template
                            id: @id
                            ips: directory.get(@id).get('ips')
                        return @
                @.$el.html @template
                    id: @id
                    type: 'server'
                    type_url: 'servers'
                    type_all_url: 'servers'
            else
                @.$el.html @template
                    id: @id
                    type: 'server'
                    type_url: 'servers'
                    type_all_url: 'servers'
            return @

    # Container
    class @Container extends Backbone.View
        className: 'machine-view'
        template: Handlebars.templates['machine_view-container-template']

        events: ->
            'click .close': 'close_alert'
            'click .tab-link': 'change_route'
            # operations in the dropdown menu
            'click .operations .rename':                'rename_machine'
            'click .operations .change-datacenter':     'change_datacenter'
            'click .operations .unassign-datacenter':   'unassign_datacenter'

        initialize: =>
            log_initial '(initializing) machine view: container'

            # Panels for machine view
            @title = new MachineView.Title model: @model
            @profile = new MachineView.Profile model: @model
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'server'
            )
            @data = new MachineView.Data model: @model
            @logs = new LogView.Container
                route: "/ajax/log/"+@model.get('id')+"?"
                type: 'machine'

            machines.on 'remove', @check_if_still_exists
            @model.on 'change:datacenter_uuid', @render_can_unassign_button

        check_if_still_exists: =>
            exist = false
            for machine in machines.models
                if machine.get('id') is @model.get('id')
                    exist = true
                    break
            if exist is false
                window.router.navigate '#servers'
                window.app.index_servers
                    alert_message: "The server <a href=\"#servers/#{@model.get('id')}\">#{@model.get('name')}</a> could not be found and was probably deleted."

        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later.
            window.router.navigate @.$(event.target).attr('href')

        render: (tab) =>
            log_render '(rendering) machine view: container'

            # create main structure
            @.$el.html @template
                server_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # fill the profile (name, reachable + performance)
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # data on this server
            @.$('.server-data').html @data.render().$el

            # log entries
            @.$('.recent-log-entries').html @logs.render().$el

            @render_can_unassign_button()

            return @

        # Hide the unassign button if the server doesn't have a datacenter, else show it.
        render_can_unassign_button: =>
            if @model.get('datacenter_uuid')? and @model.get('datacenter_uuid') isnt universe_datacenter.get('id')
                @.$('.unassign-datacenter_li').show()
            else
                @.$('.unassign-datacenter_li').hide()

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        rename_machine: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'server'
            rename_modal.render()

        change_datacenter: (event) =>
            event.preventDefault()
            set_datacenter_modal = new ServerView.SetDatacenterModal
            set_datacenter_modal.render [@model]

        unassign_datacenter: (event) =>
            event.preventDefault()
            unassign_dialog = new MachineView.UnassignModal model: @model
            unassign_dialog.render()

        destroy: =>
            machines.off 'remove', @check_if_still_exists
            @model.off 'change:datacenter_uuid', @render_can_unassign_button

            @title.destroy()
            @profile.destroy()
            @performance_graph.destroy()
            @data.destroy()
            @logs.destroy()
            @model.off '', @render_can_unassign_button

    # MachineView.Title
    class @Title extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.templates['machine_view_title-template']
        initialize: ->
            @name = @model.get('name')
            @model.on 'change:name', @update

        update: =>
            if @name isnt @model.get('name')
                @name = @model.get('name')
                @render()

        render: =>
            @.$el.html @template
                name: @name
            return @

        destroy: =>
            @model.off 'change:name', @update

    # MachineView.Profile
    class @Profile extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.templates['machine_view_profile-template']
        initialize: =>
            directory.on 'all', @render
            @model.on 'all', @render # We listen to all because we listen to stats for the uptime and that we are looking for a nested property
            @data = {}

        render: =>
            #TODO We have no guaranty that this is the main ip...
            ips = if directory.get(@model.get('id'))? then directory.get(@model.get('id')).get('ips') else null
            if ips? and ips[0]?
                main_ip = ips[0]

            # If the machine is assigned to a datacenter, add relevant json
            if @model.get('datacenter_uuid') isnt universe_datacenter.get('id')
                if datacenters.get(@model.get('datacenter_uuid'))?
                    datacenter_name = datacenters.get(@model.get('datacenter_uuid')).get('name')
                else
                    datacenter_name = 'Not found datacenter'

            data =
                main_ip: main_ip
                uptime: if @model.get_stats().proc.uptime? then $.timeago(new Date(Date.now() - @model.get_stats().proc.uptime * 1000)).slice(0, -4) else "N/A"
                assigned_to_datacenter: @model.get('datacenter_uuid') isnt universe_datacenter.get('id')
                reachability: DataUtils.get_machine_reachability(@model.get('id'))
                datacenter_name: (datacenter_name if datacenter_name?)
                #stats_up_to_date: @model.get('stats_up_to_date')

            if not _.isEqual @data, data
                @data = data
                @.$el.html @template @data

            return @

        destroy: =>
            directory.off 'all', @render
            @model.off 'all', @render

    class @UnassignModal extends UIComponents.AbstractModal
        template: Handlebars.templates['unassign-modal-template']
        alert_tmpl: Handlebars.templates['unassign-alert-template']
        class: 'unassign-dialog'

        initialize: ->
            super

        # Takes a server argument-- the server that will be unassigned from its datacenter
        render: =>
            data =
                modal_title: 'Remove datacenter'
                btn_primary_text: 'Remove'
                id: @model.get('id')
                name: @model.get('name')
                # find reasons that we cannot unassign this server from a datacenter
        
            _.extend data, @model.get_data_for_moving()

            super data

            # We give focus only on the primary_button only if there is no issue
            if data.has_warning is false
                @.$('.btn-primary').focus()

        on_submit: =>
            super

            $.ajax
                url: "/ajax/semilattice/machines/"+@model.get('id')+"/datacenter_uuid"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify null
                dataType: 'json',
                success: @on_success,
                error: @on_error

        on_success: (response) =>
            machines.get(@model.get('id')).set('datacenter_uuid', null)

            $('#user-alert-space').html @alert_tmpl

            clear_modals()

    class @Data extends Backbone.View
        template: Handlebars.templates['machine_view_data-template']

        initialize: =>
            @namespaces_with_listeners = {}

            namespaces.on 'change:blueprint', @render
            namespaces.on 'change:key_distr', @render
            namespaces.each (namespace) -> namespace.load_key_distr()
            @data = {}

        render: =>
            data_by_namespace = []
                    
            # Examine each namespace and collect info on its shards / attach listeners to count the number of keys
            namespaces.each (namespace) =>
                ns =
                    name: namespace.get('name')
                    id: namespace.get('id')
                    shards: []
                # Examine each machine's role for the namespace-- only consider namespaces that actually use this machine
                for machine_id, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_id is @model.get('id')
                        # Build up info on each shard present on this machine for this namespace
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                keys = namespace.compute_shard_rows_approximation shard
                                json_shard = $.parseJSON(shard)
                                ns.shards.push
                                    name: human_readable_shard shard
                                    shard: human_readable_shard_obj shard
                                    num_keys: keys
                                    role: role
                                    secondary: role is 'role_secondary'
                                    primary: role is 'role_primary'

                # Finished building, add it to the list (only if it has shards on this server)
                data_by_namespace.push ns if ns.shards.length > 0

            data =
                has_data: data_by_namespace.length > 0
                # Sort the tables alphabetically by name
                tables: _.sortBy(data_by_namespace, (namespace) -> namespace.name)

            if not _.isEqual data, @data
                @data = data
                @.$el.html @template @data

            return @

        destroy: =>
            namespaces.off 'change:blueprint', @render
            namespaces.off 'change:key_distr', @render

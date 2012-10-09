# Machine view
module 'MachineView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#element_view-not_found-template').html()
        ghost_template: Handlebars.compile $('#machine_view-ghost-template').html()
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
        template: Handlebars.compile $('#machine_view-container-template').html()

        events: ->
            'click .close': 'close_alert'
            'click .tab-link': 'change_route'
            'click .show-data': 'show_data'
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
            @logs = new LogView.Container
                route: "/ajax/log/"+@model.get('id')+"_?"
                template_header: Handlebars.compile $('#log-header-machine-template').html()

            machines.on 'remove', @check_if_still_exists

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

            # log entries
            @.$('.recent-log-entries').html @logs.render().$el
            
            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        # Pop up a modal to show assignments
        show_data: (event) =>
            event.preventDefault()
            modal = new MachineView.DataModal model: @model
            modal.render()

        rename_machine: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

        change_datacenter: (event) =>
            event.preventDefault()
            set_datacenter_modal = new ServerView.SetDatacenterModal
            set_datacenter_modal.render [@model]

        unassign_datacenter: (event) =>
            event.preventDefault()
            unassign_dialog = new MachineView.UnassignModal
            unassign_dialog.render @model

        destroy: =>
            @model.off 'change:name', @render
            @title.destroy()
            @profile.destroy()
            @performance_graph.destroy()
            @logs.destroy()

    # MachineView.Title
    class @Title extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.compile $('#machine_view_title-template').html()
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
        template: Handlebars.compile $('#machine_view_profile-template').html()
        initialize: =>
            directory.on 'all', @render
            @model.on 'all', @render

        render: =>
            datacenter_uuid = @model.get('datacenter_uuid')
            ips = if directory.get(@model.get('id'))? then directory.get(@model.get('id')).get('ips') else null
            json =
                name: @model.get('name')
                ips: ips
                nips: if ips then ips.length else 1
                uptime: $.timeago(new Date(Date.now() - @model.get_stats().proc.uptime * 1000)).slice(0, -4)
                datacenter_uuid: datacenter_uuid
                global_cpu_util: Math.floor(@model.get_stats().proc.global_cpu_util_avg * 100)
                global_mem_total: human_readable_units(@model.get_stats().proc.global_mem_total * 1024, units_space)
                global_mem_used: human_readable_units(@model.get_stats().proc.global_mem_used * 1024, units_space)
                global_net_sent: if @model.get_stats().proc.global_net_sent_persec? then human_readable_units(@model.get_stats().proc.global_net_sent_persec.avg, units_space) else 0
                global_net_recv: if @model.get_stats().proc.global_net_recv_persec? then human_readable_units(@model.get_stats().proc.global_net_recv_persec.avg, units_space) else 0
                machine_disk_space: human_readable_units(@model.get_used_disk_space(), units_space)
                stats_up_to_date: @model.get('stats_up_to_date')
            
            # If the machine is assigned to a datacenter, add relevant json
            if datacenters.get(datacenter_uuid)?
                json = _.extend json,
                    assigned_to_datacenter: datacenter_uuid
                    datacenter_name: datacenters.get(datacenter_uuid).get('name')


            # Reachability
            _.extend json,
                status: DataUtils.get_machine_reachability(@model.get('id'))

            @.$el.html @template(json)

            return @

        destroy: =>
            directory.off 'all', @render
            @model.off 'all', @render

    # BIG TODO WARNING
    # This is the logic that prevents unassigning this server from the
    # datacenter if it has responsibilities. It needs to be moved
    # somewhere intelligent.
    class @Operations extends Backbone.View
        className: 'namespace-other'

        template: Handlebars.compile $('#machine_view-operations-template').html()
        still_master_template: Handlebars.compile $('#reason-cannot_unassign-master-template').html()
        unsatisfiable_goals_template: Handlebars.compile $('#reason-cannot_unassign-goals-template').html()

        events: ->
            'click .rename_server-button': 'rename_server'
            'click .change_datacenter-button': 'change_datacenter'
            'click .unassign_datacenter-button': 'unassign_datacenter'
            'click .to_assignments-link': 'to_assignments'

        initialize: =>
            @data = {}
            machines.on 'all', @render
            namespaces.on 'all', @render


        to_assignments: (event) ->
            event.preventDefault()
            $('#machine-assignments').addClass('active')
            $('#machine-assignments-link').tab('show')
            window.router.navigate @.$(event.target).attr('href')

        need_update: (old_data, new_data) ->
            for key of old_data
                if not new_data[key]?
                    return true
                if new_data[key] isnt old_data[key]
                    return true
            for key of new_data
                if not old_data[key]?
                    return true
                if new_data[key] isnt old_data[key]
                    return true
            return false

        render: =>
            data = {}
            if not @model.get('datacenter_uuid')?
                data.can_assign = true
                data.can_unassign = false
                data.unassign_reason = 'This server is not part of any datacenter'
                if @need_update(@data, data)
                    @.$el.html @template data
                    @data = data
                return @

            for namespace in namespaces.models
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        for shard, role of peer_roles
                            if role is 'role_primary'
                                data.can_assign = false
                                data.can_unassign = false
                                data.unassign_reason = @still_master_template
                                    server_id: @model.get 'id'
                                data.assign_reason = data.unassign_reason
                                if @need_update(@data, data)
                                    @.$el.html @template data
                                    @data = data
                                return @

            num_machines_in_datacenter = 0
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('datacenter_uuid')
                    num_machines_in_datacenter++

            namespaces_need_less_replicas = []
            for namespace in namespaces.models
                if @model.get('datacenter_uuid') of namespace.get('replica_affinities') # If the datacenter has responsabilities
                    num_replica = namespace.get('replica_affinities')[@model.get('datacenter_uuid')]
                    if namespace.get('primary_uuid') is @model.get('datacenter_uuid')
                        num_replica++

                    if num_machines_in_datacenter <= num_replica
                        namespaces_need_less_replicas.push
                            id: namespace.get('id')
                            name: namespace.get('name')
            if namespaces_need_less_replicas.length > 0
                data.can_unassign = false
                data.unassign_reason = @unsatisfiable_goals_template
                    namespaces_need_less_replicas: namespaces_need_less_replicas
                if @need_update(@data, data)
                    @.$el.html @template data
                    @data = data
                return @

            data.can_unassign = true
            @.$el.html @template data
            return @


    class @UnassignModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#unassign-modal-template').html()
        alert_tmpl: Handlebars.compile $('#unassign-alert-template').html()
        class: 'unassign-dialog'

        initialize: ->
            super

        render: (_machine_to_unassign) ->
            @machine_to_unassign = _machine_to_unassign

            super
                modal_title: 'Remove datacenter'
                btn_primary_text: 'Remove'
                id: _machine_to_unassign.get('id')
                name: _machine_to_unassign.get('name')

            @.$('.btn-primary').focus()

        on_submit: =>
            super

            # For when /ajax will handle post request
            data = {}
            data['machines'] = {}
            for machine in machines.models
                data['machines'][machine.get('id')] = {}
                data['machines'][machine.get('id')]['name'] = machine.get('name')
                data['machines'][machine.get('id')]['datacenter_uuid'] = machine.get('datacenter_uuid')

            data['machines'][@machine_to_unassign.get('id')]['datacenter_uuid'] = null

            $.ajax
                url: "/ajax/semilattice"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                dataType: 'json',
                success: @on_success,
                error: @on_error

        on_success: (response) =>
            machines.get(@machine_to_unassign.get('id')).set('datacenter_uuid', null)
            clear_modals()

    class @DataModal extends UIComponents.AbstractModal
        render: =>
            @data = new MachineView.Data(model: @model)
            $('#modal-dialog').html @data.render().$el
            modal = $('.modal').modal
                'show': true
                'backdrop': true
                'keyboard': true

            modal.on 'hidden', =>
                modal.remove()

    class @Data extends Backbone.View
        className: 'machine-data-view modal overwrite_modal'
        template: Handlebars.compile $('#machine_view_data-template').html()

        initialize: =>
            @namespaces_with_listeners = {}
        render: =>
            json = {}
            # If the machine is reachable, add relevant json
            _namespaces = []
            for namespace in namespaces.models
                _shards = []
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                keys = namespace.compute_shard_rows_approximation shard
                                _shards[_shards.length] =
                                    shard: shard
                                    name: human_readable_shard shard
                                    keys: keys if typeof keys is 'string'
                if _shards.length > 0
                    if not @namespaces_with_listeners[namespace.get('id')]?
                        @namespaces_with_listeners[namespace.get('id')] = true
                        namespace.load_key_distr_once()
                        namespace.on 'change:key_distr', @render

                    _namespaces.push
                        shards: _shards
                        name: namespace.get('name')
                        uuid: namespace.get('id')

            json = _.extend json,
                data:
                    namespaces: _namespaces
        
            @.$el.html @template(json)
            return @

        destroy: =>
            for namespace_id of @namespaces_with_listeners
                namespaces.get(namespace_id).off 'change:key_distr', @render
                namespaces.get(namespace_id).clear_timeout()

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
            unassign_dialog = new MachineView.UnassignModal model: @model
            unassign_dialog.render()

        destroy: =>
            @model.off 'change:name', @render
            @title.destroy()
            @profile.destroy()
            @performance_graph.destroy()
            @logs.destroy()
            @model.off '', @render_can_unassign_button

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
                main_ip: ips[0] if ips?
                nips: if ips then ips.length else 1
                uptime: if @model.get_stats().proc.uptime? then $.timeago(new Date(Date.now() - @model.get_stats().proc.uptime * 1000)).slice(0, -4) else "N/A"
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
                reachability: DataUtils.get_machine_reachability(@model.get('id'))

            @.$el.html @template(json)

            return @

        destroy: =>
            directory.off 'all', @render
            @model.off 'all', @render

    class @UnassignModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#unassign-modal-template').html()
        alert_tmpl: Handlebars.compile $('#unassign-alert-template').html()
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

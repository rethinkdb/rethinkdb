# Datacenter view
module 'DatacenterView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#element_view-not_found-template').html()
        initialize: (id) ->
            @id = id
        render: =>
            @.$el.html @template
                id: @id
                type: 'datacenter'
                type_url: 'datacenters'
                type_all_url: 'servers'
            return @

    # Container
    class @Container extends Backbone.View
        className: 'datacenter-view'
        template: Handlebars.compile $('#datacenter_view-container-template').html()
        events: ->
            'click .tab-link': 'change_route'
            'click .display_more_machines': 'expand_profile'
            'click .close': 'close_alert'
            'click .show-data': 'show_data'
            # operations in the dropdown menu
            'click .operations .rename': 'rename_datacenter'
            'click .operations .delete': 'delete_datacenter'

        max_log_entries_to_render: 3

        initialize: =>
            log_initial '(initializing) datacenter view: container'

            # Panels for datacenter view
            @title = new DatacenterView.Title model: @model
            @profile = new DatacenterView.Profile model: @model
            @data = new DatacenterView.Data model: @model
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'datacenter'
            )

            filter = {}
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('id')
                    filter[machine.get('id')] = true

            @logs = new LogView.Container
                filter: filter
                type: 'datacenter'

            datacenters.on 'remove', @check_if_still_exists
        
        check_if_still_exists: =>
            exist = false
            for datacenter in datacenters.models
                if datacenter.get('id') is @model.get('id')
                    exist = true
                    break
            if exist is false
                window.router.navigate '#servers'
                window.app.index_servers
                    alert_message: "The datacenter <a href=\"#datacenters/#{@model.get('id')}\">#{@model.get('name')}</a> could not be found and was probably deleted."

        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later.
            window.router.navigate @.$(event.target).attr('href')

        render: (tab) =>
            log_render('(rendering) datacenter view: container')

            @.$el.html @template
                datacenter_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # fill the profile (name, reachable...)
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display the data on the datacenter
            @.$('.data').html @data.render().$el

            @.$('.performance-graph').html @performance_graph.render().$el

            # Filter all the machines for those belonging to this datacenter and append logs
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

            # log entries
            @.$('.recent-log-entries').html @logs.render().$el

            return @

        expand_profile: (event) ->
            event.preventDefault()
            @profile.more_link_should_be_displayed = false
            @.$('.more_machines').remove()
            @.$('.profile-expandable').css('overflow', 'auto')
            @.$('.profile-expandable').css('height', 'auto')

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())
        
        # Pop up a modal to show assignments
        show_data: (event) =>
            event.preventDefault()
            modal = new DatacenterView.DataModal model: @model
            modal.render()

        rename_datacenter: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render(@model)

        delete_datacenter: (event) ->
            event.preventDefault()
            remove_datacenter_dialog = new DatacenterView.RemoveDatacenterModal
            remove_datacenter_dialog.render @model

        destroy: =>
            @title.destroy()
            @profile.destroy()
            @data.destroy()
            @performance_graph.destroy()
            @logs.destroy()

        
    # DatacenterView.Title
    class @Title extends Backbone.View
        className: 'datacenter-info-view'
        template: Handlebars.compile $('#datacenter_view_title-template').html()
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

    class @Profile extends Backbone.View
        className: 'datacenter-info-view'

        template: Handlebars.compile $('#datacenter_view_profile-template').html()
        initialize: =>

            @model.on 'all', @render
            machines.on 'all', @render
            directory.on 'all', @render

            @more_link_should_be_displayed = true

        render: =>
            # Filter all the machines for those belonging to this datacenter
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

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

            # make sure than the list of machines return contains first the one not reachable
            machines_returned = []
            for machine in machines_in_datacenter
                status = DataUtils.get_machine_reachability(machine.get('id'))
                if status.reachable
                    machines_returned.push
                        name: machine.get('name')
                        id: machine.get('id')
                        status: status
                else
                    machines_returned.unshift
                        name: machine.get('name')
                        id: machine.get('id')
                        status: status

            # Check if data are up to date
            stats_up_to_date = true
            for machine in machines_in_datacenter
                if machine.get('stats_up_to_date') is false
                    stats_up_to_date = false
                    break

            # Generate json and render
            json =
                name: @model.get('name')
                list_machines:
                    machines: machines_returned
                    more_link_should_be_displayed: @more_link_should_be_displayed
                status: DataUtils.get_datacenter_reachability(@model.get('id'))
                stats_up_to_date: stats_up_to_date
            stats = @model.get_stats()
            json = _.extend json,
                global_cpu_util: Math.floor(stats.global_cpu_util_avg * 100)
                global_mem_total: human_readable_units(stats.global_mem_total * 1024, units_space)
                global_mem_used: human_readable_units(stats.global_mem_used * 1024, units_space)
                dc_disk_space: human_readable_units(stats.dc_disk_space, units_space)

            @.$el.html @template(json)

            return @

        destroy: =>
            @model.off 'all', @render
            machines.off 'all', @render
            directory.off 'all', @render

    class @RemoveDatacenterModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#remove_datacenter-modal-template').html()
        class: 'remove_datacenter-dialog'

        initialize: ->
            super

        # Takes a datacenter argument-- the datacenter to be removed
        render: (datacenter) ->
            @datacenter_to_delete = datacenter

            # Find the namespaces for whom this datacenter acts as primary
            namespaces_where_primary = namespaces.filter (namespace) -> namespace.get('primary_uuid') is datacenter.get('id')

            super
                modal_title: 'Remove datacenter'
                btn_primary_text: 'Remove'
                id: datacenter.get('id')
                name: datacenter.get('name')
                # reasons we can't delete the datacenter
                datacenter_is_primary: namespaces_where_primary.length > 0
                namespaces_where_primary: namespaces_where_primary

            @.$('.btn-primary').focus()

        on_submit: =>
            super

            # That creates huge logs. Should post specific data multiple times instead?
            data = {}
            data['datacenters'] = {}
            for datacenter in datacenters.models
                data['datacenters'][datacenter.get('id')] = {}
                data['datacenters'][datacenter.get('id')]['name'] = datacenter.get('name')
            data['datacenters'][@datacenter_to_delete.get('id')] = null
            
            data['machines'] = {}
            for machine in machines.models
                data['machines'][machine.get('id')] = {}
                data['machines'][machine.get('id')]['name'] = machine.get('name')
                data['machines'][machine.get('id')]['datacenter_uuid'] = if machine.get('datacenter_uuid') is @datacenter_to_delete.get('id') then null else machine.get('datacenter_uuid')

            $.ajax
                url: "/ajax/semilattice"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                dataType: 'json'
                success: @on_success
                error: @on_error

        on_success: (response) =>
            name = datacenters.get(@datacenter_to_delete.get('id')).get 'name'
            datacenters.remove @datacenter_to_delete.get('id')
            for machine in machines.models
                if machine.get('datacenter_uuid') is @datacenter_to_delete.get('id')
                    machine.set('datacenter_uuid', null)
            datacenters.trigger 'remove'

            window.router.navigate '#servers'
            window.app.index_servers
                alert_message: "The datacenter #{name} was successfully deleted."

    class @DataModal extends UIComponents.AbstractModal
        render: =>
            @data = new DatacenterView.Data(model: @model)
            $('#modal-dialog').html @data.render().$el
            modal = $('.modal').modal
                'show': true
                'backdrop': true
                'keyboard': true

            modal.on 'hidden', =>
                modal.remove()

    class @Data extends Backbone.View
        className: 'datacenter-data-view modal overwrite_modal'
        template: Handlebars.compile $('#datacenter_view_data-template').html()

        initialize: =>
            @namespaces_with_listeners = {}

        render: =>
            # Filter all the machines for those belonging to this datacenter
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

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
                    if not @namespaces_with_listeners[namespace.get('id')]?
                        @namespaces_with_listeners[namespace.get('id')] = true
                        namespace.load_key_distr_once()
                        namespace.on 'change:key_distr', @render

                    # Compute number of primaries and secondaries for each shard
                    __shards = {}
                    for shard in _shards
                        shard_repr = shard.shard.toString()
                        if not __shards[shard_repr]?
                            keys = namespace.compute_shard_rows_approximation shard.shard
                            __shards[shard_repr] =
                                shard: shard.shard
                                name: human_readable_shard shard.shard
                                nprimaries: if shard.role is 'role_primary' then 1 else 0
                                nsecondaries: if shard.role is 'role_secondary' then 1 else 0
                                keys: keys if typeof keys is 'string'
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
            json =
                data:
                    namespaces: _namespaces
            stats = @model.get_stats()
            json = _.extend json,
                global_cpu_util: Math.floor(stats.global_cpu_util_avg * 100)
                global_mem_total: human_readable_units(stats.global_mem_total * 1024, units_space)
                global_mem_used: human_readable_units(stats.global_mem_used * 1024, units_space)
                dc_disk_space: human_readable_units(stats.dc_disk_space, units_space)

            @.$el.html @template(json)

            return @

        destroy: =>
            for namespace_id of @namespaces_with_listeners
                namespaces.get(namespace_id).off 'change:key_distr', @render
                namespaces.get(namespace_id).clear_timeout()


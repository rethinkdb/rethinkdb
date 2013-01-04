# Copyright 2010-2012 RethinkDB, all rights reserved.
# Datacenter view
module 'DatacenterView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.templates['element_view-not_found-template']
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
        template: Handlebars.templates['datacenter_view-container-template']
        events: ->
            'click .tab-link': 'change_route'
            'click .close': 'close_alert'
            # operations in the dropdown menu
            'click .operations .rename': 'rename_datacenter'
            'click .operations .delete': 'delete_datacenter'

        max_log_entries_to_render: 3

        initialize: =>
            log_initial '(initializing) datacenter view: container'

            # Panels for datacenter view
            @title = new DatacenterView.Title model: @model
            @profile = new DatacenterView.Profile model: @model
            @machine_list = new DatacenterView.MachineList model: @model
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

            # display a list of servers
            @.$('.server-list').html @machine_list.render().$el

            @.$('.performance-graph').html @performance_graph.render().$el

            # Filter all the machines for those belonging to this datacenter and append logs
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

            # log entries
            @.$('.recent-log-entries').html @logs.render().$el

            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        rename_datacenter: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render(@model)

        delete_datacenter: (event) ->
            event.preventDefault()
            remove_datacenter_dialog = new ServerView.RemoveDatacenterModal
            remove_datacenter_dialog.render @model

        destroy: =>
            datacenters.off 'remove', @check_if_still_exists
            @title.destroy()
            @profile.destroy()
            @machine_list.destroy()
            @performance_graph.destroy()
            @logs.destroy()


    # DatacenterView.Title
    class @Title extends Backbone.View
        className: 'datacenter-info-view'
        template: Handlebars.templates['datacenter_view_title-template']
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

        template: Handlebars.templates['datacenter_view_profile-template']
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

            # Compute total shards and replicas on this dc
            total_shards = 0
            total_replicas = 0
            for _n in _namespaces
                for _s in _n.shards
                    total_shards += _s.nprimaries
                    total_replicas += _s.nsecondaries + _s.nprimaries

            # Generate json and render
            json =
                name: @model.get('name')
                list_machines:
                    machines: machines_returned
                    more_link_should_be_displayed: @more_link_should_be_displayed
                status: DataUtils.get_datacenter_reachability(@model.get('id'))
                stats_up_to_date: stats_up_to_date
                nshards: total_shards
                nreplicas: total_replicas
            stats = @model.get_stats()
            json = _.extend json,
                dc_disk_space: human_readable_units(stats.dc_disk_space, units_space)

            @.$el.html @template(json)

            return @

        destroy: =>
            @model.off 'all', @render
            machines.off 'all', @render
            directory.off 'all', @render

    class @MachineList extends Backbone.View
        template: Handlebars.templates['datacenter_view-machine_list-template']

        initialize: =>
            machines.on 'change:name', @render
            directory.on 'all', @render
            namespaces.on 'change:blueprint', @render
            @data = {}

        render: =>
            # Filter a list of machines in this datacenter
            machines_in_dc = machines.filter (machine) => machine.get('datacenter_uuid') is @model.get('id')
            # Collect basic info on machines in this datacenter-- we'll count up the primaries and secondaries shortly
            data_on_machines = {}
            for machine in machines_in_dc
                data_on_machines[machine.get('id')] =
                    name: machine.get('name')
                    id: machine.get('id')
                    num_primaries: 0
                    num_secondaries: 0
                    status: DataUtils.get_machine_reachability(machine.get('id'))

            # Count up the number of primaries and secondaries for each machine across all tables
            namespaces.each (namespace) =>
                for machine_id, peer_roles of namespace.get('blueprint').peers_roles
                    if data_on_machines[machine_id]?
                        for shard, role of peer_roles
                            machine = data_on_machines[machine_id]
                            machine.num_primaries += 1 if role is 'role_primary'
                            machine.num_secondaries += 1 if role is 'role_secondary'

            servers = _.sortBy(data_on_machines, (machine) -> machine.name)
            data =
                has_servers: servers.length > 0
                servers: servers

            if not _.isEqual @data, data
                @data = data
                @.$el.html @template @data
            return @

        destroy: =>
            machines.off 'change:name', @render
            directory.off 'all', @render
            namespaces.off 'change:blueprint', @render

    class @Data extends Backbone.View

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
                        namespace.load_key_distr()
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
                                keys: (keys if typeof keys is 'string')
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
                dc_disk_space: human_readable_units(stats.dc_disk_space, units_space)

            @.$el.html @template(json)

            return @

        destroy: =>
            for namespace_id of @namespaces_with_listeners
                namespaces.get(namespace_id).off 'change:key_distr', @render
                namespaces.get(namespace_id).clear_timeout()


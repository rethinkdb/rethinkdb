# Datacenter view
module 'DatacenterView', ->
    # Container
    class @Container extends Backbone.View
        className: 'datacenter-view'
        template: Handlebars.compile $('#datacenter_view-container-template').html()
        events: ->
            'click a.rename-datacenter': 'rename_datacenter'

        initialize: (id) =>
            log_initial '(initializing) datacenter view: container'
            @datacenter_uuid = id

        rename_datacenter: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render()

        wait_for_model_noop: ->
            return true

        wait_for_model: =>
            @model = datacenters.get(@datacenter_uuid)
            if not @model
                datacenters.off 'all', @render
                datacenters.on 'all', @render
                return false

            # Model is finally ready, bind necessary handlers
            datacenters.off 'all', @render
            @model.on 'all', @render
            machines.on 'all', @render
            directory.on 'all', @render

            # Everything has been set up, we don't need this logic any
            # more
            @wait_for_model = @wait_for_model_noop

            return true

        render_empty: =>
            @.$el.text 'Datacenter ' + @datacenter_uuid + ' is not available.'
            return @

        render: =>
            log_render('(rendering) datacenter view: container')

            if @wait_for_model() is false
                return @render_empty()

            # Filter all the machines for those belonging to this datacenter
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')
            # Machines we can actually reach in this datacenter
            reachable_machines = directory.filter (m) => machines.get(m.get('id')).get('datacenter_uuid') is @model.get('id')

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

            # Generate json and render
            @.$el.html @template
                name: @model.get('name')
                machines: _.map(machines_in_datacenter, (machine) ->
                    name: machine.get('name')
                    id: machine.get('id')
                    status: DataUtils.get_machine_reachability(machine.get('id'))
                )
                status: DataUtils.get_datacenter_reachability(@model.get('id'))
                data:
                    namespaces: _namespaces

            dc_log_entries = new LogEntries
            for machine in machines_in_datacenter
                if machine.get('log_entries')?
                    dc_log_entries.add machine.get('log_entries').models

            dc_log_entries.each (log_entry) =>
                view = new DatacenterView.RecentLogEntry
                    model: log_entry
                @.$('.recent-log-entries').append view.render().el

            return @

    # DatacenterView.RecentLogEntry
    class @RecentLogEntry extends Backbone.View
        className: 'datacenter-logs'
        template: Handlebars.compile $('#datacenter_view-recent_log_entry-template').html()

        render: =>
            @.$el.html @template @model.toJSON()
            @.$('abbr.timeago').timeago()
            return @

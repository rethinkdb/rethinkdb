module 'NamespaceView', ->
    class @ServerAssignments extends Backbone.View
        template: Handlebars.compile $('#namespace_view-server_assignments-template').html()
        
        initialize: =>
            @model.on 'change:blueprint', @render
            @model.on 'change:key_distr', @render

        render: =>
            data_on_machines = {}

            # Build up information for each machine-- figure out what shards are on it and other relevant details
            for machine_id, peer_roles of @model.get('blueprint').peers_roles
                data_on_machines[machine_id] =
                    name: machines.get(machine_id).get('name')
                    id: machine_id
                    num_keys: undefined
                    num_primaries: 0
                    num_secondaries: 0
                    shards: []
                # Build up information on each shard living on the machine
                for shard, role of peer_roles
                    machine = data_on_machines[machine_id]

                    # Calculate the number of keys for this particular shard
                    keys = @model.compute_shard_rows_approximation shard
                    if machine.num_keys is undefined and typeof keys is 'string'
                        machine.num_keys = 0

                    # Update the info for the machine
                    machine.num_keys += parseInt(keys) if typeof keys is 'string'
                    machine.num_primaries += 1 if role is 'role_primary'
                    machine.num_secondaries += 1 if role is 'role_secondary'
                    machine['shards'].push
                        name: human_readable_shard shard
                        keys: parseInt(keys) if typeof keys is 'string'
                        role: role
                delete data_on_machines[machine_id] if machine.num_primaries is 0 and machine.num_secondaries is 0

            # Group the machines used by this table by datacenter id
            data_grouped_by_datacenter = _.groupBy data_on_machines, (machine) -> machines.get(machine.id).get('datacenter_uuid')

            # Finalize the data used for the template by adding a bit of info
            # on the datacenter, and attaching the previously built info
            datacenters_for_table = _.map _.keys(data_grouped_by_datacenter), (dc_id) ->
                datacenter =
                    id: dc_id
                    machines: data_grouped_by_datacenter[dc_id]

                if dc_id is universe_datacenter.get('id')
                    datacenter.is_universe = true
                else datacenter.name = datacenters.get(dc_id).get('name')

                return datacenter

            # Sort the datacenters by name (making sure that universe is always last)
            datacenters_for_table.sort (a,b) ->
                return 1  if a.is_universe or b.is_universe
                return 1  if b.name > a.name
                return -1 if b.name < a.name
                return 0

            @.$el.html @template
                datacenters: datacenters_for_table

            return @
            
        destroy: =>
            @model.off 'change:blueprint'
            @model.off 'change:key_distr'

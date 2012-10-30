# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'NamespaceView', ->
    class @ServerAssignments extends Backbone.View
        template: Handlebars.compile $('#namespace_view-server_assignments-template').html()
        popup_template: Handlebars.compile $('#popup_content-template').html()

        events: ->
            'click .show_primary_link': 'show_primary'
            'click .show_secondary_link': 'show_secondary'
            'click .close': 'hide_details'

        initialize: =>
            @model.on 'change:blueprint', @render
            @model.on 'change:key_distr', @render
            @data = {}

        # Show the primary
        show_primary: (event) =>
            event.preventDefault()
            @clean_dom_listeners() # We clean it because we are going to add them later

            shards = @get_shards @.$(event.target).data('id'), 'role_primary'
            @.$('.popup_container').html @popup_template
                type_is_master: true
                shards: shards
            @position_popup event
            @set_listeners event

        position_popup: (event) =>
            @.$('.popup_container').show() # Show the popup
            margin_top = event.pageY-60-13
            margin_left= event.pageX+12
            @.$('.popup_container').css 'margin', margin_top+'px 0px 0px '+margin_left+'px' # Set the position of the popup

        # Show secondaries when the user clicked on the link
        show_secondary: (event) =>
            event.preventDefault()
            @clean_dom_listeners()

            shards = @get_shards @.$(event.target).data('id'), 'role_secondary'
            @.$('.popup_container').html @popup_template
                type_is_secondary: true
                shards: shards
            @position_popup event
            @set_listeners event

        # Create the listeners
        # If you click on the link or on the popup, we stop the propagation
        # If the event propagate to the window, we hide the popup
        set_listeners: (event) =>
            @link_clicked = @.$(event.target)
            @link_clicked.on 'mouseup', @stop_propagation
            @.$('.popup_container').on 'mouseup', @stop_propagation
            $(window).on 'mouseup', @hide_details

        # Remove the dom listeners
        clean_dom_listeners: =>
            if @link_clicked?
                @link_clicked.off 'mouseup', @stop_propagation
            @.$('.popup_container').off 'mouseup', @stop_propagation
            $(window).off 'mouseup', @hide_details

        # Stop the propagation if some click in the popup so $(window) doesn't trigger hide_details
        stop_propagation: (event) ->
            event.stopPropagation()

        # Hide the popup
        hide_details: (event) =>
            @.$('.popup_container').hide()
            @clean_dom_listeners()

        # Get the shards for machine with id and whose role is role_requested
        get_shards: (id, role_requested) =>
            shards = []
            for machine_id, peer_roles of @model.get('blueprint').peers_roles
                if machine_id is id
                    # Build up information on each shard living on the machine
                    for shard, role of peer_roles
                        if role is role_requested
                            keys = @model.compute_shard_rows_approximation shard
                            shards.push
                                name: human_readable_shard shard
                                keys: keys
                                keys_ready: keys?

            return shards

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
                    machine.shards.push
                        name: human_readable_shard shard
                        keys: (parseInt(keys) if typeof keys is 'string')
                        role: role
                delete data_on_machines[machine_id] if machine.num_primaries is 0 and machine.num_secondaries is 0

            # Set a boolean for num_keys to know if they are ready or not
            # And if they have primaries and secondaries
            for machine_id, machine of data_on_machines
                if machine.num_keys?
                    machine.keys_ready = true
                if machine.num_primaries > 0
                    machine.has_primaries = true
                if machine.num_secondaries > 0
                    machine.has_secondaries = true

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

            data =
                has_datacenters: datacenters_for_table.length > 0 # Should not be false if there is no error in the cluster
                datacenters: datacenters_for_table
            if not _.isEqual @data, data
                @data = data
                @.$el.html @template data

            @delegateEvents()
            return @
            
        destroy: =>
            @model.off 'change:blueprint', @render
            @model.off 'change:key_distr', @render

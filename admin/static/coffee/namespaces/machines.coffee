# Namespace view
module 'NamespaceView', ->
    # Modifying replica machines
    class @EditReplicaMachinesModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#edit_replica_machines-modal-template').html()
        alert_tmpl: Handlebars.compile $('#edit_machines-alert-template').html()
        class: 'edit-replica-modal'

        initialize: (namespace, secondary) ->
            @namespace = namespace
            @secondary = secondary
            _.extend @secondary,
                primary_uuid: DataUtils.get_shard_primary_uuid(namespace.get('id'), @secondary.shard)
            _.extend @secondary,
                primary_name: machines.get(@secondary.primary_uuid).get('name')

            @change_hints_state = false
            super

        get_currently_pinned: ->
            # Grab pinned machines for this shard
            currently_pinned = _.find @namespace.get('secondary_pinnings'), (pins, shard) =>
                return JSON.stringify(@secondary.shard) is JSON.stringify(shard)
            # Filter out the ones for our datacenter, since we're changing them
            currently_pinned = _.filter currently_pinned, (uuid) =>
                return machines.get(uuid).get('datacenter_uuid') isnt @secondary.datacenter_uuid
            return currently_pinned

        disable_used_options: ->
            # Make sure the same machine cannot be selected
            # twice. Also disable the machine used for the master.
            selected_machines = _.map @.$('.pinned_machine_choice option:[selected]'), (opt) -> $(opt).attr('value')
            for dropdown in @.$('.pinned_machine_choice')
                selected_option = $(dropdown).find(':selected')[0]
                for option in $('option', dropdown)
                    mid = $(option).attr('value')
                    if mid in selected_machines or mid is @secondary.primary_uuid
                        $(option).prop('disabled', 'disabled') unless option is selected_option
                    else
                        $(option).removeProp('disabled')

        render_inner: ->
            # compute all machines in the datacenter (note, it's
            # important to generate a new object every time because we
            # extend each one differently later)
            generate_dc_machines_arr = =>
                _.map DataUtils.get_datacenter_machines(@secondary.datacenter_uuid), (m) ->
                    machine_name: m.get('name')
                    machine_uuid: m.get('id')

            # Extend each machine entry with dc_machines array. Add
            # 'selected' markings, for the machines that are actually
            # used.
            actual_machines = _.pluck @secondary.machines, 'uuid'
            for m in @secondary.machines
                _dc_machines = generate_dc_machines_arr()
                pin = actual_machines.splice(0, 1)[0]
                actual_machines = _.without actual_machines, pin
                for _m in _dc_machines
                    if _m.machine_uuid is pin
                        _m['selected'] = true
                m['dc_machines'] = _dc_machines

            # render vendor shmender blender zender
            json = _.extend @secondary,
                change_hints: @change_hints_state
                replica_count: @secondary.machines.length
            @.$('.modal-body').html(@template json)

            # Bind change assignment hints action
            @.$('#btn_change_hints').click (e) =>
                e.preventDefault()
                @change_hints_state = true
                @render_inner()
            @.$('#btn_change_hints_cancel').click (e) =>
                e.preventDefault()
                @change_hints_state = false
                @render_inner()

            # Bind change events on the dropdowns and make sure the
            # user doesn't select the same machine twice
            @.$('.pinned_machine_choice').change (e) =>
                @disable_used_options()

            # Disable used options on first render
            @disable_used_options()

            return @

        render: ->
            # Render the modal
            json = _.extend @secondary,
                modal_title: 'Modify replica settings for shard ' + human_readable_shard(@secondary.shard) + ' in datacenter ' + datacenters.get(@secondary.datacenter_uuid).get('name')
                btn_primary_text: 'Commit'
            super json

            # Render inner template
            @render_inner()

        on_submit: =>
            super
            pinned_machines = _.filter @.$('form').serializeArray(), (obj) -> obj.name is 'pinned_machine_uuid'
            if pinned_machines.length is 0
                # If they didn't specify any pins, there is nothing to do
                clear_modals()
                return
            output = {}
            output[@secondary.shard] = _.union (_.map pinned_machines, (m)->m.value), @get_currently_pinned()
            output =
                secondary_pinnings: output
            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(output)
                success: @on_success

        on_success: (response) =>
            super
            namespaces.get(@namespace.id).set(response)
            $('#user-alert-space').html @alert_tmpl {}

    # Modifying master machines
    class @EditMasterMachineModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#edit_master_machine-modal-template').html()
        alert_tmpl: Handlebars.compile $('#edit_machines-alert-template').html()
        class: 'edit-master-machines-modal'
        events: ->
            return _.extend super(),
                'click #btn_change_hints': 'change_hints'
                'click #btn_change_hints_cancel': 'change_hints_cancel'

        change_hints: (e) ->
            e.preventDefault()
            @change_hints_state = true
            @render_inner()

        change_hints_cancel: (e) ->
            e.preventDefault()
            @change_hints_state = false
            @render_inner()

        initialize: (namespace, shard) ->
            @namespace = namespace
            @shard = shard
            @master_uuid = DataUtils.get_shard_primary_uuid(namespace.get('id'), @shard)
            @change_hints_state = false
            super

        compute_json: ->
            # compute all machines in the primary datacenter
            dc_machines = _.map DataUtils.get_datacenter_machines(@namespace.get('primary_uuid')), (m) =>
                machine_name: m.get('name')
                machine_uuid: m.get('id')
                selected: @master_uuid is m.get('id')
            json =
                modal_title: 'Master machine for shard ' + human_readable_shard(@shard) + ' in datacenter ' + datacenters.get(@namespace.get('primary_uuid')).get('name')
                btn_primary_text: 'Commit'
                dc_machines: dc_machines
                change_hints: @change_hints_state
                master_uuid: @master_uuid
                master_name: machines.get(@master_uuid).get('name')
                master_status: DataUtils.get_machine_reachability(@master_uuid)
                name: human_readable_shard @shard
                datacenter_uuid: @namespace.get('primary_uuid')
                datacenter_name: datacenters.get(@namespace.get('primary_uuid')).get('name')
            return json

        render_inner: ->
            # render vendor shmender blender zender
            @.$('.modal-body').html(@template(@compute_json()))
            return @

        render: ->
            # Render the modal
            super @compute_json()

        on_submit: =>
            super
            pinned_master = _.find @.$('form').serializeArray(), (obj) -> obj.name is 'pinned_master_uuid'
            if not pinned_master?
                # If they didn't specify the pin, do nothing
                clear_modals()
                return
            # We need to remove the new master pin from the secondary pinnings
            secondary_pins = _.find @namespace.get('secondary_pinnings'), (pins, shard) =>
                return shard.toString() is @shard.toString()
            secondary_pins = _.filter secondary_pins, (uuid) =>
                return uuid isnt pinned_master.value

            # Whooo
            output_master = {}
            output_master[@shard] = pinned_master.value
            output_secondaries = {}
            output_secondaries[@shard] = secondary_pins
            output =
                primary_pinnings: output_master
                secondary_pinnings: output_secondaries
            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(output)
                success: @on_success

        on_success: (response) =>
            super
            namespaces.get(@namespace.id).set(response)
            $('#user-alert-space').html @alert_tmpl {}


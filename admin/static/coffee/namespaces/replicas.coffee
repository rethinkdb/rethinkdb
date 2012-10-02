
# Namespace view
module 'NamespaceView', ->
    # Replicas view
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()
        alert_tmpl: Handlebars.compile $('#changed_primary_dc-replica-template').html()

        events:
            'click .edit_replicas': 'modify_replicas'
            'click .make_primary': 'make_primary'

        modify_replicas: (event) =>
            event.preventDefault()
            datacenter_id = @.$(event.target).data 'id'
            if datacenter_id is universe_datacenter.get('id')
                datacenter = universe_datacenter
            else
                datacenter = datacenters.get datacenter_id
            modal = new NamespaceView.ModifyReplicasModal @model, datacenter
            modal.render()

        initialize: ->
            # @model is a namespace.  somebody is supposed to pass model: namespace to the constructor.
            @model.on 'all', @render
            directory.on 'all', @render
            progress_list.on 'all', @render
            log_initial '(initializing) namespace view: replica'

        make_primary: (event) ->
            event.preventDefault()
            id = @.$(event.target).data('id')
            if datacenters.get(id)?
                new_dc = datacenters.get(id)
            else
                new_dc = universe_datacenter

            log_action 'make primary clicked'
            modal = new UIComponents.ConfirmationDialogModal
            # Increase replica affinities in the old primary
            # datacenter by one, since we're gonna loose the
            # primary replica. Decrease replica affinities in the
            # new datacenter by one, for the same reason. We don't
            # have to worry about this number going negative,
            # since we can't make primary a datacenter with no
            # replicas.

            new_affinities = {}
            if @model.get('primary_uuid') is universe_datacenter.get('id')
                old_dc = universe_datacenter
            else
                old_dc = datacenters.get(@model.get('primary_uuid'))
            new_affinities[old_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), old_dc.get('id')) + 1

            new_affinities[new_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), new_dc.get('id')) - 1
            primary_pinnings = {}
            for shard of @model.get('primary_pinnings')
                primary_pinnings[shard] = null
            data =
                primary_uuid: new_dc.get('id')
                replica_affinities: new_affinities
                primary_pinnings: primary_pinnings
            modal.render("Are you sure you want to make datacenter " + new_dc.get('name') + " primary?",
                "/ajax/semilattice/rdb_namespaces/" + @model.get('id'),
                JSON.stringify(data),
                (response) =>
                    clear_modals()
                    diff = {}
                    diff[@model.get('id')] = response
                    apply_to_collection(namespaces, add_protocol_tag(diff, "rdb"))
                    # Grab the latest view of things
                    $('#user-alert-space').html(@alert_tmpl
                        namespace_uuid: @model.get('id')
                        namespace_name: @model.get('name')
                        datacenter_uuid: new_dc.get('id')
                        datacenter_name: new_dc.get('name')
                        old_datacenter_uuid: old_dc.get('id')
                        old_datacenter_name: old_dc.get('name')
                        )
                )

        render: =>
            found_universe = false
            data = @model.toJSON()
            if not @model.get('primary_uuid')?
                primary_replica_count = 0
            else
                primary_replica_count = @model.get('replica_affinities')[@model.get('primary_uuid')]
                if not primary_replica_count?
                    primary_replica_count = 0
            if @model.get('primary_uuid') is universe_datacenter.get('id')
                found_universe = true
                data = _.extend data,
                    primary:
                        id: @model.get('primary_uuid')
                        name: universe_datacenter.get('name')
                        replicas: primary_replica_count + 1 # we're adding one because primary is also a replica
                        total_machines: machines.length
                        acks: DataUtils.get_ack_expectations(@model.get('id'), @model.get('primary_uuid'))
                        #status: DataUtils.get_namespace_status(@model.get('id'), @model.get('primary_uuid'))
            else
                data = _.extend data,
                    primary:
                        id: @model.get('primary_uuid')
                        name: datacenters.get(@model.get('primary_uuid')).get('name')
                        replicas: primary_replica_count + 1 # we're adding one because primary is also a replica
                        total_machines: DataUtils.get_datacenter_machines(@model.get('primary_uuid')).length
                        acks: DataUtils.get_ack_expectations(@model.get('id'), @model.get('primary_uuid'))
                        status: DataUtils.get_namespace_status(@model.get('id'), @model.get('primary_uuid'))

            # Walk over json and add datacenter names to the model (in addition to datacenter ids)
            secondary_affinities = {}
            _.each @model.get('replica_affinities'), (replica_count, id) =>
                if id != @model.get('primary_uuid') and replica_count > 0
                    secondary_affinities[id] = replica_count
                    if id is universe_datacenter.get('id')
                        found_universe = true

            # List of datacenters we're not replicating to
            nothings = []
            for dc in datacenters.models
                is_primary = dc.get('id') is @model.get('primary_uuid')
                is_secondary = dc.get('id') in _.map(secondary_affinities, (obj, id)->id)
                if not is_primary and not is_secondary
                    nothings.push
                        id: dc.get('id')
                        name: dc.get('name')

            if found_universe is false
                nothings.push
                    id: universe_datacenter.get('id')
                    name: universe_datacenter.get('name')
                    is_universe: true

            data = _.extend data,
                secondaries:
                    _.map secondary_affinities, (replica_count, uuid) =>
                        if datacenters.get(uuid)?
                            datacenter = datacenters.get(uuid)
                        else
                            datacenter = universe_datacenter
                        id: uuid
                        name: datacenter.get('name')
                        is_universe: true if not datacenters.get(uuid)?
                        replicas: replica_count
                        total_machines: DataUtils.get_datacenter_machines(uuid).length
                        acks: DataUtils.get_ack_expectations(@model.get('id'), uuid)
                        status: DataUtils.get_namespace_status(@model.get('id'), uuid)
                nothings: nothings

            @.$el.html @template data

            return @

        destroy: ->
            @model.off 'all', @render
            directory.off 'all', @render
            progress_list.off 'all', @render

    # Modify replica counts and ack counts in each datacenter
    class @ModifyReplicasModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#modify_replicas-modal-template').html()
        alert_tmpl: Handlebars.compile $('#modified_replica-alert-template').html()
        class: 'modify-replicas'

        initialize: (namespace, datacenter) ->
            log_initial '(initializing) modal dialog: modify replicas'
            @namespace = namespace
            @datacenter = datacenter

            if @datacenter is universe_datacenter
                @max_machines = machines.length
                for datacenter_id of @namespace.get('replica_affinities')
                    if datacenter_id isnt universe_datacenter.get('id')
                        @max_machines -= @namespace.get('replica_affinities')[datacenter_id]
                if @namespace.get('primary_uuid') isnt universe_datacenter.get('id')
                    @max_machines -= 1
            else
                @max_machines = machines.length
                for datacenter_id of @namespace.get('replica_affinities')
                    if datacenter_id isnt @datacenter.get('id')
                        @max_machines -= @namespace.get('replica_affinities')[datacenter_id]
                if @namespace.get('primary_uuid') isnt @datacenter.get('id')
                    @max_machines -= 1
                
                @need_explanation = @max_machines < DataUtils.get_datacenter_machines(@datacenter.get('id')).length
                @max_machines = Math.min @max_machines, DataUtils.get_datacenter_machines(@datacenter.get('id')).length

            @nreplicas = @adjustReplicaCount(DataUtils.get_replica_affinities(@namespace.get('id'), @datacenter.id), true)
            @nacks = DataUtils.get_ack_expectations(@namespace.get('id'), @datacenter.get('id'))
            super

        # Simple utility function to generate JSON for a set of machines
        machine_json: (machine_set) -> _.map machine_set, (machine) ->
            id: machine.get('id')
            name: machine.get('name')

        adjustReplicaCount: (numreplicas, is_output) ->
            # If the datacenter is primary for this namespace,
            # increment the number of replicas by one, since the
            # primary is also a replica
            if @namespace.get('primary_uuid') is @datacenter.get('id')
                if is_output
                    return numreplicas + 1
                else
                    return numreplicas - 1
            else
                return numreplicas

        compute_json: ->

            json =
                namespace: @namespace.toJSON()
                datacenter: @datacenter.toJSON()
                num_replicas: @nreplicas
                max_machines: @max_machines
                num_acks: @nacks
                modal_title: 'Modify replica settings'
                btn_primary_text: 'Commit'
                need_explanation: @need_explanation if @need_explanation?
            return json

        render_inner: (error_msg, nreplicas_input, nacks_input) ->
            log_render '(rendering) modify replicas dialog (inner)'
            json = @compute_json()
            if error_msg?
                @reset_buttons()
                json['error_msg'] = error_msg
            if nreplicas_input?
                json.nreplicas_input = nreplicas_input
            if nacks_input?
                json.nacks_input = nacks_input
            @.$('.modal-body').html(@template json)

        render: ->
            log_render '(rendering) modify replicas dialog (outer)'
            super(@compute_json())
            $('#focus_num_replicas').focus()

        on_submit: =>
            super
            formdata = form_data_as_object($('form', @$modal))

            # Validate first
            msg_error = []
            if DataUtils.is_integer(formdata.num_replicas) is false
                msg_error.push('The number of replicas must be an integer.')
                nreplicas_input = formdata.num_replicas
                nacks_input = formdata.num_acks
            if DataUtils.is_integer(formdata.num_acks) is false
                msg_error.push('The number of acks must be an integer.')
                nreplicas_input = formdata.num_replicas
                nacks_input = formdata.num_acks
            if formdata.num_acks is "0"
                msg_error.push('The number of acks must be greater than 0')
                nreplicas_input = formdata.num_replicas
                nacks_input = formdata.num_acks
            if msg_error.length isnt 0
                @render_inner msg_error, nreplicas_input, nacks_input
                return

            # It is now safe to use parseInt
            nreplicas_input = parseInt(formdata.num_replicas)
            nacks_input = parseInt(formdata.num_acks)

            msg_error = []
            if nreplicas_input > @max_machines
                msg_error.push('The number of replicas (' + nreplicas_input + ') cannot exceed the total number of machines (' + @max_machines + ').')
            if nreplicas_input is 0 and @namespace.get('primary_uuid') is @datacenter.get('id')
                msg_error.push('The number of replicas must be at least one because ' + @datacenter.get('name') + ' is the primary datacenter for this namespace.')
            if nacks_input > nreplicas_input
                msg_error.push('The number of acks (' + nacks_input + ') cannot exceed the total number of replicas (' + nreplicas_input + ').')

            if msg_error.length isnt 0
                @render_inner msg_error, nreplicas_input, nacks_input
                return

            # No error, we can save the change on replicas/acks
            @nreplicas = nreplicas_input
            @nacks = nacks_input
            # Generate json
            replica_affinities_to_send = {}
            replica_affinities_to_send[formdata.datacenter] = @adjustReplicaCount(@nreplicas, false)
            ack_expectations_to_send = {}
            ack_expectations_to_send[formdata.datacenter] = @nacks

            # prepare data for success template in case we succeed
            @old_replicas = @adjustReplicaCount(DataUtils.get_replica_affinities(@namespace.get('id'), @datacenter.id), true)
            @modified_replicas = @nreplicas isnt @old_replicas
            @old_acks = DataUtils.get_ack_expectations(@namespace.get('id'), @datacenter.id)
            @modified_acks = @nacks isnt @old_acks
            @datacenter_uuid = formdata.datacenter
            if @datacenter_uuid is universe_datacenter.get('id')
                @datacenter_name = universe_datacenter.get('name')
            else
                @datacenter_name = datacenters.get(@datacenter_uuid).get('name')

            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify
                    replica_affinities: replica_affinities_to_send
                    ack_expectations: ack_expectations_to_send
                success: @on_success
                error: @on_error

        on_success: (response) =>
            super
            namespaces.get(@namespace.id).set(response)
            if @modified_replicas or @modified_acks
                $('#user-alert-space').html(@alert_tmpl
                    modified_replicas: @modified_replicas
                    old_replicas: @old_replicas
                    new_replicas: @nreplicas
                    modified_acks: @modified_acks
                    old_acks: @old_acks
                    new_acks: @nacks
                    datacenter_uuid: @datacenter_uuid
                    datacenter_name: @datacenter_name
                    )


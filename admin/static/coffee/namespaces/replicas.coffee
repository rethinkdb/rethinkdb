
# Namespace view
module 'NamespaceView', ->
    # Replicas view
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()
        alert_tmpl: Handlebars.compile $('#changed_primary_dc-replica-template').html()

        initialize: ->
            # @model is a namespace.  somebody is supposed to pass model: namespace to the constructor.
            @model.on 'all', @render
            directory.on 'all', @render
            progress_list.on 'all', @render
            log_initial '(initializing) namespace view: replica'

        modify_replicas: (e, datacenter) ->
            log_action 'modify replica clicked'
            modal = new NamespaceView.ModifyReplicasModal @model, datacenter
            modal.render()
            e.preventDefault()

        make_primary: (e, new_dc) ->
            log_action 'make primary clicked'
            modal = new UIComponents.ConfirmationDialogModal
            # Increase replica affinities in the old primary
            # datacenter by one, since we're gonna loose the
            # primary replica. Decrease replica affinities in the
            # new datacenter by one, for the same reason. We don't
            # have to worry about this number going negative,
            # since we can't make primary a datacenter with no
            # replicas.
            old_dc = datacenters.get(@model.get('primary_uuid'))
            new_affinities = {}
            new_affinities[old_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), old_dc.get('id')) + 1
            new_affinities[new_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), new_dc.get('id')) - 1
            data =
                primary_uuid: new_dc.get('id')
                replica_affinities: new_affinities
            modal.render("Are you sure you want to make datacenter " + new_dc.get('name') + " primary?",
                "/ajax/memcached_namespaces/" + @model.get('id'),
                JSON.stringify(data),
                (response) =>
                    clear_modals()
                    diff = {}
                    diff[@model.get('id')] = response
                    apply_to_collection(namespaces, add_protocol_tag(diff, "memcached"))
                    # Grab the latest view of things
                    $('#user-alert-space').append (@alert_tmpl
                        namespace_uuid: @model.get('id')
                        namespace_name: @model.get('name')
                        datacenter_uuid: new_dc.get('id')
                        datacenter_name: new_dc.get('name')
                        old_datacenter_uuid: old_dc.get('id')
                        old_datacenter_name: old_dc.get('name')
                        )
                )
            e.preventDefault()

        render: =>
            log_render '(rendering) namespace view: replica'
            # Walk over json and add datacenter names to the model (in
            # addition to datacenter ids)
            secondary_affinities = {}
            _.each @model.get('replica_affinities'), (replica_count, id) =>
                if id != @model.get('primary_uuid') and replica_count > 0
                    secondary_affinities[id] = replica_count
            # List of datacenters we're not replicating to
            nothings = []
            for dc in datacenters.models
                is_primary = dc.get('id') is @model.get('primary_uuid')
                is_secondary = dc.get('id') in _.map(secondary_affinities, (obj, id)->id)
                if not is_primary and not is_secondary
                    nothings[nothings.length] =
                        id: dc.get('id')
                        name: dc.get('name')
            # create json
            primary_replica_count = @model.get('replica_affinities')[@model.get('primary_uuid')]
            if not primary_replica_count?
                # replica affinities may be missing for new namespaces
                primary_replica_count = 0
            json = _.extend @model.toJSON(),
                primary:
                    id: @model.get('primary_uuid')
                    name: datacenters.get(@model.get('primary_uuid')).get('name')
                    replicas: primary_replica_count + 1 # we're adding one because primary is also a replica
                    total_machines: DataUtils.get_datacenter_machines(@model.get('primary_uuid')).length
                    acks: DataUtils.get_ack_expectations(@model.get('id'), @model.get('primary_uuid'))
                    status: DataUtils.get_namespace_status(@model.get('id'), @model.get('primary_uuid'))
                secondaries:
                    _.map secondary_affinities, (replica_count, uuid) =>
                        id: uuid
                        name: datacenters.get(uuid).get('name')
                        replicas: replica_count
                        total_machines: DataUtils.get_datacenter_machines(uuid).length
                        acks: DataUtils.get_ack_expectations(@model.get('id'), uuid)
                        status: DataUtils.get_namespace_status(@model.get('id'), uuid)
                nothings: nothings

            @.$el.html @template(json)


            # Bind action for primary datacenter
            @.$('#edit-primary').on 'click', (e) =>
                @modify_replicas(e, datacenters.get(@model.get('primary_uuid')))

            # Bind the link actions for each datacenter with replicas
            _.each _.keys(@model.get('replica_affinities')), (dc_uuid) =>
                @.$(".edit-secondary.#{dc_uuid}").on 'click', (e) =>
                    @modify_replicas(e, datacenters.get(dc_uuid))
                @.$(".make-primary.#{dc_uuid}").on 'click', (e) =>
                    @make_primary(e, datacenters.get(dc_uuid))

            # Bind the link actions for each datacenter with no replicas
            _.each nothings, (nothing_dc) =>
                @.$(".edit-nothing.#{nothing_dc.id}").on 'click', (e) =>
                    @modify_replicas(e, datacenters.get(nothing_dc.id))

            return @

    # Modify replica counts and ack counts in each datacenter
    class @ModifyReplicasModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#modify_replicas-modal-template').html()
        alert_tmpl: Handlebars.compile $('#modified_replica-alert-template').html()
        class: 'modify-replicas'

        initialize: (namespace, datacenter) ->
            log_initial '(initializing) modal dialog: modify replicas'
            @namespace = namespace
            @datacenter = datacenter
            @total_machines = DataUtils.get_datacenter_machines(@datacenter.get('id')).length
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
                total_machines: @total_machines
                num_acks: @nacks
                modal_title: 'Modify replica settings'
                btn_primary_text: 'Commit'
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
            if msg_error.length isnt 0
                @render_inner msg_error, nreplicas_input, nacks_input
                return

            # It is now safe to use parseInt
            nreplicas_input = parseInt(formdata.num_replicas)
            nacks_input = parseInt(formdata.num_acks)

            msg_error = []
            if nreplicas_input > @total_machines
                msg_error.push('The number of replicas (' + nreplicas_input + ') cannot exceed the total number of machines (' + @total_machines + ').')
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
            @datacenter_name = datacenters.get(@datacenter_uuid).get('name')

            $.ajax
                processData: false
                url: "/ajax/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                type: 'POST'
                data: JSON.stringify
                    replica_affinities: replica_affinities_to_send
                    ack_expectations: ack_expectations_to_send
                success: @on_success

        on_success: (response) =>
            super
            namespaces.get(@namespace.id).set(response)
            if @modified_replicas or @modified_acks
                $('#user-alert-space').append (@alert_tmpl
                    modified_replicas: @modified_replicas
                    old_replicas: @old_replicas
                    new_replicas: @nreplicas
                    modified_acks: @modified_acks
                    old_acks: @old_acks
                    new_acks: @nacks
                    datacenter_uuid: @datacenter_uuid
                    datacenter_name: @datacenter_name
                    )


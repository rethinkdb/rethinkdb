
# Namespace view
module 'NamespaceView', ->
    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'
        template: Handlebars.compile $('#namespace_view-container-template').html()

        initialize: (id) =>
            log_initial '(initializing) namespace view: container'
            @namespace_uuid = id
            directory.on 'all', @render

        wait_for_model: =>
            @model = namespaces.get(@namespace_uuid)
            if not @model
                namespaces.off 'all', @render
                namespaces.on 'all', @render
                return false

            # Model is finally ready, bind necessary handlers
            namespaces.off 'all', @render
            @model.on 'all', @render

            # Some additional setup
            @replicas = new NamespaceView.Replicas model: @model
            @shards = new NamespaceView.Shards model: @model

            return true

        render_empty: =>
            @.$el.text 'Namespace ' + @namespace_uuid + ' is not available.'
            return @

        render: =>
            log_render '(rendering) namespace view: container'

            if @wait_for_model() is false
                return @render_empty()

            json = @model.toJSON()
            json = _.extend json, DataUtils.get_namespace_status(@model.get('id'))

            @.$el.html @template json

            # Add the replica and shards views
            @.$('.section.replication').html @replicas.render().el
            @.$('.section.sharding').html @shards.render().el

            return @

    # Replicas view
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()
        alert_tmpl: Handlebars.compile $('#changed_primary_dc-replica-template').html()

        initialize: ->
            # @model is a namespace.  somebody is supposed to pass model: namespace to the constructor.
            @model.on 'change', @render
            directory.on 'all', @render
            log_initial '(initializing) namespace view: replica'

        modify_replicas: (e, datacenter) ->
            log_action 'modify replica clicked'
            modal = new NamespaceView.ModifyReplicasModal @model, datacenter
            modal.render()
            e.preventDefault()

        make_primary: (e, new_dc) ->
            log_action 'make primary clicked'
            modal = new ClusterView.ConfirmationDialogModal
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

    class @AddNamespaceModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#add_namespace-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_namespace-alert-template').html()
        class: 'add-namespace'

        initialize: ->
            log_initial '(initializing) modal dialog: add namespace'
            super @template

        render: ->
            log_render '(rendering) add namespace dialog'

            # Define the validator options
            validator_options =
                rules:
                   name: 'required'

                messages:
                   name: 'Required'

                submitHandler: =>
                    formdata = form_data_as_object($('form', @$modal))

                    $.ajax
                        processData: false
                        url: '/ajax/memcached_namespaces/new'
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify({"name" : formdata.name, "primary_uuid" : formdata.primary_datacenter, "port" : parseInt(formdata.port)})

                        success: (response) =>
                            clear_modals()

                            apply_to_collection(namespaces, add_protocol_tag(response, "memcached"))
                            # the result of this operation are some attributes about the namespace we created, to be used in an alert
                            # TODO hook this up
                            for id, namespace of response
                                $('#user-alert-space').append @alert_tmpl
                                    uuid: id
                                    name: namespace.name

            json = { 'datacenters' : _.map datacenters.models, (datacenter) -> datacenter.toJSON() }

            super validator_options, json

    class @RemoveNamespaceModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#remove_namespace-modal-template').html()
        alert_tmpl: Handlebars.compile $('#removed_namespace-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: remove namespace'
            super @template

        render: (namespaces_to_delete) ->
            log_render '(rendering) remove namespace dialog'
            validator_options =
                submitHandler: =>
                    for namespace in namespaces_to_delete
                        $.ajax
                            url: "/ajax/#{namespace.get("protocol")}_namespaces/#{namespace.id}"
                            type: 'DELETE'
                            contentType: 'application/json'

                            success: (response) =>
                                clear_modals()

                                if (response)
                                    throw "Received a non null response to a delete... this is incorrect"
                                namespaces.remove(namespace.id)
                                #TODO hook this up
                                #for namespace in response_json.op_result
                                    #$('#user-alert-space').append @alert_tmpl namespace

            array_for_template = _.map namespaces_to_delete, (namespace) -> namespace.toJSON()
            super validator_options, { 'namespaces': array_for_template }

    class @EditReplicaMachinesModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#edit_replica_machines-modal-template-outer').html()
        template_inner: Handlebars.compile $('#edit_replica_machines-modal-template-inner').html()
        alert_tmpl: Handlebars.compile $('#edit_machines-alert-template').html()

        initialize: (namespace, secondary) ->
            console.log '(initializing) modal dialog: modify replica assignments changes'
            @namespace = namespace
            @secondary = secondary
            _.extend @secondary,
                primary_uuid: DataUtils.get_shard_primary_uuid(namespace.get('id'), @secondary.shard)
            _.extend @secondary,
                primary_name: machines.get(@secondary.primary_uuid).get('name')

            @change_hints_state = false
            super @template


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
                        $(option).attr('disabled', 'disabled') unless option is selected_option
                    else
                        $(option).removeAttr('disabled')

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
            @.$('.modal-body').html(@template_inner json)

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
            validator_options =
                submitHandler: =>
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
                        url: "/ajax/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                        type: 'POST'
                        data: JSON.stringify(output)
                        success: (response) =>
                            clear_modals()
                            namespaces.get(@namespace.id).set(response)
                            $('#user-alert-space').append (@alert_tmpl {})

            # Render the modal
            super validator_options, @secondary

            # Render our fun inner lady bits
            @render_inner()

    class @EditMasterMachineModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#edit_master_machine-modal-template-outer').html()
        template_inner: Handlebars.compile $('#edit_master_machine-modal-template-inner').html()
        alert_tmpl: Handlebars.compile $('#edit_machines-alert-template').html()

        initialize: (namespace, shard) ->
            console.log '(initializing) modal dialog: modify master assignment'
            @namespace = namespace
            @shard = shard
            @master_uuid = DataUtils.get_shard_primary_uuid(namespace.get('id'), @shard)
            @change_hints_state = false
            super @template


        render_inner: ->
            # compute all machines in the primary datacenter
            dc_machines = _.map DataUtils.get_datacenter_machines(@namespace.get('primary_uuid')), (m) =>
                    machine_name: m.get('name')
                    machine_uuid: m.get('id')
                    selected: @master_uuid is m.get('id')

            # render vendor shmender blender zender
            json =
                dc_machines: dc_machines
                change_hints: @change_hints_state
                master_uuid: @master_uuid
                master_name: machines.get(@master_uuid).get('name')
                master_status: DataUtils.get_machine_reachability(@master_uuid)
            @.$('.modal-body').html(@template_inner json)

            # Bind change assignment hints action
            @.$('#btn_change_hints').click (e) =>
                e.preventDefault()
                @change_hints_state = true
                @render_inner()
            @.$('#btn_change_hints_cancel').click (e) =>
                e.preventDefault()
                @change_hints_state = false
                @render_inner()

            return @

        render: ->
            validator_options =
                submitHandler: =>
                    pinned_master = _.find @.$('form').serializeArray(), (obj) -> obj.name is 'pinned_master_uuid'
                    if not pinned_master?
                        # If they didn't specify the pin, do nothing
                        clear_modals()
                        return
                    # We need to remove the new master pin from the secondary pinnings
                    secondary_pins = _.find @namespace.get('secondary_pinnings'), (pins, shard) =>
                        return shard.toString() is @shard.toString()
                    secondary_pins = _.filter secondary_pins, (uuid) =>
                        console.log uuid, pinned_master.value, uuid isnt pinned_master.value
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
                        url: "/ajax/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                        type: 'POST'
                        data: JSON.stringify(output)
                        success: (response) =>
                            clear_modals()
                            namespaces.get(@namespace.id).set(response)
                            $('#user-alert-space').append (@alert_tmpl {})

            # Render the modal
            super validator_options,
                name: human_readable_shard @shard
                datacenter_uuid: @namespace.get('primary_uuid')
                datacenter_name: datacenters.get(@namespace.get('primary_uuid')).get('name')

            # Render our fun inner lady bits
            @render_inner()

    class @ModifyReplicasModal extends ClusterView.AbstractModal
        template_inner: Handlebars.compile $('#modify_replicas-modal-template-inner').html()
        template: Handlebars.compile $('#modify_replicas-modal-template-outer').html()
        alert_tmpl: Handlebars.compile $('#modified_replica-alert-template').html()

        class: 'modify-replicas'

        initialize: (namespace, datacenter) ->
            log_initial '(initializing) modal dialog: modify replicas'
            @namespace = namespace
            @datacenter = datacenter
            @total_machines = DataUtils.get_datacenter_machines(@datacenter.get('id')).length
            @nreplicas = @adjustReplicaCount(DataUtils.get_replica_affinities(@namespace.get('id'), @datacenter.id), true)
            @nacks = DataUtils.get_ack_expectations(@namespace.get('id'), @datacenter.get('id'))
            super @template

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

        render_inner: (error_msg) ->
            log_render '(rendering) modify replicas dialog (inner)'
            # Compute json ...
            json =
                namespace: @namespace.toJSON()
                datacenter: @datacenter.toJSON()
                num_replicas: @nreplicas
                total_machines: @total_machines
                num_acks: @nacks
            if error_msg?
                json['error_msg'] = error_msg

            # Render!
            @.$('.modal-body').html(@template_inner json)


        render: ->
            log_render '(rendering) modify replicas dialog (outer)'

            # Define the validator options
            validator_options =
                submitHandler: =>
                    formdata = form_data_as_object($('form', @$modal))

                    # validate first
                    msg = ''
                    @nreplicas = parseInt(formdata.num_replicas)
                    @nacks = parseInt(formdata.num_acks)

                    if @nreplicas > @total_machines
                        msg += 'The number of replicas (' + @nreplicas + ') cannot exceed the total number of machines (' + @total_machines + ').'
                        @render_inner msg
                        return
                    if @nreplicas is 0 and @namespace.get('primary_uuid') is @datacenter.get('id')
                        msg += 'The number of replicas must be at least one because ' + @datacenter.get('name') + ' is the primary datacenter for this namespace.'
                        @render_inner msg
                        return
                    if @nacks > @nreplicas
                        msg += 'The number of acks (' + @nacks + ') cannot exceed the total number of replicas (' + @nreplicas + ').'
                        @render_inner msg
                        return

                    # Generate json
                    replica_affinities_to_send = {}
                    replica_affinities_to_send[formdata.datacenter] = @adjustReplicaCount(@nreplicas, false)
                    ack_expectations_to_send = {}
                    ack_expectations_to_send[formdata.datacenter] = @nacks

                    # prepare data for success template in case we succeed
                    old_replicas = @adjustReplicaCount(DataUtils.get_replica_affinities(@namespace.get('id'), @datacenter.id), true)
                    modified_replicas = @nreplicas isnt old_replicas
                    old_acks = DataUtils.get_ack_expectations(@namespace.get('id'), @datacenter.id)
                    modified_acks = @nacks isnt old_acks
                    datacenter_uuid = formdata.datacenter
                    datacenter_name = datacenters.get(datacenter_uuid).get('name')

                    $.ajax
                        processData: false
                        url: "/ajax/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                        type: 'POST'
                        data: JSON.stringify
                            replica_affinities: replica_affinities_to_send
                            ack_expectations: ack_expectations_to_send

                        success: (response) =>
                            clear_modals()

                            namespaces.get(@namespace.id).set(response)
                            if modified_replicas or modified_acks
                                $('#user-alert-space').append (@alert_tmpl
                                    modified_replicas: modified_replicas
                                    old_replicas: old_replicas
                                    new_replicas: @nreplicas
                                    modified_acks: modified_acks
                                    old_acks: old_acks
                                    new_acks: @nacks
                                    datacenter_uuid: datacenter_uuid
                                    datacenter_name: datacenter_name
                                    )
                        error: (response, unused, unused_2) =>
                            @render_inner(response.responseText)

            json =
                namespace: @namespace.toJSON()
                datacenter: @datacenter.toJSON()
            super validator_options, json
            @render_inner()


    compute_renderable_shards_array = (namespace_uuid, shards) ->
        ret = []
        for i in [0...shards.length]
            primary_uuid = DataUtils.get_shard_primary_uuid namespace_uuid, shards[i]
            secondary_uuids = DataUtils.get_shard_secondary_uuids namespace_uuid, shards[i]
            ret.push
                name: human_readable_shard shards[i]
                shard: shards[i]
                notlast: i != shards.length - 1
                index: i
                primary:
                    uuid: primary_uuid
                    # primary_uuid may be null when a new shard hasn't hit the server yet
                    name: if primary_uuid then machines.get(primary_uuid).get('name') else null
                    status: if primary_uuid then DataUtils.get_machine_reachability(primary_uuid) else null
                    backfill_progress: if primary_uuid then DataUtils.get_backfill_progress(namespace_uuid, shards[i], primary_uuid) else null
                secondaries: _.map(secondary_uuids, (machine_uuids, datacenter_uuid) ->
                    datacenter_uuid: datacenter_uuid
                    datacenter_name: datacenters.get(datacenter_uuid).get('name')
                    machines: _.map(machine_uuids, (machine_uuid) ->
                        uuid: machine_uuid
                        name: machines.get(machine_uuid).get('name')
                        status: DataUtils.get_machine_reachability(machine_uuid)
                        backfill_progress: DataUtils.get_backfill_progress(namespace_uuid, shards[i], machine_uuid)
                        )
                    )
        return ret

    # Shards view
    class @Shards extends Backbone.View
        className: 'namespace-shards'
        template: Handlebars.compile $('#namespace_view-sharding-template').html()

        events: ->
            'click .edit': 'modify_shards'

        modify_shards: (e) ->
            log_action 'modify shards clicked'

            modal = new NamespaceView.ModifyShardsModal(@model, @model.get('shards'))
            modal.render()
            e.preventDefault()

        initialize: ->
            log_initial '(initializing) namespace view: shards'
            @model.on 'change', @render
            directory.on 'all', @render
            progress_list.on 'all', @render
            @shards = []

        bind_edit_machines: (shard) ->
            # Master assignments
            @.$('#assign_master_' + shard.index).click (e) =>
                e.preventDefault()
                modal = new NamespaceView.EditMasterMachineModal @model, shard.shard
                modal.render()

            # Fucking JS closures
            bind_shard = (shard, secondary) =>
                @.$('#assign_machines_' + shard.index + '_' + secondary.datacenter_uuid).click (e) =>
                    e.preventDefault()
                    modal = new NamespaceView.EditReplicaMachinesModal @model, _.extend secondary,
                        name: shard.name
                        shard: shard.shard
                    modal.render()

            for secondary in shard.secondaries
                bind_shard shard, secondary

        render: =>
            log_render '(rendering) namespace view: shards'

            shards_array = compute_renderable_shards_array(@model.get('id'), @model.get('shards'))
            @.$el.html @template
                shards: shards_array

            # Bind events to 'assign machine' links
            for shard in shards_array
                @bind_edit_machines(shard)

            return @



    class @ModifyShardsModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#modify_shards_modal-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()
        class: 'modify-shards'

        initialize: (namespace, shard_set) ->
            log_initial '(initializing) modal dialog: ModifyShards'
            @namespace = namespace
            @shards = compute_renderable_shards_array(namespace.get('id'), shard_set)

            # Keep an unmodified copy of the shard boundaries with which we compare against when reviewing the changes.
            @original_shard_set = _.map(shard_set, _.identity)
            @shard_set = _.map(shard_set, _.identity)
            super @template

        insert_splitpoint: (index, splitpoint) =>
            if (0 <= index || index < @shard_set.length)
                json_repr = $.parseJSON(@shard_set[index])
                if (splitpoint <= json_repr[0] || (splitpoint >= json_repr[1] && json_repr[1] != null))
                    throw "Error invalid splitpoint"

                @shard_set.splice(index, 1, JSON.stringify([json_repr[0], splitpoint]), JSON.stringify([splitpoint, json_repr[1]]))
                clear_modals()
                @render()
            else
                # TODO handle error

        merge_shard: (index) =>
            if (index < 0 || index + 1 >= @shard_set.length)
                throw "Error invalid index"

            newshard = JSON.stringify([$.parseJSON(@shard_set[index])[0], $.parseJSON(@shard_set[index+1])[1]])
            @shard_set.splice(index, 2, newshard)
            clear_modals()
            @render()

        render: (shard_index) =>
            log_render '(rendering) ModifyShards dialog'

            # TODO render "touched" shards (that have been split or merged within this modal) a bit differently

            # TODO these validator options are complete bullshit
            validator_options =
                rules: { }
                messages: { }
                submitHandler: =>
                    formdata = form_data_as_object($('form', @$modal))
                    empty_master_pin = {}
                    empty_master_pin[JSON.stringify(["", null])] = null
                    empty_replica_pins = {}
                    empty_replica_pins[JSON.stringify(["", null])] = []
                    json =
                        shards: @shard_set
                        primary_pinnings: empty_master_pin
                        secondary_pinnings: empty_replica_pins
                    # TODO detect when there are no changes.
                    $.ajax
                        processData: false
                        url: "/ajax/#{@namespace.attributes.protocol}_namespaces/#{@namespace.id}"
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify(json)
                        success: (response) =>
                            clear_modals()
                            namespaces.get(@namespace.id).set(response)
                            $('#user-alert-space').append(@alert_tmpl({}))


            json =
                namespace: @namespace.toJSON()
                shards: compute_renderable_shards_array(@namespace.get('id'), @shard_set)

            super validator_options, json

            shard_views = _.map(compute_renderable_shards_array(@namespace.get('id'), @shard_set), (shard) => new NamespaceView.ModifyShardsModalShard @namespace, shard, @)
            @.$('.shards tbody').append view.render().el for view in shard_views

    class @ModifyShardsModalShard extends Backbone.View
        template: Handlebars.compile $('#modify_shards_modal-shard-template').html()
        editable_tmpl: Handlebars.compile $('#modify_shards_modal-edit_shard-template').html()

        tagName: 'tr'
        class: 'shard'

        events: ->
            'click .split': 'split'
            'click .commit_split': 'commit_split'
            'click .cancel_split': 'cancel_split'
            'click .merge': 'merge'
            'click .commit_merge': 'commit_merge'
            'click .cancel_merge': 'cancel_merge'

        initialize: (namespace, shard, parent_modal) ->
            @namespace = namespace
            @shard = shard
            @parent = parent_modal

        render: ->
            @.$el.html @template
                shard: @shard

            return @

        split: (e) ->
            console.log 'split event fired'
            shard_index = $(e.target).data 'index'
            log_action "split button clicked with index #{shard_index}"

            @.$el.html @editable_tmpl
                splitting: true
                shard: @shard

            e.preventDefault()

        cancel_split: (e) ->
            console.log 'cancel_split event fired'
            @render()
            e.preventDefault()

        commit_split: (e) ->
            console.log 'commit split event fired'
            splitpoint = $('input[name=splitpoint]', @el).val()
            # TODO validate splitpoint
            console.log 'splitpoint is', splitpoint
            e.preventDefault()

            @parent.insert_splitpoint(@shard.index, splitpoint);

        merge: (e) =>
            console.log 'merge event fired'

            shard_index = parseInt $(e.target).data 'index'
            log_action "merge button clicked with index #{shard_index}"
            shards = @namespace.get('shards')
            low_shard = human_readable_shard(shards[shard_index])
            high_shard = human_readable_shard(shards[shard_index + 1])

            console.log 'shards to be merged: ',[low_shard, high_shard, shards, shard_index]

            @.$el.html @editable_tmpl
                merging: true
                shard: @shard
                low_shard: low_shard
                high_shard: high_shard

            e.preventDefault()

        cancel_merge: (e) =>
            console.log 'cancel_merge event fired'
            @render()
            e.preventDefault()

        commit_merge: (e) =>
            console.log 'commit_merge event fired'

            @parent.merge_shard(@shard.index)

            e.preventDefault()


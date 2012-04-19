
# Namespace view
module 'NamespaceView', ->
    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'
        template: Handlebars.compile $('#namespace_view-container-template').html()

        initialize: (id) =>
            log_initial '(initializing) namespace view: container'
            @namespace_uuid = id

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

            @.$el.html @template @model.toJSON()

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
            json = _.extend @model.toJSON(),
                primary:
                    id: @model.get('primary_uuid')
                    name: datacenters.get(@model.get('primary_uuid')).get('name')
                    replicas: primary_replica_count + 1 # we're adding one because primary is also a replica
                    acks: DataUtils.get_ack_expectations(@model.get('id'), @model.get('primary_uuid'))
                secondaries:
                    _.map secondary_affinities, (replica_count, uuid) =>
                        id: uuid
                        name: datacenters.get(uuid).get('name')
                        replicas: replica_count
                        acks: DataUtils.get_ack_expectations(@model.get('id'), uuid)
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

    class @EditMachinesModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#edit_machines-modal-template-outer').html()
        template_inner: Handlebars.compile $('#edit_machines-modal-template-inner').html()
        # TODO
        alert_tmpl: Handlebars.compile $('#modified_replica-alert-template').html()

        initialize: (namespace, secondary) ->
            console.log '(initializing) modal dialog: modify replicas review changes'
            @namespace = namespace
            @secondary = secondary
            _.extend @secondary,
                primary_uuid: DataUtils.get_shard_primary_uuid(namespace.get('id'), @secondary.shard)
            _.extend @secondary,
                primary_name: machines.get(@secondary.primary_uuid).get('name')

            @change_hints_state = false
            super @template

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
                    # TODO: What happens if they just hit 'commit'? How do we remove pinnings? Checkbox?
                    # TODO: Remove pinnings when a new sharding plan is committed (and inform the user)
                    # TODO: confirmation alert, applying changes
                    # TODO: when a pinned machine moves to a different datacenter
                    # TODO: master
                    pinned_machines = _.filter @.$('form').serializeArray(), (obj) -> obj.name is 'pinned_machine_uuid'
                    output = {}
                    output[@secondary.shard] = _.map pinned_machines, (m)->m.value
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
                        #     if modified_replicas or modified_acks
                        #         $('#user-alert-space').append (@alert_tmpl
                        #             modified_replicas: modified_replicas
                        #             old_replicas: old_replicas
                        #             new_replicas: new_replicas
                        #             modified_acks: modified_acks
                        #             old_acks: old_acks
                        #             new_acks: new_acks
                        #             datacenter_uuid: datacenter_uuid
                        #             datacenter_name: datacenter_name
                        #             )
                        # error: (response, unused, unused_2) =>
                        #     clear_modals()
                        #     @render(response.responseText)

            # Render the modal
            super validator_options, @secondary

            # Render our fun inner lady bits
            @render_inner()

    class ReplicaPlanShard extends Backbone.View
        template: Handlebars.compile $('#modify_replica_modal-replica_plan-shard-template').html()
        editable_tmpl: Handlebars.compile $('#modify_replica_modal-replica_plan-edit_shard-template').html()

        tagName: 'tr'
        class: 'shard-row'

        events: ->
            'click .edit': 'edit'

        initialize: (machines_in_datacenter, shard) ->
            @shard = shard
            @available_machines = machines_in_datacenter

        render: ->
            console.log 'rendering with shard.machines being', @shard.machines, 'and existing =', @shard.existing_machines
            @.$el.html @template
                'adding': false
                'removing': false
                'name': @shard.name
                'shard_index': @shard.index
                'machines': _.map(@shard.machines, (machine) =>
                    machine:
                        name: machine.get('name')
                        id: machine.id
                    shard_index: @shard.index
                )
                'existing_secondary_machines': _.map(@shard.existing_machines, (machine) =>
                    machine:
                        name: machine.get('name')
                        id: machine.id
                    shard_index: @shard.index
                )
                'existing_primary_machine': # TODO This should be the actual primary machine (faked data)
                    name: @shard.existing_machines[0].get('name')
                    id: @shard.existing_machines[0].id

            return @

        edit: (e) ->
            console.log 'edit-rending with shard.machines being', @shard.machines
            @.$el.html @editable_tmpl
                'name': @shard.name
                'primary_machine_dropdown':# TODO This should be the actual primary machine (faked data)
                    'shard_index': @shard.index
                    'selected': @available_machines[0]
                    'available_machines': _.map(@available_machines, (machine) =>
                        name: machine.get('name')
                        id: machine.id
                    )
                'secondary_machine_dropdowns': _.map(@shard.machines, (machine) =>
                    'shard_index': @shard.index
                    'selected': machine
                    'available_machines': _.map(@available_machines, (machine) =>
                        name: machine.get('name')
                        id: machine.id
                    )
                )
                'adding': @shard.adding
                'removing': @shard.removing
                'existing_machines': _.map(@shard.existing_machines, (machine) =>
                    machine: machine
                    shard_index: @shard.index
                )

            e.preventDefault()
            return @

    class @ModifyReplicasModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#modify_replicas-modal-template').html()
        alert_tmpl: Handlebars.compile $('#modified_replica-alert-template').html()

        class: 'modify-replicas'

        initialize: (namespace, datacenter) ->
            log_initial '(initializing) modal dialog: modify replicas'
            @namespace = namespace
            @datacenter = datacenter
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

        render:(server_error) ->
            log_render '(rendering) modify replicas dialog'

            # Define the validator options
            validator_options =
                submitHandler: =>
                    formdata = form_data_as_object($('form', @$modal))
                    replica_affinities_to_send = {}
                    replica_affinities_to_send[formdata.datacenter] = @adjustReplicaCount(parseInt(formdata.num_replicas), false)
                    ack_expectations_to_send = {}
                    ack_expectations_to_send[formdata.datacenter] = parseInt(formdata.num_acks)

                    # prepare data for success template in case we succeed
                    old_replicas = @adjustReplicaCount(DataUtils.get_replica_affinities(@namespace.get('id'), @datacenter.id), true)
                    new_replicas = parseInt(formdata.num_replicas)
                    modified_replicas = new_replicas isnt old_replicas
                    old_acks = DataUtils.get_ack_expectations(@namespace.get('id'), @datacenter.id)
                    new_acks = parseInt(formdata.num_acks)
                    modified_acks = new_acks isnt old_acks
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
                                    new_replicas: new_replicas
                                    modified_acks: modified_acks
                                    old_acks: old_acks
                                    new_acks: new_acks
                                    datacenter_uuid: datacenter_uuid
                                    datacenter_name: datacenter_name
                                    )
                        error: (response, unused, unused_2) =>
                            clear_modals()
                            @render(response.responseText)

            # Compute json ...
            num_replicas = DataUtils.get_replica_affinities(@namespace.get('id'), @datacenter.id)
            json =
                namespace: @namespace.toJSON()
                datacenter: @datacenter.toJSON()
                num_replicas: @adjustReplicaCount(num_replicas, true)
                num_acks: DataUtils.get_ack_expectations(@namespace.get('id'), @datacenter.get('id'))
                # random machines | faked TODO
                replica_machines: @machine_json (_.shuffle machines.models)[0...num_replicas]

            if server_error?
                json['server_error'] = server_error

            super validator_options, json


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
                secondaries: _.map(secondary_uuids, (machine_uuids, datacenter_uuid) ->
                    datacenter_uuid: datacenter_uuid
                    datacenter_name: datacenters.get(datacenter_uuid).get('name')
                    machines: _.map(machine_uuids, (machine_uuid) ->
                        uuid: machine_uuid
                        name: machines.get(machine_uuid).get('name')
                        status: DataUtils.get_machine_reachability(machine_uuid)
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
            @shards = []

        render: =>
            log_render '(rendering) namespace view: shards'

            shards_array = compute_renderable_shards_array(@model.get('id'), @model.get('shards'))
            @.$el.html @template
                shards: shards_array

            # Bind events to 'assign machine' links
            for shard in shards_array
                @.$('#assign_master_' + shard.index).click (e) =>
                    e.preventDefault()
                for secondary in shard.secondaries
                    @.$('#assign_machines_' + shard.index + '_' + secondary.datacenter_uuid).click (e) =>
                        e.preventDefault()
                        modal = new NamespaceView.EditMachinesModal @model, _.extend secondary,
                            name: shard.name
                            shard: shard.shard
                        modal.render()

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
                    # TODO detect when there are no changes.
                    $.ajax
                        processData: false
                        url: "/ajax/#{@namespace.attributes.protocol}_namespaces/#{@namespace.id}"
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify({"shards": @shard_set})

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

    # to be modified TODO
    class @ShardingPlanDatacenter extends Backbone.View
        template: Handlebars.compile $('#modify_shards_modal-sharding_plan-datacenter-template').html()
        editable_tmpl: Handlebars.compile $('#modify_shards_modal-sharding_plan-edit_datacenter-template').html()

        tagName: 'tr'
        class: 'datacenter-row'

        events: ->
            'click .edit': 'edit'

        # Simple utility function to generate JSON for a set of machines
        machine_json: (machine_set) ->
            _.map machine_set, (machine) ->
                id: machine.get('id')
                name: machine.get('name')

        # Simple utility function to generate JSON based on replica affinities
        datacenter_json: (datacenter, num_replicas) =>
            name: datacenter.get('name')
            # random machines for the delta | faked TODO
            machines: @machine_json (_.shuffle machines.models)[0..num_replicas]
            # all machines | faked TODO
            existing_machines: @machine_json machines.models

        initialize: (datacenter, num_replicas) ->
            @datacenter = datacenter['datacenter']
            @num_replicas = num_replicas

        render: =>
            # all datacenters | faked TODO
            @.$el.html @template @datacenter_json @datacenter, @num_replicas
            return @

        edit: (e) =>
            json = @datacenter_json @datacenter, @num_replicas
            console.log 'json is ',json
            @.$el.html @editable_tmpl _.extend json,
                'machine_dropdowns': _.map(json.machines, (machine) =>
                    'selected': machine
                    # should be just the machines belonging to the datacenter, but for now it's all machines | faked TODO
                    'available_machines': @machine_json machines.models
                )


            e.preventDefault()

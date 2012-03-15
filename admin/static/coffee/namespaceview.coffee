
# Namespace view
module 'NamespaceView', ->
    # Profile view
    class @Profile extends Backbone.View
        className: 'namespace-profile'
        template: Handlebars.compile $('#namespace_view-profile-template').html()

        initialize: ->
            log_initial '(initializing) namespace view: profile'

        render: =>
            log_render '(rendering) namespace view: profile'
            @.$el.html @template(@model.toJSON())

            return @

    # Replicas view
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()

        events: ->
            'click #add-secondary-datacenter-button': 'add_secondary'

        initialize: ->
            # @model is a namespace.  somebody is supposed to pass model: namespace to the constructor.
            @model.on 'change', @render
            log_initial '(initializing) namespace view: replica'

        modify_replicas: (e, datacenter) ->
            log_action 'modify replica clicked'
            modal = new NamespaceView.ModifyReplicasModal @model, datacenter
            modal.render()
            e.preventDefault()

        edit_machines: (e, datacenter) ->
            log_action 'edit machines clicked'
            modal = new NamespaceView.EditMachinesModal @model, datacenter
            modal.render()
            e.preventDefault()

        make_primary: (e) ->
            log_action 'make primary clicked'
            datacenter = datacenters.get @model.get('primary_uuid')
            modal = new ClusterView.ConfirmationDialogModal
            modal.render("Are you sure you want to make " + datacenter.get('name') + " primary?")
            e.preventDefault()

        add_secondary: (e, datacenter) ->
            log_action 'add secondary clicked'
            modal = new NamespaceView.AddSecondaryModal(@model, datacenter)
            modal.render()
            e.preventDefault()

        remove_secondary: (e, datacenter) ->
            log_action 'remove secondary clicked'
            modal = new ClusterView.ConfirmationDialogModal
            modal.render("Are you sure you want to stop replicating to " + datacenter.get('name') + "?")
            e.preventDefault()

        render: =>
            log_render '(rendering) namespace view: replica'
            # Walk over json and add datacenter names to the model (in
            # addition to datacenter ids)
            secondary_affinities = {}
            _.each @model.get('replica_affinities'), (replica_obj, id) =>
                if id != @model.get('primary_uuid') then secondary_affinities[id] = replica_obj
            json = _.extend @model.toJSON(),
                'primary':
                    'id': @model.get('primary_uuid')
                    'name': datacenters.get(@model.get('primary_uuid')).get('name')
                    'replicas': @model.get('replica_affinities')[@model.get('primary_uuid')]
                'secondaries':
                    _.map secondary_affinities, (replica_count, uuid) =>
                        'id': uuid
                        'name': datacenters.get(uuid).get('name')
                        'replicas': replica_count
                'datacenters_left': datacenters.models.length > _.size(@model.get('replica_affinities'))
            @.$el.html @template(json)


            # Bind action for primary datacenter
            @.$('#edit-primary').on 'click', (e) =>
                @modify_replicas(e, datacenters.get(@model.get('primary_uuid')))

            # Bind the link actions for each datacenter with replicas
            _.each _.keys(@model.get('replica_affinities')), (dc_uuid) =>
                @.$(".edit-secondary.#{dc_uuid}").on 'click', (e) =>
                    @modify_replicas(e, datacenters.get(dc_uuid))
                @.$(".edit-machines.#{dc_uuid}").on 'click', (e) =>
                    @edit_machines(e, datacenters.get(dc_uuid))
                @.$(".make-primary.#{dc_uuid}").on 'click', (e) =>
                    @make_primary(e, datacenters.get(dc_uuid))
                @.$(".remove-secondary.#{dc_uuid}").on 'click', (e) =>
                    @remove_secondary(e, datacenters.get(dc_uuid))

            return @

#Doesnt work TODO
    class @AddSecondaryModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#add_secondary-modal-template').html()
        alert_tmpl: Handlebars.compile $('#modified_replica-alert-template').html()
        class: 'add-secondary'

        initialize: (namespace, datacenter) ->
            log_initial '(initializing) modal dialog: add secondary'
            @namespace = namespace
            @datacenter = datacenter
            super @template

        render: ->
            log_render '(rendering) add secondary dialog'

            # Define the validator options
            validator_options =
                submitHandler: =>
                    $('form', @$modal).ajaxSubmit
                        url: '/ajax/namespaces/'+@namespace.id+'/add_secondary?token=' + token
                        type: 'POST'

                        success: (response) =>
                            clear_modals()

                            # Parse the response JSON
                            response_json = $.parseJSON response
                            # apply the diffs to the backbone state
                            apply_diffs response_json.diffs
                            $('#user-alert-space').append @alert_tmpl {}

            namespace = @namespace
            _datacenters = _.filter datacenters.models, (datacenter) ->
                    uuid = datacenter.id
                    return (uuid isnt namespace.get('primary_uuid')) and (uuid not in _.keys namespace.get('replica_affinities'))
            json =
                'datacenters' : _.map(_datacenters, (datacenter) -> datacenter.toJSON())

            super validator_options, json

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
                        data: JSON.stringify({"name" : formdata.name, "primary_uuid" : formdata.primary_datacenter})

                        success: (response) =>
                            clear_modals()

                            # Parse the response JSON
                            response_json = $.parseJSON response
                            # apply the diffs to the backbone state
                            apply_diffs response_json.diffs
                            # the result of this operation are some attributes about the namespace we created, to be used in an alert
                            $('#user-alert-space').append @alert_tmpl response_json.op_result

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
                    url = '/ajax/namespaces?ids='
                    num_namespaces = namespaces_to_delete.length
                    for namespace in namespaces_to_delete
                        url += namespace.id
                        url += "," if num_namespaces-=1 > 0

                    url += '&token=' + token

                    $('form', @$modal).ajaxSubmit
                        url: url
                        type: 'DELETE'

                        success: (response) =>
                            clear_modals()

                            # Parse the response JSON, apply appropriate diffs, and show an alert
                            response_json = $.parseJSON response
                            apply_diffs response_json.diffs
                            for namespace in response_json.op_result
                                $('#user-alert-space').append @alert_tmpl namespace

            array_for_template = _.map namespaces_to_delete, (namespace) -> namespace.toJSON()
            super validator_options, { 'namespaces': array_for_template }

    class @EditMachinesModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#edit_machines-modal-template').html()
        # TODO is this the right template name, etc
        alert_tmpl: Handlebars.compile $('#modified_replica-alert-template').html()

        # TODO should the class be different?  What is class used for?
        class: 'modify-replicas'
        events: -> _.extend super,
            'click .commit-plan': 'commit_plan'

        initialize: (namespace, datacenter) ->
            console.log '(initializing) modal dialog: modify replicas review changes'
            @namespace = namespace
            @datacenter = datacenter
            super @template

        # Simple utility function to generate JSON for a set of machines
        machine_json: (machine_set) -> _.map machine_set, (machine) ->
            id: machine.get('id')
            name: machine.get('name')

        commit_plan: ->
            form_fields = $('form', @$modal).serializeArray()
            console.log 'commit changes!', $('form', @$modal).serializeArray()


        render: (num_replicas, num_acks) ->
            log_render "(rendering) modify replicas review changes dialog with #{num_replicas} replicas, #{num_acks} acks."

            validator_options =
                submitHandler: =>
                    $('form', @$modal).ajaxSubmit
                        url: '/ajax/namespaces/' + @namespace.id + '/edit_machines?token='+token
                        type: 'POST'
                        success: (response) =>
                            clear_modals()

                            # Parse the response JSON
                            response_json = $.parseJSON response
                            apply_diffs response_json.diffs
                            $('#user-alert-space').append @alert_tmpl {}


            num_changed = Math.abs(@namespace.get('replica_affinities')[@datacenter.id].desired_replication_count - num_replicas)
            adding = @namespace.get('replica_affinities')[@datacenter.id].desired_replication_count < num_replicas
            removing = @namespace.get('replica_affinities')[@datacenter.id].desired_replication_count > num_replicas

            # Build shards for template
            shards = []
            add_shard = null

            if adding
                add_shard = (index, start, end) =>
                    existing_machines = _.map(@namespace.get('replica_affinities')[@datacenter.id].machines[index], (id) -> machines.get(id))
                    shards.push
                        'adding': adding
                        'removing': removing
                        'name': "#{start} to #{end}"
                        'index': index
                        'machines': @machine_json (_.sortBy(_.difference(_.filter(machines.models, (m) => m.get('datacenter_uuid') == @datacenter.id), existing_machines), (m) -> m.get('name')))[0...num_changed]
                        'existing_machines': @machine_json existing_machines
            else if removing
                add_shard = (index, start, end) =>
                    existing_machines = _.map(@namespace.get('replica_affinities')[@datacenter.id].machines[index], (id) -> machines.get(id))
                    shards.push
                        'adding': adding
                        'removing': removing
                        'name': "#{start} to #{end}"
                        'index': index
                        'machines': @machine_json (_.sortBy existing_machines, (m) -> m.get('name'))[0...num_changed]
                        'existing_machines': @machine_json existing_machines
            else
                add_shard = (index, start, end) =>
                    existing_machines = _.map(@namespace.get('replica_affinities')[@datacenter.id].machines[index], (id) -> machines.get(id))
                    shards.push
                        'adding': adding
                        'removing': removing
                        'name': "#{start} to #{end}"
                        'index': index
                        'machines': @machine_json existing_machines
                        'existing_machines': @machine_json existing_machines

            start_split = '&minus;&infin;'
            end_split = '&infin;'
            prev_split = start_split

            index = 0
            for split in @namespace.get('shards')
                add_shard index, prev_split, split
                prev_split = split
                index += 1
            add_shard index, prev_split, end_split

#            filtered_machines = machines.filter (machine) => machine.get('datacenter_uuid') is @datacenter.get('id')
            filtered_machines = machines.filter (machine) => true

            # Right now our simulation doesn't have machines in every datacenter, so for the time being we'll just return all machines
            # TODO There's supposedly something wrong with having machines in every datacenter.
            machines_in_datacenter = @machine_json filtered_machines

            console.log 'machines_in_datacenter is', machines_in_datacenter
            console.log 'and all machines is', machines, 'filtered machines', filtered_machines
            console.log 'and datacenter is', @datacenter, 'with id', @datacenter.get('id')


            # Create a view for each shard
            shard_views = _.map shards, (shard) -> new ReplicaPlanShard machines_in_datacenter, shard

            # TODO (Holy TODO) this is a copy/paste of AbstractModal!  Holy crap!
            @$container.append @template
                'datacenter': @datacenter
                'replicas':
                    'adding': adding
                    'removing': removing
                    'num_changed': num_changed
                    'num': num_replicas
                    'shards': shards
                'acks':
                    'num': num_acks

            for view in shard_views
                renderee = view.render().el
                console.log 'appending shard_view..', renderee
                $('.shards tbody', @$container).append renderee

            # Note: Bootstrap's modal JS moves the modal from the container element to the end of the body tag in the DOM
            @$modal = $('.modal', @$container).modal
                'show': true
                'backdrop': true
                'keyboard': true
            .on 'hidden', =>
                # Removes the modal dialog from the DOM
                @$modal.remove()

            # Define @el to be the modal (the root of the view), make sure events perform on it
            @.setElement(@$modal)
            @.delegateEvents()

            # Fill in the passed validator options with default options
            validator_options = _.defaults validator_options, @validator_defaults
            @validator = $('form', @$modal).validate validator_options

            register_modal @


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
            console.log 'rendering with shard.machines being', @shard.machines
            @.$el.html @template
                'adding': @shard.adding
                'removing': @shard.removing
                'name': @shard.name
                'machines': _.map(@shard.machines, (machine) => { machine: machine, shard_index: @shard.index })  # FML
                'existing_machines': _.map(@shard.existing_machines, (machine) => { machine: machine, shard_index: @shard.index })  # FML

            return @

        edit: (e) ->
            console.log 'edit-rending with shard.machines being', @shard.machines
            @.$el.html @editable_tmpl
                'name': @shard.name
                'machine_dropdowns': _.map(@shard.machines, (machine) =>
                    'shard_index': @shard.index  # FML
                    'selected': machine
                    'available_machines': @available_machines
                )
                'adding': @shard.adding
                'removing': @shard.removing
                'existing_machines': _.map(@shard.existing_machines, (machine) => { machine: machine, shard_index: @shard.index })  # FML
            e.preventDefault()



    class @ModifyReplicasModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#modify_replicas-modal-template').html()
        # TODO: Do we ever use this alert tmpl?
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

        render: ->
            log_render '(rendering) modify replicas dialog'

            # Define the validator options
            validator_options =
                submitHandler: =>
                    $('form', @$modal).ajaxSubmit
                        url: '/ajax/namespaces/'+@namespace.id+'/set_desired_replicas_and_acks?token=' + token
                        type: 'POST'

                        success: (response) =>
                            clear_modals()

                            # Parse the response JSON
                            response_json = $.parseJSON response
                            # apply the diffs to the backbone state
                            apply_diffs response_json.diffs
                            $('#user-alert-space').append @alert_tmpl {}

            # Generate faked data TODO
            num_replicas = @namespace.get('replica_affinities')[@datacenter.id].desired_replication_count
            json =
                'namespace': @namespace.toJSON()
                'datacenter': @datacenter.toJSON()
                # Faked data TODO
                'num_replicas': num_replicas
                'num_acks': 3
                # random machines | faked TODO
                'replica_machines': @machine_json (_.shuffle machines.models)[0...num_replicas]

            super validator_options, json


    compute_renderable_shards_array = (shards) ->
        ret = []
        for i in [0...shards.length]
            json_repr = $.parseJSON(shards[i])
            ret.push
                lower: (if json_repr[0] == "" then "&minus;&infin;" else json_repr[0])
                upper: (if json_repr[1] == null then "+&infin;" else json_repr[1])
                notlast: i != shards.length
                index: i
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
            @model.on 'change', => @render()
            @shards = []

        render: =>
            log_render '(rendering) namespace view: shards'

            @.$el.html @template
                shards: compute_renderable_shards_array(@model.get('shards'))

            return @



    class @ModifyShardsModal extends ClusterView.AbstractModal
        template: Handlebars.compile $('#modify_shards_modal-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()
        class: 'modify-shards'

        initialize: (namespace, shard_set) ->
            log_initial '(initializing) modal dialog: ModifyShards'
            @namespace = namespace
            @shards = compute_renderable_shards_array(shard_set)

            # Keep an unmodified copy of the shard boundaries with which we compare against when reviewing the changes.
            @original_shard_set = _.map(shard_set, _.identity)
            @shard_set = _.map(shard_set, _.identity)
            super @template

        insert_splitpoint: (index, splitpoint) =>
            if (0 <= index || index < @shard_set.length)
                json_repr = $.parseJSON(@shard_set[index])
                if (splitpoint <= json_repr[0] || (splitpoint >= json_repr[1] && json_repr[1] != null))
                    raise "Error invalid splitpoint"

                @shard_set.splice(index, 1, JSON.stringify([json_repr[0], splitpoint]), JSON.stringify([splitpoint, json_repr[1]]))
                clear_modals()
                @render()
            else
                # TODO handle error

        merge_shard: (index) =>
            @shard_set.splice(index, 1)
            if (index < 0 || index + 1 >= @shard_set.length)
                raise "Error invalid index"

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

                            response_json = $.parseJSON(response)
                            apply_diffs(response_json.diffs)

                            # should be empty.
                            $('#user-alert-space').append(@alert_tmpl({}))


            json =
                namespace: @namespace.toJSON()
                shards: compute_renderable_shards_array(@shard_set)

            super validator_options, json

            shard_views = _.map(compute_renderable_shards_array(@shard_set), (shard) => new NamespaceView.ModifyShardsModalShard @namespace, shard, @)
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
            lower_bound = if shard_index == 0 then "&minus;&infin;" else shards[shard_index - 1]
            mid_bound = shards[shard_index]
            upper_bound = if shard_index + 1 == shards.length then "+&infin;" else shards[shard_index + 1]

            console.log 'shards to be merged: ',[lower_bound, mid_bound, upper_bound, shards, shard_index]

            @.$el.html @editable_tmpl
                merging: true
                shard: @shard
                lower_bound : lower_bound
                mid_bound: mid_bound
                upper_bound: upper_bound

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

    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'

        initialize: ->
            log_initial '(initializing) namespace view: container'
            @profile = new NamespaceView.Profile model: @model
            @replicas = new NamespaceView.Replicas model: @model
            @shards = new NamespaceView.Shards model: @model
        render: =>
            log_render '(rendering) namespace view: container'
            @.$el.append @profile.render().el
            @.$el.append @replicas.render().el
            @.$el.append @shards.render().el

            return @


# Namespace view
module 'NamespaceView', ->
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()
        datacenter_list_template: Handlebars.compile $('#namespace_view-replica-datacenters_list-template').html()
        alert_tmpl: Handlebars.compile $('#changed_primary_dc-replica-template').html()

        events: ->
            'click .nav_datacenter': 'handle_click_datacenter'
            'click .edit_mode-button': 'toggle_mode'

        initialize: =>
            datacenters.on 'add', @render_list
            datacenters.on 'remove', @render_list
            datacenters.on 'reset', @render_list

        toggle_mode: (event) =>
            console.log 'call'
            if @.$(event.target).html() is 'Edit mode'
                @datacenter_view.edit_mode_fn true
                @.$(event.target).html 'Read mode'
            else
                @datacenter_view.edit_mode_fn false
                @.$(event.target).html 'Edit mode'


        render_list: =>
            ordered_datacenters = _.map(datacenters.models, (datacenter) =>
                id: datacenter.get('id')
                name: if datacenter.get('name').length>8 then datacenter.get('name').slice(0, 8)+'...' else datacenter.get('name')
                active: @model.get('primary_uuid') is datacenter.get('id')
            )
            ordered_datacenters = ordered_datacenters.sort (a, b) ->
                if a.name > b.name
                    return 1
                else if a.name < b.name
                    return -1
                else
                    return 0

            ordered_datacenters.push
                id: universe_datacenter.get('id')
                name: universe_datacenter.get('name')
                active: @model.get('primary_uuid') is universe_datacenter.get('id')
                is_universe: true


            @.$('.datacenters_list_container').html @datacenter_list_template
                id: @model.get 'id'
                datacenters: ordered_datacenters

        handle_click_datacenter: (event) =>
            event.preventDefault()

            @render_datacenter @.$(event.target).data('datacenter_id')
            @.$('.datacenter_tab').removeClass 'active'
            @.$(event.target).parent().addClass 'active'

        render_datacenter: (datacenter_id) =>
            @datacenter_id_shown = datacenter_id
            @datacenter_view.destroy() if @datacenter_view?
            @datacenter_view = new NamespaceView.DatacenterReplicas @model, @datacenter_id_shown

            @.$('.datacenter_content').html @datacenter_view.render().$el
            @.$('.edit_mode-button').html 'Edit mode'

        render: =>
            @.$el.html @template()

            @render_list()

            if not @current_tab?
                @render_datacenter @model.get('primary_uuid')

            return @

        destroy: ->
            datacenters.off 'add', @render_list
            datacenters.off 'remove', @render_list
            datacenters.off 'reset', @render_list

    class @DatacenterReplicas extends Backbone.View
        className: 'datacenter_view'
        template: Handlebars.compile $('#namespace_view-datacenter_replica-template').html()
        edit_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-template').html()
        error_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-error-template').html()
        replicas_acks_success_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-success-template').html()
        replicas_ajax_error_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-ajax_error-template').html()
        need_replicas_template: Handlebars.compile $('#need_replicas-template').html()
        progress_bar_template: Handlebars.compile $('#backfill_progress_bar').html()

        events:
            'click .close': 'remove_parent_alert'
            'keyup #replicas_value': 'check_replicas_acks'
            'keyup #acks_value': 'check_replicas_acks'
            'click .save_replicas_and_acks': 'submit_replicas_acks'
            'click .make_primary': 'make_primary'

        initialize: (model, datacenter_id) =>
            @model = model
            @edit_mode = false
            @data = ''

            if datacenter_id is universe_datacenter.get('id')
                @datacenter = universe_datacenter
            else if datacenters.get(datacenter_id)?
                @datacenter = datacenters.get datacenter_id
            #TODO else

            @datacenter.on 'all', @render

            @model.on 'change:replica_affinities', @compute_max_machines_and_render
            @model.on 'change:primary_uuid', @compute_max_machines_and_render
            @model.on 'change:primary_uuid', @render_make_primary
            progress_list.on 'all', @render_progress
            
            @render_progress()
            @compute_max_machines_and_render()

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()

        edit_mode_fn: (edit_mode) =>
            @edit_mode = edit_mode
            @render true

        render_progress: =>
            progress_data = DataUtils.get_backfill_progress_agg @model.get('id'), @datacenter.get('id')

            if progress_data?
                @.$('.progress_bar_full_container').css 'display', 'block'
                @.$('.replica_container').css 'display', 'none'
                @.$('.progress_bar_container').html @progress_bar_template progress_data
            else
                @.$('.progress_bar_container').html ''
                @.$('.progress_bar_full_container').css 'display', 'none'
                @.$('.replica_container').css 'display', 'block'

        render: (force_render) =>
            replicas_count = DataUtils.get_replica_affinities(@model.get('id'), @datacenter.get('id'))
            if @model.get('primary_uuid') is @datacenter.get('id')
                replicas_count++


            data =
                name: @datacenter.get('name')
                total_machines: machines.length
                acks: DataUtils.get_ack_expectations(@model.get('id'), @datacenter.get('id'))
                primary: true if @model.get('primary_uuid') is @datacenter.get('id')
                replicas: replicas_count
                #status: DataUtils.get_namespace_status(@table.get('id'), uuid)
                
            data_in_json = JSON.stringify data
            if ((not force_render? or force_render is false) and @data isnt data_in_json) or (force_render is true)
                @data = data_in_json
                if @edit_mode is true
                    @.$el.html @edit_template data
                    @render_make_primary()
                else
                    @.$el.html @template data

            return @

        render_make_primary: =>
            if @edit_mode is true
                if @model.get('primary_uuid') is @datacenter.get('id')
                    can_become_primary = false
                else
                    num_replicas = @model.get('replica_affinities')?[@datacenter.get('id')]
                    if num_replicas? and parseInt(num_replicas) > 0
                        can_become_primary = true
                    else
                        can_become_primary = false
                        reason_cannot_become_primary = @need_replicas_template()

                if can_become_primary is true
                    @.$('.make_primary').removeAttr 'disabled'
                    @.$('.role_value').html 'Secondary'
                else
                    @.$('.make_primary').attr 'disabled', 'disabled'
                    @.$('.role_value').html 'Primary'

                if can_become_primary is false and reason_cannot_become_primary?
                    @.$('.reason_cannot_become_primary-alert').html @need_replicas_template()
                    @.$('.reason_cannot_become_primary-alert').slideDown 'fast'


        compute_max_machines_and_render: =>
            if @datacenter is universe_datacenter
                @max_machines = machines.length
                for datacenter_id of @model.get('replica_affinities')
                    if datacenter_id isnt universe_datacenter.get('id')
                        @max_machines -= @model.get('replica_affinities')[datacenter_id]
                if @model.get('primary_uuid') isnt universe_datacenter.get('id')
                    @max_machines -= 1
            else
                @max_machines = machines.length
                for datacenter_id of @model.get('replica_affinities')
                    if datacenter_id isnt @datacenter.get('id')
                        @max_machines -= @model.get('replica_affinities')[datacenter_id]
                if @model.get('primary_uuid') isnt @datacenter.get('id')
                    @max_machines -= 1
                
                @need_explanation = @max_machines < DataUtils.get_datacenter_machines(@datacenter.get('id')).length
                @max_machines = Math.min @max_machines, DataUtils.get_datacenter_machines(@datacenter.get('id')).length

            if @edit_mode is false
                @render()

        alert_replicas_acks: (msg_errors) =>
            @.$('.save_replicas_and_acks').prop 'disabled', 'disabled'
            @.$('.replicas_acks-alert').html @error_template
                msg_errors: msg_errors
            @.$('.replicas_acks-alert').slideDown 'fast'



        remove_alert_replicas_acks: =>
            @.$('.save_replicas_and_acks').removeProp 'disabled'
            @.$('.replicas_acks-alert').slideUp 'fast'
            @.$('.replicas_acks-alert').html ''

        check_replicas_acks: (event) =>
            if event?.which? and event.which is 13
                @submit_replicas_acks()

            num_replicas = @.$('#replicas_value').val()
            num_acks = @.$('#acks_value').val()

            # Validate first
            msg_error = []
            if num_replicas isnt '' and DataUtils.is_integer(num_replicas) is false
                msg_error.push('The number of replicas must be an integer.')
            if num_acks isnt '' and DataUtils.is_integer(num_acks) is false
                msg_error.push('The number of acks must be an integer.')

            if msg_error.length isnt 0
                @alert_replicas_acks msg_error
                return false



            num_replicas = parseInt num_replicas
            num_acks = parseInt num_acks
            # It is now safe to use parseInt

            msg_error = []
            if num_replicas > @max_machines
                msg_error.push('The number of replicas (' + num_replicas + ') cannot exceed the total number of machines (' + @max_machines + ').')
            if num_replicas is 0 and @model.get('primary_uuid') is @datacenter.get('id')
                msg_error.push('The number of replicas must be at least one because ' + @datacenter.get('name') + ' is the primary datacenter for this table.')
            if num_acks > num_replicas
                msg_error.push('The number of acks (' + num_acks + ') cannot exceed the total number of replicas (' + num_replicas + ').')
            if num_acks is 0 and num_replicas > 0
                msg_error.push('The value of acks must be greater than 0 if you have one replica or more.')

            if msg_error.length isnt 0
                @alert_replicas_acks msg_error
                return false

            @remove_alert_replicas_acks()
            return true

        adjustReplicaCount: (numreplicas, is_output) ->
            if @model.get('primary_uuid') is @datacenter.get('id')
                if is_output
                    return numreplicas + 1
                else
                    return numreplicas - 1
            else
                return numreplicas

        submit_replicas_acks: (event) =>
            if @check_replicas_acks() is false
                return

            num_replicas = parseInt @.$('#replicas_value').val()
            num_acks = parseInt @.$('#acks_value').val()

            replica_affinities_to_send = {}
            replica_affinities_to_send[@datacenter.get('id')] = @adjustReplicaCount(num_replicas, false)
            ack_expectations_to_send = {}
            ack_expectations_to_send[@datacenter.get('id')] = num_acks

            @data_cached =
                num_replicas: num_replicas
                num_acks: num_acks

            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify
                    replica_affinities: replica_affinities_to_send
                    ack_expectations: ack_expectations_to_send
                success: @on_success_replicas_and_acks
                error: @on_error

        on_success_replicas_and_acks: =>
            window.collect_progress()
            new_replicas = @model.get 'replica_affinities'
            new_replicas[@datacenter.get('id')] = @data_cached.num_replicas
            @model.set('replica_affinities', new_replicas)

            new_acks = @model.get 'ack_expectations'
            new_acks[@datacenter.get('id')] = @data_cached.num_acks
            @model.set('ack_expectations', new_acks)

            @.$('.replicas_acks-alert').html @replicas_acks_success_template()
            @.$('.replicas_acks-alert').slideDown 'fast'
            @render_make_primary()

        on_error: =>
            @.$('.replicas_acks-alert').html @replicas_ajax_error_template()
            @.$('.replicas_acks-alert').slideDown 'fast'

        make_primary: =>
            new_dc = @datacenter

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
            
            @data_switch_primary = data

            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                success: @on_success_make_primary
                error: @on_error

        on_success_make_primary: =>
            @model.set @data_switch_primary

        destroy: ->
            @model.off 'all', @render

    # Replicas view
    class @OldReplicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-old_replica-template').html()
        alert_tmpl: Handlebars.compile $('#changed_primary_dc-replica-template').html()

        events:
            'click .close': 'remove_parent_alert'
            'click .view-assignments': 'show_assignments'
            'click .edit_replicas': 'modify_replicas'
            'click .make_primary': 'make_primary'


        show_assignments: (event) =>
            event.preventDefault()
            modal = new NamespaceView.MachinesAssignmentsModal model: @model
            modal.render()

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



    class @MachinesAssignmentsModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#machine_assignments-modal-template').html()
        className: 'modal overwrite_modal'
        render: =>
            @pins = new NamespaceView.Pinning(model: @model)
            $('#modal-dialog').html @pins.render().$el
            modal = $('.modal').modal
                'show': true
                'backdrop': true
                'keyboard': true

            modal.on 'hidden', =>
                modal.remove()




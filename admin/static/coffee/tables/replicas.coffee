# Copyright 2010-2012 RethinkDB, all rights reserved.

# Namespace view
module 'TableView', ->
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.templates['namespace_view-replica-template']
        no_datacenter_template: Handlebars.templates['namespace_view-replica-no_datacenter-template']
        datacenter_list_template: Handlebars.templates['namespace_view-replica-datacenters_list-template']
        acks_greater_than_replicas_template: Handlebars.templates['namespace_view-acks_greater_than_replicas-template']
        replica_status_template: Handlebars.templates['replica_status-template']
        events:
            'click .nav_datacenter': 'handle_click_datacenter'
            'click .toggle-mdc': 'toggle_mdc'

        check_issues: =>
            # We look for vector clock conflict (cluster configuration conflict) issues related to this table
            # If there are, the server will reject any attempt to directly change the value, so
            # so we want to
            #    - tell them to fix the vector clock conflict
            #    - forbid users to try to manually change the value
            can_change_replicas = true
            for issue in issues.models
                if issue.get('type') is 'VCLOCK_CONFLICT' and
                    issue.get('object_id') is @model.get('id') and
                    issue.get('object_type') is 'namespace' and
                    (issue.get('field') is 'replica_affinities' or issue.get('field') is 'ack_expectations')
                        can_change_replicas = false
            if can_change_replicas is false
                @primary_datacenter.is_modifiable false
                @universe_replicas.is_modifiable false
                @datacenter_view?.is_modifiable false

                if @can_change_replicas is true
                    @$('.cannot_change_replicas').slideDown 'fast'
                    @can_change_replicas = can_change_replicas
            else if can_change_replicas is true
                @primary_datacenter.is_modifiable true
                @universe_replicas.is_modifiable true
                @datacenter_view?.is_modifiable true

                if @can_change_replicas is false # can_change_replicas is true
                    @$('.cannot_change_replicas').slideUp 'fast'
                    @can_change_replicas = can_change_replicas

        initialize: =>
            @can_change_replicas = true
            @progress_bar = new UIComponents.OperationProgressBar @replica_status_template

            datacenters.on 'add', @render_list
            datacenters.on 'remove', @render_list
            datacenters.on 'reset', @render_list
            issues.on 'all', @check_issues

            @model.on 'change:primary_uuid', @render_primary_not_found
            @model.on 'change:replica_affinities', @global_trigger_for_replica
            @model.on 'change:ack_expectations', @render_acks_greater_than_replicas
            @model.on 'change:shards', @render_progress_server_update
            progress_list.on 'all', @render_progress
            directory.on 'all', @render_status

            
            # Initialize @expected_num_replicas
            num_shards = @model.get('shards').length
            num_replicas = 1 # For master
            for datacenter_id of @model.get('replica_affinities')
                num_replicas += @model.get('replica_affinities')[datacenter_id]
            @expected_num_replicas = num_replicas*num_shards # Stores the number of replicas we expect

            @universe_replicas = new TableView.DatacenterReplicas universe_datacenter.get('id'), @model
            @primary_datacenter = new TableView.PrimaryDatacenter model: @model

            @render_status() # Render the status

        # A method that is going to call multiple methods (triggered when replica_affinities are changed
        global_trigger_for_replica: =>
            @render_acks_greater_than_replicas()
            @render_progress_server_update()

        # Update the progress bar if the server send us an update (so we can handle the case something was changed from another place)
        render_progress_server_update: =>
            @progress_bar.skip_to_processing() # We set the state to processing

            # Compute the number of replicas we require
            new_replicas = @model.get('replica_affinities')
            replicas_length = 1 # 1 for the master
            for datacenter_id of new_replicas
                replicas_length += new_replicas[datacenter_id]

            shards_length = @model.get('shards').length

            @render_status
                got_response: true
                replicas_length: replicas_length
                shards_length: shards_length

        # Trigger by progress_list
        render_progress: =>
            @render_status
                backfilling_updated: true

        # Compute the status of all replicas
        #   - progress_bar_info: optional argument that informs the progress bar backing this status
        render_status: (progress_bar_info) =>
            if not progress_bar_info? or typeof progress_bar_info isnt 'object'
                progress_bar_info = {}

            # If the blueprint is not ready, we just skip it
            blueprint = @model.get('blueprint').peers_roles
            if not blueprint?
                return ''


            # Compute how many replicas are ready according to the blueprint
            num_replicas_not_ready = 0
            num_replicas_ready = 0
            for machine_id of blueprint
                for shard of blueprint[machine_id]
                    found_shard = false
                    shard_ready = true

                    role = blueprint[machine_id][shard]
                    if role is 'role_nothing'
                        continue

                    if role is 'role_primary'
                        expected_status = 'primary'
                    else if role is 'role_secondary'
                        expected_status = 'secondary_up_to_date'

                    # Loop over directory
                    activities = directory.get(machine_id)?.get(@model.get('protocol')+'_namespaces')?['reactor_bcards'][@model.get('id')]?['activity_map']
                    if activities?
                        for activity_id of activities
                            activity = activities[activity_id]
                            if activity[0] is shard
                                found_shard = true
                                if activity[1]['type'] isnt expected_status
                                    shard_ready = false
                                    break

                    if found_shard is false or shard_ready is false
                        num_replicas_not_ready++
                    else
                        num_replicas_ready++
            num_replicas = num_replicas_ready + num_replicas_not_ready

            # The user just changed the number of replicas
            if progress_bar_info?.new_value?
                @.$('.replica-status').html @progress_bar.render(0, expected_num_replicas, progress_bar_info).$el
                @expected_num_replicas = progress_bar_info.new_value # Let's cache this value for the case when the blueprint was not regenerated (yet)

                # Reset the number of replicated blocks
                @total_blocks = undefined
                @replicated_blocks = undefined
            # The server did valid the changes the user just made
            else if progress_bar_info?.got_response is true
                expected_num_replicas = progress_bar_info.replicas_length*progress_bar_info.shards_length
                @.$('.replica-status').html @progress_bar.render(0, expected_num_replicas, progress_bar_info).$el

                # Reset the number of replicated blocks
                @expected_num_replicas = expected_num_replicas # Let's cache this value for the case when the blueprint was not regenerated (yet)
                @total_blocks = undefined
                @replicated_blocks = undefined

            # If we got an update from progress_list
            else if progress_bar_info?.backfilling_updated is true
                # Make sure we have a match between what we want and what we have ( = blueprints have been regenerated)
                if num_replicas_ready+num_replicas_not_ready is @expected_num_replicas
                    backfilling_info = DataUtils.get_backfill_progress_agg @model.get('id')

                    if backfilling_info is null or backfilling_info.total_blocks is -1 # If there is no backfilling
                        # Backfilling sent back non valid info, so we reset the values for replication
                        @total_blocks = undefined
                        @replicated_blocks = undefined

                        # We don't know if the backfilling hasn't started yet or is completed, so let's check the directory status
                        if num_replicas_not_ready is 0 # Well, everything is up to date
                            @.$('.replica-status').html @progress_bar.render(num_replicas_ready, num_replicas_ready, progress_bar_info).$el
                        else # We are going to backfill
                            @.$('.replica-status').html @progress_bar.render(num_replicas_ready, num_replicas_ready+num_replicas_not_ready, progress_bar_info).$el
                    else
                        # Cache replicated blocks values
                        @total_blocks = backfilling_info.total_blocks
                        @replicated_blocks = if backfilling_info.replicated_blocks>backfilling_info.replicated_blocks then backfilling_info.total_blocks else backfilling_info.replicated_blocks
                        # We can have replicated_blocks > total_blocks sometimes. Need a back end fix.
                        progress_bar_info = _.extend progress_bar_info,
                            total_blocks: @total_blocks
                            replicated_blocks: @replicated_blocks
                    
                        @.$('.replica-status').html @progress_bar.render(num_replicas_ready, num_replicas_ready+num_replicas_not_ready, progress_bar_info).$el
                else # The blueprint was not regenerated, so we can consider that no replica is up to date
                    @.$('.replica-status').html @progress_bar.render(0, @expected_num_replicas, {}).$el
            else
                # Blueprint was regenerated, so we can display the bar

                # If a replica is not ready, we must display a progress bar.
                # In case we don't, se skip to processing (we do not show the "Started stage"
                if num_replicas_not_ready > 0 and @progress_bar.stage is 'none'
                    @progress_bar.skip_to_processing() # We set the state to processing

                if @total_blocks? and @replicated_blocks?
                    # @render_status was called by a change in the directory
                    progress_bar_info = _.extend progress_bar_info,
                        total_blocks: @total_blocks
                        replicated_blocks: @replicated_blocks

                if num_replicas_ready+num_replicas_not_ready is @expected_num_replicas
                    @.$('.replica-status').html @progress_bar.render(num_replicas_ready, num_replicas_ready+num_replicas_not_ready, progress_bar_info).$el
                else # The blueprint was not regenerated, so we can consider that no replica is up to date
                    @.$('.replica-status').html @progress_bar.render(0, @expected_num_replicas, {}).$el
            return @

        # Render the list of datacenters for MDC
        render_list: =>
            that = @
            @ordered_datacenters = _.map(datacenters.models, (datacenter) ->
                id: datacenter.get('id')
                name: if datacenter.get('name').length>8 then datacenter.get('name').slice(0, 8)+'...' else datacenter.get('name')
                namespace_id: that.model.get('id')
            )
            # Sort datacenters by name
            @ordered_datacenters = @ordered_datacenters.sort (a, b) ->
                if a.name > b.name
                    return 1
                else if a.name < b.name
                    return -1
                else
                    return 0

            # If an active datacenter exists, we activate the same
            found_active = false
            if @active_datacenter_id?
                for datacenter, i in @ordered_datacenters
                    if datacenter.id is @active_datacenter_id
                        datacenter.active = true
                        found_active = true
                        break

            # If we couldn't find the last active datacenter
            if found_active is false
                # If the primary is Universe, we pick fhe first datacenter
                if @model.get('primary_uuid') is universe_datacenter.get('id')
                    if @ordered_datacenters.length > 0
                        @ordered_datacenters[0].active = true
                        @active_datacenter_id = @ordered_datacenters[0].id
                        found_active = true
                else # Else, we look for the datacenter to display
                    for datacenter, i in @ordered_datacenters
                        if datacenter.id is @model.get('primary_uuid') and datacenters.get(datacenter.id)? # We are going to display it if it's the primary AND if it does exist
                            datacenter.active = true
                            @active_datacenter_id = datacenter.id
                            found_active = true
                            break
            
            # If we still coundn't find it, we activate the first one
            if found_active is false
                if @ordered_datacenters.length > 0
                    @ordered_datacenters[0].active = true
                    @active_datacenter_id = @ordered_datacenters[0].id

            # Show the list of datacenters
            @.$('.datacenters_list_container').html @datacenter_list_template
                id: @model.get 'id'
                datacenters: @ordered_datacenters

            # If datacenter_view is null (no datacenter) or undefined (first pass), we display the active datacenter.
            if not @datacenter_view? and @active_datacenter_id?
                @render_datacenter @active_datacenter_id

            # If there is no more datacenter, we display a message saying that there is no datacenter
            if @ordered_datacenters.length is 0
                if @datacenter_view?
                    @datacenter_view.destroy()
                @datacenter_view = null
                @.$('.datacenter_content').html @no_datacenter_template()

            if @ordered_datacenters.length is 0
                @.$('.primary-dc').hide()
            else
                @.$('.primary-dc').show()

        handle_click_datacenter: (event) =>
            event.preventDefault()

            @render_datacenter @.$(event.target).data('datacenter_id')
            @.$('.datacenter_tab').removeClass 'active'
            @.$(event.target).parent().addClass 'active'

        render_datacenter: (datacenter_id) =>
            @active_datacenter_id = datacenter_id
            @datacenter_id_shown = datacenter_id
            @datacenter_view.destroy() if @datacenter_view?
            @datacenter_view = new TableView.DatacenterReplicas @datacenter_id_shown, @model

            @.$('.datacenter_content').html @datacenter_view.render().$el
            @datacenter_view.is_modifiable @can_change_replicas

        render_primary_not_found: =>
            if @model.get('primary_uuid') isnt universe_datacenter.get('id') and not datacenters.get(@model.get('primary_uuid'))?
                if @.$('.no_datacenter_found').css('display') is 'none' or @.$('.no_datacenter_found').css('display') is 'hidden' or @.$('.no_datacenter_found').css('display') is ''
                    @.$('.no_datacenter_found').show()
            else
                 if @.$('.no_datacenter_found').css('display') is 'block'
                    @.$('.no_datacenter_found').hide()
               
        # While waiting for the back end to create this unsatisfiable goals
        render_acks_greater_than_replicas: =>
            datacenters_with_issues = []
            for datacenter_id of @model.get('ack_expectations')
                # If the datacenter doesn't exist anymore, there is no problem
                if datacenters.get(datacenter_id)? or datacenter_id is universe_datacenter.get('id')
                    replicas_value = 0
                    if @model.get('replica_affinities')[datacenter_id]?
                        replicas_value = @model.get('replica_affinities')[datacenter_id]

                    if @model.get('primary_uuid') is datacenter_id
                        replicas_value++

                    if replicas_value < @model.get('ack_expectations')[datacenter_id].expectation
                        if datacenters.get(datacenter_id)?
                            datacenter_name = datacenters.get(datacenter_id).get('name')
                        else if datacenter_id is universe_datacenter.get('id')
                            datacenter_name = universe_datacenter.get('name')
                        datacenters_with_issues.push
                            id: datacenter_id
                            name: datacenter_name

            if datacenters_with_issues.length>0
                if @.$('.ack_greater_than_replicas').css('display') is 'none' or @.$('.no_datacenter_found').css('display') is 'hidden' or @.$('.no_datacenter_found').css('display') is ''
                    @.$('.ack_greater_than_replicas').html @acks_greater_than_replicas_template
                        num_datacenters_with_issues: datacenters_with_issues.length
                        datacenters_with_issues: datacenters_with_issues
                    @.$('.ack_greater_than_replicas').show()
            else
                if @.$('.ack_greater_than_replicas').css('display') is 'block'
                    @.$('.ack_greater_than_replicas').hide()


        toggle_mdc: (event) =>
            event.preventDefault()
            @.$('.mdc-options').toggleClass('hidden')
            @.$('.show-mdc, .hide-mdc').toggle()

        render_universe: =>
            @.$('.cluster_container').html @universe_replicas.render().$el

        render: =>
            @.$el.html @template()

            @render_list()
            @render_primary_not_found()
            @render_acks_greater_than_replicas()
            @render_universe()
            @render_status()

            @.$('.primary-dc').html @primary_datacenter.render().$el
            if @model.get('primary_uuid') is universe_datacenter.get('id')
                if @ordered_datacenters.length > 0
                    @datacenter_picked = @ordered_datacenters[0]
                    @render_datacenter @datacenter_picked.id
                else
                    @datacenter_view = null
                    @.$('.datacenter_content').html @no_datacenter_template()
            else
                @render_datacenter @model.get('primary_uuid')

            @check_issues()
            return @

        destroy: =>
            if @datacenter_view
                @datacenter_view.destroy()
            datacenters.off 'add', @render_list
            datacenters.off 'remove', @render_list
            datacenters.off 'reset', @render_list
            @model.off 'change:primary_uuid', @render_primary_not_found
            @model.off 'change:replica_affinities', @render_acks_greater_than_replicas
            @model.off 'change:ack_expectations', @render_acks_greater_than_replicas
            @model.off 'change:shards', @render_progress_server_update
            progress_list.off 'all', @render_progress
            directory.off 'all', @render_status

    class @DatacenterReplicas extends Backbone.View
        className: 'datacenter_view'
        template: Handlebars.templates['namespace_view-datacenter_replica-template']
        universe_template: Handlebars.templates['namespace_view-universe_replica-template']
        edit_template: Handlebars.templates['namespace_view-edit_datacenter_replica-template']
        error_template: Handlebars.templates['namespace_view-edit_datacenter_replica-error-template']
        error_msg_template: Handlebars.templates['namespace_view-edit_datacenter_replica-alert_messages-template']
        replicas_acks_success_template: Handlebars.templates['namespace_view-edit_datacenter_replica-success-template']
        replication_complete_template: Handlebars.templates['namespace_view-edit_datacenter_replica-replication_done-template']
        replication_status: Handlebars.templates['namespace_view-edit_datacenter_replica-replication_status-template']
        replicas_ajax_error_template: Handlebars.templates['namespace_view-edit_datacenter_replica-ajax_error-template']
        need_replicas_template: Handlebars.templates['need_replicas-template']
        progress_bar_template: Handlebars.templates['backfill_progress_bar']
        success_set_primary: Handlebars.templates['changed_primary_dc-replica-template']
        states: ['read_only', 'editable']

        events:
            'click .close': 'remove_parent_alert'
            'click .make-primary.btn': 'make_primary'
            'click .update-replicas.btn': 'submit_replicas_acks'
            'click .edit.btn': 'edit'
            'click .cancel.btn': 'cancel_edit'
            'keyup #replicas_value': 'keypress_replicas_acks'
            'keyup #acks_value': 'keypress_replicas_acks'

        is_modifiable: (is_modifiable) =>
            if is_modifiable is false
                @$('.edit').prop 'disabled', true
                @can_change_replicas = is_modifiable
            else if is_modifiable is true
                @$('.edit').prop 'disabled', false
                @can_change_replicas = is_modifiable

        keypress_replicas_acks: (event) =>
            if event.which is 13
                event.preventDefault()
                @submit_replicas_acks()

        initialize: (datacenter_id, model) =>
            @model = model
            @current_state = @states[0]
            @data = ''

            if datacenter_id is universe_datacenter.get('id')
                @datacenter = universe_datacenter
            else if datacenters.get(datacenter_id)?
                @datacenter = datacenters.get datacenter_id
            if @datacenter?
                @datacenter.on 'all', @render

            @model.on 'change:primary_uuid', @render
            progress_list.on 'all', @render_progress
            #TODO Clean progress bar from this view

            @model.on 'change:ack_expectations', @render_acks_replica
            @model.on 'change:replica_affinities', @render_acks_replica

            @render_progress()

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()

        edit: =>
            @current_state = @states[1]
            @render()

        cancel_edit: =>
            @current_state = @states[0]
            @render()

        render_acks_replica: =>
            if @current_state is @states[0]
                @render()

        render_progress: =>
            if not @datacenter?
                return ''
            progress_data = DataUtils.get_backfill_progress_agg @model.get('id'), @datacenter.get('id')

            if progress_data? and _.isNaN(progress_data.percentage) is false
                @is_backfilling = true
                @.$('.progress_bar_full_container').slideDown 'fast'
                @.$('.progress_bar_container').html @progress_bar_template progress_data
            else
                if @is_backfilling is true
                    collect_server_data_once()
                @is_backfilling = false
                @.$('.progress_bar_container').empty()
                @.$('.progress_bar_full_container').slideUp 'fast'

        render: =>
            if not @datacenter?
                return ''
            replicas_count = DataUtils.get_replica_affinities(@model.get('id'), @datacenter.get('id'))
            if @model.get('primary_uuid') is @datacenter.get('id')
                replicas_count++

            if @current_state is @states[1]
                max_machines = @datacenter.compute_num_machines_not_used_by_other_datacenters(@model)
                if @datacenter.get('id') isnt universe_datacenter.get('id')
                    max_machines = Math.min max_machines, DataUtils.get_datacenter_machines(@datacenter.get('id')).length

            data =
                name: @datacenter.get('name')
                total_machines: machines.length
                acks: DataUtils.get_ack_expectations(@model.get('id'), @datacenter.get('id'))
                primary: @model.get('primary_uuid') is @datacenter.get('id')
                replicas: replicas_count
                editable: @current_state is @states[1]
                max_replicas: (max_machines if max_machines?)
                max_acks: (max_machines if max_machines?)

            # Don't re-render if the data hasn't changed
            if not _.isEqual(data, @data)
                @data = data
                if @datacenter.get('id') is universe_datacenter.get('id')
                    @.$el.html @universe_template data
                else
                    @.$el.html @template data
                if @current_state is @states[1]
                    @.$('#replicas_value').focus()

            @is_modifiable @can_change_replicas
            return @

        # Compute how many replica we can set for @datacenter
        compute_max_machines: =>
            @max_machines = @datacenter.compute_num_machines_not_used_by_other_datacenters(@model)
 
            # If we can't use all our machines in the datacenter because the replicas value of Universe is too high, we give a little more explanation
            @need_explanation = @max_machines < DataUtils.get_datacenter_machines(@datacenter.get('id')).length
            if @datacenter.get('id') isnt universe_datacenter.get('id')
                @max_machines = Math.min @max_machines, DataUtils.get_datacenter_machines(@datacenter.get('id')).length

            # We render only if we are not editing
            if @current_state isnt @states[1]
                @render()

        alert_replicas_acks: (msg_errors) =>
            @.$('.save_replicas_and_acks').prop 'disabled', true
            @.$('.replicas_acks-alert').html @error_template
                msg_errors: msg_errors
            @.$('.replicas_acks-alert').slideDown 'fast'

        remove_alert_replicas_acks: =>
            @.$('.save_replicas_and_acks').prop 'disabled', false
            @.$('.replicas_acks-alert').slideUp 'fast'
            @.$('.replicas_acks-alert').html ''

        check_replicas_acks: (event) =>
            # Update @max_machines
            @compute_max_machines()

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
                msg_error.push @error_msg_template
                    too_many_replicas: true
                    num_replicas: num_replicas
                    max_machines: @max_machines
            if num_replicas is 0 and @model.get('primary_uuid') is @datacenter.get('id')
                msg_error.push @error_msg_template
                    need_at_least_one_replica: true
                    name: @datacenter.get('name')
            if num_acks > num_replicas
                msg_error.push @error_msg_template
                    too_many_acks: true
                    num_acks: num_acks
                    num_replicas: num_replicas
            if msg_error.length isnt 0
                @alert_replicas_acks msg_error
                return false

            @remove_alert_replicas_acks()
            return true

        submit_replicas_acks: (event) =>
            if @check_replicas_acks() is false
                return
            if @sending? and @sending is true
                return
            @sending = true
            @.$('.update-replicas').prop 'disabled', true

            num_replicas = parseInt @.$('#replicas_value').val()
            num_acks = parseInt @.$('#acks_value').val()

            # adjust the replica count to only include secondaries
            if @model.get('primary_uuid') is @datacenter.get('id')
                num_replicas -= 1

            replica_affinities_to_send = {}
            replica_affinities_to_send[@datacenter.get('id')] = num_replicas
            ack_expectations_to_send = {}
            ack_expectations_to_send[@datacenter.get('id')] =
                expectation: num_acks
                hard_durability: @model.get_durability()

            @data_cached =
                num_replicas: num_replicas
                num_acks: num_acks

            # Add the progress bar
            # We count master, so we don't substract one
            replicas_length = 1 # 1 for the master
            for datacenter_id of replica_affinities_to_send
                replicas_length += replica_affinities_to_send[datacenter_id]
            window.app.current_view.replicas.render_status
                new_value: replicas_length*@model.get('shards').length

            window.app.current_view.shards.render_status
                new_value: @model.get('shards').length
            window.app.current_view.server_assignments.render()

            $.ajax
                processData: false
                url: "ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify
                    replica_affinities: replica_affinities_to_send
                    ack_expectations: ack_expectations_to_send
                success: @on_success_replicas_and_acks
                error: @on_error

        on_success_replicas_and_acks: =>
            @sending = false
            @.$('.update-replicas').prop 'disabled', false
            window.collect_progress()
            new_replicas = @model.get 'replica_affinities'
            new_replicas[@datacenter.get('id')] = @data_cached.num_replicas
            @model.set('replica_affinities', new_replicas)

            new_acks = DataUtils.deep_copy @model.get 'ack_expectations' # We need a deep copy or .set() is not going to trigger any events
            new_acks[@datacenter.get('id')] =
                expectation: @data_cached.num_acks
                hard_durability: @model.get_durability()
            @model.set('ack_expectations', new_acks)

            @current_state = @states[0]
            @render()

            replicas_length = 1 # 1 for the master
            for datacenter_id of new_replicas
                replicas_length += new_replicas[datacenter_id]
            window.app.current_view.replicas.render_status
                got_response: true
                replicas_length: replicas_length
                shards_length: @model.get('shards').length

            window.app.current_view.replicas
            window.app.current_view.shards.render_status
                got_response: true
            #@.$('.replicas_acks-alert').html @replicas_acks_success_template()
            #@.$('.replicas_acks-alert').slideDown 'fast'
            # create listener + state
            @replicating = true
            @.$('.status-alert').hide()

        on_error: =>
            @sending = false
            @.$('.update-replicas').prop 'disabled', false

            @.$('.replicas_acks-alert').html @replicas_ajax_error_template()
            @.$('.replicas_acks-alert').slideDown 'fast'

        make_primary: =>
            new_dc = @datacenter

            # Checking if there is at least one replica because we need it.
            if not @model.get('replica_affinities')[new_dc.get('id')]? or @model.get('replica_affinities')[new_dc.get('id')] < 1
                @.$('.make_primary-alert-content').html @error_msg_template
                    need_replica_for_primary: true
                @.$('.make_primary-alert').slideDown 'fast'
                return ''


            new_affinities = {}
            if @model.get('primary_uuid') is universe_datacenter.get('id')
                old_dc = universe_datacenter
            else
                old_dc = datacenters.get(@model.get('primary_uuid'))
            if old_dc? # The datacenter may have been deleted
                new_affinities[old_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), old_dc.get('id')) + 1
            new_affinities[new_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), new_dc.get('id')) - 1

            primary_pinnings = {}
            for shard of @model.get('primary_pinnings')
                primary_pinnings[shard] = null

            data =
                primary_uuid: new_dc.get('id')
                replica_affinities: new_affinities
                primary_pinnings: primary_pinnings
            @data_cached = data
            $.ajax
                processData: false
                url: "ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                success: @on_success
                error: @on_error

        on_success: => #TODO
            @model.set @data_cached
            @.$('.make_primary-alert-content').html @success_set_primary()
            @.$('.make_primary-alert').slideDown 'fast'


        destroy: =>
            if @datacenter?
                @datacenter.off 'all', @render

            @model.off 'change:primary_uuid', @render
            progress_list.off 'all', @render_progress

            @model.off 'change:ack_expectations', @render_acks_replica
            @model.off 'change:replica_affinities', @render_acks_replica

    class @PrimaryDatacenter extends Backbone.View
        template: Handlebars.templates['namespace_view-primary_datacenter-template']
        content_template: Handlebars.templates['namespace_view-primary_datacenter_content-template']

        # These are the possible states for the ProgressBar
        states: ['none', 'show_primary', 'choose_primary', 'confirm_off']
        initialize: =>
            @state = 'none'
            @model.on 'change:primary_uuid', @change_pin

        is_modifiable: (is_modifiable) =>
            if is_modifiable is false
                @$('.edit').prop 'disabled', true
                @$('.switch-input, .primary-off, .primary-on').prop 'disabled', true
            else
                @$('.edit').prop 'disabled', false
                @$('.switch-input, .primary-off, .primary-on').prop 'disabled', false

        events: ->
            'click label[for=primary-on]': 'turn_primary_on'
            'click label[for=primary-off]': 'turn_primary_off'
            'click .btn.change-primary': 'change_primary'
            'click .btn.submit-change-primary': 'submit_change_primary'
            'click .btn.cancel-change-primary': 'cancel_change_primary'
            'click .btn.cancel-confirm-off': 'cancel_confirm_off'
            'click .btn.submit-confirm-off': 'submit_confirm_off'
            'click .btn.edit': 'edit_primary'
            'click .alert .close': 'close_error'

        close_error: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        change_pin: =>
            if @model.get('primary_uuid') is universe_datacenter.get('id')
                @state = 'none'
                @.$('#primary-off').trigger('click')
                @turn_primary_off()
            else
                @state = 'none'
                @.$('#primary-on').trigger('click')
                @turn_primary_on()

        edit_primary: =>
            @state = 'choose_primary'
            @turn_primary_on true
        cancel_confirm_off: =>
            @.$('#primary-on').trigger('click')
            @turn_primary_on()

        submit_confirm_off: =>
            new_primary = universe_datacenter.get('id')
            current_primary = @model.get('primary_uuid')
            @set_new_primary new_primary, current_primary, @on_success_off, @on_error_off

        on_success_off: =>
            data_to_set = @data_cached
            for dc, value of @model.get('replica_affinities')
                if not data_to_set['replica_affinities'][dc]?
                    data_to_set['replica_affinities'][dc] = value

            @model.set data_to_set
            @model.trigger 'change:primary_uuid'
            # Not working?
            @turn_primary_off()

        on_error_off: =>
            @.$('.alert-error').slideDown 'fast'

        render: =>
            data =
                force_on: false

            if @model.get('primary_uuid') isnt universe_datacenter.get('id')
                @state = 'show_primary'
                data.force_on = true
            @.$el.html @template data
            @render_content()
            return @

        render_content: (data) =>
            if not data?
                data = {}
            if @model.get('primary_uuid') is universe_datacenter
                primary_name = 'No primary' # never displayed
            else if datacenters.get(@model.get('primary_uuid'))?
                primary_name = datacenters.get(@model.get('primary_uuid')).get('name')
            else
                primary_name = 'Not found datacenter'
            data = _.extend data,
                primary_isnt_universe: @model.get('primary_uuid') isnt universe_datacenter.get('id')
                primary_id: @model.get('primary_uuid')
                primary_name: primary_name
            
            if @state is 'confirm_off'
                data.confirm_off = true
                data.primary_id = @model.get('primary_uuid')
                if @model.get('primary_uuid') is universe_datacenter.get('id')
                    datacenter_name = 'Cluster' # Should not be used
                else if datacenters.get(@model.get('primary_uuid'))?
                    datacenter_name = datacenters.get(@model.get('primary_uuid')).get('name')
                else
                    datacenter_name = 'A deleted datacenter'
                data.primary_name = datacenter_name

            if @state is 'show_primary'
                data.show_primary = true
                data.primary_id = @model.get('primary_uuid')

                if @model.get('primary_uuid') is universe_datacenter.get('id')
                    datacenter_name = 'Cluster' # Should not be used
                else if datacenters.get(@model.get('primary_uuid'))?
                    datacenter_name = datacenters.get(@model.get('primary_uuid')).get('name')
                else
                    datacenter_name = 'A deleted datacenter'
                data.primary_name = datacenter_name

            if @state is 'choose_primary'
                data.choose_primary = true
                that = @

                #TODO order these datacenters and remove the empty ones
                data.datacenters = _.map datacenters.models, (datacenter) ->
                    id: datacenter.get('id')
                    name: datacenter.get('name')
                    is_primary: datacenter.get('id') is that.model.get('primary_uuid')
                if @model.get('primary_uuid') isnt universe_datacenter.get('id')
                    data.primary_dc_uuid = @model.get('primary_uuid')
                    data.primary_dc_name = datacenters.get(@model.get('primary_uuid')).get('name')


            @.$('.content').html @content_template data

        # Event handlers that change state
        turn_primary_off: =>
            if @$('.switch-input').prop('disabled') isnt true
                if @model.get('primary_uuid') is universe_datacenter.get('id') # Universe is being used so no need to confirm
                    @state = 'none'
                else
                    @state = 'confirm_off'
                @render_content()

        turn_primary_on: (force_choose) =>
            if @$('.switch-input').prop('disabled') isnt true
                if @model.get('primary_uuid') is universe_datacenter.get('id') or force_choose is true
                    @state = 'choose_primary'
                else
                    @state = 'show_primary'
                @render_content()

        change_primary: =>
            @state = 'choose_primary'
            @render_content()

        submit_change_primary: =>
            new_primary = @.$('.datacenter_uuid_list').val()
            current_primary = @model.get('primary_uuid')

            @set_new_primary new_primary, current_primary, @on_success_pin, @on_error_pin

        set_new_primary: (new_primary, current_primary, on_success, on_error) =>

            primary_pinnings = {}
            for shard in @model.get('primary_pinnings')
                primary_pinnings[shard] = null

            # Create new replica affinities
            new_replica_affinities = {}
            # For the current primary, it's plus one
            if @model.get('replica_affinities')[current_primary]?
                new_replica_affinities[current_primary] = @model.get('replica_affinities')[current_primary]+1
            else
                new_replica_affinities[current_primary] = 1

            # For the new primary, it's -1 or 0
            if @model.get('replica_affinities')[new_primary]?
                if @model.get('replica_affinities')[new_primary] > 0
                    new_replica_affinities[new_primary] = @model.get('replica_affinities')[new_primary] - 1
                else
                    new_replica_affinities[new_primary] = 0
            else
                new_replica_affinities[new_primary] = 0

            new_ack = @model.get('ack_expectations')
            if (not @model.get('ack_expectations')[new_primary]?) or @model.get('ack_expectations')[new_primary].expectation is 0
                new_ack[new_primary] =
                    expectation: 1
                    hard_durability: @model.get_durability()



            data =
                primary_uuid: new_primary
                primary_pinnings: primary_pinnings
                replica_affinities: new_replica_affinities
                ack_expectations: new_ack

            @data_cached = data
            $.ajax
                url: "ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                success: on_success
                error: on_error



        on_success_pin: =>
            data_to_set = @data_cached
            for dc, value of @model.get('replica_affinities')
                if not data_to_set['replica_affinities'][dc]?
                    data_to_set['replica_affinities'][dc] = value

            @model.set data_to_set

            @model.trigger 'change:primary_uuid'
            @turn_primary_on()

        on_error_pin: =>
            @.$('.alert-error').slideDown 'fast'

        cancel_change_primary: =>
            if @model.get('primary_uuid') is universe_datacenter.get('id')
                @state = 'none'
            else
                @state = 'show_primary'
            @render()
    
        destroy: =>
            @model.off 'change:primary_uuid', @change_pin

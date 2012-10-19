
# Namespace view
module 'NamespaceView', ->
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()
        no_datacenter_template: Handlebars.compile $('#namespace_view-replica-no_datacenter-template').html()
        datacenter_list_template: Handlebars.compile $('#namespace_view-replica-datacenters_list-template').html()
        acks_greater_than_replicas_template: Handlebars.compile $('#namespace_view-acks_greater_than_replicas-template').html()
        events:
            'click .nav_datacenter': 'handle_click_datacenter'

        initialize: =>
            datacenters.on 'add', @render_list
            datacenters.on 'remove', @render_list
            datacenters.on 'reset', @render_list
            @model.on 'change:primary_uuid', @render_primary_not_found
            @model.on 'change:replica_affinities', @render_acks_greater_than_replicas
            @model.on 'change:ack_expectations', @render_acks_greater_than_replicas

            @universe_replicas = new NamespaceView.DatacenterReplicas universe_datacenter.get('id'), @model

        render_list: =>
            @ordered_datacenters = _.map(datacenters.models, (datacenter) =>
                id: datacenter.get('id')
                name: if datacenter.get('name').length>8 then datacenter.get('name').slice(0, 8)+'...' else datacenter.get('name')
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
                else # Else, we look for the datacenter to display
                    for datacenter, i in @ordered_datacenters
                        if datacenter.id is @model.get('primary_uuid')
                            datacenter.active = true
                            @active_datacenter_id = datacenter.id
                            break


            @.$('.datacenters_list_container').html @datacenter_list_template
                id: @model.get 'id'
                datacenters: @ordered_datacenters

            # If datacenter_view is null (there used to be no datacenter before, we display a datacenter
            if @datacenter_view is null
                @render_datacenter @active_datacenter_id

            # If there is no more datacenter, we display a message saying that there is no datacenter
            if @ordered_datacenters.length is 0
                if @datacenter_view?
                    @datacenter_view.destroy()
                @datacenter_view = null
                @.$('.datacenter_content').html @no_datacenter_template()

        handle_click_datacenter: (event) =>
            event.preventDefault()

            @render_datacenter @.$(event.target).data('datacenter_id')
            @.$('.datacenter_tab').removeClass 'active'
            @.$(event.target).parent().addClass 'active'

        render_datacenter: (datacenter_id) =>
            @active_datacenter_id = datacenter_id
            @datacenter_id_shown = datacenter_id
            @datacenter_view.destroy() if @datacenter_view?
            @datacenter_view = new NamespaceView.DatacenterReplicas @datacenter_id_shown, @model

            @.$('.datacenter_content').html @datacenter_view.render().$el

        render_primary_not_found: =>
            if @model.get('primary_uuid') isnt universe_datacenter.get('id') and not datacenters.get(@model.get('primary_uuid'))?
                if @.$('.no_datacenter_found').css('display') is 'none' or @.$('.no_datacenter_found').css('display') is 'hidden' or @.$('.no_datacenter_found').css('display') is ''
                    @.$('.no_datacenter_found').show()
            else
                 if @.$('.no_datacenter_found').css('display') is 'block'
                    @.$('.no_datacenter_found').hide()
               
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

                    if replicas_value < @model.get('ack_expectations')[datacenter_id]
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


        render_universe: =>
            @.$('.universe_container').html @universe_replicas.render().$el

        render: =>
            @.$el.html @template()

            @render_list()
            @render_primary_not_found()
            @render_acks_greater_than_replicas()
            @render_universe()

            if @model.get('primary_uuid') is universe_datacenter.get('id')
                if @ordered_datacenters.length > 0
                    @datacenter_picked = @ordered_datacenters[0]
                    @render_datacenter @datacenter_picked.id
                else
                    @datacenter_view = null
                    @.$('.datacenter_content').html @no_datacenter_template()
            else
                @render_datacenter @model.get('primary_uuid')

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


    class @DatacenterReplicas extends Backbone.View
        className: 'datacenter_view'
        template: Handlebars.compile $('#namespace_view-datacenter_replica-template').html()
        universe_template: Handlebars.compile $('#namespace_view-universe_replica-template').html()
        edit_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-template').html()
        error_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-error-template').html()
        error_msg_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-alert_messages-template').html()
        replicas_acks_success_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-success-template').html()
        replication_complete_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-replication_done-template').html()
        replication_status: Handlebars.compile $('#namespace_view-edit_datacenter_replica-replication_status-template').html()
        replicas_ajax_error_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-ajax_error-template').html()
        need_replicas_template: Handlebars.compile $('#need_replicas-template').html()
        progress_bar_template: Handlebars.compile $('#backfill_progress_bar').html()
        success_set_primary: Handlebars.compile $('#changed_primary_dc-replica-template').html()
        states: ['read_only', 'editable']

        events:
            'click .close': 'remove_parent_alert'
            'keyup #replicas_value': 'check_replicas_acks'
            'keyup #acks_value': 'check_replicas_acks'
            'click .make-primary.btn': 'make_primary'
            'click .update-replicas.btn': 'submit_replicas_acks'
            'click .edit.btn': 'edit'
            'click .cancel.btn': 'cancel_edit'

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

            @model.on 'change:blueprint', @render_status
            directory.on 'all', @render_status
            
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

            data =
                name: @datacenter.get('name')
                total_machines: machines.length
                acks: DataUtils.get_ack_expectations(@model.get('id'), @datacenter.get('id'))
                primary: @model.get('primary_uuid') is @datacenter.get('id')
                replicas: replicas_count
                editable: @current_state is @states[1]

            # Don't re-render if the data hasn't changed
            if not _.isEqual(data, @data)
                @data = data
                if @datacenter.get('id') is universe_datacenter.get('id')
                    @.$el.html @universe_template data
                else
                    @.$el.html @template data

            @render_status()

            return @

        # Compute how many replica we can set for @datacenter
        compute_max_machines: =>
            @max_machines = machines.length
            for datacenter_id of @model.get('replica_affinities')
                if datacenter_id isnt @datacenter.get('id')
                    @max_machines -= @model.get('replica_affinities')[datacenter_id]
            if @model.get('primary_uuid') isnt @datacenter.get('id')
                @max_machines -= 1
 
            # If we can't use all our machines in the datacenter because the replicas value of Universe is too high, we give a little more explanation
            @need_explanation = @max_machines < DataUtils.get_datacenter_machines(@datacenter.get('id')).length
            @max_machines = Math.min @max_machines, DataUtils.get_datacenter_machines(@datacenter.get('id')).length

            # We render only if we are not editing
            if @current_state isnt @states[1]
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
            if num_acks is 0 and num_replicas > 0
                msg_error.push @error_msg_template
                    acks_too_small: true
            if msg_error.length isnt 0
                @alert_replicas_acks msg_error
                return false

            @remove_alert_replicas_acks()
            return true

        submit_replicas_acks: (event) =>
            if @check_replicas_acks() is false
                return

            num_replicas = parseInt @.$('#replicas_value').val()
            num_acks = parseInt @.$('#acks_value').val()
            # adjust the replica count to only include secondaries
            if @model.get('primary_uuid') is @datacenter.get('id')
                num_replicas -= 1

            replica_affinities_to_send = {}
            replica_affinities_to_send[@datacenter.get('id')] = num_replicas
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

        # Compute the status of all replicas
        render_status: =>
            # If the blueprint is not ready, we just skip it
            blueprint = @model.get('blueprint').peers_roles
            if not blueprint?
                return ''

            num_replicas_not_ready = 0
            num_replicas_ready = 0

            # Loop over the blueprint
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
            data =
                num_replicas_not_ready: num_replicas_not_ready
                num_replicas_ready: num_replicas_ready
                num_replicas: num_replicas_ready+num_replicas_not_ready
            

            @.$('.status_details').html @replication_status data
            if @replicating? and @replicating is true
                if num_replicas_not_ready is 0
                    @replicating = false

                    @.$('.status-alert').html @replication_complete_template()
                    @.$('.status-alert').slideDown 'fast'

            return data

        on_success_replicas_and_acks: =>
            window.collect_progress()
            new_replicas = @model.get 'replica_affinities'
            new_replicas[@datacenter.get('id')] = @data_cached.num_replicas
            @model.set('replica_affinities', new_replicas)

            new_acks = @model.get 'ack_expectations'
            new_acks[@datacenter.get('id')] = @data_cached.num_acks
            @model.set('ack_expectations', new_acks)

            @current_state = @states[0]
            @render()

            @.$('.replicas_acks-alert').html @replicas_acks_success_template()
            @.$('.replicas_acks-alert').slideDown 'fast'
            # create listener + state
            @replicating = true
            @.$('.status-alert').hide()

        on_error: =>
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
                url: "/ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
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

            @model.off 'change:blueprint', @render_status
            directory.off 'all', @render_status
            
            @model.off 'change:ack_expectations', @render_acks_replica
            @model.off 'change:replica_affinities', @render_acks_replica

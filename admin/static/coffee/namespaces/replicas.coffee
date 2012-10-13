
# Namespace view
module 'NamespaceView', ->
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()
        datacenter_list_template: Handlebars.compile $('#namespace_view-replica-datacenters_list-template').html()
        alert_tmpl: Handlebars.compile $('#changed_primary_dc-replica-template').html()

        events:
            'click .nav_datacenter': 'handle_click_datacenter'

        initialize: =>
            datacenters.on 'add', @render_list
            datacenters.on 'remove', @render_list
            datacenters.on 'reset', @render_list

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
            @datacenter_view = new NamespaceView.DatacenterReplicas @datacenter_id_shown, @model

            @.$('.datacenter_content').html @datacenter_view.render().$el

        render: =>
            @.$el.html @template()

            @render_list()

            if not @current_tab?
                @render_datacenter @model.get('primary_uuid')

            return @

        destroy: =>
            datacenters.off 'add', @render_list
            datacenters.off 'remove', @render_list
            datacenters.off 'reset', @render_list

    class @DatacenterReplicas extends Backbone.View
        className: 'datacenter_view'
        template: Handlebars.compile $('#namespace_view-datacenter_replica-template').html()
        edit_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-template').html()
        error_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-error-template').html()
        error_msg_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-alert_messages-template').html()
        replicas_acks_success_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-success-template').html()
        replication_complete_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-replication_done-template').html()
        replication_status: Handlebars.compile $('#namespace_view-edit_datacenter_replica-replication_status-template').html()
        replicas_ajax_error_template: Handlebars.compile $('#namespace_view-edit_datacenter_replica-ajax_error-template').html()
        need_replicas_template: Handlebars.compile $('#need_replicas-template').html()
        progress_bar_template: Handlebars.compile $('#backfill_progress_bar').html()
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

            @datacenter.on 'all', @render

            @model.on 'change:replica_affinities', @compute_max_machines_and_render
            @model.on 'change:primary_uuid', @compute_max_machines_and_render
            @model.on 'change:primary_uuid', @render
            progress_list.on 'all', @render_progress

            @model.on 'change:blueprint', @render_status
            directory.on 'all', @render_status
            
            @render_progress()
            @compute_max_machines_and_render()

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

        render_progress: =>
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
            replicas_count = DataUtils.get_replica_affinities(@model.get('id'), @datacenter.get('id'))
            if @model.get('primary_uuid') is @datacenter.get('id')
                replicas_count++

            data =
                name: @datacenter.get('name')
                total_machines: machines.length
                acks: DataUtils.get_ack_expectations(@model.get('id'), @datacenter.get('id'))
                primary: true if @model.get('primary_uuid') is @datacenter.get('id')
                replicas: replicas_count
                editable: true if @current_state is @states[1]

            # Don't re-render if the data hasn't changed
            if not _.isEqual(data, @data)
                @data = data
                @.$el.html @template data

            @render_status()

            return @

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

            new_affinities[old_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), old_dc.get('id')) + 1
            new_affinities[new_dc.get('id')] = DataUtils.get_replica_affinities(@model.get('id'), new_dc.get('id')) - 1

            primary_pinnings = {}
            for shard of @model.get('primary_pinnings')
                primary_pinnings[shard] = null

            data =
                primary_uuid: new_dc.get('id')
                replica_affinities: new_affinities
                primary_pinnings: primary_pinnings
            
            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                success: => @model.set data
                error: @on_error

        destroy: =>
            @model.off 'change:replica_affinities', @compute_max_machines_and_render
            @model.off 'change:primary_uuid', @compute_max_machines_and_render
            @model.off 'change:primary_uuid', @render
            progress_list.off 'all', @render_progress

            @model.off 'change:blueprint', @render_status
            directory.off 'all', @render_status

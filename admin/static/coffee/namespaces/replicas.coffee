
# Namespace view
module 'NamespaceView', ->
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template: Handlebars.compile $('#namespace_view-replica-template').html()
        datacenter_list_template: Handlebars.compile $('#namespace_view-replica-datacenters_list-template').html()
        alert_tmpl: Handlebars.compile $('#changed_primary_dc-replica-template').html()

        events: ->
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

            if progress_data?
                @.$('.progress_bar_full_container').show()
                @.$('.replica_container').hide()
                @.$('.progress_bar_container').html @progress_bar_template progress_data
            else
                @.$('.progress_bar_container').empty()
                @.$('.progress_bar_full_container').hide()
                @.$('.replica_container').show()

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
                #status: DataUtils.get_namespace_status(@table.get('id'), uuid)
                editable: true if @current_state is @states[1]

            # Don't re-render if the data hasn't changed
            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @data = data_in_json
                @.$el.html @template data
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
            
            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@model.get("protocol")}_namespaces/#{@model.get('id')}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                success: => @model.set data
                error: @on_error

        destroy: ->
            @model.off 'all', @render

    class @MachinesAssignmentsModal extends UIComponents.AbstractModal
        render: =>
            @pins = new NamespaceView.Pinning(model: @model)
            $('#modal-dialog').html @pins.render().$el
            modal = $('.modal').modal
                'show': true
                'backdrop': true
                'keyboard': true

            modal.on 'hidden', =>
                modal.remove()

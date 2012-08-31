# Index view of datacenters and machines
# The server index view consists of several lists. Here's the hierarchy:
#   @DatacenterList
#   |-- @DatacenterListElement: a datacenter
#   |   `-- @MachineList: machines for that datacenter
#   |      `-- @MachinesListElement: individual machine in the datacenter
#   |-- @UnassignedMachineListElement: grouping for unassigned machines
#   |   `-- @MachineList: machines that are unassigned
#   |      `-- @MachinesListElement: individual unassigned machine
module 'ServerView', ->
    class @DatacenterList extends UIComponents.AbstractList
        # Use a datacenter-specific template for the datacenter list
        className: 'datacenters_list-container'
        template: Handlebars.compile $('#server_list-template').html()
        cannot_change_datacenter_alert_template: Handlebars.compile $('#cannot_change_datacenter-alert-template').html()
        alert_message_template: Handlebars.compile $('#alert_message-template').html()

        # Extend the generic list events
        events:
            'click a.btn.add-datacenter': 'add_datacenter'
            'click a.btn.set-datacenter': 'set_datacenter'
            'click .close': 'remove_parent_alert'

        initialize: ->
            @add_datacenter_dialog = new ServerView.AddDatacenterModal
            @set_datacenter_dialog = new ServerView.SetDatacenterModal

            @unassigned_machines = new ServerView.UnassignedMachinesListElement()
            @unassigned_machines.register_machine_callbacks @get_callbacks()

            super datacenters, ServerView.DatacenterListElement, 'div.datacenters'


        render: (message) =>
            super
            @.$('.unassigned-machines').html @unassigned_machines.render().el
            @update_toolbar_buttons()

            if message?
                @.$('#user-alert-space').append @alert_message_template
                    message: message

            return @

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()

        add_datacenter: (event) =>
            log_action 'add datacenter button clicked'
            @add_datacenter_dialog.render()
            event.preventDefault()

        set_datacenter: (event) =>
            log_action 'set datacenter button clicked'

            # Show the dialog and provide it with a list of selected machines
            if not $(event.currentTarget).hasClass 'disabled'
                @set_datacenter_dialog.render @get_selected_machines()
            event.preventDefault()

        # Count up the number of machines checked off across all machine lists
        get_selected_machines: =>
            # Get all the machine lists used in this list
            machine_lists = _.map @element_views.concat(@unassigned_machines), (datacenter_list_element) ->
                datacenter_list_element.machine_list

            selected_machines = []
            for machine_list in machine_lists
                selected_machines = selected_machines.concat machine_list.get_selected_elements()

            return selected_machines

        # Get a list containing all the callbacks
        get_callbacks: => [@update_toolbar_buttons]

        # Override the AbstractList.add_element method so we can register callbacks
        add_element: (element) =>
            datacenter_list_element = super element
            datacenter_list_element.register_machine_callbacks @get_callbacks()

        # Callback that will be registered: updates the toolbar buttons based on how many machines have been selected
        update_toolbar_buttons: =>
            # We need to check which machines have been checked off to decide which buttons to enable/disable
            $set_datacenter_button = @.$('.actions-bar a.btn.set-datacenter')
            $set_datacenter_button.toggleClass 'disabled', @check_can_change_datacenter()
            #$set_datacenter_button.toggleClass 'disabled', @get_selected_machines().length < 1

        check_can_change_datacenter: =>
            selected_machines = @get_selected_machines()
            reason_unmovable_machines = {}
            for machine in selected_machines
                for namespace in namespaces.models
                    for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                        if machine_uuid is machine.get 'id'
                            for shard, role of peer_roles
                                if role is 'role_primary'
                                    if not (machine.get('id') of reason_unmovable_machines)
                                        reason_unmovable_machines[machine_uuid] = {}
                                        reason_unmovable_machines[machine_uuid]['master'] = []
                                    reason_unmovable_machines[machine_uuid]['master'].push
                                        namespace_id: namespace.get 'id'
                                    break


            for selected_machine in selected_machines
                num_machines_in_datacenter = 0
                for machine in machines.models
                    if machine.get('datacenter_uuid') is selected_machine.get('datacenter_uuid')
                        num_machines_in_datacenter++

                for namespace in namespaces.models
                    if selected_machine.get('datacenter_uuid') of namespace.get('replica_affinities') # If the datacenter has responsabilities
                        num_replica = namespace.get('replica_affinities')[selected_machine.get('datacenter_uuid')]
                        if namespace.get('primary_uuid') is selected_machine.get('datacenter_uuid')
                            num_replica++
                        if num_machines_in_datacenter <= num_replica
                            if not (selected_machine.get('id') of reason_unmovable_machines)
                                reason_unmovable_machines[selected_machine.get('id')] = []
                                reason_unmovable_machines[selected_machine.get('id')]['goals'] = []
                            else if not ('goals' of reason_unmovable_machines[selected_machine.get('id')])
                                reason_unmovable_machines[selected_machine.get('id')]['goals'] = []

                            reason_unmovable_machines[selected_machine.get('id')]['goals'].push
                                namespace_id: namespace.get 'id'

            num_not_movable_machines = 0
            for machine_id of reason_unmovable_machines
                num_not_movable_machines++
            
            if num_not_movable_machines > 0
                if @.$('#reason_cannot_change_datacenter').length > 0
                    @.$('#reason_cannot_change_datacenter').remove()
                    @.$('#user-alert-space-set_datacenter').prepend @cannot_change_datacenter_alert_template
                        reasons: reason_unmovable_machines
                    @.$('#reason_cannot_change_datacenter').css 'display', 'block'
                else
                    @.$('#user-alert-space-set_datacenter').prepend @cannot_change_datacenter_alert_template
                        reasons: reason_unmovable_machines
                    @.$('#reason_cannot_change_datacenter').slideDown 200
            else
                if @.$('#reason_cannot_change_datacenter').length > 0
                    @.$('#reason_cannot_change_datacenter').slideUp 200, -> $(this).remove()
 
            return num_not_movable_machines>0 or selected_machines.length is 0

        destroy: =>
            super()
            @unassigned_machines.destroy()


    class @MachineList extends UIComponents.AbstractList
        # Use a machine-specific template for the machine list
        tagName: 'table'
        template: Handlebars.compile $('#machine_list-template').html()

        initialize: (datacenter_uuid) ->
            @datacenter_uuid = datacenter_uuid
            @callbacks = []
            super machines, ServerView.MachineListElement, 'tbody.list',
                filter: (model) -> model.get('datacenter_uuid') is @datacenter_uuid

            machines.on 'change:datacenter_uuid', @changed_datacenter
            return @

        changed_datacenter: (machine, new_datacenter_uuid) =>
            num_elements_removed = @remove_elements machine
            @render() if num_elements_removed > 0

            if new_datacenter_uuid is @datacenter_uuid
                @add_element machine
                @render()


        add_element: (element) =>
            machine_list_element = super element
            @bind_callbacks_to_machine machine_list_element

        # Add to the list of known callbacks, and register the callback with each MachineListElement
        register_machine_callbacks: (callbacks) =>
            @callbacks = callbacks
            @bind_callbacks_to_machine machine_list_element for machine_list_element in @element_views

        bind_callbacks_to_machine: (machine_list_element) =>
            machine_list_element.off 'selected', @call_all_callback
            machine_list_element.on 'selected', @call_all_callback

        call_all_callback: =>
            callback() for callback in @callbacks

        destroy: ->
            machines.off 'change:datacenter_uuid', @changed_datacenter


    # Machine list element
    class @MachineListElement extends UIComponents.CheckboxListElement
        template: Handlebars.compile $('#machine_list_element-template').html()
        summary_template: Handlebars.compile $('#machine_list_element-summary-template').html()
        className: 'element'

        threshold_alert: 90
        history:
            cpu: []
            traffic_sent: []
            traffic_recv: []

        events: ->
            _.extend super,
                'click a.rename-machine': 'rename_machine'
                'mouseenter .contains_info': 'display_popover'
                'mouseleave .contains_info': 'hide_popover'

        hide_popover: ->
            $('.tooltip').remove()

        initialize: =>
            log_initial '(initializing) list view: machine'

            #initialize history
            for i in [0..12]
                @history.cpu.push 0
                @history.traffic_sent.push 0
                @history.traffic_recv.push 0

            @model.on 'change:name', @render_summary
            #TODO change callback so we don't refresh the whole element but just the status
            directory.on 'all', @render_summary

            # Load abstract list element view with the machine template
            super @template


        render: =>
            super
            @render_summary()
            return @


        render_summary: =>
            json = _.extend @model.toJSON(),
                status: DataUtils.get_machine_reachability(@model.get('id'))
                ip: 'TBD' #TODO
                primary_count: 0
                secondary_count: 0

            # primary, secondary, and namespace counts
            _namespaces = []
            for namespace in namespaces.models
                for machine_uuid, peer_role of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        machine_active_for_namespace = false
                        for shard, role of peer_role
                            if role is 'role_primary'
                                machine_active_for_namespace = true
                                json.primary_count += 1
                            if role is 'role_secondary'
                                machine_active_for_namespace = true
                                json.secondary_count += 1
                        if machine_active_for_namespace
                            _namespaces[_namespaces.length] = namespace
            json.namespace_count = _.uniq(_namespaces).length

            if not @model.get('datacenter_uuid')?
                json.unassigned_machine = true

            # Displays bars and text in the popover
            @.$('.machine.summary').html @summary_template json

            @.delegateEvents()

        rename_machine: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

        display_popover: (event) ->
            $(event.currentTarget).tooltip('show')

        destroy: =>
            @model.off 'change:name', @render_summary
            directory.off 'all', @render_summary

    # Datacenter list element
    class @DatacenterListElement extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#datacenter_list_element-template').html()
        summary_template: Handlebars.compile $('#datacenter_list_element-summary-template').html()

        className: 'element-container'

        events: ->
            _.extend super,
               'click a.remove-datacenter': 'remove_datacenter'
               'click a.rename-datacenter': 'rename_datacenter'

        initialize: ->
            log_initial '(initializing) list view: datacenter'

            super

            @machine_list = new ServerView.MachineList @model.get('id')
            @remove_datacenter_dialog = new ServerView.RemoveDatacenterModal
            @callbacks = []
            @no_machines = true

            @model.on 'change', @render_summary
            directory.on 'all', @render_summary
            @machine_list.on 'size_changed', @ml_size_changed
            @ml_size_changed()

        ml_size_changed: =>
            num_machines = @machine_list.element_views.length

            we_should_rerender = false

            if @no_machines and num_machines > 0
                @no_machines = false
                we_should_rerender = true
            else if not @no_machines and num_machines is 0
                @no_machines = true
                we_should_rerender = true

            @render() if we_should_rerender

        render: =>
            @.$el.html @template
                no_machines: @no_machines

            @render_summary()

            # Attach a list of available machines to the given datacenter
            @.$('.element-list-container').html @machine_list.render().el

            super

            return @

        render_summary: =>
            json = _.extend @model.toJSON(),
                status: DataUtils.get_datacenter_reachability(@model.get('id'))
                primary_count: 0
                secondary_count: 0

            # primary, secondary, and namespace counts
            _namespaces = []
            for namespace in namespaces.models
                for machine_uuid, peer_role of namespace.get('blueprint').peers_roles
                    if machines.get(machine_uuid)?.get('datacenter_uuid') and machines.get(machine_uuid).get('datacenter_uuid') is @model.get('id')
                        machine_active_for_namespace = false
                        for shard, role of peer_role
                            if role is 'role_primary'
                                machine_active_for_namespace = true
                                json.primary_count += 1
                            if role is 'role_secondary'
                                machine_active_for_namespace = true
                                json.secondary_count += 1
                        if machine_active_for_namespace
                            _namespaces[_namespaces.length] = namespace
            json.namespace_count = _.uniq(_namespaces).length

            @.$('.datacenter.summary').html @summary_template json

        remove_datacenter: (event) ->
            log_action 'remove datacenter button clicked'
            if not @.$(event.currentTarget).hasClass 'disabled'
                @remove_datacenter_dialog.render @model

            event.preventDefault()

        rename_datacenter: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render()

        register_machine_callbacks: (callbacks) =>
            @callbacks = callbacks
            @machine_list.register_machine_callbacks callbacks

        destroy: ->
            @model.off 'change', @render_summary
            directory.off 'all', @render_summary
            @machine_list.off 'size_changed', @ml_size_changed

    # Equivalent of a DatacenterListElement, but for machines that haven't been assigned to a datacenter yet.
    class @UnassignedMachinesListElement extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#unassigned_machines_list_element-template').html()

        className: 'unassigned-machines element-container'

        initialize: ->
            super

            @machine_list = new ServerView.MachineList null
            @no_machines = true

            machines.on 'add', (machine) => @render() if machine.get('datacenter_uuid') is null
            @machine_list.on 'size_changed', @ml_size_changed
            @ml_size_changed()

            @callbacks = []

        ml_size_changed: =>
            num_machines = @machine_list.element_views.length

            we_should_rerender = false

            if @no_machines and num_machines > 0
                @no_machines = false
                we_should_rerender = true
            else if not @no_machines and num_machines is 0
                @no_machines = true
                we_should_rerender = true

            @render() if we_should_rerender

        render: =>
            @.$el.html @template
                no_machines: @no_machines

            # Attach a list of available machines to the given datacenter
            @.$('.element-list-container').html @machine_list.render().el

            super

            return @

        register_machine_callbacks: (callbacks) =>
            @callbacks = callbacks
            @machine_list.register_machine_callbacks callbacks

        destroy: =>
            machines.off 'add', (machine) => @render() if machine.get('datacenter_uuid') is null
            @machine_list.off 'size_changed', @ml_size_changed

    class @AddDatacenterModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#add_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_datacenter-alert-template').html()
        error_template: Handlebars.compile $('#error_input-template').html()

        class: 'add-datacenter'

        initialize: ->
            log_initial '(initializing) modal dialog: add datacenter'
            super

        render: ->
            log_render '(rendering) add datacenter dialog'
            super
                modal_title: "Add datacenter"
                btn_primary_text: "Add"
            @.$('.focus_new_name').focus()

        on_submit: ->
            super
            @formdata = form_data_as_object($('form', @$modal))

            no_error = true
            if @formdata.name is ''
                no_error = false
                template_error =
                    datacenter_is_empty: true
                $('.alert_modal').html @error_template template_error
                $('.alert_modal').alert()
                @reset_buttons()
            else
                for datacenter in datacenters.models
                    if datacenter.get('name') is @formdata.name
                        no_error = false
                        template_error =
                            datacenter_exists: true
                        $('.alert_modal').html @error_template template_error
                        $('.alert_modal').alert()
                        @reset_buttons()
                        break
            if no_error is true
                $.ajax
                    processData: false
                    url: '/ajax/semilattice/datacenters/new'
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify({"name" : @formdata.name})
                    success: @on_success
                    error: @on_error

        on_success: (response) ->
            super
            # Parse the response JSON, apply appropriate diffs, and show an alert
            apply_to_collection(datacenters, response)
            for response_uuid, blah of response
                break
            $('#user-alert-space').html @alert_tmpl
                name: @formdata.name
                uuid: response_uuid

    class @RemoveDatacenterModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#remove_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#removed_datacenter-alert-template').html()
        template_remove_error: Handlebars.compile $('#fail_delete_datacenter-template').html()
        class: 'remove-datacenter'

        initialize: ->
            log_initial '(initializing) modal dialog: remove datacenter'
            super

        render: (datacenter) ->
            log_render '(rendering) remove datacenters dialog'
            @datacenter = datacenter
            super
                datacenter: datacenter.toJSON()
                modal_title: "Remove datacenter"
                btn_primary_text: 'Remove'

        on_submit: ->
            super
            $.ajax
                url: "/ajax/semilattice/datacenters/#{@datacenter.id}"
                type: 'DELETE'
                contentType: 'application/json'
                success: @on_success
                error: @on_error


        on_success_with_error: =>
            @.$('.error_answer').html @template_remove_error

            if @.$('.error_answer').css('display') is 'none'
                @.$('.error_answer').slideDown('fast')
            else
                @.$('.error_answer').css('display', 'none')
                @.$('.error_answer').fadeIn()
            @reset_buttons()

        on_success: (response) ->
            if (response)
                @on_success_with_error()
                return

            super

            datacenters.remove(@datacenter.id)
            $('#user-alert-space').html @alert_tmpl
                name: @datacenter.get('name')

    class @SetDatacenterModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#set_datacenter-modal-template').html()
        cannot_change_datacenter_alert_template: Handlebars.compile $('#cannot_change_datacenter-alert_content-template').html()
        alert_tmpl: Handlebars.compile $('#set_datacenter-alert-template').html()
        class: 'set-datacenter-modal'

        initialize: ->
            log_initial '(initializing) modal dialog: set datacenter'
            super

        render: (_machines_list) ->
            @machines_list = _machines_list
            log_render '(rendering) set datacenter dialog'
            super
                modal_title: 'Set datacenter'
                btn_primary_text: 'Commit'
                datacenters: (datacenter.toJSON() for datacenter in datacenters.models)

        on_submit: ->
            super
            @formdata = form_data_as_object($('form', @$modal))
            # Prepare json to pass to the server
            json = {}
            for _m in @machines_list
                json[_m.get('id')] =
                    datacenter_uuid: @formdata.datacenter_uuid
            
            if @can_change_datacenter() is false
                @reset_buttons()   
                return @

            # Set the datacenters!
            $.ajax
                processData: false
                url: "/ajax/semilattice/machines"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(json)
                success: @on_success
                error: @on_error

        on_success: (response) =>
            super
            for _m_uuid, _m of response
                if machines.get(_m_uuid)? # In case the machine was declared dead
                    machines.get(_m_uuid).set(_m)

            machine_names = _.map(@machines_list, (_m) -> name: _m.get('name'))
            $('#user-alert-space').append (@alert_tmpl
                datacenter_name: datacenters.get(@formdata.datacenter_uuid).get('name')
                machines_first: machine_names[0]
                machines_rest: machine_names.splice(1)
                machine_count: @machines_list.length
            )

        can_change_datacenter: =>
            selected_machines = @machines_list
            reason_unmovable_machines = {}
            for machine in selected_machines
                for namespace in namespaces.models
                    for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                        if machine_uuid is machine.get 'id'
                            for shard, role of peer_roles
                                if role is 'role_primary'
                                    if not (machine.get('id') of reason_unmovable_machines)
                                        reason_unmovable_machines[machine_uuid] = {}
                                        reason_unmovable_machines[machine_uuid]['master'] = []
                                    reason_unmovable_machines[machine_uuid]['master'].push
                                        namespace_id: namespace.get 'id'
                                    break


            for selected_machine in selected_machines
                num_machines_in_datacenter = 0
                for machine in machines.models
                    if machine.get('datacenter_uuid') is selected_machine.get('datacenter_uuid')
                        num_machines_in_datacenter++

                for namespace in namespaces.models
                    if selected_machine.get('datacenter_uuid') of namespace.get('replica_affinities') # If the datacenter has responsabilities
                        num_replica = namespace.get('replica_affinities')[selected_machine.get('datacenter_uuid')]
                        if namespace.get('primary_uuid') is selected_machine.get('datacenter_uuid')
                            num_replica++
                        if num_machines_in_datacenter <= num_replica
                            if not (selected_machine.get('id') of reason_unmovable_machines)
                                reason_unmovable_machines[selected_machine.get('id')] = []
                                reason_unmovable_machines[selected_machine.get('id')]['goals'] = []
                            else if not ('goals' of reason_unmovable_machines[selected_machine.get('id')])
                                reason_unmovable_machines[selected_machine.get('id')]['goals'] = []

                            reason_unmovable_machines[selected_machine.get('id')]['goals'].push
                                namespace_id: namespace.get 'id'

            num_not_movable_machines = 0
            for machine_id of reason_unmovable_machines
                num_not_movable_machines++
            
            if num_not_movable_machines > 0
                @.$('.alert').html('')
                @.$('.alert').prepend @cannot_change_datacenter_alert_template
                    reasons: reason_unmovable_machines
                    red: true
                @.$('.alert').slideDown 200

            return num_not_movable_machines is 0

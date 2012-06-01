
# Index view of datacenters and machines
module 'ServerView', ->
    class @DatacenterList extends UIComponents.AbstractList
        # Use a datacenter-specific template for the datacenter list
        template: Handlebars.compile $('#server_list-template').html()

        # Extend the generic list events
        events:
            'click a.btn.add-datacenter': 'add_datacenter'
            'click a.btn.set-datacenter': 'set_datacenter'
            'click .close': 'remove_parent_alert'

        initialize: ->
            @add_datacenter_dialog = new ServerView.AddDatacenterModal
            @set_datacenter_dialog = new ServerView.SetDatacenterModal

            super datacenters, ServerView.DatacenterListElement, 'div.datacenters'

            @unassigned_machines = new ServerView.UnassignedMachinesListElement()
            @unassigned_machines.register_machine_callbacks @get_callbacks()


        render: =>
            super
            @.$('.unassigned-machines').html @unassigned_machines.render().el
            @update_toolbar_buttons()

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
            $set_datacenter_button.toggleClass 'disabled', @get_selected_machines().length < 1

    class @MachineList extends UIComponents.AbstractList
        # Use a machine-specific template for the machine list
        template: Handlebars.compile $('#machine_list-template').html()

        initialize: (datacenter_uuid) ->
            @callbacks = []
            super machines, ServerView.MachineListElement, 'tbody.list', (model) -> model.get('datacenter_uuid') is datacenter_uuid

            machines.on 'change:datacenter_uuid', (machine, new_datacenter_uuid) =>
                num_elements_removed = @remove_elements machine
                @render() if num_elements_removed > 0

                if new_datacenter_uuid is datacenter_uuid
                    @add_element machine
                    @render()

            return @

        add_element: (element) =>
            machine_list_element = super element
            @bind_callbacks_to_machine machine_list_element

        # Add to the list of known callbacks, and register the callback with each MachineListElement
        register_machine_callbacks: (callbacks) =>
            @callbacks = callbacks
            @bind_callbacks_to_machine machine_list_element for machine_list_element in @element_views

        bind_callbacks_to_machine: (machine_list_element) =>
            machine_list_element.off 'selected'
            machine_list_element.on 'selected', => callback() for callback in @callbacks

    # Machine list element
    class @MachineListElement extends UIComponents.CheckboxListElement
        template: Handlebars.compile $('#machine_list_element-template').html()
        summary_template: Handlebars.compile $('#machine_list_element-summary-template').html()
        className: 'machine element'

        events: ->
            _.extend super,
                'click a.rename-machine': 'rename_machine'

        initialize: ->
            log_initial '(initializing) list view: machine'

            @model.on 'change', @render_summary
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
            # Stats, jiga
            json = _.extend json,
                # TODO: add a helper to upgrade/downgrade units dynamically depending on the values
                global_cpu_util: Math.floor(@model.get_stats().proc.global_cpu_util_avg * 100)
                global_mem_total: human_readable_units(@model.get_stats().proc.global_mem_total * 1024, units_space)
                global_mem_used: human_readable_units(@model.get_stats().proc.global_mem_used * 1024, units_space)
                global_net_sent: human_readable_units(@model.get_stats().proc.global_net_sent_persec_avg, units_space)
                global_net_recv: human_readable_units(@model.get_stats().proc.global_net_recv_persec_avg, units_space)
                machine_disk_space: human_readable_units(@model.get_used_disk_space(), units_space)

            # Whooo
            @.$('.machine.summary').html @summary_template json

        rename_machine: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

    # Datacenter list element
    class @DatacenterListElement extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#datacenter_list_element-template').html()
        summary_template: Handlebars.compile $('#datacenter_list_element-summary-template').html()

        className: 'datacenter element'

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
            @.$('.machine-list').html @machine_list.render().el

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
                    if machines.get(machine_uuid).get('datacenter_uuid') is @model.get('id')
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

    # Equivalent of a DatacenterListElement, but for machines that haven't been assigned to a datacenter yet.
    class @UnassignedMachinesListElement extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#unassigned_machines_list_element-template').html()

        className: 'unassigned-machines element'

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
            @.$('.machine-list').html @machine_list.render().el

            super

            return @

        register_machine_callbacks: (callbacks) =>
            @callbacks = callbacks
            @machine_list.register_machine_callbacks callbacks

    class @AddDatacenterModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#add_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_datacenter-alert-template').html()
        class: 'add-datacenter'

        initialize: ->
            log_initial '(initializing) modal dialog: add datacenter'
            super

        render: ->
            log_render '(rendering) add datacenter dialog'
            super
                modal_title: "Add datacenter"
                btn_primary_text: "Add"

        on_submit: ->
            super
            @formdata = form_data_as_object($('form', @$modal))

            $.ajax
                processData: false
                url: '/ajax/datacenters/new'
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify({"name" : @formdata.name})
                success: @on_success

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
                url: "/ajax/datacenters/#{@datacenter.id}"
                type: 'DELETE'
                contentType: 'application/json'
                success: @on_success

        on_success: (response) ->
            super
            if (response)
                throw "Received a non null response to a delete... this is incorrect"
            datacenters.remove(@datacenter.id)
            $('#user-alert-space').html @alert_tmpl
                name: @datacenter.get('name')

    class @SetDatacenterModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#set_datacenter-modal-template').html()
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

            # Set the datacenters!
            $.ajax
                processData: false
                url: "/ajax/machines"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(json)
                success: @on_success

        on_success: (response) =>
            super
            for _m_uuid, _m of response
                machines.get(_m_uuid).set(_m)

            machine_names = _.map(@machines_list, (_m) -> name: _m.get('name'))
            $('#user-alert-space').append (@alert_tmpl
                datacenter_name: datacenters.get(@formdata.datacenter_uuid).get('name')
                machines_first: machine_names[0]
                machines_rest: machine_names.splice(1)
                machine_count: @machines_list.length
            )


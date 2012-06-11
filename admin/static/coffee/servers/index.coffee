
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
        className: 'machine-element'

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
            # Stats
            # Compute main traffic
            total_traffic =
                recv: 0
                sent: 0

            total_data = 0

            max_traffic = 
                recv: 0
                sent: 0

            num_machine_in_datacenter = 0
            all_machine_in_datacenter_ready = true

            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('datacenter_uuid')
                    if machine.get_stats().proc.global_net_recv_persec?
                        num_machine_in_datacenter++

                        new_traffic_recv = parseFloat machine.get_stats().proc.global_net_recv_persec.avg
                        total_traffic.recv += new_traffic_recv
                        if !isNaN(new_traffic_recv) and new_traffic_recv > max_traffic.recv
                            max_traffic.recv = new_traffic_recv

                        new_traffic_sent = parseFloat machine.get_stats().proc.global_net_sent_persec.avg
                        total_traffic.sent += new_traffic_sent
                        if !isNaN(new_traffic_sent) and new_traffic_sent > max_traffic.sent
                            max_traffic.sent = new_traffic_sent

                        total_data += parseFloat machine.get_used_disk_space()
                    else
                        all_machine_in_datacenter_ready = false

            # Generate data for bars and popover
            if @model.get_stats().proc.global_cpu_util?
                json = _.extend json,
                    # TODO: add a helper to upgrade/downgrade units dynamically depending on the values
                    global_cpu_util: Math.floor(@model.get_stats().proc.global_cpu_util.avg * 100)
                    mem_used: human_readable_units(@model.get_stats().proc.global_mem_used*1024, units_space)
                    mem_available: human_readable_units(@model.get_stats().proc.global_mem_total*1024, units_space)
                    global_net_sent: human_readable_units(@model.get_stats().proc.global_net_sent_persec.avg, units_space)
                    global_total_net_sent: human_readable_units(total_traffic.sent, units_space)
                    global_net_recv: human_readable_units(@model.get_stats().proc.global_net_recv_persec.avg, units_space)
                    global_total_net_recv: human_readable_units(total_traffic.recv, units_space)
                    machine_disk_space: human_readable_units(@model.get_used_disk_space(), units_space)
                    total_data: human_readable_units(total_data, units_space)
                    machine_disk_available: human_readable_units(@model.get_used_disk_space()*3, units_space) #TODO replace with real value
                # Test if a problem exist

                if @model.get_stats().proc.global_mem_total isnt 0
                    json.mem_used_percent = Math.floor @model.get_stats().proc.global_mem_used/@model.get_stats().proc.global_mem_total*100
                    json.mem_used_has_problem = true if json.mem_used_percent > @threshold_alert
                else
                    json.mem_used_percent = 0

                if total_data isnt 0 and all_machine_in_datacenter_ready
                    json.machine_data_percent = Math.floor @model.get_used_disk_space()/total_data*100
                    json.machine_data_has_problem = true if (json.machine_data_percent > @threshold_alert) and num_machine_in_datacenter>1 and @model.get_used_disk_space() isnt 0 
                else
                    json.machine_data_percent = 0

                if json.machine_disk_available isnt 0 
                    json.machine_disk_percent = Math.floor @model.get_used_disk_space()/(@model.get_used_disk_space()*3)*100 #TODO replace with real values
                    json.machine_disk_has_problem = true if (json.machine_disk_data_percent>@threshold_alert) and @model.get_used_disk_space() isnt 0 
                else
                    json.machine_disk_percent = 0


                # Displays bars and text in the popover
                @.$('.machine.summary').html @summary_template json


                # Update data for the sparklines
                if !isNaN(json.global_cpu_util)
                    @history.cpu.shift()
                    @history.cpu.push json.global_cpu_util

                global_net_sent_percentage = parseInt(@model.get_stats().proc.global_net_sent_persec.avg)
                if !isNaN(global_net_sent_percentage)
                    @history.traffic_sent.shift()
                    @history.traffic_sent.push global_net_sent_percentage

                global_net_recv_percentage = parseInt(@model.get_stats().proc.global_net_recv_persec.avg)
                if !isNaN(global_net_recv_percentage)
                    @history.traffic_recv.shift()
                    @history.traffic_recv.push global_net_recv_percentage


                sparkline_attr =
                    fillColor: false
                    spotColor: false
                    minSpotColor: false
                    maxSpotColor: false
                    chartRangeMin: 0
                    width: '75px'
                    height: '15px'

                # Add some parameters for the CPU sparkline and display it
                sparkline_attr_cpu =
                    chartRangeMax: 100
                if json.global_cpu_util > @threshold_alert
                    sparkline_attr_cpu.lineColor = 'red'
                _.extend sparkline_attr_cpu, sparkline_attr
                @.$('.cpu_sparkline').sparkline @history.cpu, sparkline_attr_cpu
                
                 # Add some parameters for the traffic sent sparkline and display it         
                sparkline_attr_traffic_sent =
                    chartRangeMax: human_readable_units(max_traffic.sent, units_space)
                if total_traffic.sent isnt 0 and num_machine_in_datacenter>1 and @model.get_stats().proc.global_net_sent_persec_avg/total_traffic.sent*100>@threshold_alert
                    sparkline_attr_traffic_sent.lineColor = 'red'
                _.extend sparkline_attr_traffic_sent, sparkline_attr
                @.$('.traffic_sent_sparkline').sparkline @history.traffic_sent, sparkline_attr_traffic_sent
               
                

                # Add some parameters for the traffic recv sparkline and display it         
                sparkline_attr_traffic_recv =
                    chartRangeMax: human_readable_units(max_traffic.recv, units_space)
                if total_traffic.sent isnt 0 and num_machine_in_datacenter>1 and @model.get_stats().proc.global_net_recv_persec_avg/total_traffic.recv*100>@threshold_alert
                    sparkline_attr_traffic_sent.lineColor = 'red'
                _.extend sparkline_attr_traffic_recv, sparkline_attr
                @.$('.traffic_recv_sparkline').sparkline @history.traffic_recv, sparkline_attr_traffic_recv


            @.delegateEvents()

        rename_machine: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

        display_popover: (event) ->
            $(event.currentTarget).tooltip('show')

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


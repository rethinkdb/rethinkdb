
# Main cluster view: list of datacenters, machines, and namespaces
module 'ClusterView', ->
    # Abstract list container: holds an action bar and the list elements
    class @AbstractList extends Backbone.View
        # Use a generic template by default for a list
        template: Handlebars.compile $('#abstract_list-template').html()

        # Abstract lists take several arguments
        #   collection: Backbone collection that backs the list
        #   element_view_class: Backbone view that each element in the list will be rendered with
        #   container: JQuery selector string specifying the div that each element view will be appended to
        #   filter: optional filter that defines what elements to use from the collection using a truth test
        #           (function whose argument is a Backbone model and whose output is true/false)
        initialize: (collection, element_view_class, container, filter) ->
            #log_initial '(initializing) list view: ' + class_name @collection
            @collection = collection
            @element_views = []
            @container = container
            @element_view_class = element_view_class
            @size = 0

            # This filter defines which models from the collection should be represented by this list.
            # If one was not provided, define a filter that allows all models
            if filter?
                @filter = filter
            else
                @filter = (model) -> true

            # Initially we need to populate the element views list
            @reset_element_views()

            # Collection is reset, create all new views
            @collection.on 'reset', (collection) =>
                @reset_element_views()
                @render()

            # When an element is added to the collection, create a view for it
            @collection.on 'add', (model, collection) =>
                # Make sure the model is relevant for this list before we add it
                if @filter model
                    @add_element model
                    @render()

            @collection.on 'remove', (model, collection) =>
                # Make sure the model is relevant for this list before we remove it
                if @filter model
                    @remove_elements model

        render: =>
            #log_render '(rendering) list view: ' + class_name @collection
            # Render and append the list template to the DOM
            @.$el.html(@template({}))

            #log_action '#render_elements of collection ' + class_name @collection
            # Render the view for each element in the list
            @.$(@container).append(view.render().el) for view in @element_views

            @.delegateEvents()
            return @

        reset_element_views: =>
            # Wipe out any existing list element views
            view.remove() for view in @element_views
            @element_views = []

            # Add an element view for each model in the collection that's relevant for this list
            for model in @collection.models
                @add_element model if @filter model

        add_element: (model) =>
            # adds a list element view to element_views
            element_view = new @element_view_class
                model: model
                collection: @collection

            @element_views.push element_view
            @size = @element_views.length
            @.trigger 'size_changed'

            return element_view

        remove_elements: (model) =>
            # Find any views that are based on the model being removed
            matching_views = (view for view in @element_views when view.model is model)

            # For all matching views, remove the view from the DOM and from the known element views list
            for view in matching_views
                view.remove()
                @element_views = _.without @element_views, view

            @size = @element_views.length
            @.trigger 'size_changed'

            # Return the number of views removed
            return matching_views.length


        # Return an array of elements that are selected in the current list
        get_selected_elements: =>
            selected_elements = []
            selected_elements.push view.model for view in @element_views when view.selected
            return selected_elements

        # Gets a list of UI callbacks this list will want to register (e.g. with a CheckboxListElement)
        get_callbacks: -> []


    class @NamespaceList extends @AbstractList
        # Use a namespace-specific template for the namespace list
        template: Handlebars.compile $('#namespace_list-template').html()

        # Extend the generic list events
        events: ->
            'click a.btn.add-namespace': 'add_namespace'
            'click a.btn.remove-namespace': 'remove_namespace'

        initialize: ->
            log_initial '(initializing) namespace list view'

            @add_namespace_dialog = new NamespaceView.AddNamespaceModal
            @remove_namespace_dialog = new NamespaceView.RemoveNamespaceModal

            super namespaces, ClusterView.NamespaceListElement, 'tbody.list'

        render: =>
            super
            @update_toolbar_buttons()
            return @

        # Extend the AbstractList.add_element method to bind a callback to each namespace added to the list
        add_element: (element) =>
            machine_list_element = super element
            machine_list_element.off 'selected'
            machine_list_element.on 'selected', @update_toolbar_buttons

        add_namespace: (event) =>
            log_action 'add namespace button clicked'
            @add_namespace_dialog.render()
            event.preventDefault()

        remove_namespace: (event) =>
            log_action 'remove namespace button clicked'
            # Make sure the button isn't disabled, and pass the list of namespace UUIDs selected
            if not $(event.currentTarget).hasClass 'disabled'
                @remove_namespace_dialog.render @get_selected_elements()
            event.preventDefault()

        # Callback that will be registered: updates the toolbar buttons based on how many namespaces have been selected
        update_toolbar_buttons: =>
            # We need to check how many namespaces have been checked off to decide which buttons to enable/disable
            $remove_namespaces_button = @.$('.actions-bar a.btn.remove-namespace')
            $remove_namespaces_button.toggleClass 'disabled', @get_selected_elements().length < 1

    class @DatacenterList extends @AbstractList
        # Use a datacenter-specific template for the datacenter list
        template: Handlebars.compile $('#server_list-template').html()

        # Extend the generic list events
        events:
            'click a.btn.add-datacenter': 'add_datacenter'
            'click a.btn.set-datacenter': 'set_datacenter'

        initialize: ->
            @add_datacenter_dialog = new ClusterView.AddDatacenterModal
            @set_datacenter_dialog = new ClusterView.SetDatacenterModal

            super datacenters, ClusterView.DatacenterListElement, 'div.datacenters'

            @unassigned_machines = new ClusterView.UnassignedMachinesListElement()
            @unassigned_machines.register_machine_callbacks @get_callbacks()


        render: =>
            super
            @.$('.unassigned-machines').html @unassigned_machines.render().el
            @update_toolbar_buttons()

            return @

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

    class @MachineList extends @AbstractList
        # Use a machine-specific template for the machine list
        template: Handlebars.compile $('#machine_list-template').html()

        initialize: (datacenter_uuid) ->
            @callbacks = []
            super machines, ClusterView.MachineListElement, 'tbody.list', (model) -> model.get('datacenter_uuid') is datacenter_uuid

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

    # Abstract list element that show info about the element and has a checkbox
    class @CheckboxListElement extends Backbone.View
        tagName: 'tr'
        className: 'element'

        events: ->
            'click': 'clicked'
            'click a': 'link_clicked'

        initialize: (template) ->
            @template = template
            @selected = false

        render: =>
            #log_render '(rendering) list element view: ' + class_name @model
            @.$el.html @template(@json_for_template())
            @mark_selection()
            @delegateEvents()

            return @

        json_for_template: =>
            return @model.toJSON()

        clicked: (event) =>
            @selected = not @selected
            @mark_selection()
            @.trigger 'selected'

        link_clicked: (event) =>
            # Prevents checkbox toggling when we click a link.
            event.stopPropagation()

        mark_selection: =>
            # Toggle the selected class  and check / uncheck  its checkbox
            @.$el.toggleClass 'selected', @selected
            @.$(':checkbox').prop 'checked', @selected

    # Namespace list element
    class @NamespaceListElement extends @CheckboxListElement
        template: Handlebars.compile $('#namespace_list_element-template').html()

        events: ->
            _.extend super,
                'click a.rename-namespace': 'rename_namespace'

        initialize: ->
            log_initial '(initializing) list view: namespace'
            super @template

        json_for_template: =>
            json = _.extend super(), DataUtils.get_namespace_status(@model.get('id'))
            return json

        rename_namespace: (event) ->
            event.preventDefault()
            rename_modal = new ClusterView.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()

    # Machine list element
    class @MachineListElement extends @CheckboxListElement
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

            @.$('.machine.summary').html @summary_template json

        rename_machine: (event) ->
            event.preventDefault()
            rename_modal = new ClusterView.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

    class @CollapsibleListElement extends Backbone.View
        events: ->
            'click .header': 'toggle_showing'
            'click a': 'link_clicked'

        initialize: ->
            @showing = true

        render: =>
            @show()
            @delegateEvents()

        link_clicked: (event) =>
            # Prevents collapsing when we click a link.
            event.stopPropagation()

        toggle_showing: =>
            @showing = not @showing
            @show()

        show: =>
            @.$('.machine-list').toggle @showing
            @swap_divs()

        show_with_transition: =>
            @.$('.machine-list').slideToggle @showing
            @swap_divs()

        swap_divs: =>
            $arrow = @.$('.arrow.collapsed, .arrow.expanded')

            for div in [$arrow]
                if @showing
                    div.removeClass('collapsed').addClass('expanded')
                else
                    div.removeClass('expanded').addClass('collapsed')

    # Datacenter list element
    class @DatacenterListElement extends @CollapsibleListElement
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

            @machine_list = new ClusterView.MachineList @model.get('id')
            @remove_datacenter_dialog = new ClusterView.RemoveDatacenterModal
            @callbacks = []
            @no_machines = true

            @model.on 'change', @render_summary
            directory.on 'all', @render_summary
            @machine_list.on 'size_changed', =>
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
            rename_modal = new ClusterView.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render()

        register_machine_callbacks: (callbacks) =>
            @callbacks = callbacks
            @machine_list.register_machine_callbacks callbacks

    # Equivalent of a DatacenterListElement, but for machines that haven't been assigned to a datacenter yet.
    class @UnassignedMachinesListElement extends @CollapsibleListElement
        template: Handlebars.compile $('#unassigned_machines_list_element-template').html()

        className: 'unassigned-machines element'

        initialize: ->
            super

            @machine_list = new ClusterView.MachineList null
            @no_machines = true

            machines.on 'add', (machine) => @render() if machine.get('datacenter_uuid') is null
            @machine_list.on 'size_changed', =>
                num_machines = @machine_list.element_views.length

                we_should_rerender = false
                
                if @no_machines and num_machines > 0
                    @no_machines = false
                    we_should_rerender = true
                else if not @no_machines and num_machines is 0
                    @no_machines = true
                    we_should_rerender = true

                @render() if we_should_rerender

            @callbacks = []

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

    class @AbstractModal extends Backbone.View
        className: 'modal-parent'

        events: ->
            'click .cancel': 'cancel_modal'
            'click .close': 'cancel_modal'

        # Default validation rules, meant to be extended by inheriting modal classes
        validator_defaults:
            # Add error class to parent div (supporting Bootstrap style)
            showErrors: (error_map, error_list) ->
                for element of error_map
                    $("label[for='#{ element }']", @$modal).parent().addClass 'error'
                @.defaultShowErrors()

            errorElement: 'span'
            errorClass: 'help-inline'

            success: (label) ->
                # Remove the error class from the parent div when successful
                label.parent().parent().removeClass('error')

        initialize: (template) ->
            @$container = $('#modal-dialog')
            @template = template

        # Render and show the modal.
        #   validator_options: object that defines form behavior and validation rules
        #   template_json: json to pass to the template for the modal
        render: (validator_options, template_data) =>
            # Add the modal generated from the template to the container, and show it
            template_data = {} if not template_data?
            @$container.html @template template_data

            # Note: Bootstrap's modal JS moves the modal from the container element to the end of the body tag in the DOM
            @$modal = $('.modal', @$container).modal
                'show': true
                'backdrop': true
                'keyboard': true
            .on 'hidden', =>
                # Removes the modal dialog from the DOM
                @$modal.remove()

            # Define @el to be the modal (the root of the view), make sure events perform on it
            @.setElement @$modal
            @.delegateEvents()

            # Fill in the passed validator options with default options
            validator_options = _.defaults validator_options, @validator_defaults
            @validator = $('form', @$modal).validate validator_options

            register_modal @

        hide_modal: ->
            @$modal.modal('hide') if @$modal?

        cancel_modal: (e) ->
            @.hide_modal()
            e.preventDefault()

    class @ConfirmationDialogModal extends @AbstractModal
        template: Handlebars.compile $('#confirmation_dialog-template').html()
        class: 'confirmation-dialog'

        initialize: ->
            log_initial '(initializing) modal dialog: confirmation'
            super @template

        render: (message, url, data, on_success) =>
            log_render '(rendering) add secondary dialog'

            # Define the validator options
            validator_options =
                submitHandler: =>
                    $.ajax
                        processData: false
                        url: url
                        type: 'POST'
                        contentType: 'application/json'
                        data: data
                        success: on_success

            super validator_options, { 'message': message }

    class @RenameItemModal extends @AbstractModal
        template: Handlebars.compile $('#rename_item-modal-template').html()
        alert_tmpl: Handlebars.compile $('#renamed_item-alert-template').html()

        initialize: (uuid, type, on_success) ->
            log_initial '(initializing) modal dialog: rename item'
            @item_uuid = uuid
            @item_type = type
            @on_success = on_success
            super @template

        get_item_object: ->
            switch @item_type
                when 'datacenter' then return datacenters.get(@item_uuid)
                when 'namespace' then return namespaces.get(@item_uuid)
                when 'machine' then return machines.get(@item_uuid)
                else return null

        get_item_url: ->
            switch @item_type
                when 'datacenter' then return 'datacenters'
                when 'namespace' then return namespaces.get(@item_uuid).get('protocol') + '_namespaces'
                when 'machine' then return 'machines'
                else return null

        render: ->
            log_render '(rendering) rename item dialog'

            # Define the validator options
            validator_options =
                rules:
                   name: 'required'

                messages:
                   name: 'Required'

                submitHandler: =>
                    old_name = @get_item_object().get('name')
                    formdata = form_data_as_object($('form', @$modal))

                    $.ajax
                        processData: false
                        url: "/ajax/" + @get_item_url() + "/#{@item_uuid}/name"
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify(formdata.new_name)
                        success: (response) =>
                            clear_modals()

                            # update proper model with the name
                            @get_item_object().set('name', formdata.new_name)

                            # notify the user that we succeeded
                            $('#user-alert-space').append @alert_tmpl
                                type: @item_type
                                old_name: old_name
                                new_name: formdata.new_name

                            # Call custom success function
                            if @on_success?
                                @on_success response

            super validator_options,
                type: @item_type
                old_name: @get_item_object().get('name')

    class @AddDatacenterModal extends @AbstractModal
        template: Handlebars.compile $('#add_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_datacenter-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: add datacenter'
            super @template

        render: ->
            log_render '(rendering) add datacenter dialog'

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
                        url: '/ajax/datacenters/new'
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify({"name" : formdata.name})

                        success: (response) =>
                            clear_modals()

                            # Parse the response JSON, apply appropriate diffs, and show an alert
                            apply_to_collection(datacenters, response)
                            for response_uuid, blah of response
                                break
                            $('#user-alert-space').html @alert_tmpl
                                name: formdata.name
                                uuid: response_uuid

            super validator_options

    class @RemoveDatacenterModal extends @AbstractModal
        template: Handlebars.compile $('#remove_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#removed_datacenter-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: remove datacenter'
            super @template

        render: (datacenter) ->
            log_render '(rendering) remove datacenters dialog'
            validator_options =
                submitHandler: =>
                    $.ajax
                        url: "/ajax/datacenters/#{datacenter.id}"
                        type: 'DELETE'
                        contentType: 'application/json'

                        success: (response) =>
                            clear_modals()

                            if (response)
                                throw "Received a non null response to a delete... this is incorrect"
                            datacenters.remove(datacenter.id)
                            $('#user-alert-space').html @alert_tmpl
                                name: datacenter.get('name')

            super validator_options, { 'datacenter': datacenter.toJSON() }

    class @SetDatacenterModal extends @AbstractModal
        template: Handlebars.compile $('#set_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#set_datacenter-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: set datacenter'
            super @template

        render: (machines_list) ->
            log_render '(rendering) set datacenter dialog'

            validator_options =
                rules:
                    datacenter_uuid: 'required'

                messages:
                    datacenter_uuid: 'Required'

                submitHandler: =>
                    formdata = form_data_as_object($('form', @$modal))
                    # Prepare json to pass to the server
                    json = {}
                    for _m in machines_list
                        json[_m.get('id')] =
                            datacenter_uuid: formdata.datacenter_uuid

                    # Set the datacenters!
                    $.ajax
                        processData: false
                        url: "/ajax/machines"
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify(json)

                        success: (response) =>
                            clear_modals()

                            for _m_uuid, _m of response
                                machines.get(_m_uuid).set(_m)

                            machine_names = _.map(machines_list, (_m) -> name: _m.get('name'))
                            $('#user-alert-space').append (@alert_tmpl
                                datacenter_name: datacenters.get(formdata.datacenter_uuid).get('name')
                                machines_first: machine_names[0]
                                machines_rest: machine_names.splice(1)
                                machine_count: machines_list.length
                            )

            super validator_options, { datacenters: (datacenter.toJSON() for datacenter in datacenters.models) }

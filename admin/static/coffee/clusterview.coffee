
# Main cluster view: list of datacenters, machines, and namespaces
module 'ClusterView', ->

    # Container for each tab
    # TODO: Is it okay for these three view types to have the same className?
    class @NamespacesContainer extends Backbone.View
        className: 'cluster-view'
        template: Handlebars.compile $('#namespaces_container-template').html()

        initialize: ->
            log_initial '(initializing) NamespacesContainer'

            @list_view = new ClusterView.NamespaceList
                collection: @options.namespaces
                element_view_class: ClusterView.NamespaceListElement

        render: =>
            log_render '(rendering) NamespacesContainer'
            @.$el.html @template()
            $('#namespaces-panel', @el).html(@list_view.render().el)
            return @

    class @DatacentersContainer extends Backbone.View
        className: 'cluster-view'
        template: Handlebars.compile $('#datacenters_container-template').html()

        initialize: ->
            log_initial '(initializing) DatacentersContainer'

            @list_view = new ClusterView.DatacenterList
                collection: @options.datacenters
                element_view_class: ClusterView.DatacenterListElement

        render: =>
            log_render '(rendering) DatacentersContainer'
            @.$el.html @template()
            $('#datacenters-panel', @el).html(@list_view.render().el)
            return @

    class @MachinesContainer extends Backbone.View
        className: 'cluster-view'
        template: Handlebars.compile $('#machines_container-template').html()

        initialize: ->
            log_initial '(initializing) MachinesContainer'

            @list_view = new ClusterView.MachineList
                collection: @options.machines
                element_view_class: ClusterView.MachineListElement

        render: =>
            log_render '(rendering) MachinesContainer'
            @.$el.html @template()
            $('#machines-panel', @el).html(@list_view.render().el)
            return @

    # Abstract list container: holds an action bar and the list elements
    class @AbstractList extends Backbone.View
        # Use a generic template by default for a list
        template: Handlebars.compile $('#cluster_view-abstract_list-template').html()

        className: 'list'

        initialize: ->
            log_initial '(initializing) list view: ' + class_name @collection
            @container = 'tbody'
            # List of element views
            @element_views = []

            # Initially we need to populate the element views list
            @reset_element_views()

            # Collection is reset, create all new views
            @collection.on 'reset', (collection) =>
                console.log 'reset detected, adding models to collection ' + class_name collection
                @reset_element_views()
                @render()

            # When an element is added to the collection, create a view for it
            @collection.on 'add', (model, collection) =>
                @add_element model
                @render()

            @collection.on 'remove', (model, collection) =>
                # Find any views that are based on the model being removed
                matching_views = (view for view in @element_views when view.model is model)

                # For all matching views, remove the view from the DOM and from the known element views list
                for view in matching_views
                    view.remove()
                    @element_views = _.without @element_views, view

        render: =>
            log_render '(rendering) list view: ' + class_name @collection
            # Render and append the list template to the DOM
            @.$el.html(@template({}))

            log_action '#render_elements of collection ' + class_name @collection
            # Render the view for each element in the list
            $(@container, @el).append(view.render().el) for view in @element_views

            # Check whether buttons for actions on at least one element should be enabled
            @elements_selected()

            @.delegateEvents()
            return @

        reset_element_views: =>
            # Wipe out any existing list element views
            view.remove() for view in @element_views
            @element_views = []

            # Add an element view for each element in the list
            @add_element model for model in @collection.models

        add_element: (element) =>
            # adds a list element view to element_views
            element_view = new @options.element_view_class
                model: element
                collection: @collection

            element_view.on 'selected', @elements_selected
            @element_views.push element_view

        elements_selected: =>
            # Identify buttons for actions on at least one element
            $actions = $('.actions-bar a.btn.for-multiple-elements', @el)

            # If at least one element is selected, enable these buttons
            $actions.toggleClass 'disabled', @get_selected_elements().length < 1

        # Return an array of elements that are selected in the current list
        get_selected_elements: =>
            selected_elements = []
            selected_elements.push view.model for view in @element_views when view.selected
            return selected_elements


    class @NamespaceList extends @AbstractList
        # Use a namespace-specific template for the namespace list
        template: Handlebars.compile $('#cluster_view-namespace_list-template').html()

        # Extend the generic list events
        events: ->
            'click a.btn.add-namespace': 'add_namespace'
            'click a.btn.remove-namespace': 'remove_namespace'

        initialize: ->
            log_initial '(initializing) namespace list view'

            @add_namespace_dialog = new NamespaceView.AddNamespaceModal
            @remove_namespace_dialog = new NamespaceView.RemoveNamespaceModal

            super

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

    class @DatacenterList extends @AbstractList
        # Use a datacenter-specific template for the datacenter list
        template: Handlebars.compile $('#cluster_view-datacenter_list-template').html()

        # Extend the generic list events
        events:
            'click a.btn.add-datacenter': 'add_datacenter'
            'click a.btn.remove-datacenter': 'remove_datacenter'

        initialize: ->
            @add_datacenter_dialog = new ClusterView.AddDatacenterModal
            @remove_datacenter_dialog = new ClusterView.RemoveDatacenterModal

            super

        add_datacenter: (event) =>
            log_action 'add datacenter button clicked'
            @add_datacenter_dialog.render()
            event.preventDefault()

        remove_datacenter: (event) ->
            log_action 'remove datacenter button clicked'
            if not $(event.currentTarget).hasClass 'disabled'
                @remove_datacenter_dialog.render @get_selected_elements()
            event.preventDefault()

    class @MachineList extends @AbstractList
        # Use a machine-specific template for the machine list
        template: Handlebars.compile $('#cluster_view-machine_list-template').html()

        # Extend the generic list events
        events:
            'click a.btn.set-datacenter': 'set_datacenter'

        initialize: ->
            @set_datacenter_dialog = new ClusterView.SetDatacenterModal

            super

        set_datacenter: (event) =>
            log_action 'set datacenter button clicked'
            if not $(event.currentTarget).hasClass 'disabled'
                @set_datacenter_dialog.render @get_selected_elements()
            event.preventDefault()

    # Abstract list element that shows basic info about the element and is clickable
    class @AbstractListElement extends Backbone.View
        tagName: 'tr'
        className: 'element'

        events:
            'click': 'clicked'
            'click a': 'link_clicked'

        initialize: (template) ->
            @template = template
            @selected = false

            @model.on 'change', =>
                @render()

        render: =>
            log_render '(rendering) list element view: ' + class_name @model
            @.$el.html @template(@json_for_template())
            @mark_selection()
            @delegateEvents()

            return @

        json_for_template: =>
            return @model.toJSON()

        clicked: (event) =>
            @selected = not @selected
            @.trigger 'selected'
            @mark_selection()

        link_clicked: (event) =>
            # Prevents checkbox toggling when we click a link.
            event.stopPropagation()

        mark_selection: =>
            # Toggle the selected class  and check / uncheck  its checkbox
            @.$el.toggleClass 'selected', @selected
            $(':checkbox', @el).prop 'checked', @selected


    # Namespace list element
    class @NamespaceListElement extends @AbstractListElement
        template: Handlebars.compile $('#cluster_view-namespace_list_element-template').html()

        initialize: ->
            log_initial '(initializing) list view: namespace'
            super @template

        json_for_template: =>
            stuff = super()

            stuff.nshards = 0
            stuff.nreplicas = 0
            stuff.nashards = 0
            stuff.nareplicas = 0

            _machines = []
            _datacenters = []

            for machine_uuid, role of @model.get('blueprint').peers_roles
                peer_accessible = directory.get(machine_uuid)
                _machines[_machines.length] = machine_uuid
                _datacenters[_datacenters.length] = machines.get(machine_uuid).get('datacenter_uuid')
                for shard, role_name of role
                    if role_name is 'role_primary'
                        stuff.nshards += 1
                        if peer_accessible?
                            stuff.nashards += 1
                    if role_name is 'role_secondary'
                        stuff.nreplicas += 1
                        if peer_accessible?
                            stuff.nareplicas += 1

            stuff.nmachines = _.uniq(_machines).length
            stuff.ndatacenters = _.uniq(_datacenters).length
            if stuff.nshards is stuff.nashards
                stuff.reachability = 'Live'
            else
                stuff.reachability = 'Down'

            return stuff

    # Datacenter list element
    class @DatacenterListElement extends @AbstractListElement
        template: Handlebars.compile $('#cluster_view-datacenter_list_element-template').html()

        initialize: ->
            log_initial '(initializing) list view: datacenter'
            # Any of these models may affect the view, so rerender
            directory.on 'all', =>
                @render()
            machines.on 'all', =>
                @render()
            datacenters.on 'all', =>
                @render()
            super @template

        json_for_template: =>
            stuff = super()

            try
                # total number of machines in this datacenter
                stuff.total = (_.filter machines.models, (m) => m.get('datacenter_uuid') == @model.get('id')).length
                # real number of machines in this datacenter
                stuff.reachable = (_.filter directory.models, (m) => machines.get(m.get('id')).get('datacenter_uuid') == @model.get('id')).length
                if(stuff.reachable > 0)
                    stuff.status = 'Live'
                else
                    stuff.status = 'Down'

            catch err
                stuff.total = 'N/A'
                stuff.reachable = 'N/A'
                stuff.status = 'N/A'

            # primary, secondary, and namespace counts
            stuff.primary_count = 0
            stuff.secondary_count = 0
            _namespaces = []
            for namespace in namespaces.models
                for machine_uuid, peer_role of namespace.get('blueprint').peers_roles
                    if machines.get(machine_uuid).get('datacenter_uuid') is @model.get('id')
                        _namespaces[_namespaces.length] = namespace
                        for shard, role of peer_role
                            if role is 'role_primary'
                                stuff.primary_count += 1
                            if role is 'role_secondary'
                                stuff.secondary_count += 1
            stuff.namespace_count = _.uniq(_namespaces).length

            # last_seen - go through the machines in the datacenter,
            # and find last one down
            if not stuff.reachable and stuff.total > 0
                for machine in machines.models
                    if machine.get('datacenter_uuid') is @model.get('id')
                        _last_seen = machine.get('last_seen')
                        if last_seen
                            if _last_seen > last_seen
                                last_seen = _last_seen
                        else
                            last_seen = _last_seen
                stuff.last_seen = $.timeago(new Date(parseInt(last_seen) * 1000))

            return stuff

    # Machine list element
    class @MachineListElement extends @AbstractListElement
        template: Handlebars.compile $('#cluster_view-machine_list_element-template').html()

        initialize: ->
            log_initial '(initializing) list view: machine'

            directory.on 'all', =>
                @render()

            # Load abstract list element view with the machine template
            super @template

        json_for_template: =>
            stuff = super()
            # status
            _.extend stuff, DataUtils.get_machine_reachability(@model.get('id'))

            # ip
            stuff.ip = "TBD"
            # grab datacenter name
            if @model.get('datacenter_uuid')
                # We need this in case the server disconnects/reconnects
                try
                    stuff.datacenter_name = datacenters.find((d) => d.get('id') == @model.get('datacenter_uuid')).get('name')
                catch err
                    stuff.datacenter_name = 'N/A'
            else
                stuff.datacenter_name = "Unassigned"

            # primary, secondary, and namespace counts
            stuff.primary_count = 0
            stuff.secondary_count = 0
            _namespaces = []
            for namespace in namespaces.models
                for machine_uuid, peer_role of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        _namespaces[_namespaces.length] = namespace
                        for shard, role of peer_role
                            if role is 'role_primary'
                                stuff.primary_count += 1
                            if role is 'role_secondary'
                                stuff.secondary_count += 1
            stuff.namespace_count = _.uniq(_namespaces).length

            return stuff

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
            @$container.append @template template_data
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
            console.log 'clicked cancel_modal'
            @.hide_modal()
            e.preventDefault()

    class @ConfirmationDialogModal extends @AbstractModal
        template: Handlebars.compile $('#confirmation_dialog-template').html()
        class: 'confirmation-dialog'

        initialize: ->
            log_initial '(initializing) modal dialog: confirmation'
            super @template

        render: (message, url, data, alert_tmpl) =>
            log_render '(rendering) add secondary dialog'

            # Define the validator options
            validator_options =
                submitHandler: =>
                    $('form', @$modal).ajaxSubmit
                        url: url ? '/ajax/foobarbaz'  # TODO get rid of foobarbaz, make callers be rightful
                        type: 'POST'
                        data: data

                        success: (response) =>
                            clear_modals()

                            response_json = $.parseJSON response
                            apply_diffs response_json.diffs
                            $('#user-alert-space').append alert_tmpl response_json.op_result

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
                            #$('#user-alert-space').html @alert_tmpl response_json.op_result TODO make this work again after we have alerts in our responses

            super validator_options

    class @RemoveDatacenterModal extends @AbstractModal
        template: Handlebars.compile $('#remove_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#removed_datacenter-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: remove datacenter'
            super @template

        render: (datacenters_to_delete) ->
            log_render '(rendering) remove datacenters dialog'
            validator_options =
                submitHandler: =>
                    for datacenter in datacenters_to_delete
                        $.ajax
                            url: "/ajax/datacenters/#{datacenter.id}"
                            type: 'DELETE'
                            contentType: 'application/json'

                            success: (response) =>
                                clear_modals()

                                if (response)
                                    throw "Received a non null response to a delete... this is incorrect"
                                datacenters.remove(datacenter.id)
                                #TODO hook this up
                                #for namespace in response_json.op_result
                                    #$('#user-alert-space').append @alert_tmpl namespace

            array_for_template = _.map datacenters_to_delete, (datacenter) -> datacenter.toJSON()
            super validator_options, { 'datacenters': array_for_template }

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
                    # TODO: We really should do one ajax request
                    for m in machines_list
                        $.ajax
                            processData: false
                            url: "/ajax/machines/#{m.id}"
                            type: 'POST'
                            contentType: 'application/json'
                            data: JSON.stringify({"datacenter_uuid" : formdata.datacenter_uuid})

                            success: (response) =>
                                clear_modals()

                                machines.get(m.id).set(response)

                                # We only have to do this one
                                if m is machines_list[machines_list.length - 1]
                                    $('#user-alert-space').append (@alert_tmpl
                                        datacenter_name: datacenters.get(formdata.datacenter_uuid).get('name')
                                        machines: _.map(machines_list, (_m) ->
                                            name: _m.get('name')
                                        )
                                        machine_count: machines_list.length
                                    )

            super validator_options, { datacenters: (datacenter.toJSON() for datacenter in datacenters.models) }


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
            'click a.btn.suspend-machine': 'suspend_machine'
            'click a.btn.set-datacenter': 'set_datacenter'

        initialize: ->
            @suspend_machine_dialog = new ClusterView.SuspendMachineModal
            @set_datacenter_dialog = new ClusterView.SetDatacenterModal

            super

        suspend_machine: (event) =>
            log_action 'remove namespace button clicked'
            if not $(event.currentTarget).hasClass 'disabled'
                @suspend_machine_dialog.render()
            event.preventDefault()

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

    # Datacenter list element
    class @DatacenterListElement extends @AbstractListElement
        template: Handlebars.compile $('#cluster_view-datacenter_list_element-template').html()

        initialize: ->
            log_initial '(initializing) list view: datacenter'
            super @template

    # Machine list element
    class @MachineListElement extends @AbstractListElement
        template: Handlebars.compile $('#cluster_view-machine_list_element-template').html()

        initialize: ->
            log_initial '(initializing) list view: machine'

            # Sparkline for CPU
            @cpu_sparkline =
                data : []
                total_points: 30
                update_interval: 750
            @cpu_sparkline.data[i] = 0 for i in [0...@cpu_sparkline.total_points]

            # Load abstract list element view with the machine template
            super @template
            setInterval @update_sparklines, @cpu_sparkline.update_interval

        # Update the data and render a new sparkline for this view
        update_sparklines: =>
            @cpu_sparkline.data = @cpu_sparkline.data.slice(1)
            @cpu_sparkline.data.push @model.get('cpu')
            $('.cpu-graph', @el).sparkline(@cpu_sparkline.data)

        json_for_template: =>
            stuff = super()
            if @model.get('datacenter_uuid')
                stuff.datacenter_name = datacenters.find((d) => d.get('id') == @model.get('datacenter_uuid')).get('name')
            else
                stuff.datacenter_name = "Unassigned"
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
                            response_json = $.parseJSON response
                            apply_diffs response_json.diffs
                            $('#user-alert-space').html @alert_tmpl response_json.op_result

            super validator_options

    class @RemoveDatacenterModal extends @AbstractModal
        template: Handlebars.compile $('#remove_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#removed_datacenter-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: remove datacenter'
            super @template

        render : (datacenters_to_delete) ->
            log_render '(rendering) remove datacenter dialog'

            validator_options =
                submitHandler: =>
                    url = '/ajax/datacenters?ids='
                    num_datacenters = datacenters_to_delete.length
                    for datacenter in datacenters_to_delete
                        url += datacenter.id
                        url += "," if num_datacenters-=1 > 0

                    url += '&token=' + token

                    # TODO: This action causes a bunch of machines to
                    # reference a datacenter that no longer exists,
                    # and javascript elsewhere can't handle this
                    # inconsistent data.  Generally speaking, all our
                    # javascript needs to be able to handle
                    # inconsistent data.

                    $('form', @$modal).ajaxSubmit
                        url: url
                        type: 'DELETE'

                        success: (response) =>
                            clear_modals()

                            # Parse the response JSON, apply appropriate diffs, and show an alert
                            response_json = $.parseJSON response
                            apply_diffs response_json.diffs
                            for datacenter in response_json.op_result
                                $('#user-alert-space').append @alert_tmpl datacenter

            array_for_template = _.map datacenters_to_delete, (datacenter) -> datacenter.toJSON()
            super validator_options, { 'datacenters': array_for_template }

    class @SuspendMachineModal extends @AbstractModal
        template: Handlebars.compile $('#suspend_machine-modal-template').html()
        alert_tmpl: Handlebars.compile $('#suspended_machine-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: suspend machine'
            super @template

        render: ->
            log_render '(rendering) suspend machine dialog'
            super {}

    class @SetDatacenterModal extends @AbstractModal
        template: Handlebars.compile $('#set_datacenter-modal-template').html()
        alert_tmpl: Handlebars.compile $('#set_datacenter-alert-template').html()

        initialize: ->
            log_initial '(initializing) modal dialog: set datacenter'
            super @template

        render: (machines) ->
            log_render '(rendering) set datacenter dialog'

            validator_options =
                rules:
                    datacenter_uuid: 'required'

                messages:
                    datacenter_uuid: 'Required'

                submitHandler: =>
                    formdata = form_data_as_object($('form', @$modal))
                    for m in machines
                        $.ajax
                            processData: false
                            url: "/ajax/machines/#{m.id}"
                            type: 'POST'
                            contentType: 'application/json'
                            data: JSON.stringify({"datacenter_uuid" : formdata.datacenter_uuid})

                            success: (response) =>
                                clear_modals()

                                response_json = $.parseJSON response
                                apply_diffs response_json.diffs
                                $('#user-alert-space').append (@alert_tmpl {
                                    datacenter_name: datacenters.find((d) -> d.get('id') == response_json.op_result.datacenter_uuid).get('name'),
                                    machine_name: m.get('name')
                                })


            super validator_options, { datacenters: (datacenter.toJSON() for datacenter in datacenters.models) }

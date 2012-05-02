
# This file extends the namespace module with functionality to present
# the index of namespaces.
module 'NamespaceView', ->
    # Show a list of namespaces
    class @NamespaceList extends UIComponents.AbstractList
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

            super namespaces, NamespaceView.NamespaceListElement, 'tbody.list'

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

    # Namespace list element
    class @NamespaceListElement extends UIComponents.CheckboxListElement
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
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()

    # A modal for adding namespaces
    class @AddNamespaceModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#add_namespace-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_namespace-alert-template').html()
        class: 'add-namespace'

        initialize: ->
            log_initial '(initializing) modal dialog: add namespace'
            super

        render: ->
            log_render '(rendering) add namespace dialog'

            super
                modal_title: 'Add namespace'
                btn_primary_text: 'Add'
                datacenters: _.map(datacenters.models, (datacenter) -> datacenter.toJSON())

        on_submit: ->
            super

            formdata = form_data_as_object($('form', @$modal))
            $.ajax
                processData: false
                url: '/ajax/memcached_namespaces/new'
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(
                    name: formdata.name
                    primary_uuid: formdata.primary_datacenter
                    port : parseInt(formdata.port)
                    )
                success: @on_success

        on_success: (response) =>
            super

            # the result of this operation are some
            # attributes about the namespace we
            # created, to be used in an alert
            apply_to_collection(namespaces, add_protocol_tag(response, "memcached"))

            # Notify the user
            for id, namespace of response
                $('#user-alert-space').append @alert_tmpl
                    uuid: id
                    name: namespace.name

    # A modal for removing namespaces
    class @RemoveNamespaceModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#remove_namespace-modal-template').html()
        alert_tmpl: Handlebars.compile $('#removed_namespace-alert-template').html()
        class: 'remove-namespace-dialog'

        initialize: ->
            log_initial '(initializing) modal dialog: remove namespace'
            super

        render: (_namespaces_to_delete) ->
            log_render '(rendering) remove namespace dialog'
            @namespaces_to_delete = _namespaces_to_delete

            array_for_template = _.map @namespaces_to_delete, (namespace) -> namespace.toJSON()
            super
                modal_title: 'Remove namespace'
                btn_primary_text: 'Remove'
                namespaces: array_for_template

        on_submit: ->
            super
            # TODO: change this once we have http support for deleting
            # multiple items with one http request.
            for namespace in @namespaces_to_delete
                $.ajax
                    url: "/ajax/#{namespace.get("protocol")}_namespaces/#{namespace.id}"
                    type: 'DELETE'
                    contentType: 'application/json'
                    success: @on_success
            # TODO: this should happen on success, but I'm hacking
            # this in here until we support multiple deletes in one
            # blow on the server. Remove the next two lines once
            # they're uncommented in on_success.
            for namespace in @namespaces_to_delete
                namespaces.remove(namespace.id)

        on_success: (response) ->
            super

            if (response)
                throw "Received a non null response to a delete... this is incorrect"

            # TODO: hook this up once we have support for deleting
            # multiple items with one http request.
            #for namespace in @namespaces_to_delete
            #    namespaces.remove(namespace.id)
            #
            #for namespace in response_json.op_result
            #    $('#user-alert-space').append @alert_tmpl namespace


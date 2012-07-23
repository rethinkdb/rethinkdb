
# This file extends the namespace module with functionality to present
# the index of namespaces.
module 'NamespaceView', ->
    # Show a list of namespaces
    class @NamespaceList extends UIComponents.AbstractList
        # Use a namespace-specific template for the namespace list
        template: Handlebars.compile $('#namespace_list-template').html()
        error_template: Handlebars.compile $('#error_adding_namespace-template').html()

        # Extend the generic list events
        events: ->
            'click a.btn.add-namespace': 'add_namespace'
            'click a.btn.remove-namespace': 'remove_namespace'
            'click .close': 'remove_parent_alert'

        initialize: ->
            log_initial '(initializing) namespace list view'

            @add_namespace_dialog = new NamespaceView.AddNamespaceModal
            @remove_namespace_dialog = new NamespaceView.RemoveNamespaceModal

            super namespaces, NamespaceView.NamespaceListElement, 'tbody.list'
            @to_unbind = []
        render: =>
            super
            @update_toolbar_buttons()
            return @

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()

        # Extend the AbstractList.add_element method to bind a callback to each namespace added to the list
        add_element: (element) =>
            machine_list_element = super element
            machine_list_element.off 'selected'
            machine_list_element.on 'selected', @update_toolbar_buttons


        add_namespace: (event) =>
            event.preventDefault()
            if datacenters.length is 0
                @.$('#user-alert-space').html @error_template
                @.$('#user-alert-space').alert()
            else
                log_action 'add namespace button clicked'
                @add_namespace_dialog.render()
                $('#focus_namespace_name').focus()

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

        destroy: =>
             super()
           

    # Namespace list element
    class @NamespaceListElement extends UIComponents.CheckboxListElement
        template: Handlebars.compile $('#namespace_list_element-template').html()

        history_opsec: []

        events: ->
            _.extend super,
                'click a.rename-namespace': 'rename_namespace'
                'mouseenter .contains_info': 'display_popover'
                'mouseleave .contains_info': 'hide_popover'

        hide_popover: ->
            $('.tooltip').remove()

        display_popover: (event) ->
            $(event.currentTarget).tooltip('show')

        initialize: ->
            log_initial '(initializing) list view: namespace'
            super @template

            # Initialize history
            for i in [0..40]
                @history_opsec.push 0

            @model.on 'change', @render
            machines.on 'all', @render

        update_history_opsec: =>
            @history_opsec.shift()
            @history_opsec.push @model.get_stats().keys_read + @model.get_stats().keys_set


        json_for_template: =>
            json = _.extend super(), DataUtils.get_namespace_status(@model.get('id'))
            json.nreplicas += json.nshards

            data_in_memory = 0
            data_total = 0
            for machine in machines.models
                if machine.get('stats')? and @model.get('id') of machine.get('stats') and machine.get('stats')[@model.get('id')].cache?
                    data_in_memory += machine.get('stats')[@model.get('id')].cache.block_size*machine.get('stats')[@model.get('id')].cache.blocks_in_memory
                    data_total += machine.get('stats')[@model.get('id')].cache.block_size*machine.get('stats')[@model.get('id')].cache.blocks_total
            json.data_in_memory_percent = Math.floor(data_in_memory/data_total*100)
            json.data_in_memory = human_readable_units(data_in_memory, units_space)
            json.data_total = human_readable_units(data_total, units_space)

            @update_history_opsec()
            json.opsec = @history_opsec[@history_opsec.length-1]

            return json

        render: =>
            super()

            sparkline_attr =
                fillColor: false
                spotColor: false
                minSpotColor: false
                maxSpotColor: false
                chartRangeMin: 0
                width: '75px'
                height: '15px'

            @.$('.opsec_sparkline').sparkline @history_opsec, sparkline_attr

            return @

        rename_namespace: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()

        destroy: =>
            @model.off 'change', @render
            machines.off 'all', @render

    # A modal for adding namespaces
    class @AddNamespaceModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#add_namespace-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_namespace-alert-template').html()
        error_template: Handlebars.compile $('#error_input-template').html()
        class: 'add-namespace'

        initialize: ->
            log_initial '(initializing) modal dialog: add namespace'
            super

        render: ->
            log_render '(rendering) add namespace dialog'

            default_port = 11211
            used_ports = {}
            for namespace in namespaces.models
                used_ports[namespace.get('port')] = true

            while default_port of used_ports
                default_port++

            super
                modal_title: 'Add namespace'
                btn_primary_text: 'Add'
                datacenters: _.map(datacenters.models, (datacenter) -> datacenter.toJSON())
                default_port: default_port

        on_submit: =>
            super

            formdata = form_data_as_object($('form', @$modal))

            template_error = {}
            input_error = false

            need_to_increase = false
            if DataUtils.is_integer(formdata.port) is false
                input_error = true
                template_error.port_isnt_integer = true
            else
                formdata.port = parseInt(formdata.port)
                for namespace in namespaces.models
                    if formdata.port is namespace.get('port')
                        input_error = true
                        template_error.port_is_used = true
                        break

            if formdata.name is ''
                input_error = true
                template_error.namespace_is_empty = true
            else
                for namespace in namespaces.models
                    if namespace.get('name') is formdata.name
                        input_error = true
                        template_error.namespace_exists = true
                        break

            if input_error is true
                $('.alert_modal').html @error_template template_error
                $('.alert_modal').alert()
                @reset_buttons()
            else
                $.ajax
                    processData: false
                    url: '/ajax/semilattice/memcached_namespaces/new'
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify(
                        name: formdata.name
                        primary_uuid: formdata.primary_datacenter
                        port : parseInt(formdata.port)
                        )
                    success: @on_success
                    error: @on_error

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
                namespaces_length_is_one: @namespaces_to_delete.length is 1

            @.$('.btn-primary').focus()

        on_submit: ->
            super


            # For when /ajax will handle post request
            data = {}
            for namespace in @namespaces_to_delete
                if not data[namespace.get('protocol')+'_namespaces']?
                    data[namespace.get('protocol')+'_namespaces'] = {}
                data[namespace.get('protocol')+'_namespaces'][namespace.get('id')] = null

            $.ajax
                url: "/ajax/semilattice"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                dataType: 'json',
                success: @on_success,
                error: @on_error

        on_success: (response) ->
            deleted_namespaces = []
            for namespace in @namespaces_to_delete
                deleted_namespaces.push namespaces.get(namespace.id).get('name')

            $('#user-alert-space').append @alert_tmpl 
                deleted_namespaces: deleted_namespaces
            
            apply_diffs response

            super

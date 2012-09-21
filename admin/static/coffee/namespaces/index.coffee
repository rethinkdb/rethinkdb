
# This file extends the namespace module with functionality to present
# the index of namespaces.
module 'NamespaceView', ->
    # Show a list of databases
    class @DatabaseList extends UIComponents.AbstractList
        # Use a namespace-specific template for the namespace list
        template: Handlebars.compile $('#database_list-template').html()
        className: 'databases_list-container'
        error_template: Handlebars.compile $('#error_adding_namespace-template').html()
        alert_message_template: Handlebars.compile $('#alert_message-template').html()

        events:
            'click .add-database': 'add_database'
            'click .add-namespace': 'add_namespace'
            'click .remove-namespace': 'remove_namespace'
            'click .close': 'remove_parent_alert'

        initialize: ->
            log_initial '(initializing) namespace list view'

            @add_database_dialog = new NamespaceView.AddDatabaseModal
            @add_namespace_dialog = new NamespaceView.AddNamespaceModal
            @remove_namespace_dialog = new NamespaceView.RemoveNamespaceModal

            super databases, NamespaceView.DatabaseListElement, '.collapsible-list'

            @datacenters_length = -1
            @databases_length = -1
            datacenters.on 'all', @update_button_create_namespace
            databases.on 'all', @update_button_create_namespace
        
        update_button_create_namespace: =>
            need_update = false
            if (datacenters.models.length>0) isnt (@datacenters_length>0) or @datacenters_length is -1
                need_update = true
                @datacenters_length = datacenters.models.length
            if (databases.models.length>0) isnt (@databases_length>0) or @databases_length is -1
                need_update = true
                @databases_length = databases.models.length
            
            if need_update
                if @datacenters_length isnt 0 and @databases_length isnt 0
                    @.$('.user_alert_space-cannot_create_namespace').html ''
                    @.$('.user_alert_space-cannot_create_namespace').css 'display', 'none'
                    @.$('.add-namespace').removeProp 'disabled'
                else
                    @.$('.user_alert_space-cannot_create_namespace').html @error_template
                        need_datacenter: @datacenters_length is 0
                        need_database: @databases_length is 0
                        need_something: @datacenters_length is 0 or @databases_length is 0
                    @.$('.user_alert_space-cannot_create_namespace').css 'display', 'block'
                    @.$('.add-namespace').prop 'disabled', 'disabled'


        render: (message) =>
            super
            @update_toolbar_buttons()

            if message?
                @.$('#user-alert-space').append @alert_message_template
                    message: message
            @update_button_create_namespace()
            return @

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()

        rename_namespace: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()

        add_database: (event) =>
            @add_database_dialog.render()

        add_namespace: (event) =>
            event.preventDefault()
            @add_namespace_dialog.render()
            $('#focus_namespace_name').focus()

        remove_namespace: (event) =>
            log_action 'remove namespace button clicked'
            # Make sure the button isn't disabled, and pass the list of namespace UUIDs selected
            if not $(event.currentTarget).hasClass 'disabled'
                @remove_namespace_dialog.render @get_selected_namespaces()
            event.preventDefault()

        # Extend the AbstractList.add_element method to bind a callback to each namespace added to the list
        add_element: (element) =>
            namespaces_list_element = super element
            namespaces_list_element.register_namespace_callback [@update_toolbar_buttons]
            #TODO destroy this listener

        # Count up the number of namespaces checked off across all machine lists
        get_selected_namespaces: =>
            namespaces_lists = _.map @element_views, (database_list_element) ->
                database_list_element.namespace_list

            selected_namespaces = []
            for namespaces_list in namespaces_lists
                selected_namespaces = selected_namespaces.concat namespaces_list.get_selected_elements()

            return selected_namespaces


        # Callback that will be registered: updates the toolbar buttons based on how many namespaces have been selected
        update_toolbar_buttons: =>
            @.$('.btn.remove-namespace').prop 'disabled', @get_selected_namespaces().length < 1

        destroy: =>
            super
            datacenters.off 'all', @update_button_create_namespace
            databases.off 'all', @update_button_create_namespace
            @add_database_dialog.destroy()
            @add_namespace_dialog.destroy()
            @remove_namespace_dialog.destroy()

    class @DatabaseListElement extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#database_list_element-template').html()
        summary_template: Handlebars.compile $('#database_list_element-summary-template').html()

        className: 'element-container'
        events:
            'click .delete_database-link': 'remove_database'

        initialize: ->
            log_initial '(initializing) list view: datacenter'

            super

            @namespace_list = new NamespaceView.NamespaceList @model.get('id')
            @callbacks = []
            @no_namespace = true

            @model.on 'change', @render_summary
            @namespace_list.on 'size_changed', @render

        render: =>
            @.$el.html @template({})

            @render_summary()

            # Attach a list of available machines to the given datacenter
            @.$('.element-list-container').html @namespace_list.render().$el

            super

            return @

        render_summary: =>
            json = @model.toJSON()

            @.$('.summary').html @summary_template json

        remove_database: (event) =>
            event.preventDefault()

            db = databases.get @.$(event.target).data('id')
            if db?
                remove_database_dialog = new DatabaseView.RemoveDatabaseModal
                remove_database_dialog.render db
           


        register_namespace_callback: (callbacks) =>
            @callbacks = callbacks
            @namespace_list.register_namespace_callbacks @callbacks

        rename_datacenter: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render()

        destroy: =>
            @model.off 'change', @render_summary
            @namespace_list.off 'size_changed', @nl_size_changed
            @namespace_list.destroy()

    # Show a list of namespaces
    class @NamespaceList extends UIComponents.AbstractList
        # Use a namespace-specific template for the namespace list
        tagName: 'table'
        template: Handlebars.compile $('#namespace_list-template').html()
        error_template: Handlebars.compile $('#error_adding_namespace-template').html()



        initialize:  (database_id) =>
            log_initial '(initializing) namespace list view'

            @add_namespace_dialog = new NamespaceView.AddNamespaceModal
            @remove_namespace_dialog = new NamespaceView.RemoveNamespaceModal
        
            super namespaces, NamespaceView.NamespaceListElement, 'tbody.list',
                {
                filter: (model) -> model.get('database') is database_id
                }
                , 'table', 'database'

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()

        # Extend the AbstractList.add_element method to bind a callback to each namespace added to the list
        add_element: (element) =>
            namespace_list_element = super element
            @bind_callbacks_to_namespace namespace_list_element

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

        register_namespace_callbacks: (callbacks) =>
            @callbacks = callbacks
            @bind_callbacks_to_namespace namespace_list_element for namespace_list_element in @element_views

        bind_callbacks_to_namespace: (namespace_list_element) =>
            namespace_list_element.off 'selected'
            namespace_list_element.on 'selected', => callback() for callback in @callbacks

        destroy: =>
            super()
            @add_namespace_dialog.destroy()
            @remove_namespace_dialog.destroy()

    # Namespace list element
    class @NamespaceListElement extends UIComponents.CheckboxListElement
        template: Handlebars.compile $('#namespace_list_element-template').html()

        #history_opsec: []

        hide_popover: ->
            $('.tooltip').remove()

        initialize: ->
            log_initial '(initializing) list view: namespace'
            super @template

        json_for_template: =>
            json = _.extend super(), DataUtils.get_namespace_status(@model.get('id'))
            json.nreplicas += json.nshards

            return json

        render: =>
            super

            return @

        destroy: =>
            super


    class @AddDatabaseModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#add_database-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_database-alert-template').html()
        error_template: Handlebars.compile $('#error_input-template').html()
        
        class: 'add-database'

        initialize: ->
            log_initial '(initializing) modal dialog: add datacenter'
            super

        render: ->
            log_render '(rendering) add datatabase dialog'
            super
                modal_title: "Add database"
                btn_primary_text: "Add"
            @.$('.focus_new_name').focus()

        on_submit: ->
            super
            @formdata = form_data_as_object($('form', @$modal))

            no_error = true
            if @formdata.name is ''
                no_error = false
                template_error =
                    database_is_empty: true
                $('.alert_modal').html @error_template template_error
                $('.alert_modal').alert()
                @reset_buttons()
            else
                for database in databases.models
                    if database.get('name') is @formdata.name
                        no_error = false
                        template_error =
                            database_exists: true
                        $('.alert_modal').html @error_template template_error
                        $('.alert_modal').alert()
                        @reset_buttons()
                        break
            if no_error is true
                $.ajax
                    processData: false
                    url: '/ajax/semilattice/databases/new'
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify({"name" : @formdata.name})
                    success: @on_success
                    error: @on_error

        on_success: (response) =>
            super
            apply_to_collection(databases, response)

            # Notify the user
            for id, namespace of response
                $('#user-alert-space').append @alert_tmpl
                    name: namespace.name

    # A modal for adding namespaces
    class @AddNamespaceModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#add_namespace-modal-template').html()
        alert_tmpl: Handlebars.compile $('#added_namespace-alert-template').html()
        error_template: Handlebars.compile $('#error_input-template').html()
        class: 'add-namespace'
            
        initialize: =>
            log_initial '(initializing) modal dialog: add namespace'
            super
            # Extend events
            @delegateEvents()

        show_advanced_settings: (event) =>
            event.preventDefault()
            that = @
            @.$('.show_advanced_settings-link_container').fadeOut 'fast', ->
                that.$('.hide_advanced_settings-link_container').fadeIn 'fast'
            @.$('.advanced_settings').slideDown 'fast'
        hide_advanced_settings: (event) =>
            event.preventDefault()
            that = @
            @.$('.hide_advanced_settings-link_container').fadeOut 'fast', ->
                that.$('.show_advanced_settings-link_container').fadeIn 'fast'
            @.$('.advanced_settings').slideUp 'fast'

        render: ->
            log_render '(rendering) add namespace dialog'

            for datacenter in datacenters.models
                datacenter.set 'num_machines', 0

            for machine in machines.models
                if machine.get('datacenter_uuid')?
                    datacenters.get(machine.get('datacenter_uuid')).set 'num_machines',datacenter.get('num_machines')+1

            ordered_datacenters = _.map(datacenters.models, (datacenter) ->
                id: datacenter.get('id')
                name: datacenter.get('name')
                num_machines: datacenter.get('num_machines')
            )
            ordered_datacenters = ordered_datacenters.sort (a, b) ->
                return b.num_machines-a.num_machines

            slice_index = 0
            for datacenter in ordered_datacenters
                if datacenter.num_machines is 0
                    break
                slice_index++

            ordered_datacenters = ordered_datacenters.slice 0, slice_index

            super
                modal_title: 'Add a table'
                btn_primary_text: 'Add'
                datacenters: ordered_datacenters
                all_datacenters: datacenters.length is ordered_datacenters.length
                databases: _.map(databases.models, (database) -> database.toJSON())

            @.$('.show_advanced_settings-link').click @show_advanced_settings
            @.$('.hide_advanced_settings-link').click @hide_advanced_settings

        on_submit: =>
            super

            formdata = form_data_as_object($('form', @$modal))

            template_error = {}
            input_error = false

            if formdata.name is ''
                input_error = true
                template_error.namespace_is_empty = true
            else
                for namespace in namespaces.models
                    if namespace.get('name') is formdata.name and namespace.get('database') is formdata.database
                        input_error = true
                        template_error.namespace_exists = true
                        break

            if formdata.cache_size isnt '' and DataUtils.is_integer(formdata.cache_size) is false
                input_error = true
                template_error.cache_size_format = true
            else if formdata.cache_size isnt ''
                cache_size_int = parseInt formdata.cache_size
                if cache_size_int < 16
                    input_error = true
                    template_error.cache_size_too_small = true
                else if cache_size_int > 1024*64
                    input_error = true
                    template_error.cache_size_too_big = true

            if input_error is true
                $('.alert_modal').html @error_template template_error
                $('.alert_modal').alert()
                @reset_buttons()
            else
                ack = {}
                ack[formdata.primary_datacenter] = 1

                $.ajax
                    processData: false
                    url: '/ajax/semilattice/rdb_namespaces/new'
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify(
                        name: formdata.name
                        primary_uuid: formdata.primary_datacenter
                        database: formdata.database
                        ack_expectations: ack
                        cache_size: parseInt(formdata.cache_size)*1024*1024 if formdata.cache_size isnt ''
                        primary_key: formdata.primary_key if formdata.primary_key isnt ''
                        )
                    success: @on_success
                    error: @on_error

        on_success: (response) =>
            super

            apply_to_collection(namespaces, add_protocol_tag(response, "rdb"))

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

        ton_error: =>
            @.$('.error_answer').html @template_remove_error

            if $('.error_answer').css('display') is 'none'
                $('.error_answer').slideDown('fast')
            else
                $('.error_answer').css('display', 'none')
                $('.error_answer').fadeIn()
            remove_namespace_dialog.reset_buttons()

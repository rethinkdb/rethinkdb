# Copyright 2010-2012 RethinkDB, all rights reserved.

# This file extends the namespace module with functionality to present
# the index of namespaces.
module 'TablesView', ->
    class @DatabasesContainer extends Backbone.View
        id: 'databases_container'
        template:
            main: Handlebars.templates['databases_container-template']
            alert_message: Handlebars.templates['alert_message-template']

        events:
            'click .add-database': 'add_database'
            'click .add-namespace': 'add_namespace'
            'click .remove-tables': 'delete_tables'
            'click .checkbox-container': 'update_delete_tables_button'
            'click .close': 'remove_parent_alert'

        add_database: =>
            @add_database_dialog.render()
        add_namespace: =>
            @add_namespace_dialog.render()
            @add_namespace_dialog.focus()
        delete_tables: (event) =>
            # Make sure the button isn't disabled, and pass the list of namespace UUIDs selected
            if not $(event.currentTarget).is ':disabled'
                selected_tables = []
                @$('.checkbox-table:checked').each () ->
                    selected_tables.push
                        table: $(@).data('table')
                        database: $(@).data('database')
                @remove_tables_dialog.render selected_tables

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()

        get_selected_tables: =>
            result

        update_delete_tables_button: =>
            if @$('.checkbox-table:checked').length > 0
                @.$('.remove-tables').removeAttr 'disabled'
            else
                @.$('.remove-tables').attr 'disabled', true

        initialize: =>
            @databases = new Databases
            @databases_list = new TablesView.DatabasesListView
                collection: @databases

            @fetch_servers()
            @interval = setInterval @fetch_servers, 5000

            @loading = true # TODO Render that

            @add_database_dialog = new Modals.AddDatabaseModal @databases
            @add_namespace_dialog = new Modals.AddNamespaceModal @databases
            @remove_tables_dialog = new Modals.RemoveNamespaceModal

        render: =>
            @$el.html @template.main({})
            @$('.databases_list').html @databases_list.render().$el
            @
        render_message: (message) =>
            @.$('#user-alert-space').append @template.alert_message
                message: message


        fetch_servers: =>
            #TODO Replace later with server_status
            query = r.dbList().filter( (db) ->
                db.ne('rethinkdb')
            ).orderBy(r.row).map (db) ->
                db: db
                id: db # TODO: Use db_config (when available) and remove this merge
                tables: r.db('rethinkdb').table('table_status').orderBy((table) -> table("name"))
                    .filter({db: db}).merge( (table) ->
                        shards: table("shards").count()
                        replicas: table("shards").nth(0).count()
                    ).merge( (table) ->
                        id: table("uuid")
                    )

            driver.run query, (error, result) =>
                console.log '---- err, result ---'
                console.log error
                console.log result
                if error
                    #TODO
                    console.log error
                else
                    @loading = false # TODO Move that outside the `if` statement?
                    databases = {}
                    for database, index in result
                        @databases.add new Database(database), {merge: true}
                        databases[database.id] = true

                    # Clean  removed database
                    toDestroy = []
                    for database in @databases.models
                        if databases[database.get('id')] isnt true
                            toDestroy.push database
                    for database in toDestroy
                        database.destroy()

        destroy: =>
            clearInterval @interval

    class @DatabasesListView extends Backbone.View
        className: 'database_list'
        initialize: =>
            # TODO Enforce order?
            @databases_view = []
            @collection.each (database) =>
                view = new TablesView.DatabaseView
                    model: database

                @databases_view.push view
                @$el.append view.render().$el

            @collection.on 'add', (database) =>
                view = new TablesView.DatabaseView
                    model: database

                @databases_view.push view
                @$el.append view.render().$el

            @collection.on 'remove', (database) =>
                for view in @databases_view
                    if view.model is database
                        database.destroy()
                        view.remove()
                        break

        render: =>
            @

    class @DatabaseView extends Backbone.View
        className: 'database_container section'
        template: Handlebars.templates['database-template']
        events:
           'click button.remove-database': 'remove_database'

        remove_database: =>
            remove_database_dialog = new Modals.RemoveDatabaseModal
            remove_database_dialog.render @model
            remove_database_dialog.focus()


        initialize: =>
            @$el.html @template @model.toJSON()

            @tables_views = []
            @collection = new Tables()

            @update_collection()
            @model.on 'change', @update_collection
            window.mmm = @model

            @collection.each (table) =>
                view = new TablesView.TableView
                    model: table

                @tables_views.push view
                @$('.tables_container').append view.render().$el

            @collection.on 'add', (table) =>
                view = new TablesView.TableView
                    model: table

                @tables_views.push view
                @$('.tables_container').append view.render().$el

        update_collection: =>
            to_destroy = []
            for table in @collection.models
                found = false
                for other_table in @model.get('tables')
                    if other_table.id is table.get('id')
                        found = true
                        break
                if found is false
                    to_destroy.push table

            for table in to_destroy
                for view in @tables_views
                    if view.model is table
                        view.remove()
                        break
                table.destroy()

            for table in @model.get('tables')
                @collection.add new Table table

            #TODO handle remove

        render: ->
            @$el.html @template @model.toJSON()
            for view in @tables_views
                @$('.tables_container').append view.render().$el
            @

    class @TableView extends Backbone.View
        className: 'table_container'
        template: Handlebars.templates['table-template']

        render: =>
            @$el.html @template @model.toJSON()
            @


module 'NamespacesView', ->
    # Show a list of databases
    class @DatabaseList extends UIComponents.AbstractList
        # Use a namespace-specific template for the namespace list
        template: Handlebars.templates['database_list-template']
        className: 'databases_list-container'
        alert_message_template: Handlebars.templates['alert_message-template']

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

            super databases, NamespaceView.DatabaseListElement, '.collapsible-list',
                sort: (a, b) ->
                    if b.model.get('name') < a.model.get('name')
                        return 1
                    else if b.model.get('name') > a.model.get('name')
                        return -1
                    return 0
                , 'database', 'cluster'

            @datacenters_length = -1
            @databases_length = -1
            datacenters.on 'all', @update_button_create_namespace
            databases.on 'all', @update_button_create_namespace
            @can_create_namespace = true

        update_button_create_namespace: =>
            if databases.length is 0 and @can_create_namespace is true
                @.$('.add-namespace').prop 'disabled', true
                @.$('.user_alert_space-cannot_create_namespace').show()
            else if databases.length > 0 and @can_create_namespace is false
                @.$('.add-namespace').prop 'disabled', false
                @.$('.user_alert_space-cannot_create_namespace').hide()

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
            @add_database_dialog.render @databases

        add_namespace: (event) =>
            event.preventDefault()
            @add_namespace_dialog.render()
            $('#focus_namespace_name').focus()

        remove_namespace: (event) =>
            log_action 'remove namespace button clicked'
            # Make sure the button isn't disabled, and pass the list of namespace UUIDs selected
            if not $(event.currentTarget).is ':disabled'
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
            if @get_selected_namespaces().length < 1
                @.$('.btn.remove-namespace').attr 'disabled', true
            else
                @.$('.btn.remove-namespace').removeAttr 'disabled'
            #@.$('.btn.remove-namespace').is ':disabled', @get_selected_namespaces().length < 1

        destroy: =>
            super
            datacenters.off 'all', @update_button_create_namespace
            databases.off 'all', @update_button_create_namespace
            @add_database_dialog.destroy()
            @add_namespace_dialog.destroy()
            @remove_namespace_dialog.destroy()

    class @DatabaseListElement extends UIComponents.CollapsibleListElement
        template: Handlebars.templates['database_list_element-template']
        summary_template: Handlebars.templates['database_list_element-summary-template']

        className: 'element-container'

        events: ->
            _.extend super,
               'click button.remove-database': 'remove_database'

        initialize: ->
            super

            @delegateEvents()

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
        tagName: 'div'
        template: Handlebars.templates['namespace_list-template']

        initialize:  (database_id) =>
            log_initial '(initializing) namespace list view'

            super namespaces, NamespaceView.NamespaceListElement, '.list',
                {
                filter: (model) -> model.get('database') is database_id,
                sort: (a, b) ->
                    if b.model.get('name') < a.model.get('name')
                        return 1
                    else if b.model.get('name') > a.model.get('name')
                        return -1
                    return 0
                }
                , 'table', 'database'

        # Extend the AbstractList.add_element method to bind a callback to each namespace added to the list
        add_element: (element) =>
            namespace_list_element = super element
            @bind_callbacks_to_namespace namespace_list_element

        add_namespace: (event) =>
            event.preventDefault()
            @add_namespace_dialog.render()
            $('#focus_namespace_name').focus()

        remove_namespace: (event) =>
            log_action 'remove namespace button clicked'
            # Make sure the button isn't disabled, and pass the list of namespace UUIDs selected
            if not $(event.currentTarget).is ':disabled'
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

    # Namespace list element
    class @NamespaceListElement extends UIComponents.CheckboxListElement
        template: Handlebars.templates['namespace_list_element-template']
        tagName: 'div'

        hide_popover: ->
            $('.tooltip').remove()

        initialize: ->
            log_initial '(initializing) list view: namespace'
            super @template
            directory.on 'all', @render

        json_for_template: =>
            json = _.extend super(), DataUtils.get_namespace_status(@model.get('id'))
            return json

        render: =>
            super
            return @

        destroy: =>
            directory.off 'all', @render
            super

    class @AddDatabaseModal extends UIComponents.AbstractModal
        template: Handlebars.templates['add_database-modal-template']
        alert_tmpl: Handlebars.templates['added_database-alert-template']
        error_template: Handlebars.templates['error_input-template']

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
                $('.alert_modal').html @error_template
                    database_is_empty: true
            else if /^[a-zA-Z0-9_]+$/.test(@formdata.name) is false
                no_error = false
                $('.alert_modal').html @error_template
                    special_char_detected: true
                    type: 'database'
            else
                for database in databases.models
                    if database.get('name') is @formdata.name
                        no_error = false
                        $('.alert_modal').html @error_template
                            database_exists: true
                        break
            if no_error is true
                $.ajax
                    processData: false
                    url: 'ajax/semilattice/databases/new'
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify({"name" : @formdata.name})
                    success: @on_success
                    error: @on_error
            else
                $('.alert_modal_content').slideDown 'fast'
                @reset_buttons()

        on_success: (response) =>
            super
            apply_to_collection(databases, response)

            # Notify the user
            for id, namespace of response
                $('#user-alert-space').append @alert_tmpl
                    name: namespace.name
                    id: id

    # A modal for adding namespaces
    class @AddNamespaceModal extends UIComponents.AbstractModal
        template: Handlebars.templates['add_namespace-modal-template']
        alert_tmpl: Handlebars.templates['added_namespace-alert-template']
        need_database_alert_template: Handlebars.templates['need_database-alert-template']
        error_template: Handlebars.templates['error_input-template']
        class: 'add-namespace'

        initialize: =>
            log_initial '(initializing) modal dialog: add namespace'
            super

            databases.on 'add', @check_if_can_create_table
            databases.on 'remove', @check_if_can_create_table
            databases.on 'reset', @check_if_can_create_table
            @can_create_table_status = true
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

        # Check if we have a database (if not, we cannot create a table)
        check_if_can_create_table: =>
            if databases.length is 0
                if @can_create_table_status
                    @.$('.btn-primary').prop 'disabled', true
                    @.$('.alert_modal').html 'You need to create a database before creating a table.'
            else
                if @can_create_table_status is false
                    @.$('.alert_modal').empty()
                    @.$('.btn-primary').prop 'disabled', false


        render: ->
            log_render '(rendering) add namespace dialog'

            for datacenter in datacenters.models
                datacenter.set 'num_machines', 0

            for machine in machines.models
                if machine.get('datacenter_uuid') isnt universe_datacenter.get('id')
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
            ordered_datacenters.unshift
                id: universe_datacenter.get('id')
                name: universe_datacenter.get('name')

            ordered_databases = databases.map (d) ->
                id: d.get('id')
                name: d.get('name')

            ordered_databases = _.sortBy ordered_databases, (d) -> d.name

            super
                modal_title: 'Add table'
                btn_primary_text: 'Add'
                datacenters: ordered_datacenters
                all_datacenters: datacenters.length is ordered_datacenters.length
                databases: ordered_databases

            @check_if_can_create_table()
            @.$('.show_advanced_settings-link').click @show_advanced_settings
            @.$('.hide_advanced_settings-link').click @hide_advanced_settings

        on_submit: =>
            super

            formdata = form_data_as_object($('form', @$modal))
            # Check if data is safe
            template_error = {}
            input_error = false

            # Need a name
            if formdata.name is ''
                input_error = true
                template_error.namespace_is_empty = true
            else if /^[a-zA-Z0-9_]+$/.test(formdata.name) is false
                input_error = true
                template_error.special_char_detected = true
                template_error.type = 'table'
            else # And a name that doesn't exist
                for namespace in namespaces.models
                    if namespace.get('name') is formdata.name and namespace.get('database') is formdata.database
                        input_error = true
                        template_error.namespace_exists = true
                        break
            # Need a database
            if not formdata.database? or formdata.database is ''
                input_error = true
                template_error.no_database = true

            if input_error is true
                $('.alert_modal').html @error_template template_error
                $('.alert_modal_content').slideDown 'fast'
                @reset_buttons()
            else
                ack = {}
                ack[universe_datacenter.get('id')] =
                    expectation: 1
                    hard_durability: if formdata.write_disk is 'yes' then true else false

                $.ajax
                    processData: false
                    url: 'ajax/semilattice/rdb_namespaces/new'
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify(
                        name: formdata.name
                        primary_uuid: universe_datacenter.get('id')
                        database: formdata.database
                        ack_expectations: ack
                        primary_key: (formdata.primary_key if formdata.primary_key isnt '')
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


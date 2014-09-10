# Copyright 2010-2012 RethinkDB, all rights reserved.

module 'TablesView', ->
    class @DatabasesContainer extends Backbone.View
        id: 'databases_container'
        template:
            main: Handlebars.templates['databases_container-template']
            alert_message: Handlebars.templates['alert_message-template']

        events:
            'click .add_database': 'add_database'
            'click .add_table': 'add_table'
            'click .remove-tables': 'delete_tables'
            'click .checkbox-container': 'update_delete_tables_button'
            'click .close': 'remove_parent_alert'

        add_database: =>
            @add_database_dialog.render()

        add_table: =>
            @add_table_dialog.render()
            @add_table_dialog.focus()

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

        display_add_table_button: (display) =>
            if display?
                @display = display

            if display is false
                @$('.add_table').prop 'disabled', not @display
            else
                @$('.add_table').prop 'disabled', not @display

        update_delete_tables_button: =>
            if @$('.checkbox-table:checked').length > 0
                @$('.remove-tables').prop 'disabled', false
            else
                @$('.remove-tables').prop 'disabled', true

        initialize: =>
            @databases = new Databases
            @databases_list = new TablesView.DatabasesListView
                collection: @databases
                container: @

            @fetch_data()
            @interval = setInterval @fetch_data, 5000

            @loading = true # TODO Render that

            @add_database_dialog = new Modals.AddDatabaseModal @databases
            @add_table_dialog = new Modals.AddNamespaceModal @databases
            @remove_tables_dialog = new Modals.RemoveNamespaceModal

        render: =>
            @$el.html @template.main({})
            @$('.databases_list').html @databases_list.render().$el
            @

        render_message: (message) =>
            @.$('#user-alert-space').append @template.alert_message
                message: message


        fetch_data: =>
            query = r.db(system_db).table('db_config').filter( (db) ->
                db("name").ne('rethinkdb')
            ).map (db) ->
                name: db("name")
                id: db("uuid")
                tables: r.db(system_db).table('table_status').orderBy((table) -> table("name"))
                    .filter({db: db("name")}).merge( (table) ->
                        shards: table("shards").count()
                        replicas: table("shards").nth(0).count()
                    ).merge( (table) ->
                        id: table("uuid")
                    )

            driver.run query, (error, result) =>
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

        remove: =>
            clearInterval @interval
            @add_database_dialog.remove()
            @add_table_dialog.remove()
            @remove_tables_dialog.remove()
            super()


    class @DatabasesListView extends Backbone.View
        className: 'database_list'
        template:
            no_databases: Handlebars.templates['no_databases-template']

        initialize: (data) =>
            @container = data.container

            @databases_view = []
            @collection.each (database) =>
                view = new TablesView.DatabaseView
                    model: database

                @databases_view.push view
                @$el.append view.render().$el

            if @collection.length is 0
                @$el.html @template.no_databases()
                @container.display_add_table_button false

            @listenTo @collection, 'add', (database) =>
                view = new TablesView.DatabaseView
                    model: database

                @databases_view.push view

                # Keep the view sorted
                position = @collection.indexOf database
                if @collection.length is 1
                    @$el.html view.render().$el
                else if position is 0
                    @$el.prepend view.render().$el
                else
                    @$('.database_container').eq(position-1).after view.render().$el

                if @collection.length is 1
                    @$('.no-databases').remove()
                    @container.display_add_table_button true

            @listenTo @collection, 'remove', (database) =>
                for view in @databases_view
                    if view.model is database
                        database.destroy()
                        view.remove()
                        break

                if @collection.length is 0
                    @$el.html @template.no_databases()
                    @container.display_add_table_button false

        render: =>
            @container.display_add_table_button()
            @

        remove: =>
            @stopListening()
            for view in @databases_view
                view.remove()
            super()

    class @DatabaseView extends Backbone.View
        className: 'database_container section'
        template:
            main: Handlebars.templates['database-template']
            empty: Handlebars.templates['empty_list-template']

        events:
           'click button.remove-database': 'remove_database'

        remove_database: =>
            remove_database_dialog = new Modals.RemoveDatabaseModal
            remove_database_dialog.render @model
            remove_database_dialog.focus()


        initialize: =>
            @$el.html @template.main @model.toJSON()

            @tables_views = []
            @collection = new Tables()

            @update_collection()
            @model.on 'change', @update_collection

            @collection.each (table) =>
                view = new TablesView.TableView
                    model: table

                @tables_views.push view
                @$('.tables_container').append view.render().$el

            if @collection.length is 0
                @$('.tables_container').html @template.empty
                    element: "table"
                    container: "database"

            @listenTo @collection, 'add', (table) =>
                view = new TablesView.TableView
                    model: table

                @tables_views.push view

                position = @collection.indexOf table
                if @collection.length is 1
                    @$('.tables_container').html view.render().$el
                else if position is 0
                    @$('.tables_container').prepend view.render().$el
                else
                    @$('.table_container').eq(position-1).after view.render().$el

                if @collection.length is 1
                    @$('.no_element').remove()

            @listenTo @collection, 'remove', (table) =>
                for view in @tables_views
                    if view.model is table
                        table.destroy()
                        view.remove()
                        break

                if @collection.length is 0
                    @$('.tables_container').html @template.empty
                        element: "table"
                        container: "database"

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
                table.destroy()

            for table in @model.get('tables')
                @collection.add new Table table

        render: =>
            @

        remove: =>
            @stopListening()
            for view in @tables_views
                view.remove()
            super()

    class @TableView extends Backbone.View
        className: 'table_container'
        template: Handlebars.templates['table-template']

        render: =>
            @$el.html @template @model.toJSON()
            @

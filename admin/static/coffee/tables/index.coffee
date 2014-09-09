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

            @fetch_data()
            @interval = setInterval @fetch_data, 5000

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


        fetch_data: =>
            query = r.db(system_db).table('db_config').filter( (db) ->
                db("name").ne('rethinkdb')
            ).orderBy(r.row).map (db) ->
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
                    console.log JSON.stringify(result, null, 2)
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

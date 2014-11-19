# Copyright 2010-2012 RethinkDB, all rights reserved.

module 'TablesView', ->
    class @DatabasesContainer extends Backbone.View
        id: 'databases_container'
        template:
            main: Handlebars.templates['databases_container-template']
            loading: Handlebars.templates['loading-template']
            error: Handlebars.templates['error-query-template']
            alert_message: Handlebars.templates['alert_message-template']

        events:
            'click .add_database': 'add_database'
            'click .add_table': 'add_table'
            'click .remove-tables': 'delete_tables'
            'click .checkbox-container': 'update_delete_tables_button'
            'click .close': 'remove_parent_alert'

        query_callback: (error, result) =>
            if error?
                if @error?.msg isnt error.msg
                    @error = error
                    @$el.html @template.error
                        url: '#tables'
                        error: error.message
            else
                @databases.set _.map(_.sortBy(result, (db) -> db.name), (database) -> new Database(database)), {merge: true}
                if @loading is true
                    @loading = false
                    @render()



        add_database: =>
            @add_database_dialog.render()

        add_table: =>
            @add_table_dialog.render()

        delete_tables: (event) =>
            # Make sure the button isn't disabled, and pass the list of table ids selected
            if not $(event.currentTarget).is ':disabled'
                selected_tables = []
                @$('.checkbox-table:checked').each () ->
                    selected_tables.push
                        table: JSON.parse $(@).data('table')
                        database: JSON.parse $(@).data('database')

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

            @query = r.db(system_db).table('db_config').filter( (db) ->
                db("name").ne(system_db)
            ).map (db) ->
                name: db("name")
                id: db("id")
                tables: r.db(system_db).table('table_status').orderBy((table) -> table("name"))
                    .filter({db: db("name")})
                    .merge( (table) ->
                        shards: table("shards").count()
                        replicas: table("shards").map((shard) ->
                            shard('replicas').count()).sum()
                        replicas_ready: table('shards').map((shard) ->
                            shard('replicas').filter((replica) ->
                                replica('state').eq('ready')).count()).sum()
                        status:
                            r.branch(table('status')('all_replicas_ready'), 'all_replicas_ready',
                            r.branch(table('status')('ready_for_writes'), 'ready_for_writes',
                            r.branch(table('status')('ready_for_reads'), 'ready_for_reads',
                            r.branch(table('status')('ready_for_outdated_reads'), 'ready_for_outdated_reads',
                            'not_ready'  # otherwise
                            ))))
                        id: table('id')
                    )

            @fetch_data()

            @loading = true # TODO Render that

            @add_database_dialog = new Modals.AddDatabaseModal @databases
            @add_table_dialog = new Modals.AddTableModal
                databases: @databases
                container: @
            @remove_tables_dialog = new Modals.RemoveTableModal
                collection: @databases

        render: =>
            if @loading is true
                @$el.html @template.loading
                    page: "tables"
            else
                @$el.html @template.main({})
                @$('.databases_list').html @databases_list.render().$el
            @

        render_message: (message) =>
            @$('#user-alert-space').append @template.alert_message
                message: message

        fetch_data_once: =>
            @timer = driver.run_once @query, @query_callback

        fetch_data: =>
            @timer = driver.run @query, 5000, @query_callback

        remove: =>
            driver.stop_timer @timer
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
                new_view = new TablesView.DatabaseView
                    model: database

                if @databases_view.length is 0
                    @databases_view.push new_view
                    @$el.html new_view.render().$el
                else
                    added = false
                    for view, position in @databases_view
                        if view.model.get('name') > database.get('name')
                            added = true
                            @databases_view.splice position, 0, new_view
                            if position is 0
                                @$el.prepend new_view.render().$el
                            else
                                @$('.database_container').eq(position-1).after new_view.render().$el
                            break
                    if added is false
                        @databases_view.push new_view
                        @$el.append new_view.render().$el

                if @databases_view.length is 1
                    @$('.no-databases').remove()
                    @container.display_add_table_button true

            @listenTo @collection, 'remove', (database) =>
                for view, position in @databases_view
                    if view.model is database
                        database.destroy()
                        view.remove()
                        @databases_view.splice position, 1
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
            if @remove_database_dialog?
                @remove_database_dialog.remove()
            @remove_database_dialog = new Modals.RemoveDatabaseModal
            @remove_database_dialog.render @model


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
                new_view = new TablesView.TableView
                    model: table

                if @tables_views.length is 0
                    @tables_views.push new_view
                    @$('.tables_container').html new_view.render().$el
                else
                    added = false
                    for view, position in @tables_views
                        if view.model.get('name') > table.get('name')
                            added = true
                            @tables_views.splice position, 0, new_view
                            if position is 0
                                @$('.tables_container').prepend new_view.render().$el
                            else
                                @$('.table_container').eq(position-1).after new_view.render().$el
                            break
                    if added is false
                        @tables_views.push new_view
                        @$('.tables_container').append new_view.render().$el

                if @tables_views.length is 1
                    @$('.no_element').remove()


            @listenTo @collection, 'remove', (table) =>
                for view, position in @tables_views
                    if view.model is table
                        table.destroy()
                        view.remove()
                        @tables_views.splice position, 1
                        break

                if @collection.length is 0
                    @$('.tables_container').html @template.empty
                        element: "table"
                        container: "database"

        update_collection: =>
            @collection.set _.map(@model.get("tables"), (table) -> new Table(table)), {merge: true}

        render: =>
            @

        remove: =>
            @stopListening()
            for view in @tables_views
                view.remove()
            if @remove_database_dialog?
                @remove_database_dialog.remove()
            super()

    class @TableView extends Backbone.View
        className: 'table_container'
        template: Handlebars.templates['table-template']
        initialize: =>
            @listenTo @model, 'change', @render

        render: =>
            @$el.html @template
                id: @model.get 'id'
                db_json: JSON.stringify @model.get('db')
                name_json: JSON.stringify @model.get('name')
                db: @model.get('db')
                name: @model.get('name')
                shards: @model.get 'shards'
                replicas: @model.get 'replicas'
                replicas_ready: @model.get 'replicas_ready'
                status: @model.get 'status'
            @

        remove: =>
            @stopListening()
            super()

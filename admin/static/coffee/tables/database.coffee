# Copyright 2010-2012 RethinkDB, all rights reserved.
# Database view
module 'DatabaseView', ->
    class @DatabaseContainer extends Backbone.View
        template:
            not_found: Handlebars.templates['element_view-not_found-template']
            loading: Handlebars.templates['loading-template']
            error: Handlebars.templates['error-query-template']
        className: 'database-container'
        initialize: (id) =>
            @loading = true
            @id = id
            @model = null

            @fetch_data()

        fetch_data: =>
            query = r.do(
                r.db(system_db).table('db_config').get(@id),
                r.db(system_db).table('table_status').coerceTo("ARRAY"),
                r.db(system_db).table('table_config').coerceTo("ARRAY"),
                (database, table_status, table_config) ->
                    database.merge
                        num_tables: table_status.filter({db: database("name")}).count()
                        num_available_tables: table_status.filter({db: database("name"), ready_completely: true}).count()
                        num_shards: table_config.map( (table) -> table("shards").count() ).sum()
                        num_replicas: table_config.concatMap( (table) ->
                            table("shards").concatMap (shard) ->
                                shard("replicas")
                        ).count()
                        num_servers: table_config.concatMap( (table) ->
                            table("shards").concatMap (shard) ->
                                shard("replicas")
                        ).distinct().count()
                        tables: table_status.map (table) ->
                            db: table("db")
                            name: table("name")
                            num_shards: table("shards").count()
                            num_replicas: table("shards").nth(0).filter( (shard) ->
                                shard("role").ne("nothing")
                            ).count()
                            id: table("uuid")
                            ready_completely: table("ready_completely")
            ).merge
                id: r.row 'uuid'

            @timer = driver.run query, 5000, (error, result) =>
                if error?
                    # TODO: We may want to render only if we failed to open a connection
                    @error = error
                    @render()
                else
                    @error = null
                    if result is null
                        if @loading is true
                            @loading = false
                            @render()
                        else if @model isnt null
                            @model = null
                            @indexes = null
                            @table_view = null
                            @render()
                    else
                        @loading = false
                        if @model?
                            @tables.set _.map result.tables, (table) ->
                                new Table table
                            delete result.tables

                            @model.set result
                        else
                            @tables = new Tables _.map result.tables, (table) ->
                                new Table table
                            delete result.tables

                            @model = new Database result
                            @database_view = new DatabaseView.DatabaseMainView
                                model: @model
                                tables: @tables
                            @$el.html @database_view.render().$el
                            @render()
        render: =>
            if @error?
                @$el.html @template.error
                    error: @error?.message
                    url: '#databases/'+@id
            else if @loading is true
                @$el.html @template.loading
                    page: "database"
            else
                if @database_view?
                    @$el.html @database_view.render().$el
                else # In this case, the query returned null, so the table was not found
                    @$el.html @template.not_found
                        id: @id
                        type: 'database'
                        type_url: 'databases'
                        type_all_url: 'databases'
            @


        remove: =>
            driver.stop_timer @timer
            if @database_view?
                @database_view.remove()
            super()

    # Container for the entire database view
    class @DatabaseMainView extends Backbone.View
        className: 'database-view'
        template: Handlebars.templates['database_view-container-template']
        alert_tmpl: Handlebars.templates['modify_shards-alert-template']

        events: ->
            'click .close': 'close_alert'
            # operations in the dropdown menu
            'click .operations .rename': 'rename_database'
            'click .operations .delete': 'delete_database'

        initialize: (data) =>
            @tables = data.tables

            # Panels for database view
            @title = new DatabaseView.Title
                model: @model
            @profile = new DatabaseView.Profile
                model: @model
            @tables_list = new DatabaseView.TablesListView
                model: @model
                collection: @tables

            @stats = new Stats(r.expr(
                keys_read: r.random(2000, 3000)
                keys_set: r.random(1500, 2500)
            ))
            @performance_graph = new Vis.OpsPlot(@stats.get_stats,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'database'
            )

        # Function to render the view
        render: =>
            @$el.html @template()

            # fill the title of this page
            @$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @$('.profile').html @profile.render().$el

            @$('.performance-graph').html @performance_graph.render().$el

            # Add the tables list
            @$('.table-list').html @tables_list.render().$el

            @

        # Method to close an alert/warning/arror
        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        # Create a modal to renane the database
        rename_database: (event) =>
            event.preventDefault()
            if @rename_modal?
                @rename_modal.remove()
            @rename_modal = new UIComponents.RenameItemModal
                model: @model
            @rename_modal.render()

        # Create a modal to delete the databse
        delete_database: (event) ->
            event.preventDefault()

            if @remove_database_dialog?
                @remove_database_dialog.remove()
            @remove_database_dialog = new Modals.RemoveDatabaseModal
            @remove_database_dialog.render @model

        remove: =>
            @title.remove()
            @profile.remove()
            #@performance_graph.remove()
            if @rename_modal?
                @rename_modal.remove()
            if @remove_database_dialog?
                @remove_database_dialog.remove()
            @stats.destroy()

    # DatabaseView.Title
    class @Title extends Backbone.View
        className: 'database-info-view'
        template: Handlebars.templates['database_view_title-template']

        initialize: =>
            @listenTo @model, 'change:name', @render

        # Render the view for the title
        render: =>
            @$el.html @template
                name: @model.get 'name'
            @

        remove: =>
            @stopListening()

    # Profile view
    class @Profile extends Backbone.View
        className: 'database-profile'
        template: Handlebars.templates['database_view-profile-template']
        
        initialize: =>
            @listenTo @model, 'change:num_tables', @render
            @listenTo @model, 'change:num_available_tables', @render
            @listenTo @model, 'change:num_shards', @render
            @listenTo @model, 'change:num_replicas', @render

        render: =>
            @$el.html @template
                num_tables: @model.get 'num_tables'
                num_available_tables: @model.get 'num_available_tables'
                reachability: @model.get 'completely_ready'      # Status of the table
                num_shards: @model.get 'num_shards'
                num_replicas: @model.get 'num_replicas'
                num_servers: @model.get 'num_servers'
            @

        remove: =>
            @stopListening()

    # The modal to remove a database, it extends UIComponents.AbstractModal

    # List of all the namespaces in the database
    class @TablesListView extends Backbone.View
        template: Handlebars.templates['database_view-namespace_list-template']

        # Bind some listeners
        initialize: =>
            @tables_views = []
            @$el.html @template()

            @collection.each (table) =>
                view = new DatabaseView.TableView
                    model: table
                # The first time, the collection is sorted
                @tables_views.push view
                @$('.tables').append view.render().$el

            if @collection.length is 0
                @$('.no_element').show()
            else
                @$('.no_element').hide()

            @collection.on 'add', (table) =>
                view = new DatabaseView.TableView
                    model: table
                @tables_views.push view

                position = @collection.indexOf table
                if @collection.length is 1
                    @$('.tables').html view.render().$el
                else if position is 0
                    @$('.tables').prepend view.render().$el
                else
                    @$('.table_container').eq(position-1).after view.render().$el

                @$('.no_element').hide()

            @collection.on 'remove', (table) =>
                for view in @tables_views
                    if view.model is table
                        table.destroy()
                        ((view) ->
                            view.$el.slideUp 'fast', =>
                                view.remove()
                        )(view)
                        break
                if @collection.length is 0
                    @$('.no_element').show()


        render: =>
            @

        remove: =>
            @stopListening()

    class @TableView extends Backbone.View
        template: Handlebars.templates['database-table_view-template']
        className: "table_container"

        initialize: =>
            @listenTo @model, 'change', @render

        render: =>
            @$el.html @template @model.toJSON()
            @

        remove: =>
            @stopListening()


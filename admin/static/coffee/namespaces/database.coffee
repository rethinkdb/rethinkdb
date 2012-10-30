# Copyright 2010-2012 RethinkDB, all rights reserved.
# Database view
module 'DatabaseView', ->
    # Class in case the database is not found (propably a deleted database). Links to this view can be found in old logs.
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#element_view-not_found-template').html()
        initialize: (id) =>
            @id = id

        render: =>
            @.$el.html @template
                id: @id
                type: 'database'
                type_url: 'databases' # Use to build a url to a specific database
                type_all_url: 'tables' # Use to build a url to the list of all databases
            return @

    # Container for the entire database view
    class @Container extends Backbone.View
        className: 'database-view'
        template: Handlebars.compile $('#database_view-container-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()

        events: ->
            'click .close': 'close_alert'
            # operations in the dropdown menu
            'click .operations .rename': 'rename_database'
            'click .operations .delete': 'delete_database'

        initialize: ->
            log_initial '(initializing) database view: container'

            # Panels for database view
            @title = new DatabaseView.Title model: @model
            @profile = new DatabaseView.Profile model: @model
            @namespace_list = new DatabaseView.NamespaceList model: @model
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'database'
            )

            databases.on 'remove', @check_if_still_exists

        # If we were on this view and the database has been deleted somewhere else, we redirect the user to the list of databases
        check_if_still_exists: =>
            exist = false
            for database in databases.models
                if database.get('id') is @model.get('id')
                    exist = true
                    break
            if exist is false
                window.router.navigate '#tables'
                window.app.index_namespaces
                    alert_message: "The database <a href=\"#databases/#{@model.get('id')}\">#{@model.get('name')}</a> could not be found and was probably deleted."

        # Function to render the view
        render: =>
            log_render '(rendering) database view: container'

            @.$el.html @template
                database_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # Add the namespace list
            @.$('.table-list').html @namespace_list.render().$el

            return @

        # Method to close an alert/warning/arror
        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        # Create a modal to renane the database
        rename_database: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'database'
            rename_modal.render()

        # Create a modal to delete the databse
        delete_database: (event) ->
            event.preventDefault()
            remove_database_dialog = new DatabaseView.RemoveDatabaseModal
            remove_database_dialog.render @model

        # Remove listeners
        destroy: =>
            databases.off 'remove', @check_if_still_exists
            @title.destroy()
            @profile.destroy()
            @performance_graph.destroy()

    # DatabaseView.Title
    class @Title extends Backbone.View
        className: 'database-info-view'
        template: Handlebars.compile $('#database_view_title-template').html()

        # Bind listeners
        initialize: =>
            @name = @model.get('name')
            @model.on 'change:name', @update

        # Update the title if the database has a new name
        update: =>
            if @name isnt @model.get('name')
                @name = @model.get('name')
                @render()

        # Render the view for the title
        render: =>
            @.$el.html @template
                name: @name
            return @

        # Unbind listeners
        destroy: =>
            @model.off 'change:name', @update

    # Profile view
    class @Profile extends Backbone.View
        className: 'database-profile'
        template: Handlebars.compile $('#database_view-profile-template').html()
        
        # Bind listeners and initialize the data of this view
        initialize: =>
            @data = {} # This will be used to know whether we need to rerender our view or not
            namespaces.on 'all', @render # We listen to all namespaces since they can be moved while the user is on this view

        # Render the view
        render: =>
            # Initialize the data for this view
            data =
                num_namespaces: 0       # Number of namespaces
                num_live_namespaces: 0  # Number of namespaces alived
                reachability: true      # Status of the table
                nshards: 0              # Number of shards
                nreplicas: 0            # Number of replicas (numbers of copies of each shards)
                ndatacenters: 0         # Number of datacenter
                #stats_up_to_date: true # Not display for now. We should put it back. Issue #1286
                
            datacenters_working = {}
            # Compute the data for this view
            for namespace in namespaces.models
                if namespace.get('database') is @model.get('id')
                    data.num_namespaces++
                    namespace_status = DataUtils.get_namespace_status(namespace.get('id'))
                    if namespace_status? and namespace_status.reachability is 'Live'
                        data.num_live_namespaces++
                    data.nshards += namespace_status.nshards
                    data.nreplicas += data.nshards + namespace_status.nreplicas

                    for datacenter_id of namespace.get('replica_affinities')
                        # We don't count universe, and we add the datacenter only if it has at least one replica
                        if datacenter_id isnt universe_datacenter.get('id') and (namespace.get('replica_affinities')[datacenter_id] > 0 or namespace.get('primary_uuid') is datacenter_id)
                            datacenters_working[datacenter_id] = true

            for datacenters_id of datacenters_working
                data.ndatacenters++

            # We update only if the data has changed
            if not _.isEqual @data, data
                @data = data
                @.$el.html @template data

            return @

        # Unbind listeners
        destroy: =>
            namespaces.off 'all', @render

    # The modal to remove a database, it extends UIComponents.AbstractModal
    class @RemoveDatabaseModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#remove_database-modal-template').html()
        class: 'remove_database-dialog'

        initialize: ->
            super

        render: (_database_to_delete) ->
            @database_to_delete = _database_to_delete

            # Call render of AbstractModal with the data for the template
            super
                modal_title: 'Delete database'
                btn_primary_text: 'Delete'
                id: _database_to_delete.get('id')
                name: _database_to_delete.get('name')

            # Giving focus to the "submit" button
            # Commenting the next line for now because the operation is dangerous.
            #@.$('.btn-primary').focus()

        on_submit: =>
            super

            # We remove the database
            post_data = {}
            post_data.databases = {}
            post_data.databases[@database_to_delete.get('id')] = null

            # We also remove all the namespaces in the database
            for namespace in namespaces.models
                if namespace.get('database') is @database_to_delete.get('id')
                    if not post_data[namespace.get('protocol')+'_namespaces']?
                        post_data[namespace.get('protocol')+'_namespaces'] = {}
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')] = null

            $.ajax
                processData: false
                url: '/ajax/semilattice'
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(post_data)
                success: @on_success
                error: @on_error

        on_success: (response) =>
            # If the user was on a database view, we have to redirect him
            # If he was on #tables, we are just refreshing
            window.router.navigate '#tables'
            window.app.index_namespaces
                alert_message: "The database #{@database_to_delete.get('name')} was successfully deleted."

            # Update the data
            namespace_id_to_remove = []
            for namespace in namespaces.models
                if namespace.get('database') is @database_to_delete.get('id')
                    namespace_id_to_remove.push namespace.get 'id'

            for id in namespace_id_to_remove
                namespaces.remove id

            databases.remove @database_to_delete.get 'id'


    # List of all the namespaces in the database
    class @NamespaceList extends Backbone.View
        template: Handlebars.compile $('#database_view-namespace_list-template').html()

        # Bind some listeners
        initialize: =>
            # We bind listeners on all table.
            # TODO We should add a listeners only on namespaces in the databases
            namespaces.on 'change:shards', @render
            namespaces.on 'change:primary_pinnings', @render
            namespaces.on 'change:secondary_pinnings', @render
            namespaces.on 'change:replica_affinities', @render
            @data = {}

        render: =>
            namespaces_in_db = []
            # Collect info on namespaces in this database
            namespaces.each (namespace) =>
                if namespace.get('database') is @model.get('id')
                    ns = _.extend DataUtils.get_namespace_status(namespace.get('id')),
                        name: namespace.get 'name'
                        id: namespace.get 'id'
                    ns.nreplicas += ns.nshards
                    namespaces_in_db.push ns

            data =
                has_tables: namespaces_in_db.length > 0
                tables: _.sortBy(namespaces_in_db, (namespace) -> namespace.name)
            
            # Render only if something has changed. @render is executed everytime someone change the shards on a table (even if not in the database)
            if not @data isnt data
                @data = data
                @.$el.html @template @data
            return @

        # Unbind listeners
        destroy: =>
            namespaces.off 'change:shards', @render
            namespaces.off 'change:primary_pinnings', @render
            namespaces.off 'change:secondary_pinnings', @render
            namespaces.off 'change:replica_affinities', @render

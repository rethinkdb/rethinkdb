# Database view
module 'DatabaseView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#element_view-not_found-template').html()
        initialize: (id) ->
            @id = id
        render: =>
            @.$el.html @template
                id: @id
                type: 'database'
                type_url: 'databases'
                type_all_url: 'tables'
            return @

    # Container for the entire database view
    class @Container extends Backbone.View
        #TODO Add a class to keep alert when adding multiple databases
        className: 'database-view'
        template: Handlebars.compile $('#database_view-container-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()

        events: ->
            'click .tab-link': 'change_route'
            'click .close': 'close_alert'

        initialize: ->
            log_initial '(initializing) database view: container'

            # Panels for database view
            @title = new DatabaseView.Title(model: @model)
            @profile = new DatabaseView.Profile(model: @model)
            @overview = new DatabaseView.Overview(model: @model)
            @operations = new DatabaseView.Operations(model: @model)
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance)

            databases.on 'remove', @check_if_still_exists
        
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

        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later.
            window.router.navigate @.$(event.target).attr('href')
 
        render: (tab) =>
            log_render '(rendering) database view: container'

            @.$el.html @template
                database_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display the data on the machines
            @.$('.database-overview-container').html @overview.render().$el

            # Display operations
            @.$('.operations').html @operations.render().el

            @.$('.nav-tabs').tab()
            
            if tab?
                @.$('.active').removeClass('active')
                switch tab
                    when 'overview'
                        @.$('#database-overview').addClass('active')
                        @.$('#database-overview-link').tab('show')
                    when 'operations'
                        @.$('#database-operations').addClass('active')
                        @.$('#database-operations-link').tab('show')
                    else
                        @.$('#database-overview').addClass('active')
                        @.$('#database-overview-link').tab('show')
            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        destroy: =>
            @title.destroy()
            @profile.destroy()
            @overview.destroy()
            @operations.destroy()
            @performance_graph.destroy()

    # DatabaseView.Title
    class @Title extends Backbone.View
        className: 'database-info-view'
        template: Handlebars.compile $('#database_view_title-template').html()
        initialize: ->
            @name = @model.get('name')
            @model.on 'change:name', @update
        
        update: =>
            if @name isnt @model.get('name')
                @name = @model.get('name')
                @render()

        render: =>
            @.$el.html @template
                name: @name
            return @

        destroy: =>
            @model.off 'change:name', @update

    # Profile view
    class @Profile extends Backbone.View
        className: 'database-profile'
        template: Handlebars.compile $('#database_view-profile-template').html()

        initialize: ->
            @data = {}
            namespaces.on 'all', @render

        render: =>
            data =
                num_namespaces: 0
                num_rdb: 0
                num_memcached: 0
                num_shards: 0
                reachability: true
                stats_up_to_date: true

            for namespace in namespaces.models
                if namespace.get('database') is @model.get('id')
                    data.num_namespaces++

                    if namespace.get('protocol') is 'rdb'
                        data.num_rdb++
                    else if namespace.get('protocol') is 'memcached'
                        data.num_memcached++

                    namespace_status = DataUtils.get_namespace_status(namespace.get('id'))
                    if namespace_status.reachability isnt 'Live'
                        data.reachability = false
                        console.log namespace.get('id')
                        console.log namespace_status



            data.num_namespaces = @model.get_namespaces().length

            @.$el.html @template data

            return @
        
        destroy: =>
            namespaces.off 'all', @render

    class @Overview extends Backbone.View
        className: 'database-overview'

        template: Handlebars.compile $('#database_overview-container-template').html()

        initialize: =>
            @namespaces_list = []
            @namespaces_view_list = []
            namespaces.on 'add', @update_data
            namespaces.on 'remove', @update_data
            namespaces.on 'reset', @update_data
            @update_data()

        update_data: =>
            need_update = false
            new_namespaces_list = []
            for namespace in namespaces.models
                if namespace.get('database') is @model.get('id')
                    new_namespaces_list.push namespace
                    
                    # Check if first time we see it
                    found_namespace = false
                    for namespace_in_db in @namespaces_list
                        if namespace_in_db.get('id') is namespace.get('id')
                            found_namespace = true
                            break
                    
                    if found_namespace is false
                        @namespaces_list.push namespace
                        need_update = true
                    
            if need_update is false
                if new_namespaces_list isnt @namespaces_list
                    @namespaces_list = new_namespaces_list
                    @render()
            else
                @namespaces_list = new_namespaces_list
                @render()


        render: =>
            @.$el.html @template {}

            for view in @namespaces_view_list
                view.destroy()
            @namespaces_view_list = []
            for namespace in @namespaces_list
                view = new DatabaseView.NamespaceView(model: namespace)
                @namespaces_view_list.push view
                @.$('.namespaces-list').append view.render().$el
            

            return @

        destroy: =>
            for view in @namespaces_view_list
                view.destroy()
            namespaces.off 'add', @update_data
            namespaces.off 'remove', @update_data
            namespaces.off 'reset', @update_data


    class @NamespaceView extends Backbone.View
        className: 'element-detail-container'
        template: Handlebars.compile $('#database-namespace_view-template').html()

        initialize: =>
            @model.on 'change:shards', @render
            @model.on 'change:primary_pinnings', @render
            @model.on 'change:secondary_pinnings', @render
            @model.on 'change:replica_affinities', @render

        render: =>
            json = _.extend DataUtils.get_namespace_status(@model.get('id')),
                name: @model.get 'name'
                id: @model.get 'id'
            json.nreplicas += json.nshards

            @.$el.html @template json
            return @

        destroy: =>
            @model.off 'change:shards', @render
            @model.off 'change:primary_pinnings', @render
            @model.off 'change:secondary_pinnings', @render
            @model.off 'change:replica_affinities', @render

    class @Operations extends Backbone.View
        className: 'database-operations'

        template: Handlebars.compile $('#database_operations-template').html()
        events: ->
            'click .rename_database-button': 'rename_database'
            'click .import_data-button': 'import_data'
            'click .export_data-button': 'export_data'
            'click .delete_database-button': 'delete_database'

        initialize: =>
            @model.on 'change:name', @render

        rename_database: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'database'
            rename_modal.render()

        import_data: (event) ->
            event.preventDefault()
            #TODO Implement
        
        export_data: (event) ->
            event.preventDefault()
            #TODO Implement

        delete_database: (event) ->
            event.preventDefault()
            remove_database_dialog = new DatabaseView.RemoveDatabaseModal
            remove_database_dialog.render @model

            model = @model


        render: =>
            @.$el.html @template {}
            return @

        destroy: =>
            @model.off 'change:name', @render


    class @RemoveDatabaseModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#remove_database-modal-template').html()
        class: 'remove_database-dialog'

        initialize: ->
            super

        render: (_database_to_delete) ->
            @database_to_delete = _database_to_delete

            super
                modal_title: 'Remove database'
                btn_primary_text: 'Remove'
                id: _database_to_delete.get('id')
                name: _database_to_delete.get('name')

            @.$('.btn-primary').focus()

        on_submit: =>
            super

            post_data = {}
            post_data.databases = {}
            post_data.databases[@database_to_delete.get('id')] = null
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
            window.router.navigate '#tables'
            window.app.index_namespaces
                alert_message: "The database #{@database_to_delete.get('name')} was successfully deleted."

            namespace_id_to_remove = []
            for namespace in namespaces.models
                if namespace.get('database') is @database_to_delete.get('id')
                    namespace_id_to_remove.push namespace.get 'id'

            for id in namespace_id_to_remove
                namespaces.remove id
            
            databases.remove @database_to_delete.get 'id'

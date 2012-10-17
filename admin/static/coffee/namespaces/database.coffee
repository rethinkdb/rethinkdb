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
        className: 'database-view'
        template: Handlebars.compile $('#database_view-container-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()

        events: ->
            'click .tab-link': 'change_route'
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

            # Add the namespace list
            @.$('.table-list').html @namespace_list.render().$el

            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        rename_database: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'database'
            rename_modal.render()

        delete_database: (event) ->
            event.preventDefault()
            remove_database_dialog = new DatabaseView.RemoveDatabaseModal
            remove_database_dialog.render @model

        destroy: =>
            @title.destroy()
            @profile.destroy()
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
                num_live_namespaces: 0
                reachability: true
                stats_up_to_date: true
                nshards: 0
                nreplicas: 0
                ndatacenters: 0

            for namespace in namespaces.models
                if namespace.get('database') is @model.get('id')
                    data.num_namespaces++
                    namespace_status = DataUtils.get_namespace_status(namespace.get('id'))
                    if namespace_status? and namespace_status.reachability is 'Live'
                        data.num_live_namespaces++
                    data.nshards += namespace_status.nshards
                    data.nreplicas += data.nshards + namespace_status.nreplicas
                    data.ndatacenters += namespace_status.ndatacenters

            if not _.isEqual @data, data
                @data = data
                @.$el.html @template data

            return @

        destroy: =>
            namespaces.off 'all', @render

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

    class @NamespaceList extends Backbone.View
        template: Handlebars.compile $('#database_view-namespace_list-template').html()

        initialize: =>
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

            if not @data isnt data
                @data = data
                @.$el.html @template @data
            return @

        destroy: =>
            namespaces.off 'change:shards', @render
            namespaces.off 'change:primary_pinnings', @render
            namespaces.off 'change:secondary_pinnings', @render
            namespaces.off 'change:replica_affinities', @render

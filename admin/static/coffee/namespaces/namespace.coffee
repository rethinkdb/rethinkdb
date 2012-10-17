# Namespace view
module 'NamespaceView', ->
    class @NotFound extends Backbone.View
        className: 'section'
        template: Handlebars.compile $('#element_view-not_found-template').html()
        initialize: (id) ->
            @id = id
        render: =>
            @.$el.html @template
                id: @id
                type: 'table'
                type_url: 'tables'
                type_all_url: 'tables'
            return @

    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'
        template: Handlebars.compile $('#namespace_view-container-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()

        events: ->
            'click .tab-link': 'change_route'
            'click .close': 'close_alert'
            'click .change_shards-link': 'change_shards'
            # operations in the dropdown menu
            'click .operations .rename': 'rename_namespace'
            'click .operations .delete': 'delete_namespace'

        initialize: ->
            log_initial '(initializing) namespace view: container'

            @model.load_key_distr()

            # Panels for namespace view
            @title = new NamespaceView.Title(model: @model)
            @profile = new NamespaceView.Profile(model: @model)
            @replicas = new NamespaceView.Replicas(model: @model)
            @shards = new NamespaceView.Sharding(model: @model)
            @server_assignments = new NamespaceView.ServerAssignments(model: @model)
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'table'
            )

            namespaces.on 'remove', @check_if_still_exists

        check_if_still_exists: =>
            exist = false
            for namespace in namespaces.models
                if namespace.get('id') is @model.get('id')
                    exist = true
                    break
            if exist is false
                window.router.navigate '#tables'
                window.app.index_namespaces
                    alert_message: "The table <a href=\"#tables/#{@model.get('id')}\">#{@model.get('name')}</a> could not be found and was probably deleted."
        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later. TODO
            window.router.navigate @.$(event.target).attr('href')

        render: =>
            log_render '(rendering) namespace view: container'

            @.$el.html @template
                namespace_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # Display the replicas
            @.$('.replication').html @replicas.render().el

            # Display the shards
            @.$('.sharding').html @shards.render().el

            # Display the server assignments
            @.$('.server-assignments').html @server_assignments.render().el

            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        change_shards: (event) =>
            event.preventDefault()
            @.$('#namespace-sharding-link').tab('show')

        change_pinning: (event) =>
            event.preventDefault()
            @.$('#namespace-pinning-link').tab('show')
            $(event.currentTarget).parent().parent().slideUp('fast', -> $(this).remove())

        # Rename operation
        rename_namespace: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()

        # Delete operation
        delete_namespace: (event) ->
            event.preventDefault()
            remove_namespace_dialog = new NamespaceView.RemoveNamespaceModal
            namespace_to_delete = @model

            remove_namespace_dialog.on_success = (response) =>
                window.router.navigate '#tables'
                window.app.index_namespaces
                    alert_message: "The table #{@model.get('name')} was successfully deleted."
                namespaces.remove @model.get 'id'

            remove_namespace_dialog.render [@model]

        destroy: =>
            @title.destroy()
            @profile.destroy()
            @replicas.destroy()
            @shards.destroy()
            @server_assignments.destroy()
            @performance_graph.destroy()

    # NamespaceView.Title
    class @Title extends Backbone.View
        className: 'namespace-info-view'
        template: Handlebars.compile $('#namespace_view_title-template').html()
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
        className: 'namespace-profile'
        template: Handlebars.compile $('#namespace_view-profile-template').html()

        initialize: ->
            # @model is a namespace.  somebody is supposed to pass model: namespace to the constructor.
            @model.on 'all', @render
            directory.on 'all', @render
            progress_list.on 'all', @render
            log_initial '(initializing) namespace view: replica'
            @data = {}

        render: =>
            data = DataUtils.get_namespace_status(@model.get('id'))

            #Compute the total number of keys
            data.total_keys_available = false
            if @model.get('key_distr')?
                # we can't use 'total_keys' because we need to
                # distinguish between zero and unavailable.
                data.total_keys_available = true
                data.total_keys = 0
                for key of @model.get('key_distr')
                    data.total_keys += parseInt @model.get('key_distr')[key]

            data.stats_up_to_date = true
            for machine in machines.models
                if machine.get('stats')? and @model.get('id') of machine.get('stats') and machine.is_reachable
                    if machine.get('stats_up_to_date') is false
                        data.stats_up_to_date = false
                        break

            if not _.isEqual @data, data
                @data = data
                @.$el.html @template data

            return @

        destroy: =>
            @model.off 'all', @render
            directory.off 'all', @render
            progress_list.off 'all', @render

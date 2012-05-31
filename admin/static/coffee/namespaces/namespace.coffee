
# Namespace view
module 'NamespaceView', ->
    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'
        template: Handlebars.compile $('#namespace_view-container-template').html()
        events: ->
            'click a.rename-namespace': 'rename_namespace'
            'click .close': 'close_alert'

        initialize: (id) =>
            log_initial '(initializing) namespace view: container'
            @namespace_uuid = id

        rename_namespace: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()
            @title.update()
            
        wait_for_model_noop: =>
            return true

        wait_for_model: =>
            @model = namespaces.get(@namespace_uuid)
            if not @model
                namespaces.off 'all', @render
                namespaces.on 'all', @render
                return false

            # Model is finally ready, unbind necessary handlers
            namespaces.off 'all', @render




            # We no longer need all this logic in wait_for_model, so
            # switch it to noop for the callers
            @wait_for_model = @wait_for_model_noop

            return true

        render_empty: =>
            @.$el.text 'Namespace ' + @namespace_uuid + ' is not available.'
            return @

        render: =>
            log_render '(rendering) namespace view: container'

            if @wait_for_model() is false
                return @render_empty()

            json = @model.toJSON()
            @.$el.html @template json

            # Some additional setup
            @title = new NamespaceView.Title(@namespace_uuid)
            @profile = new NamespaceView.Profile(model: @model)
            @replicas = new NamespaceView.Replicas(model: @model)
            @shards = new NamespaceView.Shards(model: @model)
            
            stats = @model.get_stats_for_performance
            @performance_graph = new Vis.OpsPlot(stats)
            @stats_panel = new Vis.StatsPanel(stats)

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display the data on the machines
            @.$('.datacenter-stats').html @stats_panel.render().$el

            # Display the replicas
            @.$('.section.replication').html @replicas.render().el

            # Display the shards
            @.$('.section.sharding').html @shards.render().el

            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())


    # NamespaceView.Title
    class @Title extends Backbone.View
        className: 'namespace-info-view'
        template: Handlebars.compile $('#namespace_view_title-template').html()
        initialize: (uuid) =>
            @uuid = uuid
            @name = namespaces.get(@uuid).get('name')
            namespaces.on 'all', @update
        
        update: =>
            if @name isnt namespaces.get(@uuid).get('name')
                @name = namespaces.get(@uuid).get('name')
                @render()

        render: =>
            json =
                name: @name
            @.$el.html @template(json)
            return @


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

        render: =>
            namespace_status = DataUtils.get_namespace_status(@model.get('id'))

            json = @model.toJSON()
            json = _.extend json, namespace_status
            json = _.extend json,
                old_alert_html: @.$('#user-alert-space').html()

            if namespace_status.reachability == 'Live'
                json.reachability = true
            else
                json.reachability = false

            @.$el.html @template json

            return @


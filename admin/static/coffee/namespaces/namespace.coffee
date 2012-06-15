# Namespace view
module 'NamespaceView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#namespace_view-not_found-template').html()
        initialize: (id) -> @id = id
        render: =>
            @.$el.html @template id: @id
            return @

    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'
        template: Handlebars.compile $('#namespace_view-container-template').html()
        events: ->
            'click a.rename-namespace': 'rename_namespace'
            'click .close': 'close_alert'

        initialize: ->
            log_initial '(initializing) namespace view: container'

            # Panels for namespace view
            @title = new NamespaceView.Title(model: @model)
            @profile = new NamespaceView.Profile(model: @model)
            @replicas = new NamespaceView.Replicas(model: @model)
            @shards = new NamespaceView.Sharding(model: @model)
            @stats_panel = new NamespaceView.StatsPanel(model: @model)
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance)

        rename_namespace: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()
            @title.update()

        render: =>
            log_render '(rendering) namespace view: container'

            json = @model.toJSON()
            @.$el.html @template json

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display the data on the machines
            @.$('.section.namespace-stats').html @stats_panel.render().el

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
        initialize: ->
            @name = @model.get('name')
            namespaces.on 'all', @update
        
        update: =>
            if @name isnt @model.get('name')
                @name = @model.get('name')
                @render()

        render: =>
            @.$el.html @template
                name: @name
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

    class @StatsPanel extends Backbone.View
        className: 'namespace-stats'

        template: Handlebars.compile $('#namespace_stats-template').html()

        history_opsec: []

        initialize: ->
            @model.on 'all', @render
            machines.on 'all', @render
            # Initialize history
            for i in [0..40]
                @history_opsec.push 0

        render: =>
            data_in_memory = 0
            data_total = 0
            #TODO These are being calculated incorrectly. As they stand, data_in_memory is correct, but data_total is measuring the disk space used by this namespace. Issue filed.
            for machine in machines.models
                if machine.get('stats')? and @model.get('id') of machine.get('stats')
                    stats = machine.get('stats')
                    if stats.cache?
                        data_in_memory += stats.cache.block_size * stats.cache.blocks_in_memory
                        data_total += stats.cache.block_size * stats.cache.blocks_total

            json =
                data_in_memory_percent: Math.floor(data_in_memory/data_total*100)
                data_in_memory: human_readable_units(data_in_memory, units_space)
                data_total: human_readable_units(data_total, units_space)

            @update_history_opsec()
            sparkline_attr =
                fillColor: false
                spotColor: false
                minSpotColor: false
                maxSpotColor: false
                chartRangeMin: 0
                width: '75px'
                height: '15px'

            @.$el.html @template json

            @.$('.opsec_sparkline').sparkline @history_opsec, sparkline_attr

            return @

        update_history_opsec: =>
            @history_opsec.shift()
            @history_opsec.push @model.get_stats().keys_read + @model.get_stats().keys_set



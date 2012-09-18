# Namespace view
module 'NamespaceView', ->
    class @NotFound extends Backbone.View
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
            'click .namespace-pinning-link': 'change_pinning'

        initialize: ->
            log_initial '(initializing) namespace view: container'

            @model.load_key_distr()

            # Panels for namespace view
            @title = new NamespaceView.Title(model: @model)
            @profile = new NamespaceView.Profile(model: @model)
            @replicas = new NamespaceView.Replicas(model: @model)
            @shards = new NamespaceView.Sharding(model: @model)
            @pins = new NamespaceView.Pinning(model: @model)
            @overview = new NamespaceView.Overview(model: @model)
            @other = new NamespaceView.Other(model: @model)

            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance)

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
            # Because we are using bootstrap tab. We should remove them later.
            window.router.navigate @.$(event.target).attr('href')
 
        render: (tab) =>
            log_render '(rendering) namespace view: container'

            @.$el.html @template
                namespace_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display the data on the machines
            @.$('.namespace-overview-container').html @overview.render().$el

            # Display the replicas
            @.$('.replication').html @replicas.render().el

            # Display the shards
            @.$('.sharding').html @shards.render().el

            # Display the pins
            @.$('.pinning').html @pins.render().el

            # Display other
            @.$('.other').html @other.render().el

            @.$('.nav-tabs').tab()
            
            if tab?
                @.$('.active').removeClass('active')
                switch tab
                    when 'overview'
                        @.$('#namespace-overview').addClass('active')
                        @.$('#namespace-overview-link').tab('show')
                    when 'replication'
                        @.$('#namespace-replication').addClass('active')
                        @.$('#namespace-replication-link').tab('show')
                    when 'shards'
                        @.$('#namespace-sharding').addClass('active')
                        @.$('#namespace-sharding-link').tab('show')
                    when 'assignments'
                        @.$('#namespace-pinning').addClass('active')
                        @.$('#namespace-pinning-link').tab('show')
                    when 'other'
                        @.$('#namespace-other').addClass('active')
                        @.$('#namespace-other-link').tab('show')
                    else
                        @.$('#namespace-overview').addClass('active')
                        @.$('#namespace-overview-link').tab('show')
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


        destroy: =>
            @model.clear_interval_key_distr()

            @title.destroy()
            @profile.destroy()
            @replicas.destroy()
            @shards.destroy()
            @pins.destroy()
            @overview.destroy()
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

            if namespace_status?.reachability? and namespace_status.reachability is 'Live'
                json.reachability = true
            else
                json.reachability = false

            #Compute the total number of keys
            json.total_keys = 0
            for key of @model.get('key_distr')
                json.total_keys += @model.get('key_distr')[key]

            json.stats_up_to_date = true
            for machine in machines.models
                if machine.get('stats')? and @model.get('id') of machine.get('stats') and machine.is_reachable
                    if machine.get('stats_up_to_date') is false
                        json.stats_up_to_date = false
                        break

            @.$el.html @template json

            return @
        
        destroy: =>
            @model.off 'all', @render
            directory.off 'all', @render
            progress_list.off 'all', @render

    class @Overview extends Backbone.View
        className: 'namespace-overview'

        container_template: Handlebars.compile $('#namespace_overview-container-template').html()
        data_in_memory_template: Handlebars.compile $('#namespace_overview-data_in_memory-template').html()
        data_repartition_template: Handlebars.compile $('#namespace_overview-data_repartition-template').html()

        initialize: =>
            machines.on 'change', @render_data_in_memory
            @json_in_memory =
                data_in_memory_percent: -1
                data_in_memory: -1
                data_total: -1

        render: =>
            @.$el.html @container_template {}
            return @

        render_data_in_memory: =>
            data_in_memory = 0
            data_total = 0
            data_is_ready = false
            for machine in machines.models
                if machine.get('stats')? and @model.get('id') of machine.get('stats') and machine.get('stats')[@model.get('id')].serializers?
                    for key of machine.get('stats')[@model.get('id')].serializers
                        if machine.get('stats')[@model.get('id')].serializers[key].cache?.blocks_in_memory?
                            data_in_memory += parseInt(machine.get('stats')[@model.get('id')].serializers[key].cache.blocks_in_memory) * parseInt(machine.get('stats')[@model.get('id')].serializers[key].cache.block_size)
                            data_total += parseInt(machine.get('stats')[@model.get('id')].serializers[key].cache.blocks_total) * parseInt(machine.get('stats')[@model.get('id')].serializers[key].cache.block_size)
                            data_is_ready = true
            
            if data_is_ready
                json =
                    data_in_memory_percent: Math.floor(data_in_memory/data_total*100).toString()+'%'
                    data_in_memory: human_readable_units(data_in_memory, units_space)
                    data_not_in_memory: if data_total-data_in_memory>=0 then human_readable_units(data_total-data_in_memory, units_space) else human_readable_units(0, units_space)
                    data_total: human_readable_units(data_total, units_space)
            else
                json =
                    data_in_memory_percent: 'loading...'
                    data_in_memory: 'loading...'
                    data_not_in_memory: 'loading...'
                    data_total: 'loading...'

            # So we don't update every seconds
            need_update = false
            for key of json
                if not @json_in_memory[key]? or @json_in_memory[key] != json[key]
                    need_update = true
                    break
            if need_update
                @json_in_memory = json
                @.$('.data_in_memory-container').html @data_in_memory_template @json_in_memory

                # Draw pie chart
                if data_in_memory isnt 0 and data_total isnt 0
                    r = 100
                    width = 270
                    height = 270
                    color = (i) ->
                        if i is 0
                            return '#1f77b4'
                        else
                            return '#f00'

                    data_pie = [data_total-data_in_memory, data_in_memory]

                    # Remove transition for the time being. We have to use transition with opacity only the first time
                    # For update, we should just move/extend pieces, too much work for now
                    #@.$('.loading_text-svg').fadeOut '600', -> $(@).remove() 
                    @.$('.loading_text-pie_chart').css 'display', 'none'

                    arc = d3.svg.arc().innerRadius(0).outerRadius(r)
                    svg = d3.select('.pie_chart-data_in_memory').attr('width', width).attr('height', height).append('svg:g').attr('transform', 'translate('+width/2+', '+height/2+')')
                    arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc)

                    # No transition for now
                    #arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc).style('opacity', 0).transition().duration(600).style('opacity', 1)

            return @ # Just to make sure that d3 doesn't return false
            
        destroy: =>
            machines.off 'change', @render_data_in_memory

    class @Other extends Backbone.View
        className: 'namespace-other'

        template: Handlebars.compile $('#namespace_other-template').html()
        events: ->
            'click .rename_namespace-button': 'rename_namespace'
            'click .import_data-button': 'import_data'
            'click .export_data-button': 'export_data'
            'click .delete_namespace-button': 'delete_namespace'

        initialize: =>
            @model.on 'change:name', @render

        rename_namespace: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()

        import_data: (event) ->
            event.preventDefault()
            #TODO Implement
        
        export_data: (event) ->
            event.preventDefault()
            #TODO Implement

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

        render: =>
            @.$el.html @template {}
            return @

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

            @model.load_key_distr()
            @model.set_interval_key_distr()
        
            # Panels for namespace view
            @title = new NamespaceView.Title(model: @model)
            @profile = new NamespaceView.Profile(model: @model)
            @replicas = new NamespaceView.Replicas(model: @model)
            @shards = new NamespaceView.Sharding(model: @model)
            @overview = new NamespaceView.Overview(model: @model)
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
            @.$('.namespace-overview-container').html @overview.render().$el

            # Display the replicas
            @.$('.replication').html @replicas.render().el

            # Display the shards
            @.$('.sharding').html @shards.render().el

            @.$('.nav-tabs').tab()

            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        destroy: =>
            @model.clear_interval_key_distr()

            @title.destroy()
            @profile.destroy()
            @replicas.destroy()
            @shards.destroy()
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
            json = _.extend json,
                old_alert_html: @.$('#user-alert-space').html()

            if namespace_status.reachability == 'Live'
                json.reachability = true
            else
                json.reachability = false

            @.$el.html @template json

            return @

    class @Overview extends Backbone.View
        className: 'namespace-overview'

        template: Handlebars.compile $('#namespace_overview-template').html()

        initialize: ->
            machines.on 'change', @render
            @json =
                data_in_memory_percent: -1
                data_in_memory: -1
                data_total: -1

        render: =>
            $('.tooltip').remove()

            data_in_memory = 0
            data_total = 0
            data_is_ready = false
            #TODO These are being calculated incorrectly. As they stand, data_in_memory is correct, but data_total is measuring the disk space used by this namespace. Issue filed.
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
                    data_not_in_memory: human_readable_units(data_total-data_in_memory, units_space)
                    data_total: human_readable_units(data_total, units_space)
            else
                json =
                    data_in_memory_percent: 'loading...'
                    data_in_memory: 'loading...'
                    data_not_in_memory: 'loading...'
                    data_total: 'loading...'
 



            shards = []
            for shard in namespaces.models[0].get('computed_shards').models
                new_shard =
                    boundaries: shard.get('shard_boundaries')
                    num_keys: @model.compute_shard_rows_approximation(shard.get('shard_boundaries'))
                shards.push new_shard

            # We could probably use apply for a cleaner code, but d3.max has a strange behavior.
            max_keys = d3.max shards, (d) -> return d.num_keys
            min_keys = d3.min shards, (d) -> return d.num_keys

            json = _.extend json,
                max_keys: max_keys
                min_keys: min_keys

            if shards.length > 1
                json.has_shards = true
                if shards.length > 6
                    json.numerous_shards = true

            # So we don't update every seconds
            need_update = false
            for key of json
                if not @json[key]? or @json[key] != json[key]
                    need_update = true
                    break
 
            if need_update
                @json = json
                @.$el.html @template @json


                # Draw pie chart
                if data_in_memory isnt 0 and data_total isnt 0
                    r = 100
                    width = 270
                    height = 270
                    color = (i) ->
                        if i is 0
                            return '#f00'
                        else
                            return '#1f77b4'

                    data_pie = [data_in_memory, data_total-data_in_memory]

                    # Remove transition for the time being. We have to use transition with opacity only the first time
                    # For update, we should just move/extend pieces, too much work for now
                    #@.$('.loading_text-svg').fadeOut '600', -> $(@).remove() 
                    @.$('.pie_chart-data_in_memory > g').remove()
                    @.$('.loading_text-pie_chart').remove()

                    

                    arc = d3.svg.arc().innerRadius(0).outerRadius(r);
                    svg = d3.select('.pie_chart-data_in_memory').attr('width', width).attr('height', height).append('svg:g').attr('transform', 'translate('+width/2+', '+height/2+')')
                    arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc)

                    # No transition for now
                    #arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc).style('opacity', 0).transition().duration(600).style('opacity', 1)


                # Draw histogram
                if json.max_keys? and shards.length isnt 0
                    @.$('.data_repartition-diagram > g').remove()
                    @.$('.loading_text-diagram').remove()
                    
                    if json.numerous_shards? and json.numerous_shards
                        svg_width = 700
                        svg_height = 350
                    else
                        svg_width = 350
                        svg_height = 270
                    margin_width = 20
                    margin_height = 20

                    width = Math.floor(svg_width/shards.length*0.7)
                    x = d3.scale.linear().domain([0, shards.length]).range([margin_width+Math.floor(width/2), svg_width-margin_width*2])
                    y = d3.scale.linear().domain([0, json.max_keys]).range([0, svg_height-margin_height*2.5])
 

                    svg = d3.select('.data_repartition-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')
                    svg.selectAll('rect').data(shards)
                        .enter()
                        .append('rect')
                        .attr('x', (d, i) -> return x(i))
                        .attr('y', (d) -> return svg_height-y(d.num_keys)-margin_height)
                        .attr('width', width)
                        .attr( 'height', (d) -> return y(d.num_keys))
                        .attr( 'title', (d) -> return 'Shard:'+d.boundaries+'<br />'+d.num_keys+' keys')
                        ###
                        .attr('data-num_keys', (d) -> return d.num_keys)
                        .attr('data-shard', (d) -> return d.boundaries)
                        ###


                    arrow_width = 4
                    arrow_length = 7
                    extra_data = []
                    extra_data.push
                        x1: margin_width
                        x2: margin_width
                        y1: margin_height
                        y2: svg_height-margin_height

                    extra_data.push
                        x1: margin_width
                        x2: svg_width-margin_width
                        y1: svg_height-margin_height
                        y2: svg_height-margin_height
 
                    svg = d3.select('.data_repartition-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')
                    svg.selectAll('line').data(extra_data).enter().append('line')
                        .attr('x1', (d) -> return d.x1)
                        .attr('x2', (d) -> return d.x2)
                        .attr('y1', (d) -> return d.y1)
                        .attr('y2', (d) -> return d.y2)
                        .style('stroke', '#000')

                    axe_legend = []
                    axe_legend.push
                        x: margin_width
                        y: Math.floor(margin_height/2)
                        string: 'Keys'
                        anchor: 'middle'
                    axe_legend.push
                        x: Math.floor(svg_width/2)
                        y: svg_height
                        string: 'Shards'
                        anchor: 'middle'
 
                    svg.selectAll('.legend')
                        .data(axe_legend).enter().append('text')
                        .attr('x', (d) -> return d.x)
                        .attr('y', (d) -> return d.y)
                        .attr('text-anchor', (d) -> return d.anchor)
                        .text((d) -> return d.string)

                    @.$('rect').tooltip
                        trigger: 'hover'

            @delegateEvents()
            return @

        update_history_opsec: =>
            @history_opsec.shift()
            @history_opsec.push @model.get_stats().keys_read + @model.get_stats().keys_set



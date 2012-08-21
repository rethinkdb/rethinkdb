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
        #TODO Add a class to keep alert when adding multiple namespaces
        className: 'namespace-view'
        template: Handlebars.compile $('#namespace_view-container-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()

        events: ->
            'click .tab-link': 'change_route'
            'click .close': 'close_alert'
            'click .rebalance_shards-link': 'rebalance_shards'
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


        rebalance_shards: (event) =>
            event.preventDefault()
            confirmation_modal = new UIComponents.ConfirmationDialogModal
            confirmation_modal.render("Are you sure you want to rebalance the shards for the namespace "+@model.get('name')+"? This operation might take some time.",
                "",
                {},
                undefined)
            confirmation_modal.on_submit = =>
                debugger
                # Because Coffeescript doesn't want me to access this and I need _this
                `this.$('.btn-primary').button('loading');
                this.$('.cancel').button('loading');`

                # grab the data
                data = @model.get('key_distr')
                distr_keys = @model.get 'key_distr_sorted'
                total_rows = _.reduce distr_keys, ((agg, key) => return agg + data[key]), 0
                desired_shards = @model.get('shards').length
                rows_per_shard = total_rows / desired_shards

                # Phew. Go through the keys now and compute the bitch ass split points.
                current_shard_count = 0
                split_points = [""]
                no_more_splits = false
                for key in distr_keys
                    # Let's not overdo it :-D
                    if split_points.length >= desired_shards
                        no_more_splits = true
                    current_shard_count += data[key]
                    if current_shard_count >= rows_per_shard and not no_more_splits
                        # Hellz yeah ho, we got our split point
                        split_points.push(key)
                        current_shard_count = 0
                split_points.push(null)

                # convert split points into whatever bitch ass format we're using here
                _shard_set = []
                for splitIndex in [0..(split_points.length - 2)]
                    _shard_set.push(JSON.stringify([split_points[splitIndex], split_points[splitIndex + 1]]))

                # See if we have enough shards
                if _shard_set.length < desired_shards
                    @error_msg = "There is only enough data to suggest " + _shard_set.length + " shards."
                    @render()
                    return


                empty_master_pin = {}
                empty_master_pin[JSON.stringify(["", null])] = null
                empty_replica_pins = {}
                empty_replica_pins[JSON.stringify(["", null])] = []
                json =
                    shards: _shard_set
                    primary_pinnings: empty_master_pin
                    secondary_pinnings: empty_replica_pins

                that = @
                @model.set 'key_distr', {}
                @model.set 'key_distr_sorted', []

                @.$('.loading_text-diagram').css 'display', 'block'


                $.ajax
                    processData: false
                    url: "/ajax/semilattice/#{@model.get('protocol')}_namespaces/#{@model.get('id')}"
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify(json)
                    success: (response) =>
                        that.model.load_key_distr() #This call will fail since we cannot make queries while sharding.
                        that.$('#user-alert-space').html @alert_tmpl({})
                        clear_modals()


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

            if namespace_status.reachability? and namespace_status.reachability is 'Live'
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
            @model.on 'change:key_distr', @render_data_repartition
            @model.on 'change:shards', @render_data_repartition
            @json_in_memory =
                data_in_memory_percent: -1
                data_in_memory: -1
                data_total: -1
            @json_repartition = {}

        render: =>
            @.$el.html @container_template {}
            @render_data_in_memory(true)
            @render_data_repartition(true)
            return @

        # TODO refactor without the boolean - Check machine view
        render_data_in_memory: (force_render) =>
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
            if need_update or force_render is true or @.$('.pie_chart-data_in_memory').children().length is 1 # Need is true for force_render since Machine is passed as an argument
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


                    if $('.pie_chart-data_in_memory').length is 1 #Check dom tree ready for d3. TODO: Replace this hack.
                        # Remove transition for the time being. We have to use transition with opacity only the first time
                        # For update, we should just move/extend pieces, too much work for now
                        #@.$('.loading_text-svg').fadeOut '600', -> $(@).remove() 
                        @.$('.loading_text-pie_chart').css 'display', 'none'
    
                        arc = d3.svg.arc().innerRadius(0).outerRadius(r)
                        svg = d3.select('.pie_chart-data_in_memory').attr('width', width).attr('height', height).append('svg:g').attr('transform', 'translate('+width/2+', '+height/2+')')
                        arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc)
    
                        # No transition for now
                        #arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc).style('opacity', 0).transition().duration(600).style('opacity', 1)
                    else
                        setTimeout(@render_data_memory, 1000)

            return @ # Just to make sure that d3 doesn't return false

        render_data_repartition: (force_render) =>
            $('.tooltip').remove()
            shards = []
            total_keys = 0
            for shard in namespaces.models[0].get('computed_shards').models
                new_shard =
                    boundaries: shard.get('shard_boundaries')
                    num_keys: parseInt @model.compute_shard_rows_approximation shard.get 'shard_boundaries'
                shards.push new_shard
                total_keys += new_shard.num_keys

            # We could probably use apply for a cleaner code, but d3.max has a strange behavior.
            max_keys = d3.max shards, (d) -> return d.num_keys
            min_keys = d3.min shards, (d) -> return d.num_keys
            json =
                max_keys: max_keys
                min_keys: min_keys
                shards_length: shards.length
                total_jeys: total_keys

            if shards.length > 1
                json.has_shards = true
                if shards.length > 6
                    json.numerous_shards = true
            else
                json.has_shards = false

            # So we don't update every seconds
            need_update = false
            for key of json
                if not @json_repartition[key]? or @json_repartition[key] != json[key]
                    need_update = true
                    break

            if need_update or force_render is true or @.$('.data_repartition-diagram').children().length is 1

                @.$('.data_repartition-container').html @data_repartition_template json

                if $('.data_repartition-diagram').length>0 #We need the dom tree to be fully available for d3
                    #TODO Replace this hack with a proper callback/listener
                    @.$('.loading_text-diagram').css 'display', 'none'
                    @json_repartition = json
                else
                    setTimeout(@render_data_repartition, 1000)


                # Draw histogram
                if json.max_keys? and not _.isNaN json.max_keys and shards.length isnt 0
                    
                    if json.numerous_shards? and json.numerous_shards
                        svg_width = 700
                        svg_height = 350
                    else
                        svg_width = 350
                        svg_height = 270
                    margin_width = 20
                    margin_height = 20

                    width = Math.floor((svg_width-margin_width*3)/shards.length*0.8)
                    margin_bar = Math.floor((svg_width-margin_width*3)/shards.length*0.2/2)
                    if shards.length is 1 # Special hack when there is just one shard
                        width = Math.floor width/2
                        margin_bar = Math.floor margin_bar+width/2
 
                    x = d3.scale.linear().domain([0, shards.length-1]).range([margin_width*1.5+margin_bar, svg_width-margin_width*1.5-margin_bar-width])
                    y = d3.scale.linear().domain([0, json.max_keys]).range([1, svg_height-margin_height*2.5])

                    svg = d3.select('.data_repartition-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')
                    svg.selectAll('rect').data(shards)
                        .enter()
                        .append('rect')
                        .attr('x', (d, i) -> return x(i))
                        .attr('y', (d) -> return svg_height-y(d.num_keys)-margin_height-1) #-1 not to overlap with axe
                        .attr('width', width)
                        .attr( 'height', (d) -> return y(d.num_keys))
                        .attr( 'title', (d) -> return 'Shard:'+d.boundaries+'<br />'+d.num_keys+' keys')
                

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
                        string: if json.shards_length > 1 then 'Shards' else 'Data'
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

        destroy: =>
            machines.off 'change', @render_data_in_memory
            @model.off 'change:key_distr', @render_data_repartition
            @model.off 'change:shards', @render_data_repartition

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
            #overwrite on_success to add a redirectiona
            namespace_to_delete = @model
        
            remove_namespace_dialog.on_success = (response) =>
                window.router.navigate '#namespaces', {'trigger': true}
                namespaces.remove @model.get 'id'

            remove_namespace_dialog.render [@model]

        render: =>
            @.$el.html @template {}
            return @

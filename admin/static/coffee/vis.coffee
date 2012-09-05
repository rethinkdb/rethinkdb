module 'Vis', ->
    # Wraps the ops graph (left) and the stats panel (right)
    class @PerformancePanel extends Backbone.View
        className: 'performance_panel'
        template: Handlebars.compile $('#performance_panel-template').html()

        initialize: (_stats_fn) =>
            log_initial '(initializing) performance panel'
            @ops_plot = new Vis.OpsPlot(_stats_fn)

            @more_stats = new Vis.MoreStats()

        render: ->
            log_render '(rendering) performance panel'
            # Render the plot container
            @.$el.html (@template {})
            # Render our elements
            @.$('.ops_plot_placeholder').html(@ops_plot.render().$el)

            @.$('.stats_placeholder').html(@more_stats.render().$el)
            return @

        destroy: =>
            @ops_plot.destroy()
            #@stats_panel.destroy()
            @more_stats.destroy()

    @num_formatter = (i) ->
        if isNaN(i)
            return 'N/A'
        if i / 1000000000 >= 1
            res = '' + ((i / 1000000000).toFixed(1))
            if res.slice(-2) is '.0'
                res = res.slice(0, -2)
            res += 'B'
        else if i / 1000000 >= 1
            res = '' + ((i / 1000000).toFixed(1))
            if res.slice(-2) is '.0'
                res = res.slice(0, -2)
            res += 'M'
        else if i / 1000 >= 1
            res = '' + ((i / 1000).toFixed(1))
            if res.slice(-2) is '.0'
                res = res.slice(0, -2)
            res += 'K'
        else
            res = '' + i.toFixed(0)
        return res

    class @InterpolatingCache
        constructor: (num_data_points, num_interpolation_points, get_data_fn) ->
            @ndp = num_data_points
            @nip = num_interpolation_points
            @get_data_fn = get_data_fn
            @values = []
            for i in [0..(@ndp-1)]
                @values.push(0)
            @next_value = null

        step: (num_points) ->
            # First, grab newest data
            @push_data()

            # Now give them what they want
            return @values.slice(-num_points)

        push_data: ->
            # Check if we need to restart interpolation
            current_value = @get_data_fn()
            if @next_value isnt current_value
                @start_value = @values[@values.length - 1]
                @next_value = current_value
                @interpolation_step = 1

            if @values[@values.length - 1] is @next_value
                value_to_push = @next_value
            else
                value_to_push = @start_value + ((@next_value - @start_value) / @nip * @interpolation_step)
                @interpolation_step += 1
                if @interpolation_step > @nip
                    value_to_push = @next_value
            @values.push(value_to_push)

            # Trim the cache
            if @values.length > @ndp
                @values = @values.slice(-@ndp)

    class @OpsPlot extends Backbone.View
        className: 'ops_plot'
        template: Handlebars.compile $('#ops_plot-template').html()

        # Please make sure the first is divisible by the second,
        # 'cause who knows wtf is gonna happen if it ain't.
        HEIGHT_IN_PIXELS: 200
        WIDTH_IN_PIXELS: 300
        WIDTH_IN_SECONDS: 60
        HAXIS_TICK_INTERVAL_IN_SECS: 15
        HAXIS_MINOR_SUBDIVISION_COUNT: 3
        VAXIS_TICK_SUBDIVISION_COUNT: 5
        VAXIS_MINOR_SUBDIVISION_COUNT: 2

        make_metric: (name) =>
            # We gonna hafta cache dem mafucken stats
            cache = new Vis.InterpolatingCache(
                @WIDTH_IN_PIXELS,
                @WIDTH_IN_PIXELS / @WIDTH_IN_SECONDS,
                (=> @stats_fn()[name]))
            _metric = (start, stop, step, callback) =>
                # Give them however many they want
                start = +start
                stop = +stop
                num_desired = (stop - start) / step
                callback(null, cache.step(num_desired))
            @context.on 'focus', (i) =>
                d3.selectAll('.value').style('right', if i is null then null else @context.size() - i + 'px')
            return @context.metric(_metric, name)

        initialize: (_stats_fn) ->
            log_initial '(initializing) ops plot'
            super
            # Set up cubism context
            @stats_fn = _stats_fn
            @context = cubism.context()
                .serverDelay(0)
                .clientDelay(0)
                .step(1000 / (@WIDTH_IN_PIXELS / @WIDTH_IN_SECONDS))
                .size(@WIDTH_IN_PIXELS)
            @read_stats = @make_metric('keys_read')
            @write_stats = @make_metric('keys_set')
            @legend = new Vis.OpsPlotLegend(@read_stats, @write_stats, @context)

        render: ->
            log_render '(rendering) ops plot'
            # Render the plot container
            @.$el.html @template({})

            # Set up the plot
            sensible_plot = @context.sensible()
                .height(@HEIGHT_IN_PIXELS)
                .colors(["#983434","#729E51"])
                .extent([0, 20000])
            d3.select(@.$('#ops_plot_container')[0]).call (div) =>
                div.data([[@read_stats, @write_stats]])
                # Chart itself
                div.append('div')
                    .attr('class', 'chart')
                    .call(sensible_plot)
                # Horizontal axis
                div.append('div')
                    .attr('class', 'haxis')
                    .call(@context.axis()
                        .orient('bottom')
                        .ticks(d3.time.seconds, @HAXIS_TICK_INTERVAL_IN_SECS)
                        .tickSubdivide(@HAXIS_MINOR_SUBDIVISION_COUNT - 1)
                        .tickSize(6, 3, 0)
                        .tickFormat(d3.time.format('%I:%M:%S')))
                # Horizontal axis grid
                div.append('div')
                    .attr('class', 'hgrid')
                    .call(@context.axis()
                        .orient('bottom')
                        .ticks(d3.time.seconds, @HAXIS_TICK_INTERVAL_IN_SECS)
                        .tickSize(204, 0, 0)
                        .tickFormat(""))
                # Vertical axis
                div.append('div')
                    .attr('class', 'vaxis')
                    .call(@context.axis(@HEIGHT_IN_PIXELS, [@read_stats, @write_stats], sensible_plot.scale())
                        .orient('left')
                        .ticks(@VAXIS_TICK_SUBDIVISION_COUNT)
                        .tickSubdivide(@VAXIS_MINOR_SUBDIVISION_COUNT - 1)
                        .tickSize(6, 3, 0)
                        .tickFormat(Vis.num_formatter))
                # Vertical axis grid
                div.append('div')
                    .attr('class', 'vgrid')
                    .call(@context.axis(@HEIGHT_IN_PIXELS, [@read_stats, @write_stats], sensible_plot.scale())
                        .orient('left')
                        .ticks(@VAXIS_TICK_SUBDIVISION_COUNT)
                        .tickSize(-(@WIDTH_IN_PIXELS + 35), 0, 0)
                        .tickFormat(""))
                # Legend
                @.$('#ops_plot_container').append(@legend.render().$el)

            return @

        destroy: =>
            @context.on 'focus'
            @legend.destroy()

    class @OpsPlotLegend extends Backbone.View
        className: 'ops_plot_legend'
        template: Handlebars.compile $('#ops_plot_legend-template').html()

        initialize: (_read_metric, _write_metric, _context) =>
            log_initial '(initializing) ops plot legend'
            @context = _context
            @reads = null
            @writes = null
            @read_metric = _read_metric
            @read_metric.on 'change', =>
                @reads = _read_metric.valueAt(@context.size() - 1)
                @writes = _write_metric.valueAt(@context.size() - 1)
                @render()

        render: ->
            log_render '(rendering) ops plot legend'
            # Render the plot container
            @.$el.html @template
                read_count: if @reads? then Vis.num_formatter(@reads) else 'N/A'
                write_count: if @writes? then Vis.num_formatter(@writes) else 'N/A'
            return @

        destroy: =>
            @read_metric.on 'change' # We remove the listener.

    class @SizeBoundedCache
        constructor: (num_data_points, _stat) ->
            @values = []
            @ndp = num_data_points
            @stat = _stat

        push: (stats) ->
            if typeof(@stat) is 'function'
                value = @stat(stats)
            else
                if stats?
                    value = stats[@stat]
            if isNaN(value)
                return
            @values.push(value)
            # Fill up the cache if there aren't enough values
            if @values.length < @ndp
                for i in [(@values.length)...(@ndp-1)]
                    @values.push(value)
            # Trim the cache if there are too many values
            if @values.length > @ndp
                @values = @values.slice(-@ndp)

        get: ->
            return @values

        get_latest: ->
            return @values[@values.length - 1]

    class @MoreStats extends Backbone.View
        className: 'more_stats'
        template: Handlebars.compile $('#more_stats-template').html()
        initialize: =>
            machines.on 'add', @render
            machines.on 'remove', @render
            datacenters.on 'add', @render
            datacenters.on 'remove', @render
            databases.on 'add', @render
            databases.on 'remove', @render
            namespaces.on 'add', @render
            namespaces.on 'remove', @render

        render: =>
            @.$el.html @template
                num_datacenters: datacenters.length
                num_servers: machines.length
                num_databases: databases.length
                num_namespaces: namespaces.length
            return @

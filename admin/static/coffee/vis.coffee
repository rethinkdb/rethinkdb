module 'Vis', ->
    # Wraps the ops graph (left) and the stats panel (right)
    class @PerformancePanel extends Backbone.View
        className: 'performance_panel'
        template: Handlebars.compile $('#performance_panel-template').html()

        initialize: (_stats_fn) =>
            log_initial '(initializing) performance panel'
            @ops_plot = new Vis.OpsPlot(_stats_fn)
            @stats_panel = new Vis.StatsPanel(_stats_fn)

        render: ->
            log_render '(rendering) performance panel'
            # Render the plot container
            @.$el.html (@template {})
            # Render our elements
            @.$('.ops_plot_placeholder').html(@ops_plot.render().$el)
            @.$('.stats_placeholder').html(@stats_panel.render().$el)
            return @

        destroy: =>
            @ops_plot.destroy()
            @stats_panel.destroy()

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
            @context.off()
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
            @read_matric.off()

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

    class @StatsPanel extends Backbone.View
        className: 'stats_panel'
        template: Handlebars.compile $('#stats_panel-template').html()
        NUM_POINTS: 25
        RENDER_INVERVAL_IN_MS: 1000
        SPARKLINE_DEFAULTS:
            fillColor: false
            spotColor: false
            minSpotColor: false
            maxSpotColor: false

        initialize: (_stats_fn) =>
            log_initial '(initializing) stats panel'
            @stats_fn = _stats_fn
            @total_ops_cache = new Vis.SizeBoundedCache(@NUM_POINTS, ((stats) -> return stats['keys_read'] + stats['keys_set']))
            @total_cpu_util_cache = new Vis.SizeBoundedCache(@NUM_POINTS, ((stats) ->
                return parseInt((stats['global_cpu_util']['avg'] * 100).toFixed(0))
            ))
            @total_disk_space_cache = new Vis.SizeBoundedCache(@NUM_POINTS, 'global_disk_space')
            @net_recv_cache = new Vis.SizeBoundedCache(@NUM_POINTS, 'avg')
            @net_sent_cache = new Vis.SizeBoundedCache(@NUM_POINTS, 'avg')

        render: =>
            log_render '(rendering) stats panel'
            # Grab latest stats
            stats = @stats_fn()
            @total_ops_cache.push(stats)
            @total_cpu_util_cache.push(stats)
            @total_disk_space_cache.push(stats)
            @net_recv_cache.push(stats.global_net_recv_persec)
            @net_sent_cache.push(stats.global_net_sent_persec)

            # Render the plot container
            @.$el.html @template
                total_ops_sec: Vis.num_formatter(@total_ops_cache.get_latest())
                total_cpu_util: @total_cpu_util_cache.get_latest()
                mem_used: human_readable_units(stats.global_mem_used * 1024, units_space)
                mem_total: human_readable_units(stats.global_mem_total * 1024, units_space)
                mem_used_percent: parseInt((stats.global_mem_used / stats.global_mem_total * 100).toFixed(0))
                disk_used: human_readable_units(stats.global_disk_space, units_space)
                global_net_recv: human_readable_units(stats.global_net_recv_persec.avg, units_space) if stats.global_net_recv_persec?
                global_net_sent: human_readable_units(stats.global_net_sent_persec.avg, units_space) if stats.global_net_sent_persec?



            # Totals ops sparkline
            @.$('ul.total_ops_sec > li.minichart > span').sparkline(@total_ops_cache.get(), @SPARKLINE_DEFAULTS)
            # CPU utilization sparkline
            cpu_sparkline_json = {}
            _.extend cpu_sparkline_json, @SPARKLINE_DEFAULTS,
                chartRangeMin: 0
                chartRangeMax: 100
            @.$('ul.total_cpu_util > li.minichart > span').sparkline(@total_cpu_util_cache.get(), cpu_sparkline_json)
            # Disk utilization sparkline
            disk_sparkline_json = {}
            _.extend disk_sparkline_json, @SPARKLINE_DEFAULTS,
                chartRangeMin: 0
            @.$('ul.total_disk_usage > li.minichart > span').sparkline(@total_disk_space_cache.get(), disk_sparkline_json)
            # Network sparklines
            @.$('ul.global_net_sent > li.minichart > span').sparkline(@net_sent_cache.get(), @SPARKLINE_DEFAULTS)
            @.$('ul.global_net_recv > li.minichart > span').sparkline(@net_recv_cache.get(), @SPARKLINE_DEFAULTS)


            # Rerender ourselves to update the data
            setTimeout(@render, @RENDER_INVERVAL_IN_MS)

            return @

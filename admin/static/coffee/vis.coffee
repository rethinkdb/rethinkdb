# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'Vis', ->
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

    class @OpsPlotLegend extends Backbone.View
        className: 'ops-plot-legend'
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
            @.$el.html @template
                read_count: if @reads? then Vis.num_formatter(@reads) else 'N/A'
                write_count: if @writes? then Vis.num_formatter(@writes) else 'N/A'
            return @

        destroy: =>
            @read_metric.on 'change' # We remove the listener.

    class @OpsPlot extends Backbone.View
        className: 'ops-plot'
        template: Handlebars.compile $('#ops_plot-template').html()
        barebones_template: Handlebars.compile $('#ops_plot-template').html()

        # default options for the plot template
        type: 'cluster'


        # default options for the plot itself
        # Please make sure the first is divisible by the second,
        # 'cause who knows wtf is gonna happen if it ain't.
        HEIGHT_IN_PIXELS: 200
        HEIGHT_IN_UNITS: 20500
        WIDTH_IN_PIXELS: 300
        WIDTH_IN_SECONDS: 60
        HAXIS_TICK_INTERVAL_IN_SECS: 15
        HAXIS_MINOR_SUBDIVISION_COUNT: 3
        VAXIS_TICK_SUBDIVISION_COUNT: 5
        VAXIS_MINOR_SUBDIVISION_COUNT: 2


        make_metric: (name) =>
            # Cache stats using the interpolating cache
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


        # The OpsPlot has one required argument:
        #   _stats_fn: function that provides data to be plotted
        # It also has one optional argument:
        #   options: an object specifying any of the following 
        #            to override default values:
        #       * height: height of the plot in pixels
        #       * width: width of the plot in pixels
        #       * height_in_units: number of units to render
        #           on the y-axis when the plot is empty
        #       * seconds: number of seconds the plot
        #           tracks; width in pixels must be cleanly
        #           divisible by the number of seconds
        #       * haxis.seconds_per_tick: how many seconds
        #           before placing a tick
        #       * haxis.ticks_per_label: how many ticks before 
        #           placing a label on the x-axis 
        #       * vaxis.num_per_tick: how many units on the y-axis
        #           before placing a tick
        #       * vaxis.ticks_per_label: how many ticks before 
        #           placing a label on the y-axis 
        #       * type: the type of plot (used to determine the title
        #           of the plot). valid values include 'cluster',
        #           'datacenter', 'server', 'database', and 'table'
        initialize: (_stats_fn, options) ->
            log_initial '(initializing) ops plot'

            if options?
                # Kludgey way to override custom options given Slava's class variable approach.
                # A sane options object for the entire class would have been preferable.
                @HEIGHT_IN_PIXELS =  options.height          if options.height?
                @HEIGHT_IN_UNITS =   options.height_in_units if options.height_in_units?
                @WIDTH_IN_PIXELS =   options.width           if options.width?
                @WIDTH_IN_SECONDS =  options.seconds         if options.seconds?
                if options.haxis?
                    @HAXIS_TICK_INTERVAL_IN_SECS =   options.haxis.seconds_per_tick if options.haxis.seconds_per_tick?
                    @HAXIS_MINOR_SUBDIVISION_COUNT = options.haxis.ticks_per_label  if options.haxis.ticks_per_label?
                if options.vaxis?
                    @VAXIS_TICK_SUBDIVISION_COUNT =  options.vaxis.num_ticks        if options.vaxis.num_ticks?
                    @VAXIS_MINOR_SUBDIVISION_COUNT = options.vaxis.ticks_per_label  if options.vaxis.ticks_per_label?
                @type =     options.type    if options.type?

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

        render: =>
            log_render '(rendering) ops plot'
            # Render the plot container
            @.$el.html @template
                cluster:    @type is 'cluster'
                datacenter: @type is 'datacenter'
                server:     @type is 'server'
                database:   @type is 'database'
                table:      @type is 'table'

            # Set up the plot
            sensible_plot = @context.sensible()
                .height(@HEIGHT_IN_PIXELS)
                .colors(["#983434","#729E51"])
                .extent([0, @HEIGHT_IN_UNITS])
            d3.select(@.$('.plot')[0]).call (div) =>
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
                        .tickSize(@HEIGHT_IN_PIXELS + 4, 0, 0)
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
                @.$('.legend-container').html @legend.render().el

            return @

        destroy: =>
            @context.on 'focus'
            @legend.destroy()

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


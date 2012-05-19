module 'Vis', ->
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

        num_formatter: (i) ->
            if i / 1000000000 >= 1
                res = '' + ((i / 1000000000).toFixed(1)) + 'B'
            else if i / 1000000 >= 1
                res = '' + ((i / 1000000).toFixed(1)) + 'M'
            else if i / 1000 >= 1
                res = '' + ((i / 1000).toFixed(1)) + 'K'
            else
                res = '' + i
            return res

        render: ->
            log_render '(rendering) ops plot'
            # Render the plot container
            @.$el.html @template({})

            # Set up the plot
            sensible_plot = @context.sensible()
                .height(@HEIGHT_IN_PIXELS)
                .colors(["#983434","#729E51"])
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
                # Vertical axis
                div.append('div')
                    .attr('class', 'vaxis')
                    .call(@context.axis(@HEIGHT_IN_PIXELS, [@read_stats, @write_stats], sensible_plot.scale())
                        .orient('left')
                        .ticks(@VAXIS_TICK_SUBDIVISION_COUNT)
                        .tickSubdivide(@VAXIS_MINOR_SUBDIVISION_COUNT - 1)
                        .tickSize(6, 3, 0)
                        .tickFormat(@num_formatter))
                # Rule
                div.append('attr')
                    .attr('class', 'rule')
                    .call(@context.rule(@.$('canvas')))

            return @


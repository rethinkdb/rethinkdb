module 'Vis', ->
    class @InterpolatingCache
        constructor: (num_data_points, num_interpolation_points, get_data_fn) ->
            @ndp = num_data_points
            @nip = num_interpolation_points
            @get_data_fn = get_data_fn
            @values = []
            for i in [0..(@ndp-1)]
                @values.push(0)

        step: (num_points) ->
            # First, grab newest data
            @push_data()

            # Now give them what they want
            return @values.slice(-num_points)

        push_data: ->
            @values.push(@get_data_fn())
            if @values.length > @ndp
               @values = @values.slice(-@ndp)

    class @OpsPlot extends Backbone.View
        className: 'ops_plot'
        template: Handlebars.compile $('#ops_plot-template').html()

        # Please make sure the first is divisible by the second,
        # 'cause who knows wtf is gonna happen if it ain't.
        WIDTH_IN_PIXELS: 300
        WIDTH_IN_SECONDS: 60

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
            return @context.metric(_metric, name)

        initialize: (_stats_fn) ->
            log_initial '(initializing) ops plot'
            super
            # Set up cubism context
            @stats_fn = _stats_fn
            @context = cubism.context()
                .serverDelay(0)
                .clientDelay(0)
                .step(200)
                .size(@WIDTH_IN_PIXELS)
            @read_stats = @make_metric('keys_read')
            @write_stats = @make_metric('keys_set')

        render: ->
            log_render '(rendering) ops plot'
            # Render the plot container
            @.$el.html @template({})

            # Set up the plot
            d3.select(@.$('#ops_plot_container')[0]).call (div) =>
                div.data([[@read_stats, @write_stats]])
                div.append('div')
                    .attr('class', 'chart')
                    .call(@context.sensible()
                        .height(200)
                        .extent([0, 2000])
                        .colors(["#ff0000","#00ff00"])
                        )
                div.append('div')
                    .attr('class', 'axis')
                    .call(@context.axis()
                        .orient('bottom')
                        .ticks(d3.time.seconds, 15)
                        .tickSubdivide(2)
                        .tickSize(6, 3, 0))

            return @


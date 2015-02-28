# Copyright 2010-2015 RethinkDB



# Hardcoded!
MAX_SHARD_COUNT = 32

class ShardDistribution extends Backbone.View
    className: 'shards_container'
    template:
        main: require('../../handlebars/shard_distribution_container.hbs')

    initialize: (data) =>
        if @collection?
            @listenTo @collection, 'update', @render_data_distribution
        @listenTo @model, 'change:info_unavailable', @set_warnings
        @render_data_distribution

    set_warnings: ->
        if @model.get('info_unavailable')
            @$('.unavailable-error').show()
        else
            @$('.unavailable-error').hide()

    set_distribution: (shards) =>
        @collection = shards
        @listenTo @collection, 'update', @render_data_distribution
        @render_data_distribution()

    render: =>
        @$el.html @template.main(@model.toJSON())
        @init_chart = false
        setTimeout(
            # Let the element be inserted in the main DOM tree
            => @render_data_distribution() # setTimeout uses the wrong "this"
        , 0)
        @set_warnings()
        return @

    prettify_num: (num) ->
        if num > 1000000000
            num = (""+num).slice(0,-9)+"M"
        if num > 1000000 and num < 1000000000
            num = (""+num).slice(0,-6)+"m"
        else if num > 1000 and num < 1000000
            num = (""+num).slice(0,-3)+"k"
        else
            num

    display_outdated: =>
        if @$('.outdated_distribution').css('display') is 'none'
            @$('.outdated_distribution').slideDown 'fast'

    render_data_distribution: =>
        $('.tooltip').remove()

        @$('.loading').slideUp 'fast'
        @$('.outdated_distribution').slideUp 'fast'
        @$('.shard-diagram').show()

        max_keys = d3.max(@collection.models, (shard) ->
            return shard.get('num_keys')) ? 100
        min_keys = d3.min(@collection.models, (shard) ->
            return shard.get('num_keys')) ? 0

        svg_width = 328 # Width of the whole svg
        svg_height = 270 # Height of the whole svg

        margin_width = 20 # Margin on left of the y-axis
        margin_height = 20 # Margin under the x-axis
        min_margin_width_inner = 20 # Min margin on the right of the y-axis

        # We have two special cases
        if @collection.length is 1
            bar_width = 100
            margin_bar = 20
        else if @collection.length is 2
            bar_width = 80
            margin_bar = 20
        else #80% for the bar, 20% for the margin
            bar_width = Math.floor(0.8*(328-margin_width*2-min_margin_width_inner*2)/@collection.length)
            margin_bar = Math.floor(0.2*(328-margin_width*2-min_margin_width_inner*2)/@collection.length)

        # size of all bars and white space between bars
        width_of_all_bars = bar_width*@collection.length + margin_bar*(@collection.length-1)

        # Update the margin on the right of the y-axis
        margin_width_inner = Math.floor((svg_width-margin_width*2-width_of_all_bars)/2)

        # Y scale
        y = d3.scale.linear().domain([0, max_keys]).range([1, svg_height-margin_height*2.5])

        # Add svg
        #svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')
        svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height)
        bars_container = svg.select('.bars_container')

        # Add rectangle
        bars = bars_container.selectAll('rect').data(@collection.map((shard, index) ->
            index: index
            num_keys: shard.get('num_keys')
        ))

        bars.enter().append('rect')
            .attr('class', 'rect')
            .attr('x', (d, i) ->
                return margin_width+margin_width_inner+bar_width*i+margin_bar*i
            )
            .attr('y', (d) -> return svg_height-y(d.num_keys)-margin_height-1) #-1 not to overlap with axe
            .attr('width', bar_width)
            .attr( 'height', (d) -> return y(d.num_keys))
            .attr( 'title', (d) ->
                return "Shard #{d.index + 1}<br />~#{d.num_keys} keys"
            )

        bars.transition()
            .duration(600)
            .attr('x', (d, i) ->
                return margin_width+margin_width_inner+bar_width*i+margin_bar*i
            )
            .attr('y', (d) -> return svg_height-y(d.num_keys)-margin_height-1) #-1 not to overlap with axe
            .attr('width', bar_width)
            .attr( 'height', (d) -> return y(d.num_keys))

        bars.exit().remove()

        # Create axes
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


        extra_container = svg.select('.extra_container')
        lines = extra_container.selectAll('.line').data(extra_data)
        lines.enter().append('line')
            .attr('class', 'line')
            .attr('x1', (d) -> return d.x1)
            .attr('x2', (d) -> return d.x2)
            .attr('y1', (d) -> return d.y1)
            .attr('y2', (d) -> return d.y2)
        lines.exit().remove()

        # Create legend
        axe_legend = []
        axe_legend.push
            x: margin_width
            y: Math.floor(margin_height/2 + 2)
            string: 'Docs'
            anchor: 'middle'
        axe_legend.push
            x: Math.floor(svg_width/2)
            y: svg_height
            string: 'Shards'
            anchor: 'middle'

        legends = extra_container.selectAll('.legend')
            .data(axe_legend)
        legends.enter().append('text')
            .attr('class', 'legend')
            .attr('x', (d) -> return d.x)
            .attr('y', (d) -> return d.y)
            .attr('text-anchor', (d) -> return d.anchor)
            .text((d) -> return d.string)
        legends.exit().remove()

        # Create ticks
        # First the tick's background
        ticks = extra_container.selectAll('.cache').data(y.ticks(5))
        ticks.enter()
            .append('rect')
            .attr('class', 'cache')
            .attr('width', (d, i) ->
                if i is 0
                    return 0 # We don't display 0
                return 4
            )
            .attr('height',18)
            .attr('x', margin_width-2)
            .attr('y', (d) -> svg_height-margin_height-y(d)-14)
            .attr('fill', '#fff')

        ticks.transition()
            .duration(600)
            .attr('y', (d) -> svg_height-margin_height-y(d)-14)
        ticks.exit().remove()

        # Then the actual tick's value
        rules = extra_container.selectAll('.rule').data(y.ticks(5))
        rules.enter()
            .append('text')
            .attr('class', 'rule')
            .attr('x', margin_width)
            .attr('y', (d) -> svg_height-margin_height-y(d))
            .attr('text-anchor', "middle")
            .text((d, i) =>
                if i isnt 0
                    return @prettify_num(d) # We don't display 0
                else
                    return ''
            )
        rules.transition()
            .duration(600)
            .attr('y', (d) -> svg_height-margin_height-y(d))

        rules.exit().remove()

        @$('rect').tooltip
            trigger: 'hover'

module.exports =
    ShardDistribution: ShardDistribution

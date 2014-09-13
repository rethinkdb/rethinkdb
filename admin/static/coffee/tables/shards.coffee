# Copyright 2010-2012 RethinkDB, all rights reserved.
# Namespace view
module 'TableView', ->
    # Hardcoded!
    MAX_SHARD_COUNT = 32

    class @Sharding extends Backbone.View
        className: 'shards_container'
        template:
            main: Handlebars.templates['shards_container-template']
            status: Handlebars.templates['shard_status-template']
            data_repartition: Handlebars.templates['data_repartition-template']

        view_template: Handlebars.templates['view_shards-template']
        edit_template: Handlebars.templates['edit_shards-template']
        feedback_template: Handlebars.templates['edit_shards-feedback-template']
        error_ajax_template: Handlebars.templates['edit_shards-ajax_error-template']
        reasons_cannot_shard_template: Handlebars.templates['shards-reason_cannot_shard-template']


        initialize: (data) =>
            @listenTo @model, 'change:num_available_shards', @render_status
            if @collection?
                @listenTo @collection, 'update', @render_data_repartition

            # Bind listener for the key distribution

            @shard_settings = new TableView.ShardSettings
                model: @model
                container: @
            @progress_bar = new UIComponents.OperationProgressBar @template.status

        set_distribution: (shards) =>
            @collection = shards
            @listenTo @collection, 'update', @render_data_repartition
            @render_data_repartition()


        # Render the status of sharding
        render_status: =>
            # If some shards are not ready, it means some replicas are also not ready
            # In this case the replicas view will call fetch_progress every seconds,
            # so we do need to set an interval to refresh more often
            progress_bar_info =
                got_response: true

            if @model.get('num_available_shards') < @model.get('num_shards')
                if @progress_bar.get_stage() is 'none'
                    @progress_bar.skip_to_processing() # if the stage is 'none', we skipt to processing

            @progress_bar.render(
                @model.get('num_available_shards'),
                @model.get('num_shards'),
                progress_bar_info
            )
            return @

        render: =>
            @$el.html @template.main()
            @$('.edit-shards').html @shard_settings.render().$el

            @$('.shard-status').html @progress_bar.render(
                @model.get('num_available_shards'),
                @model.get('num_shards'),
                {got_response: true}
            ).$el

            @init_chart = false
            setTimeout => # Let the element be inserted in the main DOM tree
                @render_data_repartition()
            , 0

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

        render_data_repartition: =>
            if not @collection?
                return 0

            $('.tooltip').remove()

            @$('.loading').slideUp 'fast'
            @$('.outdated_distribution').slideUp 'fast'
            @$('.shard-diagram').show()

            max_keys = d3.max @collection.models, (shard) -> return shard.get('num_keys')
            min_keys = d3.min @collection.models, (shard) -> return shard.get('num_keys')

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
                    return "Shard: #{d.index}<br />~#{d.num_keys} keys"
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
                y: Math.floor(margin_height/2)
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

        remove: =>
            @stopListening()

    class @ShardSettings extends Backbone.View
        template:
            main: Handlebars.templates['shard_settings-template']
            alert: Handlebars.templates['alert_shard-template']
        events:
            'click .edit': 'toggle_edit'
            'click .cancel': 'toggle_edit'
            'click .rebalance': 'shard_table'
            'keypress #num_shards': 'handle_keypress'

        render: =>
            @$el.html @template.main
                editable: @editable
                num_shards: @model.get 'num_shards'
                max_shards: MAX_SHARD_COUNT #TODO: Put something else?
            @

        initialize: (data) =>
            @editable = false
            @container = data.container

            @listenTo @model, 'change:num_shards', @render

        toggle_edit: =>
            @editable = not @editable
            @render()

            if @editable is true
                @$('#num_shards').select()

        handle_keypress: (event) =>
            if event.which is 13
                @shard_table()

        render_shards_error: (fn) =>
            if @$('.settings_alert').css('display') is 'block'
                @$('.settings_alert').fadeOut 'fast', =>
                    fn()
                    @$('.settings_alert').fadeIn 'fast'
            else
                fn()
                @$('.settings_alert').slideDown 'fast'

        shard_table: =>
            new_num_shards = parseInt @$('#num_shards').val()
            if Math.round(new_num_shards) isnt new_num_shards
                @render_shards_error () =>
                    @$('.settings_alert').html @template.alert
                        not_int: true
                return 1
            if new_num_shards > MAX_SHARD_COUNT
                @render_shards_error () =>
                    @$('.settings_alert').html @template.alert
                        too_many_shards: true
                        num_shards: new_num_shards
                        max_num_shards: MAX_SHARD_COUNT
                return 1
            if new_num_shards < 1
                @render_shards_error () =>
                    @$('.settings_alert').html @template.alert
                        need_at_least_one_shard: true
                return 1


            ignore = (shard) -> shard('role').ne('nothing')
            query = r.db(@model.get('db')).table(@model.get('name')).reconfigure(
                new_num_shards,
                r.db(system_db).table('table_status').get(@model.get('uuid'))('shards').nth(0).filter(ignore).count()
            )
            driver.run query, (error, result) =>
                if error?
                    @render_shards_error () =>
                        @$('.settings_alert').html @template.alert
                            server_error: true
                            error: error
                else
                    @toggle_edit()

                    # Triggers the start on the progress bar
                    @container.progress_bar.render(
                        0,
                        result.shards[0].directors.length,
                        {new_value: result.shards[0].directors.length}
                    )

                    @model.set
                        num_available_shards: 0
                        num_available_replicas: 0
                        num_replicas_per_shard: result.shards[0].directors.length
                        num_replicas: @model.get("num_replicas_per_shard")*result.shards[0].directors.length
            return 0


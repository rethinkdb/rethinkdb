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
            @listenTo @collection, 'update', @render_data_repartition

            # Bind listener for the key distribution

            @shard_settings = new TableView.ShardSettings
                model: @model
                container: @
            @progress_bar = new UIComponents.OperationProgressBar @template.status

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

        render_data_repartition: =>
            if not @collection?
                return 1

            if @collection.length is 1
                pointPadding = 0.3
            else if @collection.length is 1
                pointPadding = 0.2
            else
                pointPadding = 0.1

            if not @chart? or @init_chart is false
                @init_chart = true

                @chart = new Highcharts.Chart
                    chart:
                        type: 'column'
                        renderTo: 'data_repartition-container'
                        height: 350
                        backgroundColor: '#fbfbfb'
                    credits: false
                    title:
                        text: null
                    yAxis:
                        min: 0
                        title:
                            text: 'Number of documents'
                    xAxis:
                        title: null
                        categories: @collection.map((shard, index) -> "Shard #{index+1}")
                    tooltip:
                        headerFormat: '<span style="font-size:10px;display:block;text-align: center">{point.key}</span><table>'
                        pointFormat: '<tr><td style="padding:0"><b>{point.y:,.0f} docs</b></td></tr>'
                        footerFormat: '</table>'
                        shared: true
                        useHTML: true
                    plotOptions:
                        column:
                            pointPadding: pointPadding
                            groupPadding: 0
                            borderWidth: 0
                            color: "#9ad7f2"
                    series: [{ name: 'Results', data: @collection.map((shard, index) -> shard.get('num_keys')) }]
                    legend:
                        enabled: false
            else
                @chart.series[0].setData @collection.map((shard, index) -> shard.get('num_keys'))
                @chart.xAxis[0].update({categories:@collection.map( (shard, index) -> "Shard #{index+1}")}, true)

            if @pointPadding is null
                @pointPadding = pointPadding
            else if @pointPadding isnt pointPadding
                @pointPadding = pointPadding
                @chart.series[0].update {pointPadding: pointPadding}


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


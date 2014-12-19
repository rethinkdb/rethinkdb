# Copyright 2010-2012 RethinkDB, all rights reserved.

# Namespace view
module 'TableView', ->
    class @Replicas extends Backbone.View
        className: 'namespace-replicas'
        template:
            main: Handlebars.templates['replica-template']
            status: Handlebars.templates['replica_status-template']
            alert: Handlebars.templates['replica-alert-template']

        events:
            'click .edit.btn': 'toggle_edit'
            'click .cancel.btn': 'toggle_edit'
            'click .update-replicas.btn': 'update_replicas'
            'keypress #replicas_value': 'handle_keypress'

        initialize: =>
            @editable = false

            @listenTo @model, 'change:num_replicas', @render
            @listenTo @model, 'change:num_available_replicas', @render_status

            @progress_bar = new UIComponents.OperationProgressBar @template.status
            @timer = null

        toggle_edit: =>
            @editable = not @editable
            @render()

            if @editable is true
                @$('#replicas_value').select()

        handle_keypress: (event) =>
            if event.which is 13
                @update_replicas()


        # Execute a fadeOut/fn/fadeIn or fn/slideDown
        render_replicas_error: (fn) =>
            if @$('.settings_alert').css('display') is 'block'
                @$('.settings_alert').fadeOut 'fast', =>
                    fn()
                    @$('.settings_alert').fadeIn 'fast'
            else
                fn()
                @$('.settings_alert').slideDown 'fast'

        update_replicas: =>
            new_num_replicas = parseInt @$('#replicas_value').val()
            if Math.round(new_num_replicas) isnt new_num_replicas
                @render_replicas_error () =>
                    @$('.settings_alert').html @template.alert
                        not_int: true
                return 1
            if new_num_replicas > @model.get 'max_replicas_per_shard'
                @render_replicas_error () =>
                    @$('.settings_alert').html @template.alert
                        too_many_replicas: true
                        num_replicas: new_num_replicas
                        max_num_replicas: @model.get 'max_replicas_per_shard'
                return 1
            if new_num_replicas < 1
                @render_replicas_error () =>
                    @$('.settings_alert').html @template.alert
                        need_at_least_one_replica: true
                    @$('.settings_alert').fadeIn 'fast'
                return 1

            query = r.db(@model.get('db')).table(@model.get('name')).reconfigure(
                r.db(system_db).table('table_config').get(@model.get('id'))('shards').count(),
                new_num_replicas
            )
            driver.run_once query, (error, result) =>
                if error?
                    @render_replicas_error () =>
                        @$('.settings_alert').html @template.alert
                            server_error: true
                            error: error
                        @$('.settings_alert').fadeIn 'fast'
                else
                    @toggle_edit()

                    # Triggers the start on the progress bar
                    @progress_bar.render(
                        @model.get('num_available_replicas'),
                        @model.get('num_replicas'),
                        {new_value: @model.get('num_replicas')}
                    )

                    @model.set
                        num_replicas_per_shard: result.shards[0].replicas.length # = new_num_replicas
                        num_replicas: @model.get("num_shards")*result.shards[0].replicas.length
                        num_available_replicas: 0
                        num_available_shards: 0

            return 0

        fetch_progress: =>
            #Keep ignore in window?
            #We also fetch shards

            driver.run query, (error, result) =>
                if error?
                    # This can happen if the table is temporary unavailable. We log the error, and ignore it
                    console.log "Nothing bad - Could not fetch replicas statuses"
                    console.log error
                else
                    @model.set result
                    @render_status() # Force to refresh the progress bar

        # Render the status of the replicas and the progress bar if needed
        render_status: =>
            #TODO Handle backfilling when available on the API
            if @model.get('num_available_replicas') < @model.get('num_replicas')
                if not @timer?
                    ignore = (shard) -> shard('role').ne('nothing')
                    query =
                        r.db(system_db).table('table_status').get(@model.get('id')).do( (table) ->
                            r.branch(
                                table.eq(null),
                                null,
                                table.merge(
                                    num_replicas: table("shards").concatMap( (shard) -> shard ).filter(ignore).count()
                                    num_available_replicas: table("shards").concatMap( (shard) -> shard ).filter(ignore).filter({state: "ready"}).count()
                                    num_shards: table("shards").count()
                                    num_available_shards: table("shards").concatMap( (shard) -> shard ).filter({role: "director", state: "ready"}).count()
                                )
                            )
                        )

                    @timer = driver.run query, 1000, (error, result) =>
                        if error?
                            # This can happen if the table is temporary unavailable. We log the error, and ignore it
                            console.log "Nothing bad - Could not fetch replicas statuses"
                            console.log error
                        else
                            @model.set result
                            @render_status() # Force to refresh the progress bar

                if @progress_bar.get_stage() is 'none'
                    @progress_bar.skip_to_processing() # if the stage is 'none', we skipt to processing
            else if @model.get('num_available_replicas') is @model.get('num_replicas')
                if @timer?
                    driver.stop_timer @timer
                    @timer = null


            progress_bar_info =
                got_response: true

            @progress_bar.render(
                @model.get('num_available_replicas'),
                @model.get('num_replicas'),
                progress_bar_info
            )

        render: =>
            @$el.html @template.main
                editable: @editable
                max_replicas_per_shard: @model.get 'max_replicas_per_shard'
                num_replicas_per_shard: @model.get 'num_replicas_per_shard'

            if @editable is true
                @$('#replicas_value').select()

            @$('.replica-status').html @progress_bar.render(
                @model.get('num_available_replicas'),
                @model.get('num_replicas'),
                {got_response: true}
            ).$el

            @render_status()
            @

        remove: =>
            if @timer
                driver.stop_timer @timer
                @timer = null
            @progress_bar.remove()
            super()

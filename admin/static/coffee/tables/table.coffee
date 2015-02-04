# Copyright 2010-2012 RethinkDB, all rights reserved.
# Namespace view

module 'TableView', ->
    class @TableContainer extends Backbone.View
        template:
            not_found: Handlebars.templates['element_view-not_found-template']
            error: Handlebars.templates['error-query-template']
        className: 'table-view'
        initialize: (id) =>
            @id = id
            @table_found = true

            @indexes = null
            @distribution = new Distribution

            @guaranteed_timer = null
            @failable_index_timer = null
            @failable_misc_timer = null

            # Initialize the model with mostly empty dummy data so we can render it right away
            @model = new Table {id: id, info_unavailable: false}
            @listenTo @model, 'change:info_unavailable', @maybe_refetch_data
            @table_view = new TableView.TableMainView
                model: @model
                indexes: @indexes
                distribution: @distribution
                shards_assignments: @shards_assignments

            @fetch_data()

        maybe_refetch_data: (obj, info_unavailable) =>
            if info_unavailable is false
                # This is triggered when we go from table.info()
                # failing to it succeeding. If that's the case, we
                # should refetch everything. What happens is we try to
                # fetch indexes every 1s, but more expensive stuff
                # like table.info() only every 10s. To avoid having
                # the table UI in an error state longer than we need
                # to, we use the secondary index query as a canary in
                # the coalmine to decide when to try again with the
                # info queries.
                driver.stop_timer @guaranteed_timer
                driver.stop_timer @failable_index_timer
                driver.stop_timer @failable_misc_timer
                @fetch_data()

        fetch_data: =>
            this_id = @id
            # This query should succeed as long as the cluster is
            # available, since we get it from the system tables.  Info
            # we don't get from the system tables includes things like
            # table/shard counts, or secondary index status. Those are
            # obtained from table.info() and table.indexStatus() below
            # in `failable_*_query` so if they fail we still get the
            # data available in the system tables.
            guaranteed_query =
                r.do(
                    r.db(system_db).table('server_config').coerceTo('array'),
                    r.db(system_db).table('table_status').get(this_id),
                    r.db(system_db).table('table_config').get(this_id),
                    (server_config, table_status, table_config) ->
                        r.branch(
                            table_status.eq(null),
                            null,
                            table_status.merge(
                                max_shards: 32
                                num_shards: table_config("shards").count()
                                num_servers: server_config.count()
                                num_default_servers: server_config.filter((server) ->
                                    server('tags').contains('default')).count()
                                num_available_shards: table_status("shards").count((row) -> row('primary_replica').ne(null))
                                num_replicas: table_config("shards").concatMap( (shard) -> shard('replicas')).count()
                                num_available_replicas: table_status("shards").concatMap((shard) ->
                                    shard('replicas').filter({state: "ready"})).count()
                                num_replicas_per_shard: table_config("shards").map((shard) -> shard('replicas').count()).max()
                                status: table_status('status')
                                id: table_status("id")
                                # These are updated below if the table is ready
                            ).without('shards')
                        )
                    )

            # This query only checks the status of secondary indexes.
            # It can throw an exception and failif the primary
            # replica is unavailable (which happens immediately after
            # a reconfigure).
            failable_index_query = r.do(
                r.db(system_db).table('table_config').get(this_id)
                (table_config) ->
                    r.db(table_config("db"))
                    .table(table_config("name"), {useOutdated: true})
                    .indexStatus()
                    .pluck('index', 'ready', 'blocks_processed', 'blocks_total')
                    .merge( (index) -> {
                        id: index("index")
                        db: table_config("db")
                        table: table_config("name")
                    }) # add an id for backbone
            )

            # Query to load data distribution and shard assignments.
            # We keep this separate from the failable_index_query because we don't want
            # to run it as often.
            # This query can throw an exception and failif the primary
            # replica is unavailable (which happens immediately after
            # a reconfigure). Since this query makes use of
            # table.info(), it's separated from the guaranteed query above.
            failable_misc_query = r.do(
                r.db(system_db).table('table_status').get(this_id),
                r.db(system_db).table('table_config').get(this_id),
                r.db(system_db).table('server_config')
                    .map((x) -> [x('name'), x('id')]).coerceTo('ARRAY').coerceTo('OBJECT'),
                (table_status, table_config, server_name_to_id) ->
                    table_status.merge({
                        distribution: r.db(table_status('db'))
                            .table(table_status('name'), {useOutdated: true})
                            .info()('doc_count_estimates')
                            .map(r.range(), (num_keys, position) ->
                                num_keys: num_keys
                                id: position)
                            .coerceTo('array')
                        total_keys: r.db(table_status('db'))
                            .table(table_status('name'), {useOutdated: true})
                            .info()('doc_count_estimates')
                            .sum()
                        shards_assignments: table_config("shards").map(r.range(),
                            (shard, position) ->
                                id: position.add(1)
                                num_keys: r.db(table_status('db'))
                                    .table(table_status('name'), {useOutdated: true})
                                    .info()('doc_count_estimates')(position)
                                primary:
                                    id: server_name_to_id(shard("primary_replica"))
                                    name: shard("primary_replica")
                                replicas: shard("replicas")
                                    .filter( (replica) -> replica.ne(shard("primary_replica")))
                                    .map (name) ->
                                        id: server_name_to_id(name)
                                        name: name
                        ).coerceTo('array')
                    })
            )

            # This timer keeps track of the failable index query, so we can
            # cancel it when we navigate away from the table page.
            @failable_index_timer = driver.run(
              failable_index_query, 1000, (error, result) =>
                if error?
                    console.log "Shard down"
                    @model.set 'info_unavailable', true
                    return
                @model.set 'info_unavailable', false
                if @indexes?
                    @indexes.set _.map(result, (index) -> new Index index)
                else
                    @indexes = new Indexes _.map result, (index) ->
                        new Index index
                @table_view?.set_indexes @indexes

                @model.set
                    indexes: result
                if !@table_view?
                    @table_view = new TableView.TableMainView
                        model: @model
                        indexes: @indexes
                        distribution: @distribution
                        shards_assignments: @shards_assignments
                    @render()
            )
            # This timer keeps track of the failable misc query, so we can
            # cancel it when we navigate away from the table page.
            @failable_misc_timer = driver.run(
              failable_misc_query, 10000, (error, result) =>
                if error?
                    console.log "Shard down"
                    @model.set 'info_unavailable', true
                    return
                @model.set 'info_unavailable', false
                if @distribution?
                    @distribution.set _.map result.distribution, (shard) ->
                        new Shard shard
                    @distribution.trigger 'update'
                else
                    @distribution = new Distribution _.map(
                        result.distribution, (shard) -> new Shard shard)
                    @table_view?.set_distribution @distribution
                shards_assignments = []
                for shard in result.shards_assignments
                    shards_assignments.push
                        id: "start_shard_#{shard.id}"
                        shard_id: shard.id
                        num_keys: shard.num_keys
                        start_shard: true

                    shards_assignments.push
                        id: "shard_primary_#{shard.id}"
                        primary: true
                        shard_id: shard.id
                        data: shard.primary

                    for replica, position in shard.replicas
                        shards_assignments.push
                            id: "shard_replica_#{shard.id}_#{position}"
                            replica: true
                            replica_position: position
                            shard_id: shard.id
                            data: replica

                    shards_assignments.push
                        id: "end_shard_#{shard.id}"
                        shard_id: shard.id
                        end_shard: true

                if @shards_assignments?
                    @shards_assignments.set _.map shards_assignments, (shard) ->
                        new ShardAssignment shard
                else
                    @shards_assignments = new ShardAssignments(
                        _.map shards_assignments,
                            (shard) -> new ShardAssignment shard
                    )
                    @table_view?.set_assignments @shards_assignments

                @model.set result
                if !@table_view?
                    @table_view = new TableView.TableMainView
                        model: @model
                        indexes: @indexes
                        distribution: @distribution
                        shards_assignments: @shards_assignments
                    @render()
            )
            # This timer keeps track of the guaranteed query, running
            # it every 5 seconds. We cancel it when navigating away
            # from the table page.
            @guaranteed_timer = driver.run guaranteed_query, 5000, (error, result) =>
                if error?
                    # TODO: We may want to render only if we failed to open a connection
                    # TODO: Handle when the table is deleted
                    @error = error
                    @render()
                else
                    rerender = @error?
                    @error = null
                    if result is null
                        rerender = rerender or @table_found
                        @table_found = false
                        # Reset the data
                        @indexes = null
                        @render()
                    else
                        rerender = rerender or not @table_found
                        @table_found = true
                        @model.set result

                    if rerender
                        @render()

        render: =>
            if @error?
                @$el.html @template.error
                    error: @error?.message
                    url: '#tables/'+@id
            else
                if @table_found
                    @$el.html @table_view.render().$el
                else # In this case, the query returned null, so the table was not found
                    @$el.html @template.not_found
                        id: @id
                        type: 'table'
                        type_url: 'tables'
                        type_all_url: 'tables'
            @
        remove: =>
            driver.stop_timer @guaranteed_timer
            driver.stop_timer @failable_index_timer
            driver.stop_timer @failable_misc_timer
            @table_view?.remove()
            super()

    class @TableMainView extends Backbone.View
        className: 'namespace-view'
        template:
            main: Handlebars.templates['table_container-template']
            alert: Handlebars.templates['modify_shards-alert-template']

        events:
            'click .close': 'close_alert'
            'click .change_shards-link': 'change_shards'
            'click .operations .rename': 'rename_table'
            'click .operations .delete': 'delete_table'


        initialize: (data, options) =>
            @indexes = data.indexes
            @distribution = data.distribution
            @shards_assignments = data.shards_assignments

            # Panels for namespace view
            @title = new TableView.Title
                model: @model
            @profile = new TableView.Profile
                model: @model

            @secondary_indexes_view = new TableView.SecondaryIndexesView
                collection: @indexes
                model: @model
            @shard_distribution = new TableView.ShardDistribution
                collection: @distribution
                model: @model
            @server_assignments = new TableView.ShardAssignmentsView
                model: @model
                collection: @shards_assignments
            @reconfigure = new TableView.ReconfigurePanel
                model: @model

            @stats = new Stats
            @stats_timer = driver.run(
                r.db(system_db).table('stats')
                .get(["table", @model.get('id')])
                .do((stat) ->
                    keys_read: stat('query_engine')('read_docs_per_sec')
                    keys_set: stat('query_engine')('written_docs_per_sec')
                ), 1000, @stats.on_result)

            @performance_graph = new Vis.OpsPlot(@stats.get_stats,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'table'
            )

        set_indexes: (indexes) =>
            if not @indexes?
                @indexes = indexes
                @secondary_indexes_view.set_indexes @indexes

        set_distribution: (distribution) =>
            @distribution = distribution
            @shard_distribution.set_distribution @distribution

        set_assignments: (shards_assignments) =>
            @shards_assignments = shards_assignments
            @server_assignments.set_assignments @shards_assignments

        render: =>
            @$el.html @template.main
                namespace_id: @model.get 'id'

            # fill the title of this page
            @$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @$('.profile').html @profile.render().$el

            @$('.performance-graph').html @performance_graph.render().$el

            # Display the shards
            @$('.sharding').html @shard_distribution.render().el

            # Display the server assignments
            @$('.server-assignments').html @server_assignments.render().el

            # Display the secondary indexes
            @$('.secondary_indexes').html @secondary_indexes_view.render().el

            # Display server reconfiguration
            @$('.reconfigure-panel').html @reconfigure.render().el

            @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        change_shards: (event) =>
            event.preventDefault()
            @$('#namespace-sharding-link').tab('show')

        change_pinning: (event) =>
            event.preventDefault()
            @$('#namespace-pinning-link').tab('show')
            $(event.currentTarget).parent().parent().slideUp('fast', -> $(this).remove())

        # Rename operation
        rename_table: (event) =>
            event.preventDefault()
            if @rename_modal?
                @rename_modal.remove()
            @rename_modal = new UIComponents.RenameItemModal
                model: @model
            @rename_modal.render()

        # Delete operation
        delete_table: (event) ->
            event.preventDefault()
            if @remove_table_dialog?
                @remove_table_dialog.remove()
            @remove_table_dialog = new Modals.RemoveTableModal

            @remove_table_dialog.render [{
                table: @model.get 'name'
                database: @model.get 'db'
            }]


        remove: =>
            @title.remove()
            @profile.remove()
            @shard_distribution.remove()
            @server_assignments.remove()
            @performance_graph.remove()
            @secondary_indexes_view.remove()
            @reconfigure.remove()

            driver.stop_timer @stats_timer

            if @remove_table_dialog?
                @remove_table_dialog.remove()

            if @rename_modal?
                @rename_modal.remove()
            super()

    # TableView.ReconfigurePanel
    class @ReconfigurePanel extends Backbone.View
        className: 'reconfigure-panel'
        templates:
            main: Handlebars.templates['reconfigure']
            status: Handlebars.templates['replica_status-template']
        events:
            'click .reconfigure.btn': 'launch_modal'

        initialize: (obj) =>
            @model = obj.model
            @listenTo @model, 'change:num_shards', @render
            @listenTo @model, 'change:num_replicas_per_shard', @render
            @listenTo @model, 'change:num_available_replicas', @render_status
            @progress_bar = new UIComponents.OperationProgressBar @templates.status
            @timer = null

        launch_modal: =>
            if @reconfigure_modal?
                @reconfigure_modal.remove()
            @reconfigure_modal = new Modals.ReconfigureModal
                model: new Reconfigure
                    parent: @
                    id: @model.get('id')
                    db: @model.get('db')
                    name: @model.get('name')
                    total_keys: @model.get('total_keys')
                    shards: []
                    max_shards: @model.get('max_shards')
                    num_shards: @model.get('num_shards')
                    num_servers: @model.get('num_servers')
                    num_default_servers: @model.get('num_default_servers')
                    num_replicas_per_shard: @model.get('num_replicas_per_shard')
            @reconfigure_modal.render()

        remove: =>
            if @reconfigure_modal?
                @reconfigure_modal.remove()
            if @timer?
                driver.stop_timer @timer
                @timer = null
            @progress_bar.remove()
            super()

        fetch_progress: =>
            query = r.db(system_db).table('table_status')
                .get(@model.get('id'))('shards')('replicas').concatMap((x) -> x)
                .do((replicas) ->
                    num_available_replicas: replicas.filter(state: 'ready').count()
                    num_replicas: replicas.count()
                )
            if @timer?
                driver.stop_timer @timer
                @timer = null
            @timer = driver.run query, 1000, (error, result) =>
                if error?
                    # This can happen if the table is temporarily
                    # unavailable. We log the error, and ignore it
                    console.log "Nothing bad - Could not fetch replicas statuses"
                    console.log error
                else
                    @model.set result
                    @render_status()  # Force to refresh the progress bar

        # Render the status of the replicas and the progress bar if needed
        render_status: =>
            #TODO Handle backfilling when available in the api directly
            if @model.get('num_available_replicas') < @model.get('num_replicas')
                if not @timer?
                    @fetch_progress()
                    return

                if @progress_bar.get_stage() is 'none'
                    @progress_bar.skip_to_processing()

            else
                if @timer?
                    driver.stop_timer @timer
                    @timer = null

            @progress_bar.render(
                @model.get('num_available_replicas'),
                @model.get('num_replicas'),
                {got_response: true}
            )

        render: =>
            @$el.html @templates.main @model.toJSON()
            if @model.get('num_available_replicas') < @model.get('num_replicas')
                if @progress_bar.get_stage() is 'none'
                    @progress_bar.skip_to_processing()
            @$('.backfill-progress').html @progress_bar.render(
                @model.get('num_available_replicas'),
                @model.get('num_replicas'),
                {got_response: true},
            ).$el
            @

    # TableView.ReconfigureDiffView
    class @ReconfigureDiffView extends Backbone.View
        # This is just for the diff view in the reconfigure
        # modal. It's so that the diff can be rerendered independently
        # of the modal itself (which includes the input form). If you
        # rerended the entire modal, you lose focus in the text boxes
        # since handlebars creates an entirely new dom tree.
        #
        # You can find the ReconfigureModal in coffee/modals.coffee

        className: 'reconfigure-diff'
        template: Handlebars.templates['reconfigure-diff']
        initialize: =>
            @listenTo @model, 'change:shards', @render

        render: =>
            @$el.html @template @model.toJSON()
            @

    # TableView.Title
    class @Title extends Backbone.View
        className: 'namespace-info-view'
        template: Handlebars.templates['table_title-template']
        initialize: =>
            @listenTo @model, 'change:name', @render

        render: =>
            @$el.html @template
                name: @model.get 'name'
                db: @model.get 'db'
            @

        remove: =>
            @stopListening()
            super()

    # Profile view
    class @Profile extends Backbone.View
        template: Handlebars.templates['table_profile-template']

        initialize: =>
            @listenTo @model, 'change', @render

        render: =>
            @$el.html @template
                status: @model.get 'status'
                total_keys: @model.get 'total_keys'
                num_shards: @model.get 'num_shards'
                num_available_shards: @model.get 'num_available_shards'
                num_replicas: @model.get 'num_replicas'
                num_available_replicas: @model.get 'num_available_replicas'
            return @

        remove: =>
            @stopListening()
            super()

    class @SecondaryIndexesView extends Backbone.View
        template: Handlebars.templates['table-secondary_indexes-template']
        alert_message_template: Handlebars.templates['secondary_indexes-alert_msg-template']
        error_template: Handlebars.templates['secondary_indexes-error-template']
        events:
            'click .create_link': 'show_add_index'
            'click .create_btn': 'create_index'
            'keydown .new_index_name': 'handle_keypress'
            'click .cancel_btn': 'hide_add_index'
            'click .reconnect_link': 'init_connection'
            'click .close_hide': 'hide_alert'
        error_interval: 5*1000 # In case of an error, we try to retrieve the secondary index in 5 seconds
        normal_interval: 10*1000 # Retrieve secondary indexes every 10 seconds
        short_interval: 1000 # Interval when an index is being created

        initialize: (data) =>
            @indexes_view = []
            @interval_progress = null
            @collection = data.collection
            @model = data.model

            @adding_index = false

            @listenTo @model, 'change:info_unavailable', @set_warnings

            @render()
            @hook() if @collection?

        set_warnings:  ->
            if @model.get('info_unavailable')
                @$('.unavailable-error').show()
            else
                @$('.unavailable-error').hide()

        set_fetch_progress: (index) =>
            if not @interval_progress?
                @fetch_progress()
                @interval_progress = setInterval @fetch_progress, 1000

        fetch_progress: =>
            build_in_progress = false
            for index in @collection.models
                if index.get('ready') isnt true
                    build_in_progress = true
                    break

            if build_in_progress and @model.get('db')?
                query = r.db(@model.get('db')).table(@model.get('name')).indexStatus()
                    .pluck('index', 'ready', 'blocks_processed', 'blocks_total')
                    .merge( (index) => {
                        id: index("index")
                        db: @model.get("db")
                        table: @model.get("name")
                    })

                driver.run_once query, (error, result) =>
                    if error?
                        # This can happen if the table is temporarily
                        # unavailable. We log the error, and ignore it
                        console.log "Nothing bad - Could not fetch secondary indexes statuses"
                        console.log error
                    else
                        all_ready = true
                        for index in result
                            if index.ready isnt true
                                all_ready = false
                            @collection.add new Index index, {merge: true}
                        if all_ready is true
                            clearInterval @interval_progress
                            @interval_progress = null
            else
                clearInterval @interval_progress
                @interval_progress = null


        set_indexes: (indexes) =>
            @collection = indexes
            @hook()

        hook: =>
            @collection.each (index) =>
                view = new TableView.SecondaryIndexView
                    model: index
                    container: @
                # The first time, the collection is sorted
                @indexes_view.push view
                @$('.list_secondary_indexes').append view.render().$el
            if @collection.length is 0
                @$('.no_index').show()
            else
                @$('.no_index').hide()

            @listenTo @collection, 'add', (index) =>
                new_view = new TableView.SecondaryIndexView
                    model: index
                    container: @

                if @indexes_view.length is 0
                    @indexes_view.push new_view
                    @$('.list_secondary_indexes').html new_view.render().$el
                else
                    added = false
                    for view, position in @indexes_view
                        if view.model.get('index') > index.get('index')
                            added = true
                            @indexes_view.splice position, 0, new_view
                            if position is 0
                                @$('.list_secondary_indexes').prepend new_view.render().$el
                            else
                                @$('.index_container').eq(position-1).after new_view.render().$el
                            break
                    if added is false
                        @indexes_view.push new_view
                        @$('.list_secondary_indexes').append new_view.render().$el

                if @indexes_view.length is 1
                    @$('.no_index').hide()

            @listenTo @collection, 'remove', (index) =>
                for view in @indexes_view
                    if view.model is index
                        index.destroy()
                        ((view) =>
                            view.remove()
                            @indexes_view.splice(@indexes_view.indexOf(view), 1)
                        )(view)
                        break
                if @collection.length is 0
                    @$('.no_index').show()

        render_error: (args) =>
            @$('.alert_error_content').html @error_template args
            @$('.main_alert_error').show()
            @$('.main_alert').hide()

        render_feedback: (args) =>
            if @$('.main_alert').css('display') is 'none'
                @$('.alert_content').html @alert_message_template args
                @$('.main_alert').show()
            else
                @$('.main_alert').fadeOut 'fast', =>
                    @$('.alert_content').html @alert_message_template args
                    @$('.main_alert').fadeIn 'fast'
            @$('.main_alert_error').hide()

        render: =>
            @$el.html @template
                adding_index: @adding_index
            @set_warnings()
            return @

        # Show the form to add a secondary index
        show_add_index: (event) =>
            event.preventDefault()
            @$('.add_index_li').show()
            @$('.create_container').hide()
            @$('.new_index_name').focus()

        # Hide the form to add a secondary index
        hide_add_index: =>
            @$('.add_index_li').hide()
            @$('.create_container').show()
            @$('.new_index_name').val ''

        # We catch enter and esc when the user is writing a secondary index name
        handle_keypress: (event) =>
            if event.which is 13 # Enter
                event.preventDefault()
                @create_index()
            else if event.which is 27 # ESC
                event.preventDefault()
                @hide_add_index()

        on_fail_to_connect: =>
            @render_error
                connect_fail: true
            return @

        create_index: =>
            @$('.create_btn').prop 'disabled', 'disabled'
            @$('.cancel_btn').prop 'disabled', 'disabled'

            index_name = @$('.new_index_name').val()
            ((index_name) =>
                query = r.db(@model.get('db')).table(@model.get('name')).indexCreate(index_name)
                driver.run_once query, (error, result) =>
                    @$('.create_btn').prop 'disabled', false
                    @$('.cancel_btn').prop 'disabled', false
                    that = @
                    if error?
                        @render_error
                            create_fail: true
                            message: error.msg.replace('\n', '<br/>')
                    else
                        @collection.add new Index
                            id: index_name
                            index: index_name
                            db: @model.get 'db'
                            table: @model.get 'name'
                        @render_feedback
                            create_ok: true
                            name: index_name

                        @hide_add_index()
            )(index_name)

        # Hide alert BUT do not remove it
        hide_alert: (event) ->
            if event? and @$(event.target)?.data('name')?
                @deleting_secondary_index = null
            event.preventDefault()
            $(event.target).parent().hide()

        remove: =>
            @stopListening()
            for view in @indexes_view
                view.remove()
            super()

    class @SecondaryIndexView extends Backbone.View
        template: Handlebars.templates['table-secondary_index-template']
        progress_template: Handlebars.templates['simple_progressbar-template']
        events:
            'click .delete_link': 'confirm_delete'
            'click .delete_index_btn': 'delete_index'
            'click .cancel_delete_btn': 'cancel_delete'

        tagName: 'li'
        className: 'index_container'

        initialize: (data) =>
            @container = data.container

            @model.on 'change:blocks_processed', @update
            @model.on 'change:ready', @update

        update: (args) =>
            if @model.get('ready') is false
                @render_progress_bar()
            else
                if @progress_bar?
                    @render_progress_bar()
                else
                    @render()

        render: =>
            @$el.html @template
                is_empty: @model.get('name') is ''
                name: @model.get 'index'
                ready: @model.get 'ready'

            if @model.get('ready') is false
                @render_progress_bar()
                @container.set_fetch_progress()
            @

        render_progress_bar: =>
            blocks_processed = @model.get 'blocks_processed'
            blocks_total = @model.get 'blocks_total'

            if @progress_bar?
                if @model.get('ready') is true
                    @progress_bar.render 100, 100,
                        got_response: true
                        check: true
                        , => @render()
                else
                    @progress_bar.render blocks_processed, blocks_total,
                        got_response: true
                        check: true
                        , => @render()
            else
                @progress_bar = new UIComponents.OperationProgressBar @progress_template
                @$('.progress_li').html @progress_bar.render(0, Infinity, {new_value: true, check: true}).$el

        # Show a confirmation before deleting a secondary index
        confirm_delete: (event) =>
            event.preventDefault()
            @$('.alert_confirm_delete').show()

        delete_index: =>
            @$('.btn').prop 'disabled', 'disabled'
            query = r.db(@model.get('db')).table(@model.get('table')).indexDrop(@model.get('index'))
            driver.run_once query, (error, result) =>
                @$('.btn').prop 'disabled', false
                if error?
                    @container.render_error
                        delete_fail: true
                        message: error.msg.replace('\n', '<br/>')
                else if result.dropped is 1
                    @container.render_feedback
                        delete_ok: true
                        name: @model.get('index')
                    @model.destroy()
                else
                    @container.render_error
                        delete_fail: true
                        message: "Result was not {dropped: 1}"

        # Close to hide_alert, but the way to reach the alert is slightly different than with the x link
        cancel_delete: ->
            @$('.alert_confirm_delete').hide()

        remove: =>
            @progress_bar?.remove()
            super()

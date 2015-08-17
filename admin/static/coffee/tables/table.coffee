# Copyright 2010-2015 RethinkDB

models = require('../models.coffee')
modals = require('../modals.coffee')
shard_assignments = require('./shard_assignments.coffee')
shard_distribution = require('./shard_distribution.coffee')
vis = require('../vis.coffee')
ui_modals = require('../ui_components/modals.coffee')
ui_progress = require('../ui_components/progressbar.coffee')
app = require('../app.coffee')
driver = app.driver
system_db = app.system_db

r = require('rethinkdb')

class TableContainer extends Backbone.View
    template:
        not_found: require('../../handlebars/element_view-not_found.hbs')
        error: require('../../handlebars/error-query.hbs')
    className: 'table-view'
    initialize: (id) =>
        @id = id
        @table_found = true

        @indexes = null
        @distribution = new models.Distribution

        @guaranteed_timer = null
        @failable_index_timer = null
        @failable_misc_timer = null

        # This is the cache of error messages we've received from
        # @failable_index_query and @failable_misc_query
        @current_errors = {index: [], misc: []}

        # Initialize the model with mostly empty dummy data so we
        # can render it right away
        #
        # We have two types of errors that can trip us
        # up. info_unavailable errors happen when a you can't
        # fetch table.info(). index_unavailable errors happen when
        # you can't fetch table.indexStatus()
        @model = new models.Table
            id: id
            info_unavailable: false
            index_unavailable: false
        @listenTo @model, 'change:index_unavailable', (=> @maybe_refetch_data())
        @table_view = new TableMainView
            model: @model
            indexes: @indexes
            distribution: @distribution
            shards_assignments: @shards_assignments

        @fetch_data()

    maybe_refetch_data: =>
        if @model.get('info_unavailable') and not @model.get('index_unavailable')
            # This is triggered when we go from
            # table.indexStatus() failing to it succeeding. If
            # that's the case, we should refetch everything. What
            # happens is we try to fetch indexes every 1s, but
            # more expensive stuff like table.info() only every
            # 10s. To avoid having the table UI in an error state
            # longer than we need to, we use the secondary index
            # query as a canary in the coalmine to decide when to
            # try again with the info queries.

            @fetch_once()

    fetch_once: _.debounce((=>
        # fetches data once, but only does it if we flap less than
        # once every 2 seconds
        driver.run_once(@failable_misc_query, @failable_misc_handler)
        driver.run_once(@guaranteed_query, @guaranted_handler)
        ), 2000, true)

    add_error_message: (type, error) =>
        # This avoids spamming the console with the same error
        # message. It keeps a cache of messages, prints out
        # anything new, and resets the cache once we stop
        # receiving errors
        if error not in @current_errors[type]
            console.log "#{type} error: #{error}"
            @current_errors[type].push(error)

    clear_cached_error_messages: (type) =>
        # We clear the cache of errors once a successful query
        # makes it through
        if @current_errors[type].length > 0
            @current_errors[type] = []

    guaranteed_query: =>
        # This query should succeed as long as the cluster is
        # available, since we get it from the system tables.  Info
        # we don't get from the system tables includes things like
        # table/shard counts, or secondary index status. Those are
        # obtained from table.info() and table.indexStatus() below
        # in `failable_*_query` so if they fail we still get the
        # data available in the system tables.
        r.do(
            r.db(system_db).table('server_config').coerceTo('array'),
            r.db(system_db).table('table_status').get(@id),
            r.db(system_db).table('table_config').get(@id),
            (server_config, table_status, table_config) ->
                r.branch(
                    table_status.eq(null),
                    null,
                    table_status.merge(
                        max_shards: 32
                        num_shards: table_config("shards").count().default(0)
                        num_servers: server_config.count()
                        num_default_servers: server_config.filter((server) ->
                            server('tags').contains('default')).count()
                        num_primary_replicas:
                            table_status("shards").count(
                                (row) -> row('primary_replicas').isEmpty().not())
                                .default(0)
                        num_replicas: table_status("shards").default([]).map((shard) ->
                            shard('replicas').count()).sum()
                        num_available_replicas: table_status("shards").concatMap(
                            (shard) ->
                                shard('replicas').filter({state: "ready"}))
                                .count().default(0)
                        num_replicas_per_shard: table_config("shards").default([]).map(
                            (shard) -> shard('replicas').count()).max().default(0)
                        status: table_status('status')
                        id: table_status("id")
                        # These are updated below if the table is ready
                    ).without('shards')
                )
            )

    failable_index_query: =>
        # This query only checks the status of secondary indexes.
        # It can throw an exception and failif the primary
        # replica is unavailable (which happens immediately after
        # a reconfigure).
        r.do(
            r.db(system_db).table('table_config').get(@id)
            (table_config) ->
                r.db(table_config("db"))
                .table(table_config("name"), read_mode: "single")
                .indexStatus()
                .pluck('index', 'ready', 'blocks_processed', 'blocks_total')
                .merge( (index) -> {
                    id: index("index")
                    db: table_config("db")
                    table: table_config("name")
                }) # add an id for backbone
        )

    failable_misc_query: =>
        # Query to load data distribution and shard assignments.
        # We keep this separate from the failable_index_query because we don't want
        # to run it as often.
        # This query can throw an exception and failif the primary
        # replica is unavailable (which happens immediately after
        # a reconfigure). Since this query makes use of
        # table.info(), it's separated from the guaranteed query
        r.do(
            r.db(system_db).table('table_status').get(@id),
            r.db(system_db).table('table_config').get(@id),
            r.db(system_db).table('server_config')
                .map((x) -> [x('name'), x('id')]).coerceTo('ARRAY').coerceTo('OBJECT'),
            (table_status, table_config, server_name_to_id) ->
                table_status.merge({
                    distribution: r.db(table_status('db'))
                        .table(table_status('name'), read_mode: "single")
                        .info()('doc_count_estimates')
                        .map(r.range(), (num_keys, position) ->
                            num_keys: num_keys
                            id: position)
                        .coerceTo('array')
                    total_keys: r.db(table_status('db'))
                        .table(table_status('name'), read_mode: "single")
                        .info()('doc_count_estimates')
                        .sum()
                    shard_assignments: table_status.merge((table_status) ->
                        table_status('shards').default([]).map(
                            table_config('shards').default([]),
                            r.db(table_status('db'))
                                .table(table_status('name'), read_mode: "single")
                                .info()('doc_count_estimates'),
                            (status, config, estimate) ->
                                num_keys: estimate,
                                replicas: status('replicas').merge((replica) ->
                                    id: server_name_to_id(replica('server')).default(null),
                                    configured_primary: config('primary_replica').eq(replica('server'))
                                    currently_primary: status('primary_replicas').contains(replica('server'))
                                    nonvoting: config('nonvoting_replicas').contains(replica('server'))
                                ).orderBy(r.desc('currently_primary'), r.desc('configured_primary'))
                        )
                    )
                })
        )

    handle_failable_index_response: (error, result) =>
        # Handle the result of failable_index_query
        if error?
            @add_error_message('index', error.msg)
            @model.set 'index_unavailable', true
            return
        @model.set 'index_unavailable', false
        @clear_cached_error_messages('index')
        if @indexes?
            @indexes.set _.map(result, (index) -> new models.Index index)
        else
            @indexes = new models.Indexes _.map result, (index) ->
                new models.Index index
        @table_view?.set_indexes @indexes

        @model.set
            indexes: result
        if !@table_view?
            @table_view = new TableMainView
                model: @model
                indexes: @indexes
                distribution: @distribution
                shards_assignments: @shards_assignments
            @render()

    handle_failable_misc_response: (error, result) =>
        if error?
            @add_error_message('misc', error.msg)
            @model.set 'info_unavailable', true
            return
        @model.set 'info_unavailable', false
        @clear_cached_error_messages('misc')
        if @distribution?
            @distribution.set _.map result.distribution, (shard) ->
                new models.Shard shard
            @distribution.trigger 'update'
        else
            @distribution = new models.Distribution _.map(
                result.distribution, (shard) -> new models.Shard shard)
            @table_view?.set_distribution @distribution

        if @shards_assignments?
            @shards_assignments.set _.map result.shard_assignments, (shard) ->
                new models.ShardAssignment shard
        else
            @shards_assignments = new models.ShardAssignments(
                _.map result.shard_assignments,
                    (shard) -> new models.ShardAssignment shard
            )
            @table_view?.set_assignments @shards_assignments

        @model.set result
        if !@table_view?
            @table_view = new TableMainView
                model: @model
                indexes: @indexes
                distribution: @distribution
                shards_assignments: @shards_assignments
            @render()

    handle_guaranteed_response: (error, result) =>
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

    fetch_data: =>
        # This timer keeps track of the failable index query, so we can
        # cancel it when we navigate away from the table page.
        @failable_index_timer = driver.run(
            @failable_index_query(),
            1000,
            @handle_failable_index_response
        )
        # This timer keeps track of the failable misc query, so we can
        # cancel it when we navigate away from the table page.
        @failable_misc_timer = driver.run(
            @failable_misc_query(),
            10000,
            @handle_failable_misc_response,
        )

        # This timer keeps track of the guaranteed query, running
        # it every 5 seconds. We cancel it when navigating away
        # from the table page.
        @guaranteed_timer = driver.run(
            @guaranteed_query(),
            5000,
            @handle_guaranteed_response,
        )

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

class TableMainView extends Backbone.View
    className: 'namespace-view'
    template:
        main: require('../../handlebars/table_container.hbs')
        alert: null # unused, delete this

    events:
        'click .close': 'close_alert'
        'click .operations .rename': 'rename_table'
        'click .operations .delete': 'delete_table'


    initialize: (data, options) =>
        @indexes = data.indexes
        @distribution = data.distribution
        @shards_assignments = data.shards_assignments

        # Panels for namespace view
        @title = new Title
            model: @model
        @profile = new Profile
            model: @model

        @secondary_indexes_view = new SecondaryIndexesView
            collection: @indexes
            model: @model
        @shard_distribution = new shard_distribution.ShardDistribution
            collection: @distribution
            model: @model
        @server_assignments = new shard_assignments.ShardAssignmentsView
            model: @model
            collection: @shards_assignments
        @reconfigure = new ReconfigurePanel
            model: @model

        @stats = new models.Stats
        @stats_timer = driver.run(
            r.db(system_db).table('stats')
            .get(["table", @model.get('id')])
            .do((stat) ->
                keys_read: stat('query_engine')('read_docs_per_sec')
                keys_set: stat('query_engine')('written_docs_per_sec')
            ), 1000, @stats.on_result)

        @performance_graph = new vis.OpsPlot(@stats.get_stats,
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

    # Rename operation
    rename_table: (event) =>
        event.preventDefault()
        if @rename_modal?
            @rename_modal.remove()
        @rename_modal = new ui_modals.RenameItemModal
            model: @model
        @rename_modal.render()

    # Delete operation
    delete_table: (event) ->
        event.preventDefault()
        if @remove_table_dialog?
            @remove_table_dialog.remove()
        @remove_table_dialog = new modals.RemoveTableModal

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

class ReconfigurePanel extends Backbone.View
    className: 'reconfigure-panel'
    templates:
        main: require('../../handlebars/reconfigure.hbs')
        status: require('../../handlebars/replica_status.hbs')
    events:
        'click .reconfigure.btn': 'launch_modal'

    initialize: (obj) =>
        @model = obj.model
        @listenTo @model, 'change:num_shards', @render
        @listenTo @model, 'change:num_replicas_per_shard', @render
        @listenTo @model, 'change:num_available_replicas', @render_status
        @progress_bar = new ui_progress.OperationProgressBar @templates.status
        @timer = null

    launch_modal: =>
        if @reconfigure_modal?
            @reconfigure_modal.remove()
        @reconfigure_modal = new modals.ReconfigureModal
            model: new models.Reconfigure
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
            .get(@model.get('id'))('shards').default([])('replicas')
            .concatMap((x) -> x)
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


class ReconfigureDiffView extends Backbone.View
    # This is just for the diff view in the reconfigure
    # modal. It's so that the diff can be rerendered independently
    # of the modal itself (which includes the input form). If you
    # rerended the entire modal, you lose focus in the text boxes
    # since handlebars creates an entirely new dom tree.
    #
    # You can find the ReconfigureModal in coffee/modals.coffee

    className: 'reconfigure-diff'
    template: require('../../handlebars/reconfigure-diff.hbs')
    initialize: =>
        @listenTo @model, 'change:shards', @render

    render: =>
        @$el.html @template @model.toJSON()
        @

class Title extends Backbone.View
    className: 'namespace-info-view'
    template: require('../../handlebars/table_title.hbs')
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
class Profile extends Backbone.View
    template: require('../../handlebars/table_profile.hbs')

    initialize: =>
        @listenTo @model, 'change', @render
        @indicator = new TableStatusIndicator
            model: @model

    render: =>
        obj = {
            status: @model.get 'status'
            total_keys: @model.get 'total_keys'
            num_shards: @model.get 'num_shards'
            num_primary_replicas: @model.get 'num_primary_replicas'
            num_replicas: @model.get 'num_replicas'
            num_available_replicas: @model.get 'num_available_replicas'
        }

        if obj.status?.ready_for_writes and not obj?.status.all_replicas_ready
            obj.parenthetical = true

        @$el.html @template obj
        @$('.availability').prepend(@indicator.$el)

        return @
    remove: =>
        @indicator.remove()
        super()

class TableStatusIndicator extends Backbone.View
    # This is an svg element showing the table's status in the
    # appropriate color. Unlike a lot of other views, its @$el is
    # set to the svg element from the template, rather than being
    # wrapped.
    template: require('../../handlebars/table_status_indicator.hbs')

    initialize: =>
        status_class = @status_to_class(@model.get('status'))
        @setElement(@template(status_class: status_class))
        @render()
        @listenTo @model, 'change:status', @render

    status_to_class: (status) =>
        if not status
            return "unknown"
        # ensure this matches "humanize_table_readiness" in util.coffee
        if status.all_replicas_ready
            return "ready"
        else if status.ready_for_writes
            return "ready-but-with-caveats"
        else
            return "not-ready"

    render: =>
        # We don't re-render the template, just update the class of the svg
        stat_class = @status_to_class(@model.get('status'))
        # .addClass and .removeClass don't work on <svg> elements
        @$el.attr('class', "ionicon-table-status #{stat_class}")



class SecondaryIndexesView extends Backbone.View
    template: require('../../handlebars/table-secondary_indexes.hbs')
    alert_message_template: require('../../handlebars/secondary_indexes-alert_msg.hbs')
    error_template: require('../../handlebars/secondary_indexes-error.hbs')
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

        @listenTo @model, 'change:index_unavailable', @set_warnings

        @render()
        @hook() if @collection?

    set_warnings:  ->
        if @model.get('index_unavailable')
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
                        @collection.add new models.Index index, {merge: true}
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
            view = new SecondaryIndexView
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
            new_view = new SecondaryIndexView
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
                    @collection.add new models.Index
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

class SecondaryIndexView extends Backbone.View
    template: require('../../handlebars/table-secondary_index.hbs')
    progress_template: require('../../handlebars/simple_progressbar.hbs')
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
            @progress_bar = new ui_progress.OperationProgressBar @progress_template
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


exports.TableContainer = TableContainer
exports.TableMainView = TableMainView
exports.ReconfigurePanel = ReconfigurePanel
exports.ReconfigureDiffView = ReconfigureDiffView
exports.Title = Title
exports.Profile = Profile
exports.TableStatusIndicator = TableStatusIndicator
exports.SecondaryIndexesView = SecondaryIndexesView
exports.SecondaryIndexView = SecondaryIndexView

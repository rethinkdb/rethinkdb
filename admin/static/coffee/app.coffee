# Copyright 2010-2012 RethinkDB, all rights reserved.

system_db = 'rethinkdb'

r = require('rethinkdb')

is_disconnected = null

class Driver
    constructor: (args) ->
        if window.location.port is ''
            if window.location.protocol is 'https:'
                port = 443
            else
                port = 80
        else
            port = parseInt window.location.port
        @server =
            host: window.location.hostname
            port: port
            protocol: if window.location.protocol is 'https:' then 'https' else 'http'
            pathname: window.location.pathname

        @hack_driver()

        @state = 'ok'
        @timers = {}
        @index = 0

    # Hack the driver: remove .run() and add private_run()
    # We want run() to throw an error, in case a user write .run() in a query.
    # We'll internally run a query with the method `private_run`
    hack_driver: =>
        TermBase = r.expr(1).constructor.__super__.constructor.__super__
        if not TermBase.private_run?
            that = @
            TermBase.private_run = TermBase.run
            TermBase.run = ->
                throw new Error("You should remove .run() from your queries when using the Data Explorer.\nThe query will be built and sent by the Data Explorer itself.")

    # Open a connection to the server
    connect: (callback) ->
        r.connect @server, callback

    # Close a connection
    close: (conn) ->
        conn.close noreplyWait: false

    # Run a query once
    run_once: (query, callback) =>
        @connect (error, connection) =>
            if error?
                # If we cannot open a connection, we blackout the whole interface
                # And do not call the callback
                if is_disconnected?
                    if @state is 'ok'
                        is_disconnected.display_fail()
                else
                    is_disconnected = new body.IsDisconnected
                @state = 'fail'
            else
                if @state is 'fail'
                    # Force refresh
                    window.location.reload true
                else
                    @state = 'ok'
                    query.private_run connection, (err, result) =>
                        if typeof result?.toArray is 'function'
                            result.toArray (err, result) ->
                                callback(err, result)
                        else
                            callback(err, result)


    # Run the query every `delay` ms - using setTimeout
    # Returns a timeout number (to use with @stop_timer).
    # If `index` is provided, `run` will its value as a timer
    run: (query, delay, callback, index) =>
        if not index?
            @index++
        index = @index
        @timers[index] = {}
        ( (index) =>
            @connect (error, connection) =>
                if error?
                    # If we cannot open a connection, we blackout the whole interface
                    # And do not call the callback
                    if is_disconnected?
                        if @state is 'ok'
                            is_disconnected.display_fail()
                    else
                        is_disconnected = new body.IsDisconnected
                    @state = 'fail'
                else
                    if @state is 'fail'
                        # Force refresh
                        window.location.reload true
                    else
                        @state = 'ok'
                        if @timers[index]?
                            @timers[index].connection = connection
                            (fn = =>
                                try
                                    query.private_run connection, (err, result) =>
                                        if typeof result?.toArray is 'function'
                                            result.toArray (err, result) =>
                                                # This happens if people load the page with the back button
                                                # In which case, we just restart the query
                                                # TODO: Why do we sometimes get an Error object
                                                #  with message == "[...]", and other times a
                                                #  RqlClientError with msg == "[...]."?
                                                if err?.msg is "This HTTP connection is not open." \
                                                        or err?.message is "This HTTP connection is not open"
                                                    console.log "Connection lost. Retrying."
                                                    return @run query, delay, callback, index
                                                callback(err, result)
                                                if @timers[index]?
                                                    @timers[index].timeout = setTimeout fn, delay
                                        else
                                            if err?.msg is "This HTTP connection is not open." \
                                                    or err?.message is "This HTTP connection is not open"
                                                console.log "Connection lost. Retrying."
                                                return @run query, delay, callback, index
                                            callback(err, result)
                                            if @timers[index]?
                                                @timers[index].timeout = setTimeout fn, delay
                                catch err
                                    console.log err
                                    return @run query, delay, callback, index
                            )()
        )(index)
        index

    # Stop the timer and close the connection
    stop_timer: (timer) =>
        clearTimeout @timers[timer]?.timeout
        @timers[timer]?.connection?.close {noreplyWait: false}
        delete @timers[timer]
    admin: ->
        cluster_config: r.db(system_db).table('cluster_config')
        cluster_config_id: r.db(system_db).table(
            'cluster_config',identifierFormat: 'uuid')
        current_issues: r.db(system_db).table('current_issues')
        current_issues_id: r.db(system_db).table(
            'current_issues', identifierFormat: 'uuid')
        db_config: r.db(system_db).table('db_config')
        db_config_id: r.db(system_db).table(
            'db_config', identifierFormat: 'uuid')
        jobs: r.db(system_db).table('jobs')
        jobs_id: r.db(system_db).table(
            'jobs', identifierFormat: 'uuid')
        logs: r.db(system_db).table('logs')
        logs_id: r.db(system_db).table(
            'logs', identifierFormat: 'uuid')
        server_config: r.db(system_db).table('server_config')
        server_config_id: r.db(system_db).table(
            'server_config', identifierFormat: 'uuid')
        server_status: r.db(system_db).table('server_status')
        server_status_id: r.db(system_db).table(
            'server_status', identifierFormat: 'uuid')
        stats: r.db(system_db).table('stats')
        stats_id: r.db(system_db).table(
            'stats', identifierFormat: 'uuid')
        table_config: r.db(system_db).table('table_config')
        table_config_id: r.db(system_db).table(
            'table_config', identifierFormat: 'uuid')
        table_status: r.db(system_db).table('table_status')
        table_status_id: r.db(system_db).table(
            'table_status', identifierFormat: 'uuid')

    # helper methods
    helpers:
        # Macro to create a match/switch construct in reql by
        # nesting branches
        # Use like: match(doc('field'),
        #                 ['foo', some_reql],
        #                 [r.expr('bar'), other_reql],
        #                 [some_other_query, contingent_3_reql],
        #                 default_reql)
        # Throws an error if a match isn't found. The error can be absorbed
        # by tacking on a .default() if you want
        match: (variable, specs...) ->
            previous = r.error("nothing matched #{variable}")
            for [val, action] in specs.reverse()
                previous = r.branch(r.expr(variable).eq(val), action, previous)
            return previous
        identity: (x) -> x

    # common queries used in multiple places in the ui
    queries:
        all_logs: (limit) =>
            server_conf = driver.admin().server_config
            r.db(system_db)
                .table('logs', identifierFormat: 'uuid')
                .orderBy(index: r.desc('id'))
                .limit(limit)
                .map((log) ->
                    log.merge
                        server: server_conf.get(log('server'))('name')
                        server_id: log('server')
                )
        server_logs: (limit, server_id) =>
            server_conf = driver.admin().server_config
            r.db(system_db)
                .table('logs', identifierFormat: 'uuid')
                .orderBy(index: r.desc('id'))
                .filter(server: server_id)
                .limit(limit)
                .map((log) ->
                    log.merge
                        server: server_conf.get(log('server'))('name')
                        server_id: log('server')
                )

        issues_with_ids: (current_issues=driver.admin().current_issues) ->
            # we use .get on issues_id, so it must be the real table
            current_issues_id = driver.admin().current_issues_id
            current_issues.merge((issue) ->
                issue_id = current_issues_id.get(issue('id'))
                server_disconnected =
                    disconnected_server_id:
                        issue_id('info')('disconnected_server')
                    reporting_servers:
                        issue('info')('reporting_servers')
                            .map(issue_id('info')('reporting_servers'),
                                (server, server_id) ->
                                    server: server,
                                    server_id: server_id
                                )
                log_write_error =
                    servers: issue('info')('servers').map(
                        issue_id('info')('servers'),
                        (server, server_id) ->
                            server: server
                            server_id: server_id
                    )
                outdated_index =
                    tables: issue('info')('tables').map(
                        issue_id('info')('tables'),
                        (table, table_id) ->
                            db: table('db')
                            db_id: table_id('db')
                            table_id: table_id('table')
                            table: table('table')
                            indexes: table('indexes')
                    )
                invalid_config =
                    table_id: issue_id('info')('table')
                    db_id: issue_id('info')('db')
                info: driver.helpers.match(issue('type'),
                    ['server_disconnected', server_disconnected],
                    ['log_write_error', log_write_error],
                    ['outdated_index', outdated_index],
                    ['table_needs_primary', invalid_config],
                    ['data_lost', invalid_config],
                    ['write_acks', invalid_config],
                    [issue('type'), issue('info')], # default
                )
            ).coerceTo('array')

        tables_with_primaries_not_ready: (
            table_config_id=driver.admin().table_config_id,
            table_status=driver.admin().table_status) ->
                r.do(driver.admin().server_config
                        .map((x) ->[x('id'), x('name')]).coerceTo('ARRAY').coerceTo('OBJECT'),
                    (server_names) ->
                        table_status.map(table_config_id, (status, config) ->
                            id: status('id')
                            name: status('name')
                            db: status('db')
                            shards: status('shards').map(
                                r.range(), config('shards'), (shard, pos, conf_shard) ->
                                    primary_id = conf_shard('primary_replica')
                                    primary_name = server_names(primary_id)
                                    position: pos.add(1)
                                    num_shards: status('shards').count()
                                    primary_id: primary_id
                                    primary_name: primary_name.default('-')
                                    primary_state: shard('replicas').filter(
                                        server: primary_name,
                                        {default: r.error()}
                                    )('state')(0).default('missing')
                            ).filter((shard) ->
                                r.expr(['ready', 'looking_for_primary_replica'])
                                    .contains(shard('primary_state')).not()
                            ).coerceTo('array')
                        ).filter((table) -> table('shards').isEmpty().not())
                        .coerceTo('array')
                )

        tables_with_replicas_not_ready: (
            table_config_id=driver.admin().table_config_id,
            table_status=driver.admin().table_status) ->
                table_status.map(table_config_id, (status, config) ->
                    id: status('id')
                    name: status('name')
                    db: status('db')
                    shards: status('shards').map(
                        r.range(), config('shards'), (shard, pos, conf_shard) ->
                            position: pos.add(1)
                            num_shards: status('shards').count(),
                            replicas: shard('replicas')
                                .filter((replica) ->
                                    r.expr(['ready',
                                        'looking_for_primary_replica',
                                        'offloading_data'])
                                        .contains(replica('state')).not()
                                ).map(conf_shard('replicas'), (replica, replica_id) ->
                                    replica_id: replica_id
                                    replica_name: replica('server')
                                ).coerceTo('array')
                    ).coerceTo('array')
                ).filter((table) -> table('shards')(0)('replicas').isEmpty().not())
                .coerceTo('array')
        num_primaries: (table_config_id=driver.admin().table_config_id) ->
            table_config_id('shards')
                .map((x) -> x.count()).sum()

        num_connected_primaries: (table_status=driver.admin().table_status) ->
            table_status.map((table) ->
                table('shards')('primary_replica').count((primary) -> primary.ne(null))
            ).sum()

        num_replicas: (table_config_id=driver.admin().table_config_id) ->
            table_config_id('shards')
                .map((shards) ->
                    shards.map((shard) ->
                        shard("replicas").count()
                    ).sum()
                ).sum()

        num_connected_replicas: (table_status=driver.admin().table_status) ->
            table_status('shards')
                .map((shards) ->
                    shards('replicas').map((replica) ->
                        replica('state').count((replica) ->
                            r.expr(['ready', 'looking_for_primary_replica']).contains(replica))
                    ).sum()
                ).sum()

        disconnected_servers: (server_status=driver.admin().server_status) ->
            server_status.filter((server) ->
                server("status").ne("connected")
            ).map((server) ->
                time_disconnected: server('connection')('time_disconnected')
                name: server('name')
            ).coerceTo('array')

        num_disconnected_tables: (table_status=driver.admin().table_status) ->
            table_status.count((table) ->
                shard_is_down = (shard) -> shard('primary_replica').eq(null)
                table('shards').map(shard_is_down).contains(true)
            )

        num_tables_w_missing_replicas: (table_status=driver.admin().table_status) ->
            table_status.count((table) ->
                table('status')('all_replicas_ready').not()
            )

        num_connected_servers: (server_status=driver.admin().server_status) ->
            server_status.count((server) ->
                server('status').eq("connected")
            )

        num_sindex_issues: (current_issues=driver.admin().current_issues) ->
            current_issues.count((issue) -> issue('type').eq('outdated_index'))

        num_sindexes_constructing: (jobs=driver.admin().jobs) ->
            jobs.count((job) -> job('type').eq('index_construction'))

$ ->
    body = require('./body.coffee')
    data_explorer_view = require('./dataexplorer.coffee')
    main_container = new body.MainContainer()
    app = require('./app.coffee').main = main_container
    $('body').html(main_container.render().$el)
    # We need to start the router after the main view is bound to the DOM
    main_container.start_router()

    Backbone.sync = (method, model, success, error) ->
        return 0

    data_explorer_view.Container.prototype.set_docs reql_docs

# Create a driver - providing sugar on top of the raw driver
driver = new Driver

exports.driver = driver
    # Some views backup their data here so that when you return to them
    # the latest data can be retrieved quickly.
exports.view_data_backup = {}
exports.main = null
    # The system database
exports.system_db = system_db

# Copyright 2010-2015 RethinkDB


r = require('rethinkdb')
h = require('virtual-dom/h')
diff = require('virtual-dom/diff')
patch = require('virtual-dom/patch')
createElement = require('virtual-dom/create-element')
moment = require('moment')

util = require('../util.coffee')
app = require('../app.coffee')
system_db = app.system_db
driver = app.driver
models = require('../models.coffee')


class Model extends Backbone.Model
    @query: (server_configs, server_statuses, table_status) ->
        servers: server_configs.map(server_statuses, (sconfig, sstatus) ->
            id: sconfig('id')
            name: sconfig('name')
            tags: sconfig('tags')
            time_started: sstatus('process')('time_started')
            hostname: sstatus('network')('hostname')
            cache_size: sconfig('cache_size_mb')
            primary_count: table_status.map((table) ->
                table('shards').default([]).count((shard) ->
                    shard('primary_replicas').contains(sconfig('name')))
            ).sum()
            secondary_count:
                table_status.map((table) ->
                    table('shards').default([]).count (shard) ->
                        shard('replicas')('server').contains(sconfig('name')).and(
                            shard('primary_replicas').contains(sconfig('name')).not())
                ).sum()
        ).coerceTo('array')
    defaults:
        servers: []

class View extends Backbone.View
    initialize: (options) ->
        if not app.view_data_backup.servers_view?
            @model = app.view_data_backup.servers_view = new Model()
        else
            @model = app.view_data_backup.servers_view

        @listenTo @model, "change", @render
        @current_vdom_tree = @render_vdom()
        @setElement(createElement(@current_vdom_tree))

        @fetch()

    fetch: ->
        server_config = r.db(system_db).table('server_config')
        server_status = r.db(system_db).table('server_status')
        table_status = r.db(system_db).table('table_status')

        query = r.expr(
            Model.query(server_config, server_status, table_status)
        )
        @timer = driver.run query, 5000, (error, result) =>
            if error?
                console.log error
            else
                @model.set(result)
    remove: ->
        driver.stop_timer @timer
        super

    render: ->
        new_tree = @render_vdom()
        patch(@el, diff(@current_vdom_tree, new_tree))
        @current_vdom_tree = new_tree
        @

    render_vdom: ->
        h "div.servers-container", [
            h "h1.title", "Servers connected to the cluster"
            h "table.servers-list.section",
                @model.get('servers').map(render_server)
        ]

render_server = (server) ->
    h "tr.server-container", [
        h "td.name", [
            h "a", href: "#servers/#{server.id}", server.name
            h "span.tags", title: server.tags.join(", "),
                server.tags.join(', ')
        ]
        h "td.quick-info", [
            "#{server.primary_count} "
            "#{util.pluralize_noun('primary', server.primary_count)}, "
            "#{server.secondary_count} "
            "#{util.pluralize_noun('secondary', server.secondary_count)}"
        ]
        h "td.quick-info", [
            h("span.highlight", server.hostname)
            ", up for "
            moment(server.time_started).fromNow(true)
        ]
   ]

exports.View = View
exports.Model = Model

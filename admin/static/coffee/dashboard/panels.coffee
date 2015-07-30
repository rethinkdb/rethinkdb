# Copyright 2010-2015 RethinkDB, all rights reserved.

r = require('rethinkdb')
h = require('virtual-dom/h')
vdom = require('../vdom_util.coffee')
util = require('../util.coffee')

app = require('../app.coffee')
system_db = app.system_db

systable = (name) => r.db(system_db).table(name)

class ServersModel extends Backbone.Model
    @query: (server_status=systable('server_status'), \
             table_config=systable('table_config')) ->
        r.do(
            # All connected servers
            server_status('name').coerceTo('array')
            # All servers assigned to tables
            table_config
                .concatMap((row)->row('shards').default([]))
                .concatMap((row)->row('replicas'))
                .distinct()
            (connected_servers, assigned_servers) ->
                servers_connected:
                    connected_servers.count()
                servers_missing:
                    assigned_servers.setDifference(connected_servers)
                unknown_missing:
                    table_config.filter((row)->
                        row.hasFields('shards').not())('name').isEmpty().not()
        )

    defaults:
        servers_connected: 0
        servers_missing: []
        unknown_missing: false


class ServersView extends vdom.VirtualDomView
    render_vdom: ->
        model = @model.toJSON()
        num_servers_missing = model.servers_missing.length
        unknown_missing = model.unknown_missing
        servers_missing =
            if unknown_missing and num_servers_missing > 0
                "#{num_servers_missing}+"
            else if unknown_missing and num_servers_missing <= 0
                "1+"
            else
                "#{num_servers_missing}"
        problems_exist = unknown_missing or num_servers_missing != 0
        elements = [
            [h("span.highlight", "#{model.servers_connected} "),
             util.pluralize_noun('server', model.servers_connected),
             " connected"]
            [h("span.highlight", "#{servers_missing} "),
             util.pluralize_noun('server', model.servers_missing.length),
             " missing"]
        ]
        render_panel("Servers", problems_exist, elements)


class TablesModel extends Backbone.Model
    @query: (table_status=systable('table_status')) ->
        tables_ready: table_status.count((row) ->
            row('status')('all_replicas_ready'))
        tables_unready: table_status.count((row) ->
            row('status')('all_replicas_ready').not())

    defaults:
        tables_ready: 0
        tables_unready: 0


class TablesView extends vdom.VirtualDomView
    render_vdom: ->
        model = @model.toJSON()
        problems_exist = model.tables_unready > 0
        elements = [
            [h("span.highlight", "#{model.tables_ready} "),
              util.pluralize_noun("table", model.tables_ready),
              " ready"]
            [h("span.highlight", "#{model.tables_unready} "),
              util.pluralize_noun("table", model.tables_unready),
              " with issues"]
        ]
        render_panel("Tables", problems_exist, elements)

class IndexesModel extends Backbone.Model
    @query: (table_config=systable('table_config'), \
             table_status=systable('table_status'), \
             jobs=systable('jobs'), \
             current_issues=systable('current_issues')) =>
        num_sindexes:
            table_config.map((table) ->
                r.branch(
                    table_status.get(table('id'))('status')('ready_for_outdated_reads'),
                    r.db(table('db')).table(table('name')).indexList().count(),
                    0
                )
            ).sum()
        sindexes_constructing:
            jobs.filter(type: 'index_construction')('info').map((row) ->
                db: row('db')
                table: row('table')
                index: row('index')
                progress: row('progress')
            ).coerceTo('array')

    defaults:
        num_sindexes: 0
        sindexes_constructing: []

class IndexesView extends vdom.VirtualDomView
    render_vdom: ->
        model = @model.toJSON()
        problems_exist = false
        elements = [
            [h("span.highlight", "#{model.num_sindexes}"),
            " secondary indexes"]
            [h("span.highlight", "#{model.sindexes_constructing.length}"),
             " indexes building"]
        ]
        render_panel("Indexes", problems_exist, elements)

class ResourcesModel extends Backbone.Model
    @query: (stats=systable('stats'), \
             table_status=systable('server_status')) =>
        cache_used:
            stats.filter((stat)->stat('id').contains('table_server'))('storage_engine')\
                ('cache')('in_use_bytes').sum()
        cache_total:
            table_status('process')('cache_size_mb').map(
                (row)->row.mul(1024*1024)).sum()
        disk_used:
            stats.filter((row)->row('id').contains('table_server'))\
                ('storage_engine')('disk')('space_usage')
                .map((data)->
                    data('data_bytes').add( \
                    data('garbage_bytes')).add( \
                    data('metadata_bytes')).add( \
                    data('preallocated_bytes'))
                ).sum()
    defaults:
        cache_used: 0
        cache_total: 0
        disk_used: 0

class ResourcesView extends vdom.VirtualDomView
    render_vdom: ->
        model = @model.toJSON()
        problems_exist = false
        cache_percent = Math.ceil((model.cache_used / model.cache_total) * 100)
        cache_used = util.format_bytes(model.cache_used)
        cache_total = util.format_bytes(model.cache_total)
        disk_used = util.format_bytes(model.disk_used)
        elements = [
            [h("span.highlight", "#{cache_percent}%"), " cache used"]
            [h("span.highlight", "#{disk_used}"), " disk used"]
        ]
        render_panel("Resources", problems_exist, elements)

render_panel = (panel_name, problems_exist, elements) =>
    problems_class = if problems_exist
        "problems-detected"
    else
        "no-problems-detected"
    h "div.dashboard-panel.servers", [
        h "div.status.#{problems_class}", [
            h "h3", panel_name
            h "ul", elements.map((element) ->
                h "li", element)
        ]
    ]

exports.ServersView = ServersView
exports.ServersModel = ServersModel
exports.TablesView = TablesView
exports.TablesModel = TablesModel
exports.IndexesView = IndexesView
exports.IndexesModel = IndexesModel
exports.ResourcesView = ResourcesView
exports.ResourcesModel = ResourcesModel

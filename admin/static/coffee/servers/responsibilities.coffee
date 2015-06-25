# Server responsibilities view and model

r = require('rethinkdb')

h = require('virtual-dom/h')
diff = require('virtual-dom/diff')
patch = require('virtual-dom/patch')
createElement = require('virtual-dom/create-element')

util = require('../util.coffee')


class Model extends Backbone.Model
    @query: (table_config, table_status, server_name) ->
        tables: table_config.filter((config) ->
            config('shards')('replicas').concatMap((x)->x).contains(server_name)
          ).map((config) ->
            status = table_status.get(config('id'))
            db: config('db')
            name: config('name')
            id: config('id')
            shards: config('shards').map(status('shards'), r.range(),
                (sconfig, sstatus, shardId) ->
                    shard_id: shardId.add(1),
                    total_shards: config('shards').count(),
                    inshard: sconfig('replicas').contains(server_name),
                    currently_primary:
                        sstatus('primary_replicas').contains(server_name)
                    configured_primary: sconfig('primary_replica').eq(server_name)
                    nonvoting: sconfig('nonvoting_replicas').contains(server_name)
                ).filter(inshard: true).without('inshard').coerceTo('array')
          ).coerceTo('array')
    defaults:
        tables: []

class View extends Backbone.View
    initialize: (options) ->
        @listenTo @model, "change", @render
        @current_vdom_tree = @render_vdom()
        @setElement(createElement(@current_vdom_tree))

    render: () ->
        new_tree = @render_vdom()
        patch(@el, diff(@current_vdom_tree, new_tree))
        @current_vdom_tree = new_tree
        @

    render_vdom: () ->
        h "div", [
            h "h2.title", "Table shards on this server"
            h "ul.parents", @model.get('tables').map(render_table)
        ]

render_table = (table) ->
    h "li.parent", [
        h "div.parent-heading",
            h "span.parent-title", [
                "Table ",
                h("a", href: "#/tables/#{table.id}",
                    "#{table.db}.#{table.name}")
            ]
        h "ul.children", table.shards.map(render_shard)
    ]

render_shard = (shard) ->
    h "li.child", [
        h("span.child-name", ["Shard #{shard.shard_id}/#{shard.total_shards}"]),
        h("span.child-role.#{util.replica_roleclass(shard)}",
            util.replica_rolename(shard))
    ]

exports.Model = Model
exports.View = View

# Copyright 2010-2015 RethinkDB

models = require('../models.coffee')
util = require('../util.coffee')
h = require('virtual-dom/h')
diff = require('virtual-dom/diff')
patch = require('virtual-dom/patch')

class ShardAssignmentsView extends Backbone.View
    initialize: (data) =>
        @listenTo @model, 'change', @render
        if data.collection?
            @collection = data.collection
        @current_vdom_tree = h "div"

    set_assignments: (assignments) =>
        @collection = assignments
        @listenTo @collection, 'change', @render
        @render()

    render: =>
        new_tree = render_assignments(
            @model.get('info_unavailable'), @collection?.toJSON())
        patches = diff(@current_vdom_tree, new_tree)
        patch(@$el.get(0), patches)
        @current_vdom_tree = new_tree
        @

    remove: =>
        @stopListening()

render_assignments = (info_unavailable, shard_assignments) ->
    h "div", [
        h "h2.title", "Servers used by this table"
        render_warning(info_unavailable)
        h "ul.parents", shard_assignments?.map(render_shard)
    ]

render_warning = (info_unavailable) ->
    if info_unavailable
        h "div.unavailable-error", [
            h "p", """Document estimates cannot be updated while not \
                       enough replicas are available"""
        ]

render_shard = (shard, index) ->
    h "li.parent", [
        h "div.parent-heading", [
            h "span.parent-title", "Shard #{index + 1}"
            h "span.numkeys", ["~", util.approximate_count(shard.num_keys), " ",
                util.pluralize_noun('document', shard.num_keys)]
        ]
        h "ul.children", shard.replicas.map(render_replica)
    ]

render_replica = (replica) ->
    h "li.child", [
        h "span.child-name", [
            if replica.state == 'ready'
                h "a", href: "#servers/#{replica.id}", replica.server
            else
                replica.server
        ]
        h "span.child-role.#{util.replica_roleclass(replica)}",
            util.replica_rolename(replica)
        h "span.state.#{util.state_color(replica.state)}",
            util.humanize_state_string(replica.state)
    ]

exports.ShardAssignmentsView = ShardAssignmentsView

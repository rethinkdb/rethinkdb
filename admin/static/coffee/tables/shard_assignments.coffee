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
        new_tree = render_assignments(@model.toJSON(), @collection?.toJSON())
        patches = diff(@current_vdom_tree, new_tree)
        patch(@$el.get(0), patches)
        @current_vdom_tree = new_tree
        @

    remove: =>
        @stopListening()

render_assignments = (model, shard_assignments) =>
    if model.info_unavailable
        warning = h "div.unavailable-error", [
            h "p", ["""Document estimates cannot be updated while not \
                       enough replicas are available"""]
        ]
    else
        warning = null
    h "div", [
        h "h2.title", ["Servers used by this table"]
        warning
        h "ul.assignments_list", [
            h "li.tree-line"
            shard_assignments?.map(render_shard)
        ]
    ]

# Used to highlight an element
highlight = (value) =>
    h "span.highlight", [value]

render_shard = (shard, index) =>
    h "div.assignment_container", [
        h "li.datacenter.parent", [
            h "div.datacenter-info.parent-info", [
                h "p.name", [
                    h "span.num-keys", [
                        highlight [
                            "~"
                            util.approximate_count(shard.num_keys)
                        ]
                        " "
                        util.pluralize_noun "document", shard.num_keys
                    ]
                    h "span", ["Shard ", String(index + 1)]
                ]
            ]
        ]
        shard.replicas.map(render_replica)
    ]

render_replica = (replica) =>
    role = replica_role(replica.configured_primary, replica.currently_primary)
    h("li.server.child", [
        h "div.tree-node"
        h "div.server-info.child-info", [
            h "p.name", [
                h "a", {href: "/#servers/"+replica.id}, [
                    replica.server
                ]
            ]
            h "p."+role, [
                highlight("Primary replica") if role == 'primary'
                "Configured primary replica" if role == 'configured.primary'
                highlight("Elected primary replica") if role == 'elected.primary'
                "Secondary replica" if role == 'secondary'
            ]
        ]
    ])

replica_role = (configured_primary, currently_primary) ->
    if configured_primary
        if currently_primary
            "primary"
        else
            "configured.primary"
    else
        if currently_primary
            "elected.primary"
        else
            "secondary"


exports.ShardAssignmentsView = ShardAssignmentsView

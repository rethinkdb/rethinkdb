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

render_assignments = (model, collection) =>
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
            collection?.map(render_assignment)
        ]
    ]

# Used to highlight an element
highlight = (value) =>
    h "span.highlight", [value]

render_assignment = (assignment) =>
    pclass = if assignment.primary then "primary" else "secondary"
    h "div.assignment_container", [
        h("li.datacenter.parent", [
            h "div.datacenter-info.parent-info", [
                h "p.name", [
                    h "span.num-keys", [
                        highlight([
                            "~"
                            util.approximate_count(assignment.num_keys)
                        ])
                        " "
                        util.pluralize_noun "document", assignment.num_keys
                    ]
                    h "span", ["Shard ", String(assignment.shard_id)]
                ]
            ]
        ]) if assignment.start_shard
        h("li.server.child", [
            h "div.tree-node"
            h "div.server-info.child-info", [
                h "p.name", [
                    h "a", {href: "/#servers/"+assignment.data.id}, [
                        assignment.data.name
                    ]
                ]
                h "p."+pclass, [
                    highlight("Primary replica") if assignment.primary
                    "Secondary replica" if assignment.replica
                ]
            ]
        ]) if assignment.primary or assignment.replica
    ]


exports.ShardAssignmentsView = ShardAssignmentsView

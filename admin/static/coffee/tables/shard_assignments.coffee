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
        h "ul.shards", shard_assignments?.map(render_shard)
    ]

render_warning = (info_unavailable) ->
    if info_unavailable
        h "div.unavailable-error", [
            h "p", """Document estimates cannot be updated while not \
                       enough replicas are available"""
        ]

render_shard = (shard, index) ->
  h "li.shard", [
    h "div.shard-heading", [
      h "span.shard-title", "Shard #{index + 1}"
      h "span.numkeys", ["~", util.approximate_count(shard.num_keys), " ",
                         util.pluralize_noun('document', shard.num_keys)]
    ]
    h "ul.replicas", shard.replicas.map(render_replica)
  ]

render_replica = (replica) ->
    h "li.replica", [
        h "span.server-name.#{state_color(replica.state)}",
            h "a", href: "#servers/#{replica.id}", replica.server
        h "span.replica-role.#{replica_roleclass(replica)}",
            replica_rolename(replica)
        h "span.state.#{state_color(replica.state)}",
            humanize_state_string(replica.state)
    ]

state_color = (state) ->
  switch state
      when "ready"        then "green"
      when "disconnected" then "red"
      else                     "yellow"

replica_rolename = ({configured_primary: configured, \
                     currently_primary: currently, \
                     nonvoting: nonvoting, \
                     }) ->
    if configured and currently
        "Primary replica"
    else if configured and not currently
        "Goal primary replica"
    else if not configured and currently
        "Acting primary replica"
    else if nonvoting
        "Non-voting secondary replica"
    else
        "Secondary replica"

replica_roleclass = ({configured_primary: configured, currently_primary: currently}) ->
    if configured and currently
        "primary"
    else if configured and not currently
        "goal.primary"
    else if not configured and currently
        "acting.primary"
    else
        "secondary"

humanize_state_string = (state_string) ->
    state_string.replace(/_/g, ' ')

exports.ShardAssignmentsView = ShardAssignmentsView

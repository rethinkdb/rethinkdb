// Copyright 2010-2015 RethinkDB

const models = require('../models')
const util = require('../util')
const h = require('virtual-dom/h')
const diff = require('virtual-dom/diff')
const patch = require('virtual-dom/patch')

class ShardAssignmentsView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.set_assignments = this.set_assignments.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  initialize ({ collection }) {
    this.listenTo(this.model, 'change', this.render)
    if (collection != null) {
      this.collection = collection
    }
    return this.current_vdom_tree = h('div')
  }

  set_assignments (assignments) {
    this.collection = assignments
    this.listenTo(this.collection, 'change', this.render)
    return this.render()
  }

  render () {
    const new_tree = render_assignments(
      this.model.get('info_unavailable'),
      __guard__(this.collection, x => x.toJSON())
    )
    const patches = diff(this.current_vdom_tree, new_tree)
    patch(this.$el.get(0), patches)
    this.current_vdom_tree = new_tree
    return this
  }

  remove () {
    return this.stopListening()
  }
}

var render_assignments = (info_unavailable, shard_assignments) =>
  h('div', [
    h('h2.title', 'Servers used by this table'),
    render_warning(info_unavailable),
    h('ul.parents', __guard__(shard_assignments, x => x.map(render_shard))),
  ])

var render_warning = info_unavailable => {
  if (info_unavailable) {
    return h('div.unavailable-error', [
      h(
        'p',
        `Document estimates cannot be updated while not \
enough replicas are available`
      ),
    ])
  }
}

var render_shard = ({ num_keys, replicas }, index) =>
  h('li.parent', [
    h('div.parent-heading', [
      h('span.parent-title', `Shard ${index + 1}`),
      h('span.numkeys', [
        '~',
        util.approximate_count(num_keys),
        ' ',
        util.pluralize_noun('document', num_keys),
      ]),
    ]),
    h('ul.children', replicas.map(render_replica)),
  ])

var render_replica = replica =>
  h('li.child', [
    h('span.child-name', [
      replica.state !== 'disconnected'
        ? h('a', { href: `#servers/${replica.id}` }, replica.server)
        : replica.server,
    ]),
    h(`span.child-role.${util.replica_roleclass(replica)}`, util.replica_rolename(replica)),
    h(`span.state.${util.state_color(replica.state)}`, util.humanize_state_string(replica.state)),
  ])

exports.ShardAssignmentsView = ShardAssignmentsView

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

// Server responsibilities view and model

const r = require('rethinkdb')

const h = require('virtual-dom/h')
const diff = require('virtual-dom/diff')
const patch = require('virtual-dom/patch')
const createElement = require('virtual-dom/create-element')

const util = require('../util.coffee')

class Model extends Backbone.Model {
  static initClass () {
    this.prototype.defaults = { tables: [] }
  }
  static query (table_config, table_status, server_name) {
    return {
      tables: table_config
        .filter(config => config('shards')('replicas').concatMap(x => x).contains(server_name))
        .map(config => {
          const status = table_status.get(config('id'))
          return {
            db    : config('db'),
            name  : config('name'),
            id    : config('id'),
            shards: config('shards')
              .map(status('shards'), r.range(), (sconfig, sstatus, shardId) => ({
                shard_id          : shardId.add(1),
                total_shards      : config('shards').count(),
                inshard           : sconfig('replicas').contains(server_name),
                currently_primary : sstatus('primary_replicas').contains(server_name),
                configured_primary: sconfig('primary_replica').eq(server_name),
                nonvoting         : sconfig('nonvoting_replicas').contains(server_name),
              }))
              .filter({ inshard: true })
              .without('inshard')
              .coerceTo('array'),
          }
        })
        .coerceTo('array'),
    }
  }
}
Model.initClass()

class View extends Backbone.View {
  initialize (options) {
    this.listenTo(this.model, 'change', this.render)
    this.current_vdom_tree = this.render_vdom()
    return this.setElement(createElement(this.current_vdom_tree))
  }

  render () {
    const new_tree = this.render_vdom()
    patch(this.el, diff(this.current_vdom_tree, new_tree))
    this.current_vdom_tree = new_tree
    return this
  }

  render_vdom () {
    return h('div', [
      h('h2.title', 'Table shards on this server'),
      h('ul.parents', this.model.get('tables').map(render_table)),
    ])
  }
}

var render_table = ({ id, db, name, shards }) =>
  h('li.parent', [
    h(
      'div.parent-heading',
      h('span.parent-title', ['Table ', h('a', { href: `#/tables/${id}` }, `${db}.${name}`)])
    ),
    h('ul.children', shards.map(render_shard)),
  ])

var render_shard = shard =>
  h('li.child', [
    h('span.child-name', [`Shard ${shard.shard_id}/${shard.total_shards}`]),
    h(`span.child-role.${util.replica_roleclass(shard)}`, util.replica_rolename(shard)),
  ])

exports.Model = Model
exports.View = View

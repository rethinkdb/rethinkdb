// Copyright 2010-2015 RethinkDB

const r = require('rethinkdb')
const h = require('virtual-dom/h')
const diff = require('virtual-dom/diff')
const patch = require('virtual-dom/patch')
const createElement = require('virtual-dom/create-element')
const moment = require('moment')

const util = require('../util')
const app = require('../app')
const { system_db } = app
const { driver } = app
const models = require('../models')

class Model extends Backbone.Model {
  static initClass () {
    this.prototype.defaults = { servers: [] }
  }
  static query (server_configs, server_statuses, table_status) {
    return {
      servers: server_configs
        .map(server_statuses, (sconfig, sstatus) => ({
          id           : sconfig('id'),
          name         : sconfig('name'),
          tags         : sconfig('tags'),
          time_started : sstatus('process')('time_started'),
          hostname     : sstatus('network')('hostname'),
          cache_size   : sconfig('cache_size_mb'),
          primary_count: table_status
            .map(table =>
              table('shards')
                .default([])
                .count(shard => shard('primary_replicas').contains(sconfig('name'))))
            .sum(),
          secondary_count: table_status
            .map(table =>
              table('shards')
                .default([])
                .count(shard =>
                  shard('replicas')('server')
                    .contains(sconfig('name'))
                    .and(shard('primary_replicas').contains(sconfig('name')).not())))
            .sum(),
        }))
        .coerceTo('array'),
    }
  }
}
Model.initClass()

class View extends Backbone.View {
  initialize (options) {
    if (app.view_data_backup.servers_view == null) {
      this.model = app.view_data_backup.servers_view = new Model()
    } else {
      this.model = app.view_data_backup.servers_view
    }

    this.listenTo(this.model, 'change', this.render)
    this.current_vdom_tree = this.render_vdom()
    this.setElement(createElement(this.current_vdom_tree))

    return this.fetch()
  }

  fetch () {
    const args = {
      server_config: r.db(system_db).table('server_config').coerceTo('array'),
      server_status: r.db(system_db).table('server_status').coerceTo('array'),
      table_status : r.db(system_db).table('table_status').coerceTo('array'),
    }

    const query = r
      .expr(args)
      .do(a => Model.query(a('server_config'), a('server_status'), a('table_status')))
    return this.timer = driver.run(query, 5000, (error, result) => {
      if (error != null) {
        return console.log(error)
      } else {
        return this.model.set(result)
      }
    })
  }
  remove (...args) {
    driver.stop_timer(this.timer)
    return super.remove(...args)
  }

  render () {
    const new_tree = this.render_vdom()
    patch(this.el, diff(this.current_vdom_tree, new_tree))
    this.current_vdom_tree = new_tree
    return this
  }

  render_vdom () {
    return h('div.servers-container', [
      h('h1.title', 'Servers connected to the cluster'),
      h('table.servers-list.section', this.model.get('servers').map(render_server)),
    ])
  }
}

var render_server = server =>
  h('tr.server-container', [
    h('td.name', [
      h('a', { href: `#servers/${server.id}` }, server.name),
      h('span.tags', { title: server.tags.join(', ') }, server.tags.join(', ')),
    ]),
    h('td.quick-info', [
      `${server.primary_count} `,
      `${util.pluralize_noun('primary', server.primary_count)}, `,
      `${server.secondary_count} `,
      `${util.pluralize_noun('secondary', server.secondary_count)}`,
    ]),
    h('td.quick-info', [
      h('span.highlight', server.hostname),
      ', up for ',
      moment(server.time_started).fromNow(true),
    ]),
  ])

exports.View = View
exports.Model = Model

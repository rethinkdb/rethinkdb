// Copyright 2010-2015 RethinkDB, all rights reserved.

const r = require('rethinkdb')
const h = require('virtual-dom/h')
const vdom = require('../vdom_util')
const util = require('../util')

const app = require('../app')
const { system_db } = app

const systable = name => r.db(system_db).table(name)

class ServersModel extends Backbone.Model {
  static initClass () {
    this.prototype.defaults = {
      servers_connected: 0,
      servers_missing  : [],
      unknown_missing  : false,
    }
  }
  static query (server_status = systable('server_status'), table_config = systable('table_config')) {
    return r.do(
      // All connected servers
      server_status('name').coerceTo('array'),
      // All servers assigned to tables
      table_config
        .concatMap(row => row('shards').default([]))
        .concatMap(row => row('replicas'))
        .distinct(),
      (connected_servers, assigned_servers) => ({
        servers_connected: connected_servers.count(),
        servers_missing  : assigned_servers.setDifference(connected_servers),
        unknown_missing  : table_config
          .filter(row => row.hasFields('shards').not())('name')
          .isEmpty()
          .not(),
      })
    )
  }
}
ServersModel.initClass()

class ServersView extends vdom.VirtualDomView {
  render_vdom () {
    const model = this.model.toJSON()
    const num_servers_missing = model.servers_missing.length
    const { unknown_missing } = model
    const servers_missing = unknown_missing && num_servers_missing > 0
      ? `${num_servers_missing}+`
      : unknown_missing && num_servers_missing <= 0 ? '1+' : `${num_servers_missing}`
    const problems_exist = unknown_missing || num_servers_missing !== 0
    const elements = [
      [
        h('span.highlight', `${model.servers_connected} `),
        util.pluralize_noun('server', model.servers_connected),
        ' connected',
      ],
      [
        h('span.highlight', `${servers_missing} `),
        util.pluralize_noun('server', model.servers_missing.length),
        ' missing',
      ],
    ]
    return render_panel('Servers', problems_exist, elements)
  }
}

class TablesModel extends Backbone.Model {
  static initClass () {
    this.prototype.defaults = {
      tables_ready  : 0,
      tables_unready: 0,
    }
  }
  static query (table_status = systable('table_status')) {
    return {
      tables_ready  : table_status.count(row => row('status')('all_replicas_ready')),
      tables_unready: table_status.count(row => row('status')('all_replicas_ready').not()),
    }
  }
}
TablesModel.initClass()

class TablesView extends vdom.VirtualDomView {
  render_vdom () {
    const model = this.model.toJSON()
    const problems_exist = model.tables_unready > 0
    const elements = [
      [
        h('span.highlight', `${model.tables_ready} `),
        util.pluralize_noun('table', model.tables_ready),
        ' ready',
      ],
      [
        h('span.highlight', `${model.tables_unready} `),
        util.pluralize_noun('table', model.tables_unready),
        ' with issues',
      ],
    ]
    return render_panel('Tables', problems_exist, elements)
  }
}

class IndexesModel extends Backbone.Model {
  static initClass () {
    this.prototype.defaults = {
      num_sindexes         : 0,
      sindexes_constructing: [],
    }
  }
  static query (
    table_config = systable('table_config'),
    table_status = systable('table_status'),
    jobs = systable('jobs'),
    current_issues = systable('current_issues')
  ) {
    return {
      num_sindexes         : table_config.sum(row => row('indexes').count()),
      sindexes_constructing: jobs
        .filter({ type: 'index_construction' })('info')
        .map(row => ({
          db      : row('db'),
          table   : row('table'),
          index   : row('index'),
          progress: row('progress'),
        }))
        .coerceTo('array'),
    }
  }
}
IndexesModel.initClass()

class IndexesView extends vdom.VirtualDomView {
  render_vdom () {
    const model = this.model.toJSON()
    const problems_exist = false
    const elements = [
      [h('span.highlight', `${model.num_sindexes}`), ' secondary indexes'],
      [h('span.highlight', `${model.sindexes_constructing.length}`), ' indexes building'],
    ]
    return render_panel('Indexes', problems_exist, elements)
  }
}

class ResourcesModel extends Backbone.Model {
  static initClass () {
    this.prototype.defaults = {
      cache_used : 0,
      cache_total: 0,
      disk_used  : 0,
    }
  }
  static query (stats = systable('stats'), table_status = systable('server_status')) {
    return {
      cache_used: stats
        .filter(stat =>
          stat('id').contains('table_server'))('storage_engine')('cache')('in_use_bytes')
        .sum(),
      cache_total: table_status('process')('cache_size_mb').map(row => row.mul(1024 * 1024)).sum(),
      disk_used  : stats
        .filter(row => row('id').contains('table_server'))('storage_engine')('disk')('space_usage')
        .map(data =>
          data('data_bytes')
            .add(data('garbage_bytes'))
            .add(data('metadata_bytes'))
            .add(data('preallocated_bytes')))
        .sum(),
    }
  }
}
ResourcesModel.initClass()

class ResourcesView extends vdom.VirtualDomView {
  render_vdom () {
    const model = this.model.toJSON()
    const problems_exist = false
    const cache_percent = Math.ceil(model.cache_used / model.cache_total * 100)
    const cache_used = util.format_bytes(model.cache_used)
    const cache_total = util.format_bytes(model.cache_total)
    const disk_used = util.format_bytes(model.disk_used)
    const elements = [
      [h('span.highlight', `${cache_percent}%`), ' cache used'],
      [h('span.highlight', `${disk_used}`), ' disk used'],
    ]
    return render_panel('Resources', problems_exist, elements)
  }
}

var render_panel = (panel_name, problems_exist, elements) => {
  const problems_class = problems_exist ? 'problems-detected' : 'no-problems-detected'
  return h('div.dashboard-panel.servers', [
    h(`div.status.${problems_class}`, [
      h('h3', panel_name),
      h('ul', elements.map(element => h('li', element))),
    ]),
  ])
}

exports.ServersView = ServersView
exports.ServersModel = ServersModel
exports.TablesView = TablesView
exports.TablesModel = TablesModel
exports.IndexesView = IndexesView
exports.IndexesModel = IndexesModel
exports.ResourcesView = ResourcesView
exports.ResourcesModel = ResourcesModel

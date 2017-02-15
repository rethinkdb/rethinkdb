// Copyright 2010-2015 RethinkDB, all rights reserved.

const vdom = require('../vdom_util.coffee')
const h = require('virtual-dom/h')
const r = require('rethinkdb')

const panels = require('./panels.coffee')
const util = require('../util.coffee')
const app = require('../app.coffee')
const { driver } = app
const { system_db } = app
const models = require('../models.coffee')
const log_view = require('../log_view.coffee')
const vis = require('../vis.coffee')

class View extends vdom.VirtualDomView {
  constructor (...args) {
    this.cleanup = this.cleanup.bind(this)
    this.render_vdom = this.render_vdom.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.events = { 'click .view-logs': 'show_all_logs' }
    this.prototype.fetch = {
      5000: {
        servers  : panels.ServersModel,
        tables   : panels.TablesModel,
        indexes  : panels.IndexesModel,
        resources: panels.ResourcesModel,
      },
    }
  }
  // TODO: convert stats timer to this format
  // 1000:
  //     stats: models.Stats

  init (options) {
    this.stats = new models.Stats()
    return this.stats_timer = driver.run(
      r.db(system_db).table('stats').get(['cluster']).do(stat => ({
        keys_read: stat('query_engine')('read_docs_per_sec'),
        keys_set : stat('query_engine')('written_docs_per_sec'),
      })),
      1000,
      this.stats.on_result
    )
  }

  cleanup () {
    return driver.stop_timer(this.stats_timer)
  }

  show_all_logs () {
    return app.main.router.navigate('#logs', { trigger: true })
  }

  render_vdom () {
    if (this.query_error != null) {
      console.log(this.query_error.msg)
      return h('div.section', [
        h('h2.title', 'Error'),
        h('p', [
          'The request to retrieve data failed. You can try to ',
          h('a', { href: '#' }, 'refresh'),
        ]),
        h('p', 'Error:'),
        this.query_error.msg != null ? h('pre.main_query_error', this.query_error.msg) : undefined,
      ])
    } else {
      return h('div.dashboard-container', [
        h('div.section.cluster-status', [
          h('div.panel.servers', new vdom.BackboneViewWidget(() => {
            return new panels.ServersView({ model: this.submodels.servers })
          })),
          h('div.panel.tables', new vdom.BackboneViewWidget(() => {
            return new panels.TablesView({ model: this.submodels.tables })
          })),
          h('div.panel.indexes', new vdom.BackboneViewWidget(() => {
            return new panels.IndexesView({ model: this.submodels.indexes })
          })),
          h('div.panel.resources', new vdom.BackboneViewWidget(() => {
            return new panels.ResourcesView({ model: this.submodels.resources })
          })),
        ]),
        h('div.section#cluster_performance_container', new vdom.BackboneViewWidget(() => {
          return new vis.OpsPlot(this.stats.get_stats, {
            width  : 833,
            height : 300,
            seconds: 119,
            type   : 'cluster',
          })
        })),
        h('div.section.recent-logs', [
          h('button.btn.btn-primary.view-logs', 'View All'),
          h('h2.title', 'Recent log entries'),
          h('div.recent-log-entries-container', new vdom.BackboneViewWidget(() => {
            return new log_view.LogsContainer({
              limit: 5,
              query: driver.queries.all_logs,
            })
          })),
        ]),
      ])
    }
  }
}
View.initClass()

exports.View = View

// Copyright 2010-2012 RethinkDB, all rights reserved.
const dashboard = require('./dashboard/dashboard')
const log_view = require('./log_view')
const dataexplorer_view = require('./dataexplorer')
const tables_view = require('./tables/index')
const table_view = require('./tables/table')
const servers_index = require('./servers/index')
const server_view = require('./servers/server')
const app = require('./app')

class BackboneCluster extends Backbone.Router {
  constructor (...args) {
    this.update_active_tab = this.update_active_tab.bind(this)
    super(...args)
  }

  static initClass () {
    // Routes can end with a slash too - This is useful is the user
    // tries to manually edit the route
    this.prototype.routes = {
      ''             : 'dashboard',
      tables         : 'index_tables',
      'tables/'      : 'index_tables',
      'tables/:id'   : 'table',
      'tables/:id/'  : 'table',
      servers        : 'index_servers',
      'servers/'     : 'index_servers',
      'servers/:id'  : 'server',
      'servers/:id/' : 'server',
      dashboard      : 'dashboard',
      'dashboard/'   : 'dashboard',
      logs           : 'logs',
      'logs/'        : 'logs',
      dataexplorer   : 'dataexplorer',
      'dataexplorer/': 'dataexplorer',
    }
  }

  initialize ({ navbar }) {
    super.initialize(...arguments)
    this.navbar = navbar

    this.container = $('#cluster')
    this.current_view = new Backbone.View()

    return this.bind('route', this.update_active_tab)
  }

  // Highlight the link of the current view
  update_active_tab (route) {
    return this.navbar.set_active_tab(route)
  }

  index_tables () {
    this.current_view.remove()
    this.current_view = new tables_view.DatabasesContainer()
    return this.container.html(this.current_view.render().$el)
  }

  index_servers (data) {
    this.current_view.remove()
    this.current_view = new servers_index.View()
    return this.container.html(this.current_view.render().$el)
  }

  dashboard () {
    this.current_view.remove()
    this.current_view = new dashboard.View()
    return this.container.html(this.current_view.render().$el)
  }

  logs () {
    this.current_view.remove()
    this.current_view = new log_view.LogsContainer()
    return this.container.html(this.current_view.render().$el)
  }

  dataexplorer () {
    this.current_view.remove()
    this.current_view = new dataexplorer_view.Container({
      state: dataexplorer_view.dataexplorer_state,
    })
    this.container.html(this.current_view.render().$el)
    this.current_view.init_after_dom_rendered() // Need to be called once the view is in the DOM tree
    return this.current_view.results_view_wrapper.set_scrollbar() // In case we check the data explorer, leave and come back
  }

  table (id) {
    this.current_view.remove()
    this.current_view = new table_view.TableContainer(id)
    return this.container.html(this.current_view.render().$el)
  }

  server (id) {
    this.current_view.remove()
    this.current_view = new server_view.View(id)
    return this.container.html(this.current_view.render().$el)
  }
}
BackboneCluster.initClass()

exports.BackboneCluster = BackboneCluster

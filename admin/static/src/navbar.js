// Copyright 2010-2015 RethinkDB
// TopBar module

const app = require('./app')

class NavBarView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.init_typeahead = this.init_typeahead.bind(this)
    this.render = this.render.bind(this)
    this.set_active_tab = this.set_active_tab.bind(this)
    this.update_cog_icon = this.update_cog_icon.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.id = 'navbar'
    this.prototype.className = 'container'
    this.prototype.template = require('../handlebars/navbar_view.hbs')
    this.prototype.events = { 'click .options_link': 'update_cog_icon' }
  }

  initialize ({ databases, tables, servers, container }) {
    this.databases = databases
    this.tables = tables
    this.servers = servers

    this.container = container

    return this.options_state = 'hidden' // can be 'hidden' or 'visible'
  }

  init_typeahead () {
    // Has to be called after we have injected the template
    return this.$('input.search-box').typeahead({
      source: (typeahead, query) => {
        const servers = _.map(this.servers.models, server => ({
          id  : server.get('id'),
          name: `${server.get('name')} (server)`,
          type: 'servers',
        }))
        const tables = _.map(this.tables.models, table => ({
          id  : table.get('id'),
          name: `${table.get('name')} (table)`,
          type: 'tables',
        }))
        const databases = _.map(this.databases.models, database => ({
          id  : database.get('id'),
          name: `${database.get('name')} (database)`,
          type: 'databases',
        }))
        return servers.concat(tables).concat(databases)
      },
      property: 'name',
      onselect ({ type, id }) {
        return app.main.router.navigate(`#${type}/${id}`, { trigger: true })
      },
    })
  }

  render (route) {
    this.$el.html(this.template())
    this.set_active_tab(route)
    return this
  }

  set_active_tab (route) {
    if (route != null) {
      switch (route) {
        case 'dashboard':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-dashboard').addClass('active')
        case 'index_tables':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-tables').addClass('active')
        case 'table':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-tables').addClass('active')
        case 'database':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-tables').addClass('active')
        case 'index_servers':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-servers').addClass('active')
        case 'server':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-servers').addClass('active')
        case 'datacenter':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-servers').addClass('active')
        case 'dataexplorer':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-dataexplorer').addClass('active')
        case 'logs':
          this.$('ul.nav-left li').removeClass('active')
          return this.$('li#nav-logs').addClass('active')
      }
    }
  }

  update_cog_icon (event) {
    this.$('.cog_icon').toggleClass('active')
    return this.container.toggle_options(event)
  }
}
NavBarView.initClass()

exports.NavBarView = NavBarView

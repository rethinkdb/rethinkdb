// TopBar view

const issues = require('./issues.coffee')
const app = require('./app.coffee')
const { driver } = app

class Container extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove_parent_alert = this.remove_parent_alert.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'sidebar-container'
    this.prototype.template = require('../handlebars/sidebar-container.hbs')
  }

  initialize (data) {
    this.model = data.model
    this.issues = data.issues

    this.client_panel = new ClientConnectionStatus({
      model: this.model,
    })
    this.servers_panel = new ServersConnected({
      model: this.model,
    })
    this.tables_panel = new TablesAvailable({
      model: this.model,
    })
    this.issues_panel = new Issues({
      model: this.model,
    })
    return this.issues_banner = new issues.IssuesBanner({
      model     : this.model,
      collection: this.issues,
    })
  }

  render () {
    this.$el.html(this.template())

    // Render connectivity status
    this.$('.client-connection-status').html(this.client_panel.render().el)
    this.$('.servers-connected').html(this.servers_panel.render().el)
    this.$('.tables-available').html(this.tables_panel.render().el)

    // Render issue summary and issue banner
    this.$('.issues').html(this.issues_panel.render().el)
    this.$el.append(this.issues_banner.render().el)
    return this
  }

  remove_parent_alert ({ target }) {
    const element = $(target).parent()
    return element.slideUp('fast', () => {
      element.remove()
      this.issues_being_resolved()
      return this.issues_banner.render()
    })
  }

  remove () {
    this.client_panel.remove()
    this.servers_panel.remove()
    this.tables_panel.remove()
    this.issues_panel.remove()
    this.issues_banner.remove()
    return super.remove()
  }
}
Container.initClass()

class ClientConnectionStatus extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'client-connection-status'
    this.prototype.template = require('../handlebars/sidebar-client_connection_status.hbs')
  }

  initialize () {
    return this.listenTo(this.model, 'change:me', this.render)
  }

  render () {
    this.$el.html(
      this.template({
        disconnected: false,
        me          : this.model.get('me'),
      })
    )
    return this
  }

  remove () {
    this.stopListening()
    return super.remove()
  }
}
ClientConnectionStatus.initClass()

class ServersConnected extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = require('../handlebars/sidebar-servers_connected.hbs')
  }

  initialize () {
    return this.listenTo(this.model, 'change:num_servers', this.render)
  }

  render () {
    this.$el.html(
      this.template({
        num_servers: this.model.get('num_servers'),
      })
    )
    return this
  }

  remove () {
    this.stopListening()
    return super.remove()
  }
}
ServersConnected.initClass()

class TablesAvailable extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = require('../handlebars/sidebar-tables_available.hbs')
  }

  initialize () {
    this.listenTo(this.model, 'change:num_tables', this.render)
    return this.listenTo(this.model, 'change:num_available_tables', this.render)
  }

  render () {
    this.$el.html(
      this.template({
        num_tables          : this.model.get('num_tables'),
        num_available_tables: this.model.get('num_available_tables'),
        error               : this.model.get('num_available_tables') < this.model.get('num_tables'),
      })
    )
    return this
  }

  remove () {
    this.stopListening()
    return super.remove()
  }
}
TablesAvailable.initClass()

// Issue count panel at the top
class Issues extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'issues'
    this.prototype.template = require('../handlebars/sidebar-issues.hbs')
  }

  initialize () {
    return this.listenTo(this.model, 'change:num_issues', this.render)
  }

  render () {
    this.$el.html(
      this.template({
        num_issues: this.model.get('num_issues'),
      })
    )
    return this
  }

  remove () {
    this.stopListening()
    return super.remove()
  }
}
Issues.initClass()

exports.Container = Container
exports.ClientConnectionStatus = ClientConnectionStatus
exports.ServersConnected = ServersConnected
exports.TablesAvailable = TablesAvailable
exports.Issues = Issues

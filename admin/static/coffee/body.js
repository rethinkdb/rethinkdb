// Copyright 2010-2012 RethinkDB, all rights reserved.

const app = require('./app.coffee')
const { driver } = app
const { system_db } = app
const topbar = require('./topbar.coffee')
const navbar = require('./navbar.coffee')
const models = require('./models.coffee')
const modals = require('./modals.coffee')
const router = require('./router.coffee')
const VERSION = require('rethinkdb-version')

const r = require('rethinkdb')

class MainContainer extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.start_router = this.start_router.bind(this)
    this.fetch_ajax_data = this.fetch_ajax_data.bind(this)
    this.fetch_data = this.fetch_data.bind(this)
    this.render = this.render.bind(this)
    this.hide_options = this.hide_options.bind(this)
    this.toggle_options = this.toggle_options.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = require('../handlebars/body-structure.hbs')
    this.prototype.id = 'main_view'
  }

  initialize () {
    this.fetch_ajax_data()

    this.databases = new models.Databases()
    this.tables = new models.Tables()
    this.servers = new models.Servers()
    this.issues = new models.Issues()
    this.dashboard = new models.Dashboard()

    this.alert_update_view = new AlertUpdates()

    this.options_view = new OptionsView({
      alert_update_view: this.alert_update_view,
    })
    this.options_state = 'hidden'

    this.navbar = new navbar.NavBarView({
      databases   : this.databases,
      tables      : this.tables,
      servers     : this.servers,
      options_view: this.options_view,
      container   : this,
    })

    return this.topbar = new topbar.Container({
      model : this.dashboard,
      issues: this.issues,
    })
  }

  // Should be started after the view is injected in the DOM tree
  start_router () {
    this.router = new router.BackboneCluster({
      navbar: this.navbar,
    })
    Backbone.history.start()

    return this.navbar.init_typeahead()
  }

  fetch_ajax_data () {
    return driver.connect((error, conn) => {
      if (error != null) {
        console.log(error)
        return this.fetch_data(null)
      } else {
        return conn.server((error, me) => {
          driver.close(conn)
          if (error != null) {
            console.log(error)
            return this.fetch_data(null)
          } else {
            return this.fetch_data(me)
          }
        })
      }
    })
  }

  fetch_data ({ proxy, name }) {
    const query = r.expr({
      tables              : r.db(system_db).table('table_config').pluck('db', 'name', 'id').coerceTo('ARRAY'),
      servers             : r.db(system_db).table('server_config').pluck('name', 'id').coerceTo('ARRAY'),
      issues              : driver.queries.issues_with_ids(),
      num_issues          : r.db(system_db).table('current_issues').count(),
      num_servers         : r.db(system_db).table('server_config').count(),
      num_tables          : r.db(system_db).table('table_config').count(),
      num_available_tables: r
        .db(system_db)
        .table('table_status')('status')
        .filter(status => status('all_replicas_ready'))
        .count(),
      me: proxy ? '<proxy node>' : name,
    })

    return this.timer = driver.run(query, 5000, (error, result) => {
      if (error != null) {
        return console.log(error)
      } else {
        for (let table of Array.from(result.tables)) {
          this.tables.add(new models.Table(table), { merge: true })
          delete result.tables
        }
        for (let server of Array.from(result.servers)) {
          this.servers.add(new models.Server(server), { merge: true })
          delete result.servers
        }
        this.issues.set(result.issues)
        delete result.issues

        return this.dashboard.set(result)
      }
    })
  }

  render () {
    this.$el.html(this.template())
    this.$('#updates_container').html(this.alert_update_view.render().$el)
    this.$('#options_container').html(this.options_view.render().$el)
    this.$('#topbar').html(this.topbar.render().$el)
    this.$('#navbar-container').html(this.navbar.render().$el)

    return this
  }

  hide_options () {
    return this.$('#options_container').slideUp('fast')
  }

  toggle_options (event) {
    event.preventDefault()

    if (this.options_state === 'visible') {
      this.options_state = 'hidden'
      return this.hide_options(event)
    } else {
      this.options_state = 'visible'
      return this.$('#options_container').slideDown('fast')
    }
  }

  remove () {
    driver.stop_timer(this.timer)
    this.alert_update_view.remove()
    this.options_view.remove()
    return this.navbar.remove()
  }
}
MainContainer.initClass()

class OptionsView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.turn_updates_on = this.turn_updates_on.bind(this)
    this.turn_updates_off = this.turn_updates_off.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'options_background'
    this.prototype.template = require('../handlebars/options_view.hbs')

    this.prototype.events = {
      'click label[for=updates_yes]': 'turn_updates_on',
      'click label[for=updates_no]' : 'turn_updates_off',
    }
  }

  initialize ({ alert_update_view }) {
    return this.alert_update_view = alert_update_view
  }

  render () {
    this.$el.html(
      this.template({
        check_update: (
          __guard__(window.localStorage, ({ check_updates }) => check_updates) != null
            ? JSON.parse(window.localStorage.check_updates)
            : true
        ),
        version: VERSION,
      })
    )
    return this
  }

  turn_updates_on (event) {
    window.localStorage.check_updates = JSON.stringify(true)
    window.localStorage.removeItem('ignore_version')
    return this.alert_update_view.check()
  }

  turn_updates_off (event) {
    window.localStorage.check_updates = JSON.stringify(false)
    return this.alert_update_view.hide()
  }
}
OptionsView.initClass()

class AlertUpdates extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.close = this.close.bind(this)
    this.hide = this.hide.bind(this)
    this.check = this.check.bind(this)
    this.render_updates = this.render_updates.bind(this)
    this.compare_version = this.compare_version.bind(this)
    this.render = this.render.bind(this)
    this.deactivate_update = this.deactivate_update.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.has_update_template = require('../handlebars/has_update.hbs')
    this.prototype.className = 'settings alert'

    this.prototype.events = {
      'click .no_update_btn': 'deactivate_update',
      'click .close'        : 'close',
    }
  }

  initialize () {
    if (window.localStorage != null) {
      let check_updates
      try {
        check_updates = JSON.parse(window.localStorage.check_updates)
        if (check_updates !== false) {
          return this.check()
        }
      } catch (err) {
        // Non valid json doc in check_updates. Let's reset the setting
        window.localStorage.check_updates = JSON.stringify(true)
        return this.check()
      }
    } else {
      // No localstorage, let's just check for updates
      return this.check()
    }
  }

  // If the user close the alert, we hide the alert + save the version so we can ignore it
  close (event) {
    event.preventDefault()
    if (this.next_version != null) {
      window.localStorage.ignore_version = JSON.stringify(this.next_version)
    }
    return this.hide()
  }

  hide () {
    return this.$el.slideUp('fast')
  }

  check () {
    // If it's fail, it's fine - like if the user is just on a local network without access to the Internet.
    return $.getJSON(
      `https://update.rethinkdb.com/update_for/${VERSION}?callback=?`,
      this.render_updates
    )
  }

  // Callback on the ajax request
  render_updates ({ status, last_version, link_changelog }) {
    if (status === 'need_update') {
      let ignored_version
      try {
        ignored_version = JSON.parse(window.localStorage.ignore_version)
      } catch (err) {
        ignored_version = null
      }
      if (!ignored_version || this.compare_version(ignored_version, last_version) < 0) {
        this.next_version = last_version // Save it so users can ignore the update
        this.$el.html(
          this.has_update_template({
            last_version,
            link_changelog,
          })
        )
        return this.$el.slideDown('fast')
      }
    }
  }

  // Compare version with the format %d.%d.%d
  compare_version (v1, v2) {
    const v1_array_str = v1.split('.')
    const v2_array_str = v2.split('.')
    const v1_array = []
    for (var value of Array.from(v1_array_str)) {
      v1_array.push(parseInt(value))
    }
    const v2_array = []
    for (value of Array.from(v2_array_str)) {
      v2_array.push(parseInt(value))
    }

    for (let index = 0; index < v1_array.length; index++) {
      value = v1_array[index]
      if (value < v2_array[index]) {
        return -1
      } else if (value > v2_array[index]) {
        return 1
      }
    }
    return 0
  }

  render () {
    return this
  }

  deactivate_update () {
    this.$el.slideUp('fast')
    if (window.localStorage != null) {
      return window.localStorage.check_updates = JSON.stringify(false)
    }
  }
}
AlertUpdates.initClass()

class IsDisconnected extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.animate_loading = this.animate_loading.bind(this)
    this.display_fail = this.display_fail.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.el = 'body'
    this.prototype.className = 'is_disconnected_view'
    this.prototype.template = require('../handlebars/is_disconnected.hbs')
    this.prototype.message = require('../handlebars/is_disconnected_message.hbs')
  }
  initialize () {
    this.render()
    return setInterval(() => driver.run_once(r.expr(1)), 2000)
  }

  render () {
    this.$('#modal-dialog > .modal').css('z-index', '1')
    this.$('.modal-backdrop').remove()
    this.$el.append(this.template())
    this.$('.is_disconnected').modal({
      show    : true,
      backdrop: 'static',
    })
    return this.animate_loading()
  }

  animate_loading () {
    if (this.$('.three_dots_connecting')) {
      if (this.$('.three_dots_connecting').html() === '...') {
        this.$('.three_dots_connecting').html('')
      } else {
        this.$('.three_dots_connecting').append('.')
      }
      return setTimeout(this.animate_loading, 300)
    }
  }

  display_fail () {
    return this.$('.animation_state').fadeOut('slow', () => {
      $('.reconnecting_state').html(this.message)
      return $('.animation_state').fadeIn('slow')
    })
  }
}
IsDisconnected.initClass()

exports.MainContainer = MainContainer
exports.OptionsView = OptionsView
exports.AlertUpdates = AlertUpdates
exports.IsDisconnected = IsDisconnected

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

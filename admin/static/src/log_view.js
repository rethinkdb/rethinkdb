// Copyright 2010-2012 RethinkDB, all rights reserved.

const app = require('./app')
const { driver } = app
const models = require('./models')

class LogsContainer extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.increase_limit = this.increase_limit.bind(this)
    this.fetch_data = this.fetch_data.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = {
      main         : require('../handlebars/logs_container.hbs'),
      error        : require('../handlebars/error-query.hbs'),
      alert_message: require('../handlebars/alert_message.hbs'),
    }

    this.prototype.events = { 'click .next-log-entries': 'increase_limit' }
  }

  initialize (obj = {}) {
    this.limit = obj.limit || 20
    this.current_limit = this.limit
    this.server_id = obj.server_id
    this.container_rendered = false
    this.query = obj.query || driver.queries.all_logs
    this.logs = new models.Logs()
    this.logs_list = new LogsListView({
      collection: this.logs,
    })
    return this.fetch_data()
  }

  increase_limit (e) {
    e.preventDefault()
    this.current_limit += this.limit
    driver.stop_timer(this.timer)
    return this.fetch_data()
  }

  fetch_data () {
    return this.timer = driver.run(this.query(this.current_limit, this.server_id), 5000, (
      error,
      result
    ) => {
      if (error != null) {
        if (__guard__(this.error, ({ msg }) => msg) !== error.msg) {
          this.error = error
          return this.$el.html(
            this.template.error({
              url  : '#logs',
              error: error.message,
            })
          )
        }
      } else {
        result.forEach((log, index) => {
          this.logs.add(new models.Log(log), { merge: true, index })
        })

        return this.render()
      }
    })
  }

  render () {
    if (!this.container_rendered) {
      this.$el.html(this.template.main)
      this.$('.log-entries-wrapper').html(this.logs_list.render().$el)
      this.container_rendered = true
    } else {
      this.logs_list.render()
    }
    return this
  }

  remove () {
    driver.stop_timer(this.timer)
    return super.remove()
  }
}
LogsContainer.initClass()

class LogsListView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.tagName = 'ul'
    this.prototype.className = 'log-entries'
  }
  initialize () {
    this.log_view_cache = {}

    if (this.collection.length === 0) {
      this.$('.no-logs').show()
    } else {
      this.$('.no-logs').hide()
    }

    this.listenTo(this.collection, 'add', log => {
      let view = this.log_view_cache[log.get('id')]
      if (view == null) {
        view = new LogView({
          model: log,
        })
        this.log_view_cache[log.get('id')]
      }

      const position = this.collection.indexOf(log)
      if (this.collection.length === 1) {
        this.$el.html(view.render().$el)
      } else if (position === 0) {
        this.$('.log-entry').prepend(view.render().$el)
      } else {
        this.$('.log-entry').eq(position - 1).after(view.render().$el)
      }

      return this.$('.no-logs').hide()
    })

    return this.listenTo(this.collection, 'remove', log => {
      const log_view = log_view_cache[log.get('id')]
      log_view.$el.slideUp('fast', () => {
        return log_view.remove()
      })
      delete log_view[log.get('id')]
      if (this.collection.length === 0) {
        return this.$('.no-logs').show()
      }
    })
  }
  render () {
    return this
  }
}
LogsListView.initClass()

class LogView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.tagName = 'li'
    this.prototype.className = 'log-entry'
    this.prototype.template = require('../handlebars/log-entry.hbs')
  }
  initialize () {
    return this.model.on('change', this.render)
  }

  render () {
    const template_model = this.model.toJSON()
    template_model['iso'] = template_model.timestamp.toISOString()
    this.$el.html(this.template(template_model))
    this.$('time.timeago').timeago()
    return this
  }

  remove () {
    this.stopListening()
    return super.remove()
  }
}
LogView.initClass()

exports.LogsContainer = LogsContainer
exports.LogsListView = LogsListView
exports.LogView = LogView

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

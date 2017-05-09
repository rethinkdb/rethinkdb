// Copyright 2010-2015 RethinkDB

const app = require('../app')
const { driver } = app
const { system_db } = app
const models = require('../models')
const modals = require('../modals')

const r = require('rethinkdb')

class DatabasesContainer extends Backbone.View {
  constructor (...args) {
    this.query_callback = this.query_callback.bind(this)
    this.add_database = this.add_database.bind(this)
    this.delete_tables = this.delete_tables.bind(this)
    this.update_delete_tables_button = this.update_delete_tables_button.bind(this)
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.render_message = this.render_message.bind(this)
    this.fetch_data_once = this.fetch_data_once.bind(this)
    this.fetch_data = this.fetch_data.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.id = 'databases_container'
    this.prototype.template = {
      main         : require('../../handlebars/databases_container.hbs'),
      error        : require('../../handlebars/error-query.hbs'),
      alert_message: require('../../handlebars/alert_message.hbs'),
    }

    this.prototype.events = {
      'click .add_database'      : 'add_database',
      'click .remove-tables'     : 'delete_tables',
      'click .checkbox-container': 'update_delete_tables_button',
      'click .close'             : 'remove_parent_alert',
    }
  }

  query_callback (error, result) {
    this.loading = false
    if (error != null) {
      if (__guard__(this.error, ({ msg }) => msg) !== error.msg) {
        this.error = error
        return this.$el.html(
          this.template.error({
            url  : '#tables',
            error: error.message,
          })
        )
      }
    } else {
      return this.databases.set(
        _.map(_.sortBy(result, ({ name }) => name), database => new models.Database(database)),
        { merge: true }
      )
    }
  }

  add_database () {
    return this.add_database_dialog.render()
  }

  delete_tables ({ currentTarget }) {
    // Make sure the button isn't disabled, and pass the list of table ids selected
    if (!$(currentTarget).is(':disabled')) {
      const selected_tables = []
      this.$('.checkbox-table:checked').each(function () {
        return selected_tables.push({
          table   : JSON.parse($(this).data('table')),
          database: JSON.parse($(this).data('database')),
        })
      })

      return this.remove_tables_dialog.render(selected_tables)
    }
  }

  remove_parent_alert (event) {
    event.preventDefault()
    const element = $(event.target).parent()
    return element.slideUp('fast', () => element.remove())
  }

  update_delete_tables_button () {
    if (this.$('.checkbox-table:checked').length > 0) {
      return this.$('.remove-tables').prop('disabled', false)
    } else {
      return this.$('.remove-tables').prop('disabled', true)
    }
  }

  initialize () {
    if (app.view_data_backup.tables_view_databases == null) {
      app.view_data_backup.tables_view_databases = new models.Databases()
      this.loading = true
    } else {
      this.loading = false
    }
    this.databases = app.view_data_backup.tables_view_databases

    this.databases_list = new DatabasesListView({
      collection: this.databases,
      container : this,
    })

    this.query = r
      .db(system_db)
      .table('db_config')
      .filter(db => db('name').ne(system_db))
      .map(db => ({
        name  : db('name'),
        id    : db('id'),
        tables: r
          .db(system_db)
          .table('table_status')
          .orderBy(table => table('name'))
          .filter({ db: db('name') })
          .merge(table => ({
            shards        : table('shards').count().default(0),
            replicas      : table('shards').default([]).map(shard => shard('replicas').count()).sum(),
            replicas_ready: table('shards')
              .default([])
              .map(shard =>
                shard('replicas').filter(replica => replica('state').eq('ready')).count())
              .sum(),
            status: table('status'),
            id    : table('id'),
          })),
      }))

    this.fetch_data()

    this.add_database_dialog = new modals.AddDatabaseModal(this.databases)
    return this.remove_tables_dialog = new modals.RemoveTableModal({
      collection: this.databases,
    })
  }

  render () {
    this.$el.html(this.template.main({}))
    this.$('.databases_list').html(this.databases_list.render().$el)
    return this
  }

  render_message (message) {
    return this.$('#user-alert-space').append(
      this.template.alert_message({
        message,
      })
    )
  }

  fetch_data_once () {
    return this.timer = driver.run_once(this.query, this.query_callback)
  }

  fetch_data () {
    return this.timer = driver.run(this.query, 5000, this.query_callback)
  }

  remove () {
    driver.stop_timer(this.timer)
    this.add_database_dialog.remove()
    this.remove_tables_dialog.remove()
    return super.remove()
  }
}
DatabasesContainer.initClass()

class DatabasesListView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'database_list'
    this.prototype.template = {
      no_databases     : require('../../handlebars/no_databases.hbs'),
      loading_databases: require('../../handlebars/loading_databases.hbs'),
    }
  }

  initialize ({ container }) {
    let position
    let view
    this.container = container

    this.databases_view = []
    this.collection.each(database => {
      view = new DatabaseView({
        model: database,
      })

      this.databases_view.push(view)
      return this.$el.append(view.render().$el)
    })

    if (this.collection.length === 0) {
      this.$el.html(this.template.no_databases())
    } else if (this.container.loading) {
      this.$el.html(this.template.loading_databases())
    }

    this.listenTo(this.collection, 'add', database => {
      const new_view = new DatabaseView({
        model: database,
      })

      if (this.databases_view.length === 0) {
        this.databases_view.push(new_view)
        this.$el.html(new_view.render().$el)
      } else {
        let added = false
        for (position = 0; position < this.databases_view.length; position++) {
          view = this.databases_view[position]
          if (view.model.get('name') > database.get('name')) {
            added = true
            this.databases_view.splice(position, 0, new_view)
            if (position === 0) {
              this.$el.prepend(new_view.render().$el)
            } else {
              this.$('.database_container').eq(position - 1).after(new_view.render().$el)
            }
            break
          }
        }
        if (added === false) {
          this.databases_view.push(new_view)
          this.$el.append(new_view.render().$el)
        }
      }

      if (this.databases_view.length === 1) {
        return this.$('.no-databases').remove()
      }
    })

    return this.listenTo(this.collection, 'remove', database => {
      for (position = 0; position < this.databases_view.length; position++) {
        view = this.databases_view[position]
        if (view.model === database) {
          database.destroy()
          view.remove()
          this.databases_view.splice(position, 1)
          break
        }
      }

      if (this.collection.length === 0) {
        return this.$el.html(this.template.no_databases())
      }
    })
  }

  render () {
    return this
  }

  remove () {
    this.stopListening()
    for (let view of Array.from(this.databases_view)) {
      view.remove()
    }
    return super.remove()
  }
}
DatabasesListView.initClass()

class DatabaseView extends Backbone.View {
  constructor (...args) {
    this.delete_database = this.delete_database.bind(this)
    this.add_table = this.add_table.bind(this)
    this.initialize = this.initialize.bind(this)
    this.update_collection = this.update_collection.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'database_container section'
    this.prototype.template = {
      main : require('../../handlebars/database.hbs'),
      empty: require('../../handlebars/empty_list.hbs'),
    }

    this.prototype.events = {
      'click button.add-table'      : 'add_table',
      'click button.delete-database': 'delete_database',
    }
  }

  delete_database () {
    if (this.delete_database_dialog != null) {
      this.delete_database_dialog.remove()
    }
    this.delete_database_dialog = new modals.DeleteDatabaseModal()
    return this.delete_database_dialog.render(this.model)
  }

  add_table () {
    if (this.add_table_dialog != null) {
      this.add_table_dialog.remove()
    }
    this.add_table_dialog = new modals.AddTableModal({
      db_id  : this.model.get('id'),
      db_name: this.model.get('name'),
      tables : this.model.get('tables'),
    })
    return this.add_table_dialog.render()
  }

  initialize () {
    let position
    let view
    this.$el.html(this.template.main(this.model.toJSON()))

    this.tables_views = []
    this.collection = new models.Tables()

    this.update_collection()
    this.model.on('change', this.update_collection)

    this.collection.each(table => {
      view = new TableView({
        model: table,
      })

      this.tables_views.push(view)
      return this.$('.tables_container').append(view.render().$el)
    })

    if (this.collection.length === 0) {
      this.$('.tables_container').html(
        this.template.empty({
          element  : 'table',
          container: 'database',
        })
      )
    }

    this.listenTo(this.collection, 'add', table => {
      const new_view = new TableView({
        model: table,
      })

      if (this.tables_views.length === 0) {
        this.tables_views.push(new_view)
        this.$('.tables_container').html(new_view.render().$el)
      } else {
        let added = false
        for (position = 0; position < this.tables_views.length; position++) {
          view = this.tables_views[position]
          if (view.model.get('name') > table.get('name')) {
            added = true
            this.tables_views.splice(position, 0, new_view)
            if (position === 0) {
              this.$('.tables_container').prepend(new_view.render().$el)
            } else {
              this.$('.table_container').eq(position - 1).after(new_view.render().$el)
            }
            break
          }
        }
        if (added === false) {
          this.tables_views.push(new_view)
          this.$('.tables_container').append(new_view.render().$el)
        }
      }

      if (this.tables_views.length === 1) {
        return this.$('.no_element').remove()
      }
    })

    return this.listenTo(this.collection, 'remove', table => {
      for (position = 0; position < this.tables_views.length; position++) {
        view = this.tables_views[position]
        if (view.model === table) {
          table.destroy()
          view.remove()
          this.tables_views.splice(position, 1)
          break
        }
      }

      if (this.collection.length === 0) {
        return this.$('.tables_container').html(
          this.template.empty({
            element  : 'table',
            container: 'database',
          })
        )
      }
    })
  }

  update_collection () {
    return this.collection.set(_.map(this.model.get('tables'), table => new models.Table(table)), {
      merge: true,
    })
  }

  render () {
    return this
  }

  remove () {
    this.stopListening()
    for (let view of Array.from(this.tables_views)) {
      view.remove()
    }
    if (this.delete_database_dialog != null) {
      this.delete_database_dialog.remove()
    }
    if (this.add_table_dialog != null) {
      this.add_table_dialog.remove()
    }
    return super.remove()
  }
}
DatabaseView.initClass()

class TableView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'table_container'
    this.prototype.template = require('../../handlebars/table.hbs')
  }
  initialize () {
    return this.listenTo(this.model, 'change', this.render)
  }

  render () {
    this.$el.html(
      this.template({
        id            : this.model.get('id'),
        db_json       : JSON.stringify(this.model.get('db')),
        name_json     : JSON.stringify(this.model.get('name')),
        db            : this.model.get('db'),
        name          : this.model.get('name'),
        shards        : this.model.get('shards'),
        replicas      : this.model.get('replicas'),
        replicas_ready: this.model.get('replicas_ready'),
        status        : this.model.get('status'),
      })
    )
    return this
  }

  remove () {
    this.stopListening()
    return super.remove()
  }
}
TableView.initClass()

exports.DatabasesContainer = DatabasesContainer
exports.DatabasesListView = DatabasesListView
exports.DatabaseView = DatabaseView
exports.TableView = TableView

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

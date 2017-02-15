// Copyright 2010-2015 RethinkDB

const models = require('../models')
const modals = require('../modals')
const shard_assignments = require('./shard_assignments')
const shard_distribution = require('./shard_distribution')
const vis = require('../vis')
const ui_modals = require('../ui_components/modals')
const ui_progress = require('../ui_components/progressbar')
const app = require('../app')
const { driver } = app
const { system_db } = app

const r = require('rethinkdb')

class TableContainer extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.maybe_refetch_data = this.maybe_refetch_data.bind(this)
    this.add_error_message = this.add_error_message.bind(this)
    this.clear_cached_error_messages = this.clear_cached_error_messages.bind(this)
    this.guaranteed_query = this.guaranteed_query.bind(this)
    this.failable_index_query = this.failable_index_query.bind(this)
    this.failable_misc_query = this.failable_misc_query.bind(this)
    this.handle_failable_index_response = this.handle_failable_index_response.bind(this)
    this.handle_failable_misc_response = this.handle_failable_misc_response.bind(this)
    this.handle_guaranteed_response = this.handle_guaranteed_response.bind(this)
    this.fetch_data = this.fetch_data.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = {
      not_found: require('../../handlebars/element_view-not_found.hbs'),
      error    : require('../../handlebars/error-query.hbs'),
    }
    this.prototype.className = 'table-view'

    this.prototype.fetch_once = _.debounce(
      () => {
        // fetches data once, but only does it if we flap less than
        // once every 2 seconds
        driver.run_once(this.failable_misc_query, this.failable_misc_handler)
        return driver.run_once(this.guaranteed_query, this.guaranted_handler)
      },
      2000,
      true
    )
  }
  initialize (id) {
    this.id = id
    this.table_found = true

    this.indexes = null
    this.distribution = new models.Distribution()

    this.guaranteed_timer = null
    this.failable_index_timer = null
    this.failable_misc_timer = null

    // This is the cache of error messages we've received from
    // @failable_index_query and @failable_misc_query
    this.current_errors = { index: [], misc: [] }

    // Initialize the model with mostly empty dummy data so we
    // can render it right away
    //
    // We have two types of errors that can trip us
    // up. info_unavailable errors happen when a you can't
    // fetch table.info(). index_unavailable errors happen when
    // you can't fetch table.indexStatus()
    this.model = new models.Table({
      id,
      info_unavailable : false,
      index_unavailable: false,
    })
    this.listenTo(this.model, 'change:index_unavailable', () => this.maybe_refetch_data())
    this.table_view = new TableMainView({
      model             : this.model,
      indexes           : this.indexes,
      distribution      : this.distribution,
      shards_assignments: this.shards_assignments,
    })

    return this.fetch_data()
  }

  maybe_refetch_data () {
    if (this.model.get('info_unavailable') && !this.model.get('index_unavailable')) {
      // This is triggered when we go from
      // table.indexStatus() failing to it succeeding. If
      // that's the case, we should refetch everything. What
      // happens is we try to fetch indexes every 1s, but
      // more expensive stuff like table.info() only every
      // 10s. To avoid having the table UI in an error state
      // longer than we need to, we use the secondary index
      // query as a canary in the coalmine to decide when to
      // try again with the info queries.

      return this.fetch_once()
    }
  }

  add_error_message (type, error) {
    // This avoids spamming the console with the same error
    // message. It keeps a cache of messages, prints out
    // anything new, and resets the cache once we stop
    // receiving errors
    if (!Array.from(this.current_errors[type]).includes(error)) {
      console.log(`${type} error: ${error}`)
      return this.current_errors[type].push(error)
    }
  }

  clear_cached_error_messages (type) {
    // We clear the cache of errors once a successful query
    // makes it through
    if (this.current_errors[type].length > 0) {
      return this.current_errors[type] = []
    }
  }

  guaranteed_query () {
    // This query should succeed as long as the cluster is
    // available, since we get it from the system tables.  Info
    // we don't get from the system tables includes things like
    // table/shard counts, or secondary index status. Those are
    // obtained from table.info() and table.indexStatus() below
    // in `failable_*_query` so if they fail we still get the
    // data available in the system tables.
    return r.do(
      r.db(system_db).table('server_config').coerceTo('array'),
      r.db(system_db).table('table_status').get(this.id),
      r.db(system_db).table('table_config').get(this.id),
      (server_config, table_status, table_config) => r.branch(
        table_status.eq(null),
        null,
        table_status
          .merge({
            max_shards         : 64,
            num_shards         : table_config('shards').count().default(0),
            num_servers        : server_config.count(),
            num_default_servers: server_config
              .filter(server => server('tags').contains('default'))
              .count(),
            max_num_shards: r
              .expr([
                64,
                server_config.filter(server => server('tags').contains('default')).count(),
              ])
              .min(),
            num_primary_replicas: table_status('shards')
              .count(row => row('primary_replicas').isEmpty().not())
              .default(0),
            num_replicas: table_status('shards')
              .default([])
              .map(shard => shard('replicas').count())
              .sum(),
            num_available_replicas: table_status('shards')
              .concatMap(shard => shard('replicas').filter({ state: 'ready' }))
              .count()
              .default(0),
            num_replicas_per_shard: table_config('shards')
              .default([])
              .map(shard => shard('replicas').count())
              .max()
              .default(0),
            status: table_status('status'),
            id    : table_status('id'),
            // These are updated below if the table is ready
          })
          .without('shards')
      )
    )
  }

  failable_index_query () {
    // This query only checks the status of secondary indexes.
    // It can throw an exception and failif the primary
    // replica is unavailable (which happens immediately after
    // a reconfigure).
    return r.do(
      r.db(system_db).table('table_config').get(this.id),
      table_config =>
        r
          .db(table_config('db'))
          .table(table_config('name'), { read_mode: 'single' })
          .indexStatus()
          .pluck('index', 'ready', 'progress')
          .merge(index => ({
            id   : index('index'),
            db   : table_config('db'),
            table: table_config('name'),
          })) // add an id for backbone
    )
  }

  failable_misc_query () {
    // Query to load data distribution and shard assignments.
    // We keep this separate from the failable_index_query because we don't want
    // to run it as often.
    // This query can throw an exception and failif the primary
    // replica is unavailable (which happens immediately after
    // a reconfigure). Since this query makes use of
    // table.info(), it's separated from the guaranteed query
    return r.do(
      r.db(system_db).table('table_status').get(this.id),
      r.db(system_db).table('table_config').get(this.id),
      r
        .db(system_db)
        .table('server_config')
        .map(x => [x('name'), x('id')])
        .coerceTo('ARRAY')
        .coerceTo('OBJECT'),
      (table_status, table_config, server_name_to_id) => table_status.merge({
        distribution: r
          .db(table_status('db'))
          .table(table_status('name'), { read_mode: 'single' })
          .info()('doc_count_estimates')
          .map(r.range(), (num_keys, position) => ({
            num_keys,
            id: position,
          }))
          .coerceTo('array'),
        total_keys: r
          .db(table_status('db'))
          .table(table_status('name'), { read_mode: 'single' })
          .info()('doc_count_estimates')
          .sum(),
        shard_assignments: table_status.merge(table_status =>
          table_status('shards')
            .default([])
            .map(
              table_config('shards').default([]),
              r.db(table_status('db')).table(table_status('name'), { read_mode: 'single' }).info()(
                'doc_count_estimates'
              ),
              (status, config, estimate) => ({
                num_keys: estimate,
                replicas: status('replicas')
                  .merge(replica => ({
                    id                : server_name_to_id(replica('server')).default(null),
                    configured_primary: config('primary_replica').eq(replica('server')),
                    currently_primary : status('primary_replicas').contains(replica('server')),
                    nonvoting         : config('nonvoting_replicas').contains(replica('server')),
                  }))
                  .orderBy(r.desc('currently_primary'), r.desc('configured_primary')),
              })
            )),
      })
    )
  }

  handle_failable_index_response (error, result) {
    // Handle the result of failable_index_query
    if (error != null) {
      this.add_error_message('index', error.msg)
      this.model.set('index_unavailable', true)
      return
    }
    this.model.set('index_unavailable', false)
    this.clear_cached_error_messages('index')
    if (this.indexes != null) {
      this.indexes.set(_.map(result, index => new models.Index(index)))
    } else {
      this.indexes = new models.Indexes(_.map(result, index => new models.Index(index)))
    }
    __guard__(this.table_view, x => x.set_indexes(this.indexes))

    this.model.set({
      indexes: result,
    })
    if (this.table_view == null) {
      this.table_view = new TableMainView({
        model             : this.model,
        indexes           : this.indexes,
        distribution      : this.distribution,
        shards_assignments: this.shards_assignments,
      })
      return this.render()
    }
  }

  handle_failable_misc_response (error, result) {
    if (error != null) {
      this.add_error_message('misc', error.msg)
      this.model.set('info_unavailable', true)
      return
    }
    this.model.set('info_unavailable', false)
    this.clear_cached_error_messages('misc')
    if (this.distribution != null) {
      this.distribution.set(_.map(result.distribution, shard => new models.Shard(shard)))
      this.distribution.trigger('update')
    } else {
      this.distribution = new models.Distribution(
        _.map(result.distribution, shard => new models.Shard(shard))
      )
      __guard__(this.table_view, x => x.set_distribution(this.distribution))
    }

    if (this.shards_assignments != null) {
      this.shards_assignments.set(
        _.map(result.shard_assignments, shard => new models.ShardAssignment(shard))
      )
    } else {
      this.shards_assignments = new models.ShardAssignments(
        _.map(result.shard_assignments, shard => new models.ShardAssignment(shard))
      )
      __guard__(this.table_view, x1 => x1.set_assignments(this.shards_assignments))
    }

    this.model.set(result)
    if (this.table_view == null) {
      this.table_view = new TableMainView({
        model             : this.model,
        indexes           : this.indexes,
        distribution      : this.distribution,
        shards_assignments: this.shards_assignments,
      })
      return this.render()
    }
  }

  handle_guaranteed_response (error, result) {
    if (error != null) {
      // TODO: We may want to render only if we failed to open a connection
      // TODO: Handle when the table is deleted
      this.error = error
      return this.render()
    } else {
      let rerender = this.error != null
      this.error = null
      if (result === null) {
        rerender = rerender || this.table_found
        this.table_found = false
        // Reset the data
        this.indexes = null
        this.render()
      } else {
        rerender = rerender || !this.table_found
        this.table_found = true
        this.model.set(result)
      }

      if (rerender) {
        return this.render()
      }
    }
  }

  fetch_data () {
    // This timer keeps track of the failable index query, so we can
    // cancel it when we navigate away from the table page.
    this.failable_index_timer = driver.run(
      this.failable_index_query(),
      1000,
      this.handle_failable_index_response
    )
    // This timer keeps track of the failable misc query, so we can
    // cancel it when we navigate away from the table page.
    this.failable_misc_timer = driver.run(
      this.failable_misc_query(),
      10000,
      this.handle_failable_misc_response
    )

    // This timer keeps track of the guaranteed query, running
    // it every 5 seconds. We cancel it when navigating away
    // from the table page.
    return this.guaranteed_timer = driver.run(
      this.guaranteed_query(),
      5000,
      this.handle_guaranteed_response
    )
  }

  render () {
    if (this.error != null) {
      this.$el.html(
        this.template.error({
          error: __guard__(this.error, ({ message }) => message),
          url  : `#tables/${this.id}`,
        })
      )
    } else {
      if (this.table_found) {
        this.$el.html(this.table_view.render().$el)
      } else {
        // In this case, the query returned null, so the table was not found
        this.$el.html(
          this.template.not_found({
            id          : this.id,
            type        : 'table',
            type_url    : 'tables',
            type_all_url: 'tables',
          })
        )
      }
    }
    return this
  }
  remove () {
    driver.stop_timer(this.guaranteed_timer)
    driver.stop_timer(this.failable_index_timer)
    driver.stop_timer(this.failable_misc_timer)
    __guard__(this.table_view, x => x.remove())
    return super.remove()
  }
}
TableContainer.initClass()

class TableMainView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.set_indexes = this.set_indexes.bind(this)
    this.set_distribution = this.set_distribution.bind(this)
    this.set_assignments = this.set_assignments.bind(this)
    this.render = this.render.bind(this)
    this.rename_table = this.rename_table.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'namespace-view'
    this.prototype.template = {
      main : require('../../handlebars/table_container.hbs'),
      alert: null, // unused, delete this
    }

    this.prototype.events = {
      'click .close'             : 'close_alert',
      'click .operations .rename': 'rename_table',
      'click .operations .delete': 'delete_table',
    }
  }

  initialize ({ indexes, distribution, shards_assignments }, options) {
    this.indexes = indexes
    this.distribution = distribution
    this.shards_assignments = shards_assignments

    // Panels for namespace view
    this.title = new Title({
      model: this.model,
    })
    this.profile = new Profile({
      model: this.model,
    })

    this.secondary_indexes_view = new SecondaryIndexesView({
      collection: this.indexes,
      model     : this.model,
    })
    this.shard_distribution = new shard_distribution.ShardDistribution({
      collection: this.distribution,
      model     : this.model,
    })
    this.server_assignments = new shard_assignments.ShardAssignmentsView({
      model     : this.model,
      collection: this.shards_assignments,
    })
    this.reconfigure = new ReconfigurePanel({
      model: this.model,
    })

    this.stats = new models.Stats()
    this.stats_timer = driver.run(
      r.db(system_db).table('stats').get(['table', this.model.get('id')]).do(stat => ({
        keys_read: stat('query_engine')('read_docs_per_sec'),
        keys_set : stat('query_engine')('written_docs_per_sec'),
      })),
      1000,
      this.stats.on_result
    )

    return this.performance_graph = new vis.OpsPlot(this.stats.get_stats, {
      width  : 564, // width in pixels
      height : 210, // height in pixels
      seconds: 73, // num seconds to track
      type   : 'table',
    })
  }

  set_indexes (indexes) {
    if (this.indexes == null) {
      this.indexes = indexes
      return this.secondary_indexes_view.set_indexes(this.indexes)
    }
  }

  set_distribution (distribution) {
    this.distribution = distribution
    return this.shard_distribution.set_distribution(this.distribution)
  }

  set_assignments (shards_assignments) {
    this.shards_assignments = shards_assignments
    return this.server_assignments.set_assignments(this.shards_assignments)
  }

  render () {
    this.$el.html(
      this.template.main({
        namespace_id: this.model.get('id'),
      })
    )

    // fill the title of this page
    this.$('.main_title').html(this.title.render().$el)

    // Add the replica and shards views
    this.$('.profile').html(this.profile.render().$el)

    this.$('.performance-graph').html(this.performance_graph.render().$el)

    // Display the shards
    this.$('.sharding').html(this.shard_distribution.render().el)

    // Display the server assignments
    this.$('.server-assignments').html(this.server_assignments.render().el)

    // Display the secondary indexes
    this.$('.secondary_indexes').html(this.secondary_indexes_view.render().el)

    // Display server reconfiguration
    this.$('.reconfigure-panel').html(this.reconfigure.render().el)

    return this
  }

  close_alert (event) {
    event.preventDefault()
    return $(event.currentTarget).parent().slideUp('fast', function () {
      return $(this).remove()
    })
  }

  // Rename operation
  rename_table (event) {
    event.preventDefault()
    if (this.rename_modal != null) {
      this.rename_modal.remove()
    }
    this.rename_modal = new ui_modals.RenameItemModal({
      model: this.model,
    })
    return this.rename_modal.render()
  }

  // Delete operation
  delete_table (event) {
    event.preventDefault()
    if (this.remove_table_dialog != null) {
      this.remove_table_dialog.remove()
    }
    this.remove_table_dialog = new modals.RemoveTableModal()

    return this.remove_table_dialog.render([
      {
        table   : this.model.get('name'),
        database: this.model.get('db'),
      },
    ])
  }

  remove () {
    this.title.remove()
    this.profile.remove()
    this.shard_distribution.remove()
    this.server_assignments.remove()
    this.performance_graph.remove()
    this.secondary_indexes_view.remove()
    this.reconfigure.remove()

    driver.stop_timer(this.stats_timer)

    if (this.remove_table_dialog != null) {
      this.remove_table_dialog.remove()
    }

    if (this.rename_modal != null) {
      this.rename_modal.remove()
    }
    return super.remove()
  }
}
TableMainView.initClass()

class ReconfigurePanel extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.launch_modal = this.launch_modal.bind(this)
    this.remove = this.remove.bind(this)
    this.fetch_progress = this.fetch_progress.bind(this)
    this.render_status = this.render_status.bind(this)
    this.render = this.render.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'reconfigure-panel'
    this.prototype.templates = {
      main  : require('../../handlebars/reconfigure.hbs'),
      status: require('../../handlebars/replica_status.hbs'),
    }
    this.prototype.events = { 'click .reconfigure.btn': 'launch_modal' }
  }

  initialize ({ model }) {
    this.model = model
    this.listenTo(this.model, 'change:num_shards', this.render)
    this.listenTo(this.model, 'change:num_replicas_per_shard', this.render)
    this.listenTo(this.model, 'change:num_available_replicas', this.render_status)
    this.progress_bar = new ui_progress.OperationProgressBar(this.templates.status)
    return this.timer = null
  }

  launch_modal () {
    if (this.reconfigure_modal != null) {
      this.reconfigure_modal.remove()
    }
    this.reconfigure_modal = new modals.ReconfigureModal({
      model: new models.Reconfigure({
        parent                : this,
        id                    : this.model.get('id'),
        db                    : this.model.get('db'),
        name                  : this.model.get('name'),
        total_keys            : this.model.get('total_keys'),
        shards                : [],
        max_shards            : this.model.get('max_shards'),
        num_shards            : this.model.get('num_shards'),
        num_servers           : this.model.get('num_servers'),
        num_default_servers   : this.model.get('num_default_servers'),
        max_num_shards        : this.model.get('max_num_shards'),
        num_replicas_per_shard: this.model.get('num_replicas_per_shard'),
      }),
    })
    return this.reconfigure_modal.render()
  }

  remove () {
    if (this.reconfigure_modal != null) {
      this.reconfigure_modal.remove()
    }
    if (this.timer != null) {
      driver.stop_timer(this.timer)
      this.timer = null
    }
    this.progress_bar.remove()
    return super.remove()
  }

  fetch_progress () {
    const query = r
      .db(system_db)
      .table('table_status')
      .get(this.model.get('id'))('shards')
      .default([])('replicas')
      .concatMap(x => x)
      .do(replicas => ({
        num_available_replicas: replicas.filter({ state: 'ready' }).count(),
        num_replicas          : replicas.count(),
      }))
    if (this.timer != null) {
      driver.stop_timer(this.timer)
      this.timer = null
    }
    return this.timer = driver.run(query, 1000, (error, result) => {
      if (error != null) {
        // This can happen if the table is temporarily
        // unavailable. We log the error, and ignore it
        console.log('Nothing bad - Could not fetch replicas statuses')
        return console.log(error)
      } else {
        this.model.set(result)
        return this.render_status()
      }
    }) // Force to refresh the progress bar
  }

  // Render the status of the replicas and the progress bar if needed
  render_status () {
    // TODO Handle backfilling when available in the api directly
    if (this.model.get('num_available_replicas') < this.model.get('num_replicas')) {
      if (this.timer == null) {
        this.fetch_progress()
        return
      }

      if (this.progress_bar.get_stage() === 'none') {
        this.progress_bar.skip_to_processing()
      }
    } else {
      if (this.timer != null) {
        driver.stop_timer(this.timer)
        this.timer = null
      }
    }

    return this.progress_bar.render(
      this.model.get('num_available_replicas'),
      this.model.get('num_replicas'),
      { got_response: true }
    )
  }

  render () {
    this.$el.html(this.templates.main(this.model.toJSON()))
    if (this.model.get('num_available_replicas') < this.model.get('num_replicas')) {
      if (this.progress_bar.get_stage() === 'none') {
        this.progress_bar.skip_to_processing()
      }
    }
    this
      .$('.backfill-progress')
      .html(
        this.progress_bar.render(
          this.model.get('num_available_replicas'),
          this.model.get('num_replicas'),
          { got_response: true }
        ).$el
      )
    return this
  }
}
ReconfigurePanel.initClass()

class ReconfigureDiffView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    super(...args)
  }

  static initClass () {
    // This is just for the diff view in the reconfigure
    // modal. It's so that the diff can be rerendered independently
    // of the modal itself (which includes the input form). If you
    // rerended the entire modal, you lose focus in the text boxes
    // since handlebars creates an entirely new dom tree.
    //
    // You can find the ReconfigureModal in src/modals.js

    this.prototype.className = 'reconfigure-diff'
    this.prototype.template = require('../../handlebars/reconfigure-diff.hbs')
  }
  initialize () {
    return this.listenTo(this.model, 'change:shards', this.render)
  }

  render () {
    this.$el.html(this.template(this.model.toJSON()))
    return this
  }
}
ReconfigureDiffView.initClass()

class Title extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'namespace-info-view'
    this.prototype.template = require('../../handlebars/table_title.hbs')
  }
  initialize () {
    return this.listenTo(this.model, 'change:name', this.render)
  }

  render () {
    this.$el.html(
      this.template({
        name: this.model.get('name'),
        db  : this.model.get('db'),
      })
    )
    return this
  }

  remove () {
    this.stopListening()
    return super.remove()
  }
}
Title.initClass()

// Profile view
class Profile extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = require('../../handlebars/table_profile.hbs')
  }

  initialize () {
    this.listenTo(this.model, 'change', this.render)
    return this.indicator = new TableStatusIndicator({
      model: this.model,
    })
  }

  render () {
    const obj = {
      status                : this.model.get('status'),
      total_keys            : this.model.get('total_keys'),
      num_shards            : this.model.get('num_shards'),
      num_primary_replicas  : this.model.get('num_primary_replicas'),
      num_replicas          : this.model.get('num_replicas'),
      num_available_replicas: this.model.get('num_available_replicas'),
    }

    if (
      __guard__(obj.status, ({ ready_for_writes }) => ready_for_writes) &&
      !__guard__(obj, ({ status }) => status.all_replicas_ready)
    ) {
      obj.parenthetical = true
    }

    this.$el.html(this.template(obj))
    this.$('.availability').prepend(this.indicator.$el)

    return this
  }
  remove () {
    this.indicator.remove()
    return super.remove()
  }
}
Profile.initClass()

class TableStatusIndicator extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.status_to_class = this.status_to_class.bind(this)
    this.render = this.render.bind(this)
    super(...args)
  }

  static initClass () {
    // This is an svg element showing the table's status in the
    // appropriate color. Unlike a lot of other views, its @$el is
    // set to the svg element from the template, rather than being
    // wrapped.
    this.prototype.template = require('../../handlebars/table_status_indicator.hbs')
  }

  initialize () {
    const status_class = this.status_to_class(this.model.get('status'))
    this.setElement(this.template({ status_class }))
    this.render()
    return this.listenTo(this.model, 'change:status', this.render)
  }

  status_to_class (status) {
    if (!status) {
      return 'unknown'
    }
    // ensure this matches "humanize_table_readiness" in util.js
    if (status.all_replicas_ready) {
      return 'ready'
    } else if (status.ready_for_writes) {
      return 'ready-but-with-caveats'
    } else {
      return 'not-ready'
    }
  }

  render () {
    // We don't re-render the template, just update the class of the svg
    const stat_class = this.status_to_class(this.model.get('status'))
    // .addClass and .removeClass don't work on <svg> elements
    return this.$el.attr('class', `ionicon-table-status ${stat_class}`)
  }
}
TableStatusIndicator.initClass()

class SecondaryIndexesView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.set_fetch_progress = this.set_fetch_progress.bind(this)
    this.fetch_progress = this.fetch_progress.bind(this)
    this.set_indexes = this.set_indexes.bind(this)
    this.hook = this.hook.bind(this)
    this.render_error = this.render_error.bind(this)
    this.render_feedback = this.render_feedback.bind(this)
    this.render = this.render.bind(this)
    this.show_add_index = this.show_add_index.bind(this)
    this.hide_add_index = this.hide_add_index.bind(this)
    this.handle_keypress = this.handle_keypress.bind(this)
    this.on_fail_to_connect = this.on_fail_to_connect.bind(this)
    this.create_index = this.create_index.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = require('../../handlebars/table-secondary_indexes.hbs')
    this.prototype.alert_message_template = require(
      '../../handlebars/secondary_indexes-alert_msg.hbs'
    )
    this.prototype.error_template = require('../../handlebars/secondary_indexes-error.hbs')
    this.prototype.events = {
      'click .create_link'     : 'show_add_index',
      'click .create_btn'      : 'create_index',
      'keydown .new_index_name': 'handle_keypress',
      'click .cancel_btn'      : 'hide_add_index',
      'click .reconnect_link'  : 'init_connection',
      'click .close_hide'      : 'hide_alert',
    }
    this.prototype.error_interval = 5 * 1000 // In case of an error, we try to retrieve the secondary index in 5 seconds
    this.prototype.normal_interval = 10 * 1000 // Retrieve secondary indexes every 10 seconds
    this.prototype.short_interval = 1000
    // Interval when an index is being created
  }

  initialize ({ collection, model }) {
    this.indexes_view = []
    this.interval_progress = null
    this.collection = collection
    this.model = model

    this.adding_index = false

    this.listenTo(this.model, 'change:index_unavailable', this.set_warnings)

    this.render()
    if (this.collection != null) {
      return this.hook()
    }
  }

  set_warnings () {
    if (this.model.get('index_unavailable')) {
      return this.$('.unavailable-error').show()
    } else {
      return this.$('.unavailable-error').hide()
    }
  }

  set_fetch_progress (index) {
    if (this.interval_progress == null) {
      this.fetch_progress()
      return this.interval_progress = setInterval(this.fetch_progress, 1000)
    }
  }

  fetch_progress () {
    let build_in_progress = false
    for (var index of Array.from(this.collection.models)) {
      if (index.get('ready') !== true) {
        build_in_progress = true
        break
      }
    }

    if (build_in_progress && this.model.get('db') != null) {
      const query = r
        .db(this.model.get('db'))
        .table(this.model.get('name'))
        .indexStatus()
        .pluck('index', 'ready', 'progress')
        .merge(index => ({
          id   : index('index'),
          db   : this.model.get('db'),
          table: this.model.get('name'),
        }))

      return driver.run_once(query, (error, result) => {
        if (error != null) {
          // This can happen if the table is temporarily
          // unavailable. We log the error, and ignore it
          console.log('Nothing bad - Could not fetch secondary indexes statuses')
          return console.log(error)
        } else {
          let all_ready = true
          for (index of Array.from(result)) {
            if (index.ready !== true) {
              all_ready = false
            }
            this.collection.add(new models.Index(index, { merge: true }))
          }
          if (all_ready === true) {
            clearInterval(this.interval_progress)
            return this.interval_progress = null
          }
        }
      })
    } else {
      clearInterval(this.interval_progress)
      return this.interval_progress = null
    }
  }

  set_indexes (indexes) {
    this.collection = indexes
    return this.hook()
  }

  hook () {
    let view
    this.collection.each(index => {
      view = new SecondaryIndexView({
        model    : index,
        container: this,
      })
      // The first time, the collection is sorted
      this.indexes_view.push(view)
      return this.$('.list_secondary_indexes').append(view.render().$el)
    })
    if (this.collection.length === 0) {
      this.$('.no_index').show()
    } else {
      this.$('.no_index').hide()
    }

    this.listenTo(this.collection, 'add', index => {
      const new_view = new SecondaryIndexView({
        model    : index,
        container: this,
      })

      if (this.indexes_view.length === 0) {
        this.indexes_view.push(new_view)
        this.$('.list_secondary_indexes').html(new_view.render().$el)
      } else {
        let added = false
        for (let position = 0; position < this.indexes_view.length; position++) {
          view = this.indexes_view[position]
          if (view.model.get('index') > index.get('index')) {
            added = true
            this.indexes_view.splice(position, 0, new_view)
            if (position === 0) {
              this.$('.list_secondary_indexes').prepend(new_view.render().$el)
            } else {
              this.$('.index_container').eq(position - 1).after(new_view.render().$el)
            }
            break
          }
        }
        if (added === false) {
          this.indexes_view.push(new_view)
          this.$('.list_secondary_indexes').append(new_view.render().$el)
        }
      }

      if (this.indexes_view.length === 1) {
        return this.$('.no_index').hide()
      }
    })

    return this.listenTo(this.collection, 'remove', index => {
      for (view of Array.from(this.indexes_view)) {
        if (view.model === index) {
          index.destroy();
          (view => {
            view.remove()
            return this.indexes_view.splice(this.indexes_view.indexOf(view), 1)
          })(view)
          break
        }
      }
      if (this.collection.length === 0) {
        return this.$('.no_index').show()
      }
    })
  }

  render_error (args) {
    this.$('.alert_error_content').html(this.error_template(args))
    this.$('.main_alert_error').show()
    return this.$('.main_alert').hide()
  }

  render_feedback (args) {
    if (this.$('.main_alert').css('display') === 'none') {
      this.$('.alert_content').html(this.alert_message_template(args))
      this.$('.main_alert').show()
    } else {
      this.$('.main_alert').fadeOut('fast', () => {
        this.$('.alert_content').html(this.alert_message_template(args))
        return this.$('.main_alert').fadeIn('fast')
      })
    }
    return this.$('.main_alert_error').hide()
  }

  render () {
    this.$el.html(
      this.template({
        adding_index: this.adding_index,
      })
    )
    this.set_warnings()
    return this
  }

  // Show the form to add a secondary index
  show_add_index (event) {
    event.preventDefault()
    this.$('.add_index_li').show()
    this.$('.create_container').hide()
    return this.$('.new_index_name').focus()
  }

  // Hide the form to add a secondary index
  hide_add_index () {
    this.$('.add_index_li').hide()
    this.$('.create_container').show()
    return this.$('.new_index_name').val('')
  }

  // We catch enter and esc when the user is writing a secondary index name
  handle_keypress (event) {
    if (event.which === 13) {
      // Enter
      event.preventDefault()
      return this.create_index()
    } else if (event.which === 27) {
      // ESC
      event.preventDefault()
      return this.hide_add_index()
    }
  }

  on_fail_to_connect () {
    this.render_error({
      connect_fail: true,
    })
    return this
  }

  create_index () {
    this.$('.create_btn').prop('disabled', 'disabled')
    this.$('.cancel_btn').prop('disabled', 'disabled')

    const index_name = this.$('.new_index_name').val()
    return (index_name => {
      const query = r
        .db(this.model.get('db'))
        .table(this.model.get('name'))
        .indexCreate(index_name)
      return driver.run_once(query, (error, result) => {
        this.$('.create_btn').prop('disabled', false)
        this.$('.cancel_btn').prop('disabled', false)
        const that = this
        if (error != null) {
          return this.render_error({
            create_fail: true,
            message    : error.msg.replace('\n', '<br/>'),
          })
        } else {
          this.collection.add(new models.Index({
            id   : index_name,
            index: index_name,
            db   : this.model.get('db'),
            table: this.model.get('name'),
          }))
          this.render_feedback({
            create_ok: true,
            name     : index_name,
          })

          return this.hide_add_index()
        }
      })
    })(index_name)
  }

  // Hide alert BUT do not remove it
  hide_alert (event) {
    if (event != null && __guard__(this.$(event.target), x => x.data('name')) != null) {
      this.deleting_secondary_index = null
    }
    event.preventDefault()
    return $(event.target).parent().hide()
  }

  remove () {
    this.stopListening()
    for (let view of Array.from(this.indexes_view)) {
      view.remove()
    }
    return super.remove()
  }
}
SecondaryIndexesView.initClass()

class SecondaryIndexView extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.update = this.update.bind(this)
    this.render = this.render.bind(this)
    this.render_progress_bar = this.render_progress_bar.bind(this)
    this.confirm_delete = this.confirm_delete.bind(this)
    this.delete_index = this.delete_index.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = require('../../handlebars/table-secondary_index.hbs')
    this.prototype.progress_template = require('../../handlebars/simple_progressbar.hbs')
    this.prototype.events = {
      'click .delete_link'      : 'confirm_delete',
      'click .delete_index_btn' : 'delete_index',
      'click .cancel_delete_btn': 'cancel_delete',
    }

    this.prototype.tagName = 'li'
    this.prototype.className = 'index_container'
  }

  initialize ({ container }) {
    this.container = container

    this.model.on('change:progress', this.update)
    return this.model.on('change:ready', this.update)
  }

  update (args) {
    if (this.model.get('ready') === false) {
      return this.render_progress_bar()
    } else {
      if (this.progress_bar != null) {
        return this.render_progress_bar()
      } else {
        return this.render()
      }
    }
  }

  render () {
    this.$el.html(
      this.template({
        is_empty: this.model.get('name') === '',
        name    : this.model.get('index'),
        ready   : this.model.get('ready'),
      })
    )

    if (this.model.get('ready') === false) {
      this.render_progress_bar()
      this.container.set_fetch_progress()
    }
    return this
  }

  render_progress_bar () {
    const progress = this.model.get('progress')

    if (this.progress_bar != null) {
      if (this.model.get('ready') === true) {
        return this.progress_bar.render(
          100,
          100,
          {
            got_response: true,
            check       : true,
          },
          () => this.render()
        )
      } else {
        return this.progress_bar.render(
          progress,
          1,
          {
            got_response: true,
            check       : true,
          },
          () => this.render()
        )
      }
    } else {
      this.progress_bar = new ui_progress.OperationProgressBar(this.progress_template)
      return this
        .$('.progress_li')
        .html(this.progress_bar.render(0, Infinity, { new_value: true, check: true }).$el)
    }
  }

  // Show a confirmation before deleting a secondary index
  confirm_delete (event) {
    event.preventDefault()
    return this.$('.alert_confirm_delete').show()
  }

  delete_index () {
    this.$('.btn').prop('disabled', 'disabled')
    const query = r
      .db(this.model.get('db'))
      .table(this.model.get('table'))
      .indexDrop(this.model.get('index'))
    return driver.run_once(query, (error, { dropped }) => {
      this.$('.btn').prop('disabled', false)
      if (error != null) {
        return this.container.render_error({
          delete_fail: true,
          message    : error.msg.replace('\n', '<br/>'),
        })
      } else if (dropped === 1) {
        this.container.render_feedback({
          delete_ok: true,
          name     : this.model.get('index'),
        })
        return this.model.destroy()
      } else {
        return this.container.render_error({
          delete_fail: true,
          message    : 'Result was not {dropped: 1}',
        })
      }
    })
  }

  // Close to hide_alert, but the way to reach the alert is slightly different than with the x link
  cancel_delete () {
    return this.$('.alert_confirm_delete').hide()
  }

  remove () {
    __guard__(this.progress_bar, x => x.remove())
    return super.remove()
  }
}
SecondaryIndexView.initClass()

exports.TableContainer = TableContainer
exports.TableMainView = TableMainView
exports.ReconfigurePanel = ReconfigurePanel
exports.ReconfigureDiffView = ReconfigureDiffView
exports.Title = Title
exports.Profile = Profile
exports.TableStatusIndicator = TableStatusIndicator
exports.SecondaryIndexesView = SecondaryIndexesView
exports.SecondaryIndexView = SecondaryIndexView

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

// Copyright 2010-2012 RethinkDB, all rights reserved.

const system_db = 'rethinkdb'

const r = require('rethinkdb')

let is_disconnected = null

class Driver {
  static initClass () {
    // helper methods
    let server_conf
    this.prototype.helpers = {
      // Macro to create a match/switch construct in reql by
      // nesting branches
      // Use like: match(doc('field'),
      //                 ['foo', some_reql],
      //                 [r.expr('bar'), other_reql],
      //                 [some_other_query, contingent_3_reql],
      //                 default_reql)
      // Throws an error if a match isn't found. The error can be absorbed
      // by tacking on a .default() if you want
      match (variable, ...specs) {
        let previous = r.error(`nothing matched ${variable}`)
        for (let [val, action] of Array.from(specs.reverse())) {
          previous = r.branch(r.expr(variable).eq(val), action, previous)
        }
        return previous
      },
      identity (x) {
        return x
      },
    }

    // common queries used in multiple places in the ui
    this.prototype.queries = {
      all_logs: limit => {
        server_conf = driver.admin().server_config
        return r
          .db(system_db)
          .table('logs', { identifierFormat: 'uuid' })
          .orderBy({ index: r.desc('id') })
          .limit(limit)
          .map(log => log.merge({
            server   : server_conf.get(log('server'))('name'),
            server_id: log('server'),
          }))
      },
      server_logs: (limit, server_id) => {
        server_conf = driver.admin().server_config
        return r
          .db(system_db)
          .table('logs', { identifierFormat: 'uuid' })
          .orderBy({ index: r.desc('id') })
          .filter({ server: server_id })
          .limit(limit)
          .map(log => log.merge({
            server   : server_conf.get(log('server'))('name'),
            server_id: log('server'),
          }))
      },

      issues_with_ids (current_issues = driver.admin().current_issues) {
        // we use .get on issues_id, so it must be the real table
        const { current_issues_id } = driver.admin()
        return current_issues
          .merge(issue => {
            const issue_id = current_issues_id.get(issue('id'))
            const log_write_error = {
              servers: issue('info')('servers').map(issue_id('info')('servers'), (
                server,
                server_id
              ) => ({
                server,
                server_id,
              })),
            }
            const memory_error = {
              servers: issue('info')('servers').map(issue_id('info')('servers'), (
                server,
                server_id
              ) => ({
                server,
                server_id,
              })),
            }
            const non_transitive_error = {
              servers: issue('info')('servers').map(issue_id('info')('servers'), (
                server,
                server_id
              ) => ({
                server,
                server_id,
              })),
            }
            const outdated_index = {
              tables: issue('info')('tables').map(issue_id('info')('tables'), (
                table,
                table_id
              ) => ({
                db      : table('db'),
                db_id   : table_id('db'),
                table_id: table_id('table'),
                table   : table('table'),
                indexes : table('indexes'),
              })),
            }
            const table_avail = issue('info').merge({
              table_id       : issue_id('info')('table'),
              shards         : issue('info')('shards').default([]),
              missing_servers: issue('info')('shards')
                .default([])('replicas')
                .concatMap(x => x)
                .filter({ state: 'disconnected' })('server')
                .distinct(),
            })
            return {
              info: driver.helpers.match(
                issue('type'),
                ['log_write_error', log_write_error],
                ['memory_error', memory_error],
                ['non_transitive_error', non_transitive_error],
                ['outdated_index', outdated_index],
                ['table_availability', table_avail],
                [issue('type'), issue('info')] // default
              ),
            }
          })
          .coerceTo('array')
      },

      tables_with_primaries_not_ready (
        table_config_id = driver.admin().table_config_id,
        table_status = driver.admin().table_status
      ) {
        return r.do(
          driver
            .admin()
            .server_config.map(x => [x('id'), x('name')])
            .coerceTo('ARRAY')
            .coerceTo('OBJECT'),
          server_names => table_status
            .map(table_config_id, (status, config) => ({
              id    : status('id'),
              name  : status('name'),
              db    : status('db'),
              shards: status('shards')
                .default([])
                .map(r.range(), config('shards').default([]), (shard, pos, conf_shard) => {
                  const primary_id = conf_shard('primary_replica')
                  const primary_name = server_names(primary_id)
                  return {
                    position     : pos.add(1),
                    num_shards   : status('shards').count().default(0),
                    primary_id,
                    primary_name : primary_name.default('-'),
                    primary_state: shard('replicas')
                      .filter({ server: primary_name }, { default: r.error() })('state')(0)
                      .default('disconnected'),
                  }
                })
                .filter(shard => shard('primary_state').ne('ready'))
                .coerceTo('array'),
            }))
            .filter(table => table('shards').isEmpty().not())
            .coerceTo('array')
        )
      },

      tables_with_replicas_not_ready (
        table_config_id = driver.admin().table_config_id,
        table_status = driver.admin().table_status
      ) {
        return table_status
          .map(table_config_id, (status, config) => ({
            id    : status('id'),
            name  : status('name'),
            db    : status('db'),
            shards: status('shards')
              .default([])
              .map(r.range(), config('shards').default([]), (shard, pos, conf_shard) => ({
                position  : pos.add(1),
                num_shards: status('shards').count().default(0),
                replicas  : shard('replicas')
                  .filter(replica =>
                    r
                      .expr(['ready', 'looking_for_primary_replica', 'offloading_data'])
                      .contains(replica('state'))
                      .not())
                  .map(conf_shard('replicas'), (replica, replica_id) => ({
                    replica_id,
                    replica_name: replica('server'),
                  }))
                  .coerceTo('array'),
              }))
              .coerceTo('array'),
          }))
          .filter(table => table('shards')(0)('replicas').isEmpty().not())
          .coerceTo('array')
      },
      num_primaries (table_config_id = driver.admin().table_config_id) {
        return table_config_id('shards').default([]).map(x => x.count()).sum()
      },

      num_connected_primaries (table_status = driver.admin().table_status) {
        return table_status
          .map(table =>
            table('shards').default([])('primary_replicas').count(arr => arr.isEmpty().not()))
          .sum()
      },

      num_replicas (table_config_id = driver.admin().table_config_id) {
        return table_config_id('shards')
          .default([])
          .map(shards => shards.map(shard => shard('replicas').count()).sum())
          .sum()
      },

      num_connected_replicas (table_status = driver.admin().table_status) {
        return table_status('shards')
          .default([])
          .map(shards =>
            shards('replicas')
              .map(replica =>
                replica('state').count(replica =>
                  r.expr(['ready', 'looking_for_primary_replica']).contains(replica)))
              .sum())
          .sum()
      },

      disconnected_servers (server_status = driver.admin().server_status) {
        return server_status
          .filter(server => server('status').ne('connected'))
          .map(server => ({
            time_disconnected: server('connection')('time_disconnected'),
            name             : server('name'),
          }))
          .coerceTo('array')
      },

      num_disconnected_tables (table_status = driver.admin().table_status) {
        return table_status.count(table => {
          const shard_is_down = shard => shard('primary_replicas').isEmpty().not()
          return table('shards').default([]).map(shard_is_down).contains(true)
        })
      },

      num_tables_w_missing_replicas (table_status = driver.admin().table_status) {
        return table_status.count(table => table('status')('all_replicas_ready').not())
      },

      num_connected_servers (server_status = driver.admin().server_status) {
        return server_status.count(server => server('status').eq('connected'))
      },

      num_sindex_issues (current_issues = driver.admin().current_issues) {
        return current_issues.count(issue => issue('type').eq('outdated_index'))
      },

      num_sindexes_constructing (jobs = driver.admin().jobs) {
        return jobs.count(job => job('type').eq('index_construction'))
      },
    }
  }
  constructor (args) {
    let port
    this.hack_driver = this.hack_driver.bind(this)
    this.run_once = this.run_once.bind(this)
    this.run = this.run.bind(this)
    this.stop_timer = this.stop_timer.bind(this)
    if (window.location.port === '') {
      if (window.location.protocol === 'https:') {
        port = 443
      } else {
        port = 80
      }
    } else {
      port = parseInt(window.location.port)
    }
    this.server = {
      host    : window.location.hostname,
      port,
      protocol: window.location.protocol === 'https:' ? 'https' : 'http',
      pathname: window.location.pathname,
    }

    this.hack_driver()

    this.state = 'ok'
    this.timers = {}
    this.index = 0
  }

  // Hack the driver: remove .run() and add private_run()
  // We want run() to throw an error, in case a user write .run() in a query.
  // We'll internally run a query with the method `private_run`
  hack_driver () {
    const TermBase = r.expr(1).constructor.__super__.constructor.__super__
    if (TermBase.private_run == null) {
      const that = this
      TermBase.private_run = TermBase.run
      return TermBase.run = () => {
        throw new Error(
          'You should remove .run() from your queries when using the Data Explorer.\nThe query will be built and sent by the Data Explorer itself.'
        )
      }
    }
  }

  // Open a connection to the server
  connect (callback) {
    return r.connect(this.server, callback)
  }

  // Close a connection
  close (conn) {
    return conn.close({ noreplyWait: false })
  }

  // Run a query once
  run_once (query, callback) {
    return this.connect((error, connection) => {
      if (error != null) {
        // If we cannot open a connection, we blackout the whole interface
        // And do not call the callback
        if (is_disconnected != null) {
          if (this.state === 'ok') {
            is_disconnected.display_fail()
          }
        } else {
          is_disconnected = new require('body').IsDisconnected
        }
        return this.state = 'fail'
      } else {
        if (this.state === 'fail') {
          // Force refresh
          return window.location.reload(true)
        } else {
          this.state = 'ok'
          return query.private_run(connection, (err, result) => {
            if (
              __guard__(result, ({ value }) => value) != null &&
              __guard__(result, ({ profile }) => profile) != null
            ) {
              result = result.value
            }
            if (typeof __guard__(result, ({ toArray }) => toArray) === 'function') {
              return result.toArray((err, result) => callback(err, result))
            } else {
              return callback(err, result)
            }
          })
        }
      }
    })
  }

  // Run the query every `delay` ms - using setTimeout
  // Returns a timeout number (to use with @stop_timer).
  // If `index` is provided, `run` will its value as a timer
  run (query, delay, callback, index) {
    if (index == null) {
      this.index++
    }
    ({ index } = this)
    this.timers[index] = {};
    (index => {
      return this.connect((error, connection) => {
        if (error != null) {
          // If we cannot open a connection, we blackout the whole interface
          // And do not call the callback
          if (is_disconnected != null) {
            if (this.state === 'ok') {
              is_disconnected.display_fail()
            }
          } else {
            const body = require('./body')
            is_disconnected = new body.IsDisconnected()
          }
          return this.state = 'fail'
        } else {
          if (this.state === 'fail') {
            // Force refresh
            return window.location.reload(true)
          } else {
            this.state = 'ok'
            if (this.timers[index] != null) {
              let fn
              this.timers[index].connection = connection
              return (fn = () => {
                try {
                  return query.private_run(connection, (err, result) => {
                    // All http connections have an
                    // implicit profile now, so we
                    // have to pull it out
                    if (
                      __guard__(result, ({ value }) => value) != null &&
                      __guard__(result, ({ profile }) => profile) != null
                    ) {
                      result = result.value
                    }
                    if (typeof __guard__(result, ({ toArray }) => toArray) === 'function') {
                      return result.toArray((err, result) => {
                        // This happens if people load the page with the back button
                        // In which case, we just restart the query
                        // TODO: Why do we sometimes get an Error object
                        //  with message == "[...]", and other times a
                        //  RqlClientError with msg == "[...]."?
                        if (
                          __guard__(err, ({ msg }) => msg) ===
                            'This HTTP connection is not open.' ||
                          __guard__(err, ({ message }) => message) ===
                            'This HTTP connection is not open'
                        ) {
                          console.log('Connection lost. Retrying.')
                          return this.run(query, delay, callback, index)
                        }
                        callback(err, result)
                        if (this.timers[index] != null) {
                          return this.timers[index].timeout = setTimeout(fn, delay)
                        }
                      })
                    } else {
                      if (
                        __guard__(err, ({ msg }) => msg) === 'This HTTP connection is not open.' ||
                        __guard__(err, ({ message }) => message) ===
                          'This HTTP connection is not open'
                      ) {
                        console.log('Connection lost. Retrying.')
                        return this.run(query, delay, callback, index)
                      }
                      callback(err, result)
                      if (this.timers[index] != null) {
                        return this.timers[index].timeout = setTimeout(fn, delay)
                      }
                    }
                  })
                } catch (err) {
                  console.log(err)
                  return this.run(query, delay, callback, index)
                }
              })()
            }
          }
        }
      })
    })(index)
    return index
  }

  // Stop the timer and close the connection
  stop_timer (timer) {
    clearTimeout(__guard__(this.timers[timer], ({ timeout }) => timeout))
    __guard__(__guard__(this.timers[timer], ({ connection }) => connection), x1 =>
      x1.close({ noreplyWait: false }))
    return delete this.timers[timer]
  }
  admin () {
    return {
      cluster_config   : r.db(system_db).table('cluster_config'),
      cluster_config_id: r.db(system_db).table('cluster_config', { identifierFormat: 'uuid' }),
      current_issues   : r.db(system_db).table('current_issues'),
      current_issues_id: r.db(system_db).table('current_issues', { identifierFormat: 'uuid' }),
      db_config        : r.db(system_db).table('db_config'),
      db_config_id     : r.db(system_db).table('db_config', { identifierFormat: 'uuid' }),
      jobs             : r.db(system_db).table('jobs'),
      jobs_id          : r.db(system_db).table('jobs', { identifierFormat: 'uuid' }),
      logs             : r.db(system_db).table('logs'),
      logs_id          : r.db(system_db).table('logs', { identifierFormat: 'uuid' }),
      server_config    : r.db(system_db).table('server_config'),
      server_config_id : r.db(system_db).table('server_config', { identifierFormat: 'uuid' }),
      server_status    : r.db(system_db).table('server_status'),
      server_status_id : r.db(system_db).table('server_status', { identifierFormat: 'uuid' }),
      stats            : r.db(system_db).table('stats'),
      stats_id         : r.db(system_db).table('stats', { identifierFormat: 'uuid' }),
      table_config     : r.db(system_db).table('table_config'),
      table_config_id  : r.db(system_db).table('table_config', { identifierFormat: 'uuid' }),
      table_status     : r.db(system_db).table('table_status'),
      table_status_id  : r.db(system_db).table('table_status', { identifierFormat: 'uuid' }),
    }
  }
}
Driver.initClass()

$(() => {
  const body = require('./body')
  const data_explorer_view = require('./dataexplorer')
  const main_container = new body.MainContainer()
  const app = require('./app').main = main_container
  $('body').html(main_container.render().$el)
  // We need to start the router after the main view is bound to the DOM
  main_container.start_router()

  Backbone.sync = (method, model, success, error) => 0

  return data_explorer_view.Container.prototype.set_docs(reql_docs)
})

// Create a driver - providing sugar on top of the raw driver
var driver = new Driver()

exports.driver = driver
// Some views backup their data here so that when you return to them
// the latest data can be retrieved quickly.
exports.view_data_backup = {}
exports.main = null
// The system database
exports.system_db = system_db

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

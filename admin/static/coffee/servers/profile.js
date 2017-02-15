// Server profile

const h = require('virtual-dom/h')
const r = require('rethinkdb')
const diff = require('virtual-dom/diff')
const patch = require('virtual-dom/patch')
const createElement = require('virtual-dom/create-element')
const moment = require('moment')

const util = require('../util.coffee')

class Model extends Backbone.Model {
  static initClass () {
    this.prototype.defaults = {
      version      : null,
      time_started : null,
      hostname     : 'Unknown',
      tags         : [],
      cache_size_mb: null,
    }
  }
  static query (server_config, server_status) {
    return {
      version     : server_status('process')('version'),
      time_started: server_status('process')('time_started'),
      hostname    : server_status('network')('hostname'),
      tags        : server_config('tags'),
      cache_size  : server_status('process')('cache_size_mb').mul(1024 * 1024),
    }
  }
}
Model.initClass()

class View extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    super(...args)
  }

  initialize () {
    this.listenTo(this.model, 'change', this.render)
    this.current_vdom_tree = this.render_vdom(this.model.toJSON())

    return this.setElement(createElement(this.current_vdom_tree))
  }

  render () {
    const new_tree = this.render_vdom(this.model.toJSON())
    const patches = diff(this.current_vdom_tree, new_tree)
    patch(this.el, patches)
    return this
  }

  render_vdom (model) {
    const version = __guard__(model.version, x => x.split(' ')[1].split('-')[0])
    const approx_cache_size = util.format_bytes(model.cache_size)
    return h(
      'div.server-info-view',
      h('div.summary', [
        render_profile_row('hostname', 'hostname', model.hostname || 'N/A'),
        version != null ? render_profile_row('version', 'version', version) : undefined,
        model.time_started != null
          ? render_profile_row(
              'uptime',
              'uptime',
              moment(model.time_started).fromNow(true),
              model.time_started.toString()
            )
          : undefined,
        model.cache_size != null
          ? render_profile_row('cache-size', 'cache size', `${approx_cache_size}`)
          : undefined,
        model.tags != null ? render_tag_row(model.tags) : undefined,
      ])
    )
  }
}

var render_profile_row = (className, name, value, title = null) =>
  h(`div.profile-row.${className}`, [
    h(`p.${className}`, [h('span.big', { title }, value), ` ${name}`]),
  ])

var render_tag_row = tags =>
  h('div.profile-row.tags', [
    tags.length > 0 ? h('p', [h('span.tags', tags.join(', ')), ' tags']) : undefined,
    tags.length === 0 ? h('p.admonition', 'This server has no tags.') : undefined,
  ])

exports.View = View
exports.Model = Model

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

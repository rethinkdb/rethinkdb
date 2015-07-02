# Server profile

h = require('virtual-dom/h')
r = require('rethinkdb')
diff = require('virtual-dom/diff')
patch = require('virtual-dom/patch')
createElement = require('virtual-dom/create-element')
moment = require('moment')

util = require('../util.coffee')

class Model extends Backbone.Model
    @query: (server_config, server_status) ->
        version: server_status('process')('version')
        time_started: server_status('process')('time_started')
        hostname: server_status('network')('hostname')
        tags: server_config('tags')
        cache_size_mb: server_status('process')('cache_size_mb')

    defaults:
        version: null
        time_started: null
        hostname: "Unknown"
        tags: []
        cache_size_mb: null

class View extends Backbone.View

    initialize: =>
        @listenTo @model, 'change', @render
        @current_vdom_tree = @render_vdom(@model.toJSON())

        @setElement(createElement(@current_vdom_tree))

    render: =>
        new_tree = @render_vdom(@model.toJSON())
        patches = diff(@current_vdom_tree, new_tree)
        patch(@el, patches)
        @

    render_vdom: (model) ->
        version = model.version?.split(' ')[1].split('-')[0]
        approx_cache_size = util.format_bytes(model.cache_size_mb * 1024, 1)
        h "div.server-info-view",
            h "div.summary", [
                render_profile_row("hostname", "hostname", model.hostname or "N/A")
                render_profile_row("version", "version", version) if version?
                render_profile_row(
                    "uptime",
                    "uptime",
                    moment(model.time_started).fromNow(true),
                    model.time_started.toString()) if model.time_started?
                render_profile_row(
                    "cache-size",
                    "cache size",
                    "#{approx_cache_size}") if model.cache_size_mb?
                render_tag_row(model.tags) if model.tags?
            ]

render_profile_row = (className, name, value, title=null) ->
    h "div.profile-row.#{className}", [
        h "p.#{className}", [
            h "span.big", title: title, value
            " #{name}"
        ]
    ]

render_tag_row = (tags) ->
    h "div.profile-row.tags", [
        h "p", [
            h "span.tags", tags.join(', ')
            " tags"
        ] if tags.length > 0
        h("p.admonition", "This server has no tags.") if tags.length == 0
    ]

exports.View = View
exports.Model = Model

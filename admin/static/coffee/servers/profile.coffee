# Server profile

h = require('virtual-dom/h')
r = require('rethinkdb')
diff = require('virtual-dom/diff')
patch = require('virtual-dom/patch')
createElement = require('virtual-dom/create-element')
moment = require('moment')

class Model extends Backbone.Model
    @query: (server_config, server_status) ->
        version: server_status('process')('version')
        time_started: server_status('process')('time_started')
        hostname: server_status('network')('hostname')
        tags: server_config('tags')
        cache_size: server_status('process')('cache_size_mb')

    defaults:
        version: null
        time_started: null
        hostname: "Unknown"
        tags: []
        cache_size: null

class View extends Backbone.View

    initialize: =>
        @listenTo @model, 'change', @render
        @current_vdom_tree = @render_vdom(@model.toJSON())

        @setElement(createElement(@current_vdom_tree))

    render: =>
        new_tree = @render_vdom(@model.toJSON())
        patches = diff(@current_vdom_tree, new_tree)
        patch(@el, patches)
        # tooltip still in jquery :(
        @$('.tags, .admonition').tooltip
            for_dataexplorer: false
            trigger: 'hover'
            placement: 'bottom'
        @

    render_vdom: (model) ->
        version = model.version?.split(' ')[1].split('-')[0]
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
                    "#{model.cache_size} MB") if model.cache_size?
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
            h "span.tags",
                dataset:
                    originalTitle:
                        "To change server tags, see the \
                        rethinkdb.server_config table."
                tags.join(", ")
            " tags"
        ] if tags.length > 0
        h("p.admonition",
            dataset:
                originalTitle:
                    "It will not be used by new tables, and the reconfigure \
                    command will ignore it"
            "This server has no tags."
        ) if tags.length == 0
    ]

exports.View = View
exports.Model = Model

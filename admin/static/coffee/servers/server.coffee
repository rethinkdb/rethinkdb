# Copyright 2010-2015 RethinkDB
# Main server view for the server page

r = require('rethinkdb')
h = require("virtual-dom/h")
patch = require("virtual-dom/patch")
diff = require("virtual-dom/diff")
createElement = require("virtual-dom/create-element")

util = require('../util.coffee')
models = require('../models.coffee')
app = require('../app.coffee')
driver = app.driver
system_db = app.system_db
vis = require('../vis.coffee')
log_view = require('../log_view.coffee')
ui_modals = require('../ui_components/modals.coffee')

server_profile = require('./profile.coffee')
server_resp = require('./responsibilities.coffee')


class Model extends Backbone.Model
    @query: (server_status) ->
        name: server_status('name')
        id: server_status('id')

    defaults:
        name: null
        id: null

class View extends Backbone.View
    rename_server: (event) =>
        event.preventDefault()
        if @rename_modal?
            @rename_modal.remove()
        @rename_modal = new ui_modals.RenameItemModal
            model: @model
            item_type: "server"
        @rename_modal.render()

    fetch_server: =>
        query = r.do(
            r.db(system_db).table('server_config').get(@model.get('id')),
            r.db(system_db).table('server_status').get(@model.get('id')),
            (server_config, server_status) ->
                table_config = r.db(system_db).table('table_config')
                table_status = r.db(system_db).table('table_status')
                profile: server_profile.Model.query(server_config, server_status)
                main: Model.query(server_status)
                resp: server_resp.Model.query(
                    table_config, table_status, server_config('name'))
        )

        @timer = driver.run query, 5000, (error, result) =>
            if error?
                @error = error
                console.log("Error:", error.message)
            else
                @error = null
                @model.set result.main
                @profile_model.set result.profile
                @resp_model.set result.resp
            @render()

    fetch_stats: =>
        @stats_timer = driver.run(
            r.db(system_db).table('stats')
                .get(['server', @model.get('id')])
                .do((stat) ->
                    keys_read: stat('query_engine')('read_docs_per_sec'),
                    keys_set: stat('query_engine')('written_docs_per_sec'),
                ),
            1000, # run every second
            @stats.on_result, # callback
        )

    initialize: (id) =>
        @model = new Model(id: id)
        @profile_model = new server_profile.Model()
        @resp_model = new server_resp.Model()
        @stats = new models.Stats()

        @timer = null # set by fetch_server()
        @error = null # set by fetch_server (if there's an error)
        @stats_timer = null # set by fetch_stats

        @current_vdom_tree = @render_vdom()
        @setElement(createElement(@current_vdom_tree))
        @fetch_server()
        @fetch_stats()

    render: =>
        new_tree = @render_vdom()
        patches = patch(@el, diff(@current_vdom_tree, new_tree))
        @

    remove: =>
        driver.stop_timer @timer
        driver.stop_timer @stats_timer
        if @rename_modal?
            @rename_modal.remove()
        @cleanup_widgets()

    cleanup_widgets: () ->
        # This is a bit of a hack. basically the widgets containing
        # other Backbone views won't get their .remove methods called
        # unless patch notices they need to be removed from the
        # DOM. So we patch to an empty DOM before navigating somewhere
        # else. The less stateful views we have, the less this hack
        # will be needed.
        patch(@el, diff(@current_vdom_tree, h("div")))

    render_vdom: ->
        if @error?
            return render_not_found(@model.get('id'))
        h "div#server-view", [
            h "div.operations",
                h "div.dropdown", [
                    h "button.btn.dropdown-toggle",
                        dataset: toggle: "dropdown",
                        "Operations"
                    h "ul.dropdown-menu",
                        h "li",
                            h "a.rename",
                                href: '#'
                                onclick: @rename_server
                                "Rename server"
                ]
            h "div.main_title",
                h "h1.title",
                    "Server overview for #{@model.get('name')}"
            h "div.user-alert-space"
            h "div.section.statistics", [
                h "h2.title", "Statistics"
                h "div.content.row-fluid", [
                    h "div.span4.profile",
                        new util.BackboneViewWidget(=>
                            new server_profile.View(model: @profile_model)
                        )
                    h "div.span8.performance-graph",
                        new util.BackboneViewWidget(=>
                            new vis.OpsPlot(
                                @stats.get_stats,
                                width:  564             # width in pixels
                                height: 210             # height in pixels
                                seconds: 73             # num seconds to track
                                type: 'server'
                            )
                        )
                ]
            ]
            h "div.section.responsibilities.tree-view2",
                new util.BackboneViewWidget(=>
                    new server_resp.View(model: @resp_model)
                )
            h "div.section.recent-logs", [
                h "h2.title", "Recent log entries"
                h "div.recent-log-entries",
                    new util.BackboneViewWidget(=>
                        new log_view.LogsContainer(
                            server_id: @model.get('id'),
                            limit: 5
                            query: driver.queries.server_logs
                        )
                    )
            ]
        ]

render_not_found = (server_id) ->
    h "div.section", [
        h "h2.title", "Error"
        h "p", "Server #{server_id} could not be found or is unavailable"
        h "p", [
            "You can try to "
            h("a", href: "#/servers/#{server_id}", "refresh")
            " or see a list of all "
            h("a", href: "#/servers", "servers")
            "."
        ]
    ]

exports.Model = Model
exports.View = View

# Copyright 2010-2015 RethinkDB, all rights reserved.

vdom = require('../vdom_util.coffee')
h = require('virtual-dom/h')
r = require('rethinkdb')

panels = require('./panels.coffee')
util = require('../util.coffee')
app = require('../app.coffee')
driver = app.driver
system_db = app.system_db
models = require('../models.coffee')
log_view = require('../log_view.coffee')
vis = require('../vis.coffee')

class View extends vdom.VirtualDomView
    events:
        'click .view-logs': 'show_all_logs'
    fetch:
        5000:
            servers: panels.ServersModel
            tables: panels.TablesModel
            indexes: panels.IndexesModel
            resources: panels.ResourcesModel
        # TODO: convert stats timer to this format
        # 1000:
        #     stats: models.Stats

    init: (options) ->
        @stats = new models.Stats()
        @stats_timer = driver.run(
            r.db(system_db)
            .table('stats').get(['cluster'])
            .do((stat) ->
                keys_read: stat('query_engine')('read_docs_per_sec')
                keys_set: stat('query_engine')('written_docs_per_sec')
            ), 1000, @stats.on_result
        )

    cleanup: =>
        driver.stop_timer(@stats_timer)

    show_all_logs: ->
        app.main.router.navigate("#logs", trigger: true)

    render_vdom: =>
        if @query_error?
            h "div.section", [
                h "h2.title", "Error"
                h "p", [
                    "The request to retrieve data failed. You can try to "
                    h "a", href: "#", "refresh"
                ]
                h "p", "Error:"
                h("pre.main_query_error", @query_error.error) if @query_error.error?
            ]
        else
            h "div.dashboard-container", [
                h "div.section.cluster-status", [
                    h "div.panel.servers",
                        new vdom.BackboneViewWidget(=>
                            new panels.ServersView(model: @submodels.servers))
                    h "div.panel.tables",
                        new vdom.BackboneViewWidget(=>
                            new panels.TablesView(model: @submodels.tables))
                    h "div.panel.indexes",
                        new vdom.BackboneViewWidget(=>
                            new panels.IndexesView(model: @submodels.indexes))
                    h "div.panel.resources",
                        new vdom.BackboneViewWidget(=>
                            new panels.ResourcesView(model: @submodels.resources))
                ]
                h "div.section#cluster_performance_container",
                    new vdom.BackboneViewWidget(=>
                        new vis.OpsPlot(@stats.get_stats,
                            width: 833
                            height: 300
                            seconds: 119
                            type: 'cluster'
                        )
                    )
                h "div.section.recent-logs", [
                    h "button.btn.btn-primary.view-logs", "View All"
                    h "h2.title", "Recent log entries"
                    h "div.recent-log-entries-container",
                        new vdom.BackboneViewWidget(=>
                            new log_view.LogsContainer
                                limit: 5
                                query: driver.queries.all_logs
                        )
                ]
            ]

exports.View = View

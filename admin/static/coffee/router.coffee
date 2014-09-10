# Copyright 2010-2012 RethinkDB, all rights reserved.
# Extend Backbone View
#TODO Remove
Backbone.View.prototype.destroy = ->
    return

# Router for Backbone.js
class BackboneCluster extends Backbone.Router
    #TODO Add human readable routes
    routes:
        '': 'dashboard'
        'databases': 'index_tables'
        'databases/': 'index_tables'
        'databases/:id': 'database'
        'databases/:id/': 'database'
        'tables': 'index_tables'
        'tables/': 'index_tables'
        'tables/:id': 'table'
        'tables/:id/': 'table'
        'servers': 'index_servers'
        'servers/': 'index_servers'
        'servers/:id': 'server'
        'servers/:id/': 'server'
        'dashboard': 'dashboard'
        'dashboard/': 'dashboard'
        'resolve_issues': 'resolve_issues'
        'resolve_issues/': 'resolve_issues'
        'logs': 'logs'
        'logs/': 'logs'
        'dataexplorer': 'dataexplorer'
        'dataexplorer/': 'dataexplorer'

    initialize: (data) ->
        super
        @navbar = data.navbar
        window.app = @

        @container = $('#cluster')
        @current_view = new Backbone.View

        @bind 'route', @update_active_tab

    update_active_tab: (route) =>
        @navbar.set_active_tab route

    set_stats_call: (url) =>
        return

    index_tables: ->
        clear_modals()
        @current_view.remove()
        @current_view = new TablesView.DatabasesContainer
        @container.html @current_view.render().el

    index_servers: (data) ->
        clear_modals()
        @current_view.remove()
        @current_view = new ServersView.ServersContainer
        @container.html @current_view.render().el

    dashboard: ->
        clear_modals()
        @current_view.remove()
        @current_view = new DashboardView.DashboardContainer
        @container.html @current_view.render().el

    resolve_issues: ->
        @set_stats_call ''
        log_router '/resolve_issues'
        clear_modals()
        @current_view.remove()
        @current_view = new ResolveIssuesView.Container
        @container.html @current_view.render().el

    logs: ->
        @set_stats_call ''
        log_router '/logs'
        clear_modals()
        @current_view.remove()
        @current_view = new LogView.Container
        @container.html @current_view.render().el

    dataexplorer: ->
        @set_stats_call ''
        log_router '/dataexplorer'
        clear_modals()
        @current_view.remove()
        @current_view = new DataExplorerView.Container
            state: DataExplorerView.state
        @container.html @current_view.render().el
        @current_view.init_after_dom_rendered() # Need to be called once the view is in the DOM tree
        @current_view.results_view.set_scrollbar() # In case we check the data explorer, leave and come back

    database: (id) ->
        clear_modals()
        @current_view.remove()
        @current_view = new DatabaseView.DatabaseContainer id

        @container.html @current_view.render().el

    table: (id) ->
        clear_modals()
        @current_view.remove()
        @current_view = new TableView.TableContainer id
        @container.html @current_view.render().el

    server: (id) ->
        log_router '/servers/' + id
        clear_modals()

        @current_view.remove()
        @current_view = new ServerView.ServerContainer id

        @container.html @current_view.render().el

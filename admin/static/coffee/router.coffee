# Copyright 2010-2012 RethinkDB, all rights reserved.

class BackboneCluster extends Backbone.Router
    # Routes can end with a slash too - This is useful is the user
    # tries to manually edit the route
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
        'logs': 'logs'
        'logs/': 'logs'
        'dataexplorer': 'dataexplorer'
        'dataexplorer/': 'dataexplorer'

    initialize: (data) ->
        super
        @navbar = data.navbar
        # We bind the router to window.app to be able to have access to the root view
        # ie. window.app.current_view
        window.app = @

        @container = $('#cluster')
        @current_view = new Backbone.View

        @bind 'route', @update_active_tab

    # Highlight the link of the current view
    update_active_tab: (route) =>
        @navbar.set_active_tab route

    index_tables: ->
        @current_view.remove()
        @current_view = new TablesView.DatabasesContainer
        @container.html @current_view.render().$el

    index_servers: (data) ->
        @current_view.remove()
        @current_view = new ServersView.ServersContainer
        @container.html @current_view.render().$el

    dashboard: ->
        @current_view.remove()
        @current_view = new DashboardView.DashboardContainer
        @container.html @current_view.render().$el

    logs: ->
        @current_view.remove()
        @current_view = new LogView.LogsContainer
        @container.html @current_view.render().$el

    dataexplorer: ->
        @current_view.remove()
        @current_view = new DataExplorerView.Container
            state: DataExplorerView.state
        @container.html @current_view.render().$el
        @current_view.init_after_dom_rendered() # Need to be called once the view is in the DOM tree
        @current_view.results_view_wrapper.set_scrollbar() # In case we check the data explorer, leave and come back

    database: (id) ->
        @current_view.remove()
        @current_view = new DatabaseView.DatabaseContainer id
        @container.html @current_view.render().$el

    table: (id) ->
        @current_view.remove()
        @current_view = new TableView.TableContainer id
        @container.html @current_view.render().$el

    server: (id) ->
        @current_view.remove()
        @current_view = new ServerView.ServerContainer id
        @container.html @current_view.render().$el

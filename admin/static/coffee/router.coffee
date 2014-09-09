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
        'databases/:id': 'database'
        'databases/:id/:tab': 'database'
        'tables': 'index_tables'
        'tables/:id': 'table'
        'servers': 'index_servers'
        'datacenters/:id': 'datacenter'
        'datacenters/:id/:tab': 'datacenter'
        'servers/:id': 'server'
        'dashboard': 'dashboard'
        'resolve_issues': 'resolve_issues'
        'logs': 'logs'
        'dataexplorer': 'dataexplorer'

    initialize: ->
        log_initial '(initializing) router'
        super
        window.app = @

        @$container = $('#cluster')
        @current_view = new Backbone.View

        # Add and render the sidebar (visible across all views)
        @$sidebar = $('#sidebar')
        @sidebar = new Sidebar.Container
        @render_sidebar()

        # Render navbar for the first time
        @navbar = new NavBarView
        @render_navbar()

        @.bind 'all', (route, router) ->
            @navbar.set_active_tab route

    render_sidebar: -> @$sidebar.html @sidebar.render().el
    render_navbar: -> $('#navbar-container').html @navbar.render().el

    set_stats_call: (url) =>
        return

    index_tables: ->
        clear_modals()
        @current_view.remove()
        @current_view = new TablesView.DatabasesContainer
        @$container.html @current_view.render().el

    index_servers: (data) ->
        clear_modals()
        @current_view.remove()
        @current_view = new ServersView.ServersContainer
        @$container.html @current_view.render().el

    dashboard: ->
        clear_modals()
        @current_view.remove()
        @current_view = new DashboardView.DashboardContainer
        @$container.html @current_view.render().el

    resolve_issues: ->
        @set_stats_call ''
        log_router '/resolve_issues'
        clear_modals()
        @current_view.remove()
        @current_view = new ResolveIssuesView.Container
        @$container.html @current_view.render().el

    logs: ->
        @set_stats_call ''
        log_router '/logs'
        clear_modals()
        @current_view.remove()
        @current_view = new LogView.Container
        @$container.html @current_view.render().el

    dataexplorer: ->
        @set_stats_call ''
        log_router '/dataexplorer'
        clear_modals()
        @current_view.remove()
        @current_view = new DataExplorerView.Container
            state: DataExplorerView.state
        @$container.html @current_view.render().el
        @current_view.init_after_dom_rendered() # Need to be called once the view is in the DOM tree
        @current_view.results_view.set_scrollbar() # In case we check the data explorer, leave and come back

    #TODO Clean the next 3 methods. We don't need tab anymore
    database: (id, tab) ->
        #TODO We can make it better
        @set_stats_call 'ajax/stat?filter=.*/serializers'
        log_router '/databases/' + id
        clear_modals()
        database = databases.get(id)

        @current_view.remove()
        if database? then @current_view = new DatabaseView.Container model: database
        else @current_view = new DatabaseView.NotFound id

        @$container.html @current_view.render().el

    namespace: (id, tab) ->
        @set_stats_call 'ajax/stat?filter='+id+'/serializers'
        log_router '/namespaces/' + id
        clear_modals()
        namespace = namespaces.get(id)

        @current_view.remove()
        if namespace?
            @current_view = new NamespaceView.Container model:namespace
        else
            @current_view = new NamespaceView.NotFound id

        @$container.html @current_view.render().el

    table: (id) ->
        clear_modals()
        @current_view.remove()
        @current_view = new TableView.TableContainer id
        @$container.html @current_view.render().el

    datacenter: (id, tab) ->
        @set_stats_call 'ajax/stat?filter=.*/serializers,proc,sys'
        log_router '/datacenters/' + id
        clear_modals()
        datacenter = datacenters.get(id)

        @current_view.remove()
        if datacenter?
            @current_view = new DatacenterView.Container model: datacenter
        else
            @current_view = new DatacenterView.NotFound id

        @$container.html @current_view.render().el

    server: (id) ->
        log_router '/servers/' + id
        clear_modals()

        @current_view.remove()
        @current_view = new ServerView.ServerContainer id

        @$container.html @current_view.render().el

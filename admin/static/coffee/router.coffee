# Extend Backbone View 
Backbone.View.prototype.destroy = ->
    return

# Router for Backbone.js
class BackboneCluster extends Backbone.Router
    routes:
        '': 'dashboard'
        'databases': 'index_namespaces'
        'databases/:id': 'database'
        'databases/:id/:tab': 'database'
        'tables': 'index_namespaces'
        'tables/:id': 'namespace'
        'tables/:id/:tab': 'namespace'
        'servers': 'index_servers'
        'datacenters/:id': 'datacenter'
        'datacenters/:id/:tab': 'datacenter'
        'servers/:id': 'server'
        'servers/:id/:tab': 'server'
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

        if not $.cookie('rethinkdb-admin')?
            $.cookie('rethinkdb-admin', new Date())
            @render_walkthrough_popup()

        @.bind 'all', (route, router) ->
            @navbar.set_active_tab route

    render_sidebar: -> @$sidebar.html @sidebar.render().el
    render_navbar: -> $('#navbar-container').html @navbar.render().el
    render_walkthrough_popup: -> $('.walkthrough-popup').html (new Walkthrough).render().el

    index_namespaces: (data) ->
        log_router '/index_namespaces'
        clear_modals()
        @current_view.destroy()
        @current_view = new NamespaceView.DatabaseList
        if data?.alert_message?
            @$container.html @current_view.render(data.alert_message).el
        else
            @$container.html @current_view.render().el
        @sidebar.set_type_view()

    index_servers: (data) ->
        log_router '/index_servers'
        clear_modals()
        @current_view.destroy()
        @current_view = new ServerView.DatacenterList
        if data?.alert_message?
            @$container.html @current_view.render(data.alert_message).el
        else
            @$container.html @current_view.render().el
        @sidebar.set_type_view()

    dashboard: ->
        log_router '/dashboard'
        clear_modals()
        @current_view.destroy()
        @current_view = new DashboardView.Container
        @$container.html @current_view.render().el
        @sidebar.set_type_view()

    resolve_issues: ->
        log_router '/resolve_issues'
        clear_modals()
        @current_view.destroy()
        @current_view = new ResolveIssuesView.Container
        @$container.html @current_view.render().el
        @sidebar.set_type_view()

    logs: ->
        log_router '/logs'
        clear_modals()
        @current_view.destroy()
        @current_view = new LogView.Container
        @$container.html @current_view.render().el
        @sidebar.set_type_view()

    dataexplorer: ->
        log_router '/dataexplorer'
        clear_modals()
        @current_view.destroy()
        @current_view = new DataExplorerView.Container
        @$container.html @current_view.render().el
        @current_view.call_codemirror()
        @sidebar.set_type_view('dataexplorer')

    database: (id, tab) ->
        log_router '/databases/' + id
        clear_modals()
        database = databases.get(id)
        
        @current_view.destroy()
        if database? then @current_view = new DatabaseView.Container model: database
        else @current_view = new DatabaseView.NotFound id

        if tab?
            @$container.html @current_view.render(tab).el
        else
            @$container.html @current_view.render().el

        @sidebar.set_type_view()

    namespace: (id, tab) ->
        log_router '/namespaces/' + id
        clear_modals()
        namespace = namespaces.get(id)
        
        @current_view.destroy()
        if namespace?
            @current_view = new NamespaceView.Container model:namespace
        else
            @current_view = new NamespaceView.NotFound id

        if tab?
            @$container.html @current_view.render(tab).el
        else
            @$container.html @current_view.render().el
        
        if namespace?
            @current_view.shards.render_data_repartition()

        @sidebar.set_type_view()

    datacenter: (id, tab) ->
        log_router '/datacenters/' + id
        clear_modals()
        datacenter = datacenters.get(id)
        
        @current_view.destroy()
        if datacenter?
            @current_view = new DatacenterView.Container model: datacenter
        else
            @current_view = new DatacenterView.NotFound id

        if tab?
            @$container.html @current_view.render(tab).el
        else
            @$container.html @current_view.render().el

        if datacenter?
            @current_view.overview.render_ram_repartition()
            @current_view.overview.render_disk_repartition()

        @sidebar.set_type_view()

    server: (id, tab) ->
        log_router '/servers/' + id
        clear_modals()
        machine = machines.get(id)

        @current_view.destroy()
        if machine?
            @current_view = new MachineView.Container model: machine
        else
            @current_view = new MachineView.NotFound id
        
        if tab?
            @$container.html @current_view.render(tab).el
        else
            @$container.html @current_view.render().el
        if machine?
            @current_view.overview.render_pie_disk()
            @current_view.overview.render_pie_ram()

        @sidebar.set_type_view()

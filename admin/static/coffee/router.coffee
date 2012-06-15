
# Router for Backbone.js
class BackboneCluster extends Backbone.Router
    routes:
        '': 'dashboard'
        'namespaces': 'index_namespaces'
        'namespaces/:id': 'namespace'
        'servers': 'index_servers'
        'datacenters/:id': 'datacenter'
        'machines/:id': 'machine'
        'dashboard': 'dashboard'
        'resolve_issues': 'resolve_issues'
        'logs': 'logs'

    initialize: ->
        log_initial '(initializing) router'
        super
        window.app = @

        @$container = $('#cluster')

        @namespace_list = new NamespaceView.NamespaceList
        @server_list = new ServerView.DatacenterList
        @dashboard = new DashboardView.Container
        @navbar = new NavBarView

        # Add and render the sidebar (visible across all views)
        @$sidebar = $('#sidebar')
        @sidebar = new Sidebar.Container
        @render_sidebar()

        # Render navbar for the first time
        @render_navbar()

        @resolve_issues = new ResolveIssuesView.Container
        @logs = new LogView.Container

        if not $.cookie('rethinkdb-admin')?
            #$.cookie('rethinkdb-admin', new Date())
            @render_walkthrough_popup()

    render_sidebar: -> @$sidebar.html @sidebar.render().el
    render_navbar: -> $('#navbar-container').html @navbar.render().el
    render_walkthrough_popup: -> $('.walkthrough-popup').html (new Walkthrough.Popup).render().el

    index_namespaces: ->
        log_router '/index_namespaces'
        clear_modals()
        @$container.html @namespace_list.render().el

    index_servers: ->
        log_router '/index_servers'
        clear_modals()
        @$container.html @server_list.render().el

    dashboard: ->
        log_router '/dashboard'
        clear_modals()
        @$container.html @dashboard.render().el

    resolve_issues: ->
        log_router '/resolve_issues'
        clear_modals()
        @$container.html @resolve_issues.render().el

    logs: ->
        log_router '/logs'
        clear_modals()
        @$container.html @logs.render().el

    namespace: (id) ->
        log_router '/namespaces/' + id
        clear_modals()
        namespace = namespaces.get(id)
        
        if namespace? then view = new NamespaceView.Container model:namespace
        else view = new NamespaceView.NotFound id

        @$container.html view.render().el

    datacenter: (id) ->
        log_router '/datacenters/' + id
        clear_modals()
        datacenter = datacenters.get(id)
        
        if datacenter? then view = new DatacenterView.Container model: datacenter
        else view = new DatacenterView.NotFound id

        @$container.html view.render().el

    machine: (id) ->
        log_router '/machines/' + id
        clear_modals()
        machine = machines.get(id)
        
        if machine? then view = new MachineView.Container model: machine
        else view = new MachineView.NotFound id

        @$container.html view.render().el

# Extend Backbone View 
Backbone.View.prototype.destroy = ->
    return

# Router for Backbone.js
class BackboneCluster extends Backbone.Router
    routes:
        '': 'dashboard'
        'namespaces': 'index_namespaces'
        'namespaces/:id': 'namespace'
        'namespaces/:id/:tab': 'namespace'
        'servers': 'index_servers'
        'datacenters/:id': 'datacenter'
        'datacenters/:id/:tab': 'datacenter'
        'machines/:id': 'machine'
        'machines/:id/:tab': 'machine'
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

    render_sidebar: -> @$sidebar.html @sidebar.render().el
    render_navbar: -> $('#navbar-container').html @navbar.render().el
    render_walkthrough_popup: -> $('.walkthrough-popup').html (new Walkthrough).render().el

    index_namespaces: ->
        log_router '/index_namespaces'
        clear_modals()
        @current_view.destroy()
        @current_view = new NamespaceView.DatabaseList
        @$container.html @current_view.render().el
        @sidebar.set_type_view()

    index_servers: ->
        log_router '/index_servers'
        clear_modals()
        @current_view.destroy()
        @current_view = new ServerView.DatacenterList
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
        @sidebar.set_type_view('dataexplorer')

    namespace: (id, tab) ->
        log_router '/namespaces/' + id
        clear_modals()
        namespace = namespaces.get(id)
        
        @current_view.destroy()
        if namespace? then @current_view = new NamespaceView.Container model:namespace
        else @current_view = new NamespaceView.NotFound id

        if tab?
            @$container.html @current_view.render(tab).el
        else
            @$container.html @current_view.render().el

        @sidebar.set_type_view()

    datacenter: (id, tab) ->
        log_router '/datacenters/' + id
        clear_modals()
        datacenter = datacenters.get(id)
        
        @current_view.destroy()
        if datacenter? then @current_view = new DatacenterView.Container model: datacenter
        else @current_view = new DatacenterView.NotFound id

        if tab?
            @$container.html @current_view.render(tab).el
        else
            @$container.html @current_view.render().el

        @sidebar.set_type_view()

    machine: (id, tab) ->
        log_router '/machines/' + id
        clear_modals()
        machine = machines.get(id)

        @current_view.destroy()
        if machine? then @current_view = new MachineView.Container model: machine
        else @current_view = new MachineView.NotFound id
        
        if tab?
            @$container.html @current_view.render(tab).el
        else
            @$container.html @current_view.render().el

        @sidebar.set_type_view()

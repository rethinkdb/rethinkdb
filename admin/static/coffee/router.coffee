
# Router for Backbone.js
class BackboneCluster extends Backbone.Router
    routes:
        '': 'dashboard'
        'namespaces': 'index_namespaces'
        'namespaces/:id': 'namespace'
        'namespaces/:id/sharding_scheme': 'sharding_scheme'
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

    render_sidebar: -> @$sidebar.html @sidebar.render().el
    render_navbar: -> $('#navbar-container').html @navbar.render().el

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

        # Helper function to build the namespace view
        build_namespace_view = (id) =>
            namespace_view = new NamespaceView.Container id
            @$container.html namespace_view.render().el

        # Return the existing namespace from the collection if it exists
        return build_namespace_view id

    sharding_scheme: (id) ->
        log_router '/namespaces/' + id + '/sharding_scheme'
        clear_modals()

        # Helper function to build the namespace view
        build_sharding_scheme_view = (id) =>
            sharding_scheme_view = new NamespaceView.ModifyShards(id)
            @$container.html sharding_scheme_view.render().el

        # Return the existing namespace from the collection if it exists
        return build_sharding_scheme_view id

    datacenter: (id) ->
        log_router '/datacenters/' + id
        clear_modals()

        # Helper function to build the datacenter view
        build_datacenter_view = (id) =>
            datacenter_view = new DatacenterView.Container id
            @$container.html datacenter_view.render().el

        # Return the existing datacenter from the collection if it exists
        return build_datacenter_view id

    machine: (id) ->
        log_router '/machines/' + id
        clear_modals()

        # Helper function to build the machine view
        build_machine_view = (id) =>
            machine_view = new MachineView.Container id
            @$container.html machine_view.render().el

        # Return the existing machine from the collection if it exists
        return build_machine_view id


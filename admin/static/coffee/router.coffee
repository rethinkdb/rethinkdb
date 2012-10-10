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
    render_walkthrough_popup: ->
        # TODO: add this back when we're ready to do it
        # $('.walkthrough-popup').html (new Walkthrough).render().el

    set_stats_call: (url) =>
        clearTimeout stats_param.timeout
        if url isnt ''
            stats_param.url = url
            collect_stat_data()

    index_namespaces: (data) ->
        @set_stats_call ''
        clear_modals()
        @current_view.destroy()
        @current_view = new NamespaceView.DatabaseList
        if data?.alert_message?
            @$container.html @current_view.render(data.alert_message).el
        else
            @$container.html @current_view.render().el

    index_servers: (data) ->
        @set_stats_call ''
        clear_modals()
        @current_view.destroy()
        @current_view = new ServerView.DatacenterList
        if data?.alert_message?
            @$container.html @current_view.render(data.alert_message).el
        else
            @$container.html @current_view.render().el

    dashboard: ->
        @set_stats_call '/ajax/stat?filter=.*/serializers,proc,sys'
        clear_modals()
        @current_view.destroy()
        @current_view = new DashboardView.Container
        @$container.html @current_view.render().el

    resolve_issues: ->
        @set_stats_call ''
        log_router '/resolve_issues'
        clear_modals()
        @current_view.destroy()
        @current_view = new ResolveIssuesView.Container
        @$container.html @current_view.render().el

    logs: ->
        @set_stats_call ''
        log_router '/logs'
        clear_modals()
        @current_view.destroy()
        @current_view = new LogView.Container
        @$container.html @current_view.render().el

    dataexplorer: ->
        @set_stats_call ''
        log_router '/dataexplorer'
        clear_modals()
        @current_view.destroy()
        @current_view = new DataExplorerView.Container
        @$container.html @current_view.render().el
        @current_view.call_codemirror()

    database: (id, tab) ->
        #TODO We can make it better
        @set_stats_call '/ajax/stat?filter=.*/serializers'
        log_router '/databases/' + id
        clear_modals()
        database = databases.get(id)

        @current_view.destroy()
        if database? then @current_view = new DatabaseView.Container model: database
        else @current_view = new DatabaseView.NotFound id

        @$container.html @current_view.render().el

    namespace: (id, tab) ->
        @set_stats_call '/ajax/stat?filter='+id+'/serializers'
        log_router '/namespaces/' + id
        clear_modals()
        namespace = namespaces.get(id)

        @current_view.destroy()
        if namespace?
            @current_view = new NamespaceView.Container model:namespace
        else
            @current_view = new NamespaceView.NotFound id

        @$container.html @current_view.render().el

        if namespace?
            @current_view.shards.render_data_repartition()

    datacenter: (id, tab) ->
        @set_stats_call '/ajax/stat?filter=.*/serializers,proc,sys'
        log_router '/datacenters/' + id
        clear_modals()
        datacenter = datacenters.get(id)

        @current_view.destroy()
        if datacenter?
            @current_view = new DatacenterView.Container model: datacenter
        else
            @current_view = new DatacenterView.NotFound id

        @$container.html @current_view.render().el

    server: (id, tab) ->
        @set_stats_call '/ajax/stat?machine_whitelist='+id+'&filter=.*/serializers,proc,sys'
        log_router '/servers/' + id
        clear_modals()
        machine = machines.get(id)

        @current_view.destroy()
        if machine?
            @current_view = new MachineView.Container model: machine
        else
            @current_view = new MachineView.NotFound id

        @$container.html @current_view.render().el

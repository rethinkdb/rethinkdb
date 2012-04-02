# Models for Backbone.js
class Namespace extends Backbone.Model
    urlRoot: '/ajax/namespaces'
    initialize: ->
        log_initial '(initializing) namespace model'

    # Is x between the lower and upper splitpoints (the open interval) for the given index?
    splitpoint_between: (shard_index, sp) =>
        console.log "splitpoint_between #{shard_index}, #{sp}"
        all_sps = @.get('splitpoints')
        return (shard_index == 0 || all_sps[shard_index - 1] < sp) && (shard_index == all_sps.length || sp < all_sps[shard_index])

class Datacenter extends Backbone.Model

class Machine extends Backbone.Model

class Event extends Backbone.Model

class Issue extends Backbone.Model

# this is a hook into the directory
class MachineAttributes extends Backbone.Model

class ConnectionStatus extends Backbone.Model

class DataStream extends Backbone.Model
    max_cached: 250
    cache_ready: false

    initialize: ->
        @.set('cached_data': [])

    update: (timestamp) => @.get('data').each (data_point) => @update_data_point data_point, timestamp

    update_data_point: (data_point, timestamp) =>
        # Attributes of the data point
        uuid = data_point.get('id')
        existing_val = data_point.get('value')

        # Get a random number between -1 and 1, using a normal distribution
        random = d3.random.normal(0,0.2)
        delta = random()
        delta = -1 if delta < -1
        delta = 1 if delta > 1

        # Multiply by a constant factor to get a delta for the next walk
        delta = Math.floor(delta * 100)

        # Pop the oldest data point off the cached data (if we've filled the cache)
        cached_data = @.get('cached_data')
        if not cached_data[uuid]?
            cached_data[uuid] = []
        if cached_data[uuid].length >= @num_cached
            cached_data[uuid].shift()

        # Cache the existing value (create a copy based on the existing object)
        cached_data[uuid].push new DataPoint data_point.toJSON()

        if not @cache_ready and cached_data[uuid].length >= 2
            @cache_ready = true
            @.trigger 'cache_ready'

        # Logging (remove TODO)
        ###
        name = data_point.get('collection').get(uuid).get('name')
        if name is 'usa_1' and @.get('name') is 'mem_usage_data'
            console.log name, "in dataset #{@.get('name')} now includes the data", _.map cached_data["01f04592-e403-4abc-a845-83d43f6fd967"], (data_point) -> data_point.get('value')
        ###

        # Make sure the new value is non-negative
        new_val = existing_val + delta
        new_val = existing_val if new_val <= 0

        # Set the new value
        data_point.set
            value: new_val
            # Assumption: every datapoint across datastreams at the time of sampling will have the same datetime stamp
            time: timestamp

class DataPoint extends Backbone.Model

class DataPoints extends Backbone.Collection
    model: DataPoint

class ColorMap extends Backbone.Model

# Collections for Backbone.js
class Namespaces extends Backbone.Collection
    model: Namespace
    name: 'Namespaces'

class Datacenters extends Backbone.Collection
    model: Datacenter
    name: 'Datacenters'

class Machines extends Backbone.Collection
    model: Machine
    name: 'Machines'

class Events extends Backbone.Collection
    model: Event
    comparator: (event) ->
        # sort strings in reverse order (return a negated string)
        String.fromCharCode.apply String,
            _.map(event.get('datetime').split(''), (c) -> 0xffff - c.charCodeAt())

class Issues extends Backbone.Collection
    model: Issue
    url: '/ajax/issues'

# hook into directory
class Directory extends Backbone.Collection
    model: MachineAttributes
    url: '/ajax/directory'

# Router for Backbone.js
class BackboneCluster extends Backbone.Router
    routes:
        '': 'dashboard'
        'namespaces': 'index_namespaces'
        'namespaces/:id': 'namespace'
        'datacenters': 'index_datacenters'
        'datacenters/:id': 'datacenter'
        'machines': 'index_machines'
        'machines/:id': 'machine'
        'dashboard': 'dashboard'
        'resolve_issues': 'resolve_issues'
        'events': 'events'

    initialize: ->
        log_initial '(initializing) router'

        @$container = $('#cluster')

        @namespaces_cluster_view = new ClusterView.NamespacesContainer
            namespaces: namespaces

        @datacenters_cluster_view = new ClusterView.DatacentersContainer
            datacenters: datacenters

        @machines_cluster_view = new ClusterView.MachinesContainer
            machines: machines

        @dashboard_view = new DashboardView

        # Add and render the sidebar (visible across all views)
        @$sidebar = $('#sidebar')
        window.sidebar = new Sidebar.Container()
        @sidebar = window.sidebar
        @render_sidebar()

        @resolve_issues_view = new ResolveIssuesView.Container
        @events_view = new EventsView.Container

    render_sidebar: -> @$sidebar.html @sidebar.render().el

    index_namespaces: ->
        log_router '/index_namespaces'
        clear_modals()
        $('ul.nav li').removeClass('active')
        $('ul.nav li#nav-namespaces').addClass('active')
        @$container.html @namespaces_cluster_view.render().el

    index_datacenters: ->
        log_router '/index_datacenters'
        clear_modals()
        $('ul.nav li').removeClass('active')
        $('ul.nav li#nav-datacenters').addClass('active')
        @$container.html @datacenters_cluster_view.render().el

    index_machines: ->
        log_router '/index_machines'
        clear_modals()
        $('ul.nav li').removeClass('active')
        $('ul.nav li#nav-machines').addClass('active')
        @$container.html @machines_cluster_view.render().el

    dashboard: ->
        log_router '/dashboard'
        clear_modals()
        $('ul.nav li').removeClass('active')
        $('ul.nav li#nav-dashboard').addClass('active')
        @$container.html @dashboard_view.render().el

    resolve_issues: ->
        log_router '/resolve_issues'
        clear_modals()
        $('ul.nav li').removeClass('active')
        @$container.html @resolve_issues_view.render().el

    events: ->
        log_router '/events'
        clear_modals()
        $('ul.nav li').removeClass('active')
        @$container.html @events_view.render().el

    namespace: (id) ->
        log_router '/namespaces/' + id
        clear_modals()
        $('ul.nav li').removeClass('active')

        # Helper function to build the namespace view
        build_namespace_view = (namespace) =>
            namespace_view = new NamespaceView.Container model: namespace
            @$container.html namespace_view.render().el

        # Return the existing namespace from the collection if it exists
        return build_namespace_view namespaces.get(id) if namespaces.get(id)?

        # Otherwise, show an error message stating that the namespace does not exist
        @$container.empty().text 'Namespace '+id+' does not exist.'

    datacenter: (id) ->
        log_router '/datacenters/' + id
        clear_modals()
        $('ul.nav li').removeClass('active')

        # Helper function to build the datacenter view
        build_datacenter_view = (datacenter) =>
            datacenter_view = new DatacenterView.Container model: datacenter
            @$container.html datacenter_view.render().el

        # Return the existing datacenter from the collection if it exists
        return build_datacenter_view datacenters.get(id) if datacenters.get(id)?

        # Otherwise, show an error message stating that the datacenter does not exist
        @$container.empty().text 'Datacenter '+id+' does not exist.'


    machine: (id) ->
        log_router '/machines/' + id
        clear_modals()
        $('ul.nav li').removeClass('active')

        # Helper function to build the machine view
        build_machine_view = (machine) =>
            machine_view = new MachineView.Container model: machine
            @$container.html machine_view.render().el

        # Return the existing machine from the collection if it exists
        return build_machine_view machines.get(id) if machines.get(id)?

        # Otherwise, show an error message stating that the machine does not exist
        @$container.empty().text 'Machine '+id+' does not exist.'

modal_registry = []
clear_modals = ->
    modal.hide_modal() for modal in modal_registry
    modal_registry = []
register_modal = (modal) -> modal_registry.push(modal)

updateInterval = 5000

declare_client_connected = ->
    window.connection_status.set({client_disconnected: false})
    clearTimeout(window.apply_diffs_timer)
    window.apply_diffs_timer = setTimeout (-> window.connection_status.set({client_disconnected: true})), 2 * updateInterval

apply_to_collection = (collection, collection_data) ->
    for id, data of collection_data
        if data
            if collection.get(id)
                collection.get(id).set(data)
            else
                data.id = id
                collection.add(new collection.model(data))
        else
            if collection.get(id)
                collection.remove(id)

add_protocol_tag = (data, tag) ->
    f = (unused,id) ->
        if (data[id])
            data[id].protocol = tag
    _.each(data, f)
    return data

reset_collections = () ->
    namespaces.reset()
    datacenters.reset()
    machines.reset()
    issues.reset()
    directory.reset()

# Process updates from the server and apply the diffs to our view of the data. Used by our version of Backbone.sync and POST / PUT responses for form actions
apply_diffs = (updates) ->
    declare_client_connected()

    if (not connection_status.get('contact_machine_id'))
        connection_status.set('contact_machine_id', updates["me"])
    else
        if (updates["me"] != connection_status.get('contact_machine_id'))
            reset_collections()
            connection_status.set('contact_machine_id', updates["me"])

    for collection_id, collection_data of updates
        switch collection_id
            when 'dummy_namespaces'
                apply_to_collection(namespaces, add_protocol_tag(collection_data, "dummy"))
            when 'memcached_namespaces'
                apply_to_collection(namespaces, add_protocol_tag(collection_data, "memcached"))
            when 'datacenters'
                apply_to_collection(datacenters, collection_data)
            when 'machines'
                apply_to_collection(machines, collection_data)
            when 'me' then continue
            else
                console.log "Unhandled element update: " + updates
    return

set_issues = (issue_data_from_server) -> issues.reset(issue_data_from_server)

set_directory = (attributes_from_server) ->
    # Convert directory representation from RethinkDB into backbone friendly one
    dir_machines = []
    for key, value of attributes_from_server
        value['id'] = key
        dir_machines[dir_machines.length] = value
    directory.reset(dir_machines)

$ ->
    bind_dev_tools()

    # Initializing the Backbone.js app
    window.datacenters = new Datacenters
    window.namespaces = new Namespaces
    window.machines = new Machines
    window.issues = new Issues
    window.directory = new Directory
    window.events = new Events
    window.connection_status = new ConnectionStatus

    window.last_update_tstamp = 0

    # Add fake issues and events for testing
    generate_fake_events(events)
    #generate_fake_issues(issues)

    # Load the data bootstrapped from the HTML template
    # reset_collections()
    reset_token()

    # Log all events fired for the namespaces collection (debugging)
    namespaces.on 'all', (event_name) ->
        console.log 'event fired: '+event_name

    # Override the default Backbone.sync behavior to allow reading diffs
    legacy_sync = Backbone.sync
    Backbone.sync = (method, model, success, error) ->
        if method is 'read'
            $.getJSON('/ajax', apply_diffs)
            $.getJSON('/ajax/issues', set_issues)
            $.getJSON('/ajax/directory', set_directory)
        else
            legacy_sync method, model, success, error


    # This object is for events triggered by views on the router; this exists because the router is unavailable when first initializing
    window.app_events = {}

    window.app = new BackboneCluster
    Backbone.history.start()

    # Signal that the router is ready to be bound to
    $(window.app_events).trigger('on_ready')

    setInterval (-> Backbone.sync 'read', null), updateInterval
    declare_client_connected()

    $.getJSON('/ajax', apply_diffs)
    $.getJSON('/ajax/issues', set_issues)
    $.getJSON('/ajax/directory', set_directory)

    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true

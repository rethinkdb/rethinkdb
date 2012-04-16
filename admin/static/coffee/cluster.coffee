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

module 'DataUtils', ->
    @get_machine_reachability = (machine_uuid) ->
        reachable = directory.get(machine_uuid)?
        if not reachable
            last_seen = machines.get(machine_uuid).get('last_seen')
            if last_seen
                last_seen = $.timeago(new Date(parseInt(last_seen) * 1000))
        json =
            reachable: reachable
            last_seen: last_seen
        return json

    @get_datacenter_reachability = (datacenter_uuid) ->
        total = (_.filter machines.models, (m) => m.get('datacenter_uuid') == datacenter_uuid).length
        reachable = (_.filter directory.models, (m) => machines.get(m.get('id')).get('datacenter_uuid') == datacenter_uuid).length

        if reachable == 0 and total > 0
            for machine in machines.models
                if machine.get('datacenter_uuid') is datacenter_uuid
                    _last_seen = machine.get('last_seen')
                    if last_seen
                        if _last_seen > last_seen
                            last_seen = _last_seen
                    else
                        last_seen = _last_seen
            last_seen = $.timeago(new Date(parseInt(last_seen) * 1000))

        json =
            total: total
            reachable: reachable
            last_seen: last_seen

        return json

    @get_shard_primary_uuid = (namespace_uuid, shard) ->
        for machine_uuid, peers_roles of namespaces.get(namespace_uuid).get('blueprint').peers_roles
            for _shard, role of peers_roles
                if shard.toString() is _shard.toString() and role is 'role_primary'
                    return machine_uuid
        return null

    @get_shard_secondary_uuids = (namespace_uuid, shard) ->
        secondaries = []
        for machine_uuid, peers_roles of namespaces.get(namespace_uuid).get('blueprint').peers_roles
            for _shard, role of peers_roles
                if shard.toString() is _shard.toString() and role is 'role_secondary'
                    secondaries[secondaries.length] = machine_uuid
        return _.uniq(secondaries)

    @get_ack_expectations = (namespace_uuid, datacenter_uuid) ->
        namespace = namespaces.get(namespace_uuid)
        datacenter = datacenters.get(datacenter_uuid)
        acks = namespace.get('ack_expectations')[datacenter.get('id')]
        if acks?
            return acks
        else
            return 0

    @get_replica_affinities = (namespace_uuid, datacenter_uuid) ->
        namespace = namespaces.get(namespace_uuid)
        datacenter = datacenters.get(datacenter_uuid)
        affs = namespace.get('replica_affinities')[datacenter.get('id')]
        if affs?
            return affs
        else
            return 0

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

# navigation bar view. Not sure where it should go, putting it here
# because it's global.
class NavBarView extends Backbone.View
    className: 'navbar-view'
    template: Handlebars.compile $('#navbar_view-template').html()
    first_render: true

    initialize: ->
        log_initial '(initializing) NavBarView'
        # rerender when route changes
        $(window.app_events).on "on_ready", =>
            # Render every time the route changes
            window.app.on "all", @render

    init_typeahead: ->
        $('input.search-query').typeahead
            source: (typeahead, query) ->
                _machines = _.map machines.models, (machine) ->
                    uuid: machine.get('id')
                    name: machine.get('name') + ' (machine)'
                    type: 'machines'
                _datacenters = _.map datacenters.models, (datacenter) ->
                    uuid: datacenter.get('id')
                    name: datacenter.get('name') + ' (datacenter)'
                    type: 'datacenters'
                _namespaces = _.map namespaces.models, (namespace) ->
                    uuid: namespace.get('id')
                    name: namespace.get('name') + ' (namespace)'
                    type: 'namespaces'
                return _machines.concat(_datacenters).concat(_namespaces)
            property: 'name'
            onselect: (obj) ->
                window.app.navigate('#' + obj.type + '/' + obj.uuid , { trigger: true })

    render: (route) =>
        log_render '(rendering) NavBarView'
        @.$el.html @template()
        # set active tab
        if route?
            @.$('ul.nav li').removeClass('active')
            if route is 'route:dashboard'
                $('ul.nav li#nav-dashboard').addClass('active')
            else if route is 'route:index_namespaces'
                $('ul.nav li#nav-namespaces').addClass('active')
            else if route is 'route:index_datacenters'
                $('ul.nav li#nav-datacenters').addClass('active')
            else if route is 'route:index_machines'
                $('ul.nav li#nav-machines').addClass('active')

        if @first_render?
            # Initialize typeahead
            @init_typeahead()
            @first_render = false

        return @

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

        @navbar_view = new NavBarView

        # Add and render the sidebar (visible across all views)
        @$sidebar = $('#sidebar')
        window.sidebar = new Sidebar.Container()
        @sidebar = window.sidebar
        @render_sidebar()

        # Render navbar for the first time
        @render_navbar()

        @resolve_issues_view = new ResolveIssuesView.Container
        @events_view = new EventsView.Container

    render_sidebar: -> @$sidebar.html @sidebar.render().el
    render_navbar: -> $('#navbar-container').html @navbar_view.render().el

    index_namespaces: ->
        log_router '/index_namespaces'
        clear_modals()
        @$container.html @namespaces_cluster_view.render().el

    index_datacenters: ->
        log_router '/index_datacenters'
        clear_modals()
        @$container.html @datacenters_cluster_view.render().el

    index_machines: ->
        log_router '/index_machines'
        clear_modals()
        @$container.html @machines_cluster_view.render().el

    dashboard: ->
        log_router '/dashboard'
        clear_modals()
        @$container.html @dashboard_view.render().el

    resolve_issues: ->
        log_router '/resolve_issues'
        clear_modals()
        @$container.html @resolve_issues_view.render().el

    events: ->
        log_router '/events'
        clear_modals()
        @$container.html @events_view.render().el

    namespace: (id) ->
        log_router '/namespaces/' + id
        clear_modals()

        # Helper function to build the namespace view
        build_namespace_view = (id) =>
            namespace_view = new NamespaceView.Container id
            @$container.html namespace_view.render().el

        # Return the existing namespace from the collection if it exists
        return build_namespace_view id

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

set_last_seen = (last_seen) ->
    # Expand machines model with this data
    for machine_uuid, timestamp of last_seen
        _m = machines.get machine_uuid
        if _m
            _m.set('last_seen', timestamp)

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

    # Override the default Backbone.sync behavior to allow reading diffs
    legacy_sync = Backbone.sync
    Backbone.sync = (method, model, success, error) ->
        if method is 'read'
            $.getJSON('/ajax', apply_diffs)
            $.getJSON('/ajax/issues', set_issues)
            $.getJSON('/ajax/directory', set_directory)
            $.getJSON('/ajax/last_seen', set_last_seen)
        else
            legacy_sync method, model, success, error


    # This object is for events triggered by views on the router; this exists because the router is unavailable when first initializing
    window.app_events = {}

    # Create the app
    window.app = new BackboneCluster

    # Signal that the router is ready to be bound to
    $(window.app_events).trigger('on_ready')

    # Now that we're all bound, start routing
    Backbone.history.start()

    setInterval (-> Backbone.sync 'read', null), updateInterval
    declare_client_connected()

    $.getJSON('/ajax', apply_diffs)
    $.getJSON('/ajax/issues', set_issues)
    $.getJSON('/ajax/directory', set_directory)
    $.getJSON('/ajax/last_seen', set_last_seen)

    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true

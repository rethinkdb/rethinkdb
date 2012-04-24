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

class LogEntry extends Backbone.Model

class Issue extends Backbone.Model

class Progress extends Backbone.Model

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
        total = (_.filter machines.models, (m) => m.get('datacenter_uuid') is datacenter_uuid).length
        reachable = (_.filter directory.models, (m) => machines.get(m.get('id')).get('datacenter_uuid') is datacenter_uuid).length

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
        # We're organizing secondaries per datacenter
        secondaries = {}
        for machine_uuid, peers_roles of namespaces.get(namespace_uuid).get('blueprint').peers_roles
            datacenter_uuid = machines.get(machine_uuid).get('datacenter_uuid')
            for _shard, role of peers_roles
                if shard.toString() is _shard.toString() and role is 'role_secondary'
                    if not secondaries[datacenter_uuid]?
                        secondaries[datacenter_uuid] = []
                    secondaries[datacenter_uuid][secondaries[datacenter_uuid].length] = machine_uuid
        return secondaries

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

    @get_datacenter_machines = (datacenter_uuid) ->
        return _.filter(machines.models, (m) -> m.get('datacenter_uuid') is datacenter_uuid)

    # Return a list of machines relevant to a namespace
    @get_namespace_machines = (namespace_uuid) ->
        mids = []
        for mid, roles of namespaces.get(namespace_uuid).get('blueprint').peers_roles
            mids.push(mid)
        return mids

    # Organizes the directory as a map of activity ids
    @get_directory_activities = ->
        activities = {}
        for machine in directory.models
            bcards = machine.get('memcached_namespaces')['reactor_bcards']
            for namespace_id, activity_map of bcards
                activity_map = activity_map['activity_map']
                for activity_id, activity of activity_map
                    activities[activity_id] =
                        value: activity
                        machine_id: machine.get('id')
                        namespace_id: namespace_id
        return activities

    # Computes backfill progress for a given (namespace, shard,
    # machine) tripple. All arguments correspond to the objects that
    # are *receiving* data.
    @get_backfill_progress = (namespace_uuid, shard, machine_uuid) ->
        activity_map = @get_directory_activities()
        _output_json = []
        if typeof(shard) isnt 'string'
            shard = JSON.stringify(shard)

        # grab the right machine from the progress list
        progress = progress_list.get(machine_uuid)
        if not progress?
            return null

        # next level of organization is by namespaces, grab the
        # one we need
        progress = progress.get(namespace_uuid)
        if not progress?
            return null

        # The level of organization after that is by activity
        # ids. Each activity corresponds to the shard we're
        # backfilling into.
        for activity_uuid, shard_map of progress
            # Grab the activity from the activity map and break it up
            # into some useful information.
            activity = activity_map[activity_uuid]
            if not activity?
                # This probably means directory data hasn't caught up
                # with the progress data on the client yet.
                return null
            activity_type = activity.value[1].type
            into_shard = activity.value[0]

            # Check that we're on the right shard (our C++ and JS JSON
            # stringifiers have different policies with respect to
            # spaces, so removing spaces here. We really need a
            # standard way to compare shards - TODO here and in many
            # many other places)
            if into_shard.replace(/\s/g, '') isnt shard.replace(/\s/g, '')
                continue

            # Collect activity info about all the shards we're
            # replicating from
            senders_activity_id = []
            if activity_type is 'secondary_backfilling'
                senders_activity_id.push(activity.value[1].backfiller.activity_id)
            else if activity_type is 'primary_when_safe'
                for backfiller in activity.value[1].backfillers
                    senders_activity_id.push(backfiller.activity_id)


            # We now have a map from one shard (union of all shards
            # we're replicating from), to an array of progress
            # info. Grab these.
            union_shard = null
            progress_info_arr = null
            for _us, _pia of shard_map
                union_shard = _us
                progress_info_arr = _pia
                break

            # We now have two arrays: senders and progress infos
            # (which should theoretically be the same size). Combine
            # them into data points, push each datapoint onto
            # _output_json. We'll aggregate after the loop is done.
            for i in [0..(progress_info_arr.length - 1)]
                progress_info = progress_info_arr[i]
                sender = activity_map[senders_activity_id[i]]
                ratio_available = typeof(progress_info) isnt 'string'
                json =
                    replicated_blocks:    if ratio_available then progress_info[0] else null
                    total_blocks:         if ratio_available then progress_info[1] else null
                    block_info_available: ratio_available and progress_info[0] isnt -1 and progress_info[1] isnt -1
                    ratio_available:      ratio_available
                    machine_id:           sender.machine_id
                _output_json.push json

        # Make sure there is stuff to report
        if _output_json.length is 0
            return null

        # Now we can aggregate the results for this shard by summing
        # progress from every shard it's replicating from
        agg_start =
            total_blocks: -1
            replicated_blocks: -1
            block_info_available: false
            ratio_available: false
            backfiller_machines: []
        agg_json = _.reduce(_output_json, ((agg, val) ->
            if val.block_info_available
                agg.total_blocks      = 0 if agg.total_blocks is -1
                agg.replicated_blocks = 0 if agg.replicated_blocks is -1
                agg.total_blocks      += val.total_blocks
                agg.replicated_blocks += val.replicated_blocks
                agg.block_info_available = true
            if val.ratio_available
                agg.ratio_available = true
            if typeof(val.machine_id) is 'string'
                agg.backfiller_machines.push
                    machine_id:   val.machine_id
                    machine_name: machines.get(val.machine_id).get('name')
            return agg
            ), agg_start)
        # Phew, final output
        output_json =
            ratio:             agg_json.replicated_blocks / agg_json.total_blocks
            percentage:        Math.round(agg_json.replicated_blocks / agg_json.total_blocks * 100)
        output_json = _.extend output_json, agg_json

        return output_json

    # Computes aggregate backfill progress for a namespace. The
    # datacenter argument can optionally limit the aggreggate values
    # to the datacenter.
    @get_backfill_progress_agg = (namespace_uuid, datacenter_uuid) ->
        # Find a list of machines we care about
        namespace_machines = @get_namespace_machines(namespace_uuid)
        if datacenter_uuid?
            datacenter_machines = _.map @get_datacenter_machines(datacenter_uuid), (m) -> m.get('id')
        else
            datacenter_machines = namespace_machines
        machines = _.intersection namespace_machines, datacenter_machines
        # Collect backfill progress for each machine
        backfills = []
        for machine_uuid in machines
            for shard in namespaces.get(namespace_uuid).get('shards')
                backfills.push(@get_backfill_progress(namespace_uuid, shard, machine_uuid))
        backfills = _.without backfills, null
        if backfills.length is 0
            return null
        # Aggregate backfill results
        agg_start =
            total_blocks: -1
            replicated_blocks: -1
            block_info_available: false
            ratio_available: false
        agg_json = _.reduce(backfills, ((agg, val) ->
            if not val?
                return agg
            if val.block_info_available
                agg.total_blocks      = 0 if agg.total_blocks is -1
                agg.replicated_blocks = 0 if agg.replicated_blocks is -1
                agg.total_blocks      += val.total_blocks
                agg.replicated_blocks += val.replicated_blocks
                agg.block_info_available = true
            if val.ratio_available
                agg.ratio_available = true
            return agg
            ), agg_start)
        # Phew, final output
        output_json =
            ratio:             agg_json.replicated_blocks / agg_json.total_blocks
            percentage:        Math.round(agg_json.replicated_blocks / agg_json.total_blocks * 100)
        output_json = _.extend output_json, agg_json

        return output_json

    # datacenter_uuid is optional and limits the information to a
    # specific datacenter
    @get_namespace_status = (namespace_uuid, datacenter_uuid) ->
        namespace = namespaces.get(namespace_uuid)

        json =
            nshards: 0
            nreplicas: 0
            nashards: 0
            nareplicas: 0

        # machine and datacenter counts
        _machines = []
        _datacenters = []

        for machine_uuid, role of namespace.get('blueprint').peers_roles
            if datacenter_uuid? and
               machines.get(machine_uuid) and
               machines.get(machine_uuid).get('datacenter_uuid') isnt datacenter_uuid
                continue
            peer_accessible = directory.get(machine_uuid)
            machine_active_for_namespace = false
            for shard, role_name of role
                if role_name is 'role_primary'
                    machine_active_for_namespace = true
                    json.nshards += 1
                    if peer_accessible?
                        json.nashards += 1
                if role_name is 'role_secondary'
                    machine_active_for_namespace = true
                    json.nreplicas += 1
                    if peer_accessible?
                        json.nareplicas += 1
            if machine_active_for_namespace
                _machines[_machines.length] = machine_uuid
                _datacenters[_datacenters.length] = machines.get(machine_uuid).get('datacenter_uuid')

        json.nmachines = _.uniq(_machines).length
        json.ndatacenters = _.uniq(_datacenters).length
        if json.nshards is json.nashards
            json.reachability = 'Live'
        else
            json.reachability = 'Down'

        json.backfill_progress = @get_backfill_progress_agg(namespace_uuid, datacenter_uuid)

        return json

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

class LogEntries extends Backbone.Collection
    model: LogEntry
    comparator: (log_entry) ->
        # sort strings in reverse order (return a negated string)
        #String.fromCharCode.apply String,
        #    _.map(log_entry.get('datetime').split(''), (c) -> 0xffff - c.charCodeAt())

class Issues extends Backbone.Collection
    model: Issue
    url: '/ajax/issues'

class ProgressList extends Backbone.Collection
    model: Progress
    url: '/ajax/progress'

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
        @.$('input.search-query').typeahead
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
            else if route is 'route:index_servers'
                $('ul.nav li#nav-servers').addClass('active')

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
        'servers': 'index_servers'
        'datacenters/:id': 'datacenter'
        'machines/:id': 'machine'
        'dashboard': 'dashboard'
        'resolve_issues': 'resolve_issues'
        'logs': 'logs'

    initialize: ->
        log_initial '(initializing) router'

        @$container = $('#cluster')

        @namespace_list = new ClusterView.NamespaceList
        @server_list = new ClusterView.DatacenterList
        @dashboard = new DashboardView
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

set_progress = (progress_data_from_server) ->
    # Convert progress representation from RethinkDB into backbone friendly one
    _pl = []
    for key, value of progress_data_from_server
        value['id'] = key
        _pl.push(value)
    progress_list.reset(_pl)

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

set_log_entries = (log_data_from_server) ->
    all_log_entries = []
    for machine_uuid, log_entries of log_data_from_server
        _m_collection = new LogEntries
        for json in log_entries
            entry = new LogEntry json
            _m_collection.add entry
            all_log_entries.push entry

        machines.get(machine_uuid).set('log_entries', _m_collection)
        _m = machines.get(machine_uuid)

    recent_log_entries.reset(all_log_entries)

$ ->
    bind_dev_tools()

    # Initializing the Backbone.js app
    window.datacenters = new Datacenters
    window.namespaces = new Namespaces
    window.machines = new Machines
    window.issues = new Issues
    window.progress_list = new ProgressList
    window.directory = new Directory
    window.recent_log_entries = new LogEntries
    window.connection_status = new ConnectionStatus

    window.last_update_tstamp = 0

    # Add fake issues and events for testing | DELETE TODO
    #generate_fake_events(events)
    #generate_fake_issues(issues)

    # Load the data bootstrapped from the HTML template
    # reset_collections()
    reset_token()

    # A helper function to collect data from all of our shitty
    # routes. TODO: somebody fix this in the server for heaven's
    # sakes!!!
    collect_server_data = =>
        $.getJSON('/ajax', apply_diffs)
        $.getJSON('/ajax/issues', set_issues)
        $.getJSON('/ajax/progress', set_progress)
        $.getJSON('/ajax/directory', set_directory)
        $.getJSON('/ajax/last_seen', set_last_seen)
        $.getJSON('/ajax/log/_?max_length=10', set_log_entries)

    # Override the default Backbone.sync behavior to allow reading diffs
    legacy_sync = Backbone.sync
    Backbone.sync = (method, model, success, error) ->
        if method is 'read'
            collect_server_data()
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

    collect_server_data()

    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true

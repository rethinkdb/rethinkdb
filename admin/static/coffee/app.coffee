# Copyright 2010-2012 RethinkDB, all rights reserved.
# This file contains all the good stuff we do to set up the
# application. We should refactor this at some point, but I'm leaving
# it as is for now.

modal_registry = []
clear_modals = ->
    modal.hide_modal() for modal in modal_registry
    modal_registry = []
register_modal = (modal) -> modal_registry.push(modal)

updateInterval = 5000
statUpdateInterval = 1000
progress_interval_default_value = 5000
progress_interval_value = 5000
progress_short_interval = 1000

#TODO Duplicate this function, and remove element not found in case of a call to /ajax
apply_to_collection = (collection, collection_data) ->
    for id, data of collection_data
        if data isnt null
            if data.protocol? and data.protocol is 'rdb'  # We check that the machines in the blueprint do exist
                if collection_data[id].blueprint? and collection_data[id].blueprint.peers_roles?
                    for machine_uuid of collection_data[id].blueprint.peers_roles
                        if !machines.get(machine_uuid)?
                            delete collection_data[id].blueprint.peers_roles[machine_uuid]
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

# Process updates from the server and apply the diffs to our view of the data.
# Used by our version of Backbone.sync and POST / PUT responses for form actions
apply_diffs = (updates) ->
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
            when 'databases'
                apply_to_collection(databases, collection_data)
            when 'rdb_namespaces'
                apply_to_collection(namespaces, add_protocol_tag(collection_data, "rdb"))
            when 'datacenters'
                apply_to_collection(datacenters, collection_data)
            when 'machines'
                apply_to_collection(machines, collection_data)
            when 'me' then continue
            else
                #console.log "Unhandled element update: " + collection_id + "."
    return

set_issues = (issue_data_from_server) -> issues.reset(issue_data_from_server)

set_progress = (progress_data_from_server) ->
    is_empty = true
    # Convert progress representation from RethinkDB into backbone friendly one
    _pl = []
    for key, value of progress_data_from_server
        is_empty = false
        value['id'] = key
        _pl.push(value)
    progress_list.reset(_pl)

    if is_empty is false and progress_interval_value is progress_interval_default_value
        clearInterval window.progress_interval
        progress_interval_value = progress_short_interval
        window.progress_interval = setInterval collect_progress, progress_interval_value
    else if is_empty is true and progress_interval_value is progress_short_interval
        clearInterval window.progress_interval
        progress_interval_value = progress_interval_default_value
        window.progress_interval = setInterval collect_progress, progress_interval_value
    
    # Since backfilling is half working, we want to often check directory/blueprint to give users feedback
    if is_empty is false
        setTimeout collect_server_data_async, 2500

set_directory = (attributes_from_server) ->
    # Convert directory representation from RethinkDB into backbone friendly one
    dir_machines = []
    for key, value of attributes_from_server
        if value.peer_type is 'server'
            value['id'] = key
            dir_machines[dir_machines.length] = value
    directory.reset(dir_machines)

set_last_seen = (last_seen_from_server) ->
    # Expand machines model with this data
    for machine_uuid, timestamp of last_seen_from_server
        _m = machines.get machine_uuid
        if _m
            _m.set('last_seen', timestamp)

set_stats = (stat_data) ->
    for machine_id, data of stat_data
        if machines.get(machine_id)? #if the machines are not ready, we just skip the current stats
            machines.get(machine_id).set('stats', data)
        else if machine_id is 'machines' # It would be nice if the back end could update that.
            for mid in data.known
                machines.get(mid)?.set('stats_up_to_date', true)
            for mid in data.timed_out
                machines.get(mid)?.set('stats_up_to_date', false)
            ###
            # Ignore these cases for the time being. When we'll consider these, 
            # we might need an integer instead of a boolean
            for mid in data.dead
                machines.get(mid)?.set('stats_up_to_date', false)
            for mid in data.ghosts
                machines.get(mid)?.set('stats_up_to_date', false)
            ###


set_reql_docs = (data) ->
    DataExplorerView.Container.prototype.set_docs data

error_load_reql_docs = ->
    #TODO Do we need to display a nice message?
    console.log 'Could not load reql documentation'

collections_ready = ->
    # Data is now ready, let's get rockin'!
    render_body()
    window.router = new BackboneCluster
    Backbone.history.start()

collect_reql_doc = ->
    $.ajax
        url: '/js/reql_docs.json'
        dataType: 'json'
        contentType: 'application/json'
        success: set_reql_docs
        error: error_load_reql_docs

# A helper function to collect data from all of our shitty
# routes. TODO: somebody fix this in the server for heaven's
# sakes!!!
#   - an optional callback can be provided. Currently this callback will only be called after the /ajax route (metadata) is collected
# To avoid memory leak, we use function declaration (so with pure javascript since coffeescript can't do it)
# Using setInterval seems to be safe, TODO
collect_server_data_once = (async, optional_callback) ->
    $.ajax
        url: '/ajax'
        dataType: 'json'
        contentType: 'application/json'
        async: async
        success: (updates) ->
            if window.is_disconnected?
                delete window.is_disconnected
                window.location.reload(true)

            apply_diffs(updates.semilattice)
            set_issues(updates.issues)
            set_directory(updates.directory)
            set_last_seen(updates.last_seen)
            if optional_callback?
                optional_callback()
        error: ->
            window.connection_status.set({client_disconnected: false})
            if window.is_disconnected?
                window.is_disconnected.display_fail()
            else
                window.is_disconnected = new IsDisconnected
        timeout: updateInterval*3

collect_progress = ->
    $.ajax
        contentType: 'application/json',
        url: '/ajax/progress',
        dataType: 'json',
        success: set_progress

collect_server_data_async = ->
    collect_server_data_once true


stats_param =
    url: '/ajax/stat'
    fail: false
collect_stat_data = ->
    $.ajax
        url: stats_param.url
        dataType: 'json'
        contentType: 'application/json'
        success: (data) ->
            set_stats(data)
            stats_param.fail = false
            stats_param.timeout = setTimeout collect_stat_data, 1000
        error: ->
            stats_param.fail = true
            stats_param.timeout = setTimeout collect_stat_data, 1000


# driver_connected: State of the driver
# - true => connected
# - false => error
# - null => not connected ( left the data explorer or never came )

# Method called if the driver just connected
driver_connect_success = ->
    window.driver_connected_previous_state = window.driver_connected
    window.driver_connected = true
    # If the view is DataExplorerView.Container and if we were disconencted before, we'll say that we did reconnect
    if Backbone.history.fragment is 'dataexplorer'
        window.router.current_view?.success_on_connect()

# Method called if the driver fail to connect
driver_connect_fail = ->
    window.driver_connected_previous_state = window.driver_connected
    window.driver_connected = false
    # If the view is DataExplorerView.Container and if we were connected before, we'll display an error
    if Backbone.history.fragment is 'dataexplorer'
        window.router.current_view?.error_on_connect()

# Define the server to which the javascript is going to connect to
# Tweaking the value of server.host or server.port can trigger errors for testing
if window.location.port is ''
    if window.location.protocol is 'https:'
        port = 443
    else
        port = 80
else
    port = parseInt window.location.port
window.server =
    host: window.location.hostname
    port: port
    protocol: if window.location.protocol is 'https:' then 'https' else 'http'

# Connect the driver every five minutes (the connection times out every 5-10 minutes)
driver_connect = ->
    # Whether we are going to reconnect or not, the cursor might have timed out.
    DataExplorerView.Container.prototype.cursor_timed_out = true

    # We try to close the connection if we were connected
    if window.conn?
        if window.driver_connected is true
            try
                window.conn.close()
            #catch
            #   console.log 'Could not destroy connection'
            #   Can it fail?

    # We are going to reconnect only if we still need to (that is to say the user is viewing the data explorer)
    if Backbone.history.fragment is 'dataexplorer'
        window.conn = r.connect window.server, driver_connect_success, driver_connect_fail

        if window.timeout_driver_connect?
            clearTimeout window.timeout_driver_connect
        window.timeout_driver_connect = setTimeout driver_connect, 5*60*1000
    else
        # driver_connected_previous_state tracks the previous state of the driver.
        # We need it to know for instance when we reconnect if we had an error before or if it's the first time we connect.
        # In case there was an error, we display a message to say that we successfully reconnect
        window.driver_connected_previous_state = window.driver_connected
        window.driver_connected = null


$ ->
    render_loading()
    bind_dev_tools()

    # Initializing the Backbone.js app
    window.datacenters = new Datacenters
    window.databases = new Databases
    window.namespaces = new Namespaces
    window.machines = new Machines
    window.issues = new Issues
    window.progress_list = new ProgressList
    window.directory = new Directory
    window.issues_redundancy = new IssuesRedundancy
    window.connection_status = new ConnectionStatus
    window.computed_cluster = new ComputedCluster

    window.last_update_tstamp = 0
    window.universe_datacenter = new Datacenter
        id: '00000000-0000-0000-0000-000000000000'
        name: 'Universe'

    # Override the default Backbone.sync behavior to allow reading diff
    Backbone.sync = (method, model, success, error) ->
        if method is 'read'
            collect_server_data()
        else
            Backbone.sync method, model, success, error

    # Collect the first time
    collect_server_data_once(true, collections_ready)

    # Set interval to update the data
    setInterval collect_server_data_async, updateInterval
    window.progress_interval = setInterval collect_progress, progress_interval_value

    # Collect reql docs
    collect_reql_doc()

    # Set namespace for the javascript driver
    if rethinkdb?
        window.r = rethinkdb

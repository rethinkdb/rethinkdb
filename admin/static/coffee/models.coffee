# Copyright 2010-2012 RethinkDB, all rights reserved.
#Models for Backbone.js

class Servers extends Backbone.Collection
    model: Server
    name: 'Servers'

class Server extends Backbone.Model

class Tables extends Backbone.Collection
    model: Table
    name: 'Tables'
    ###
    initialize: (models, options) ->
        @document = options.tables
        super()
    ###

class Table extends Backbone.Model

class Databases extends Backbone.Collection
    model: Database
    name: 'Databases'

class Database extends Backbone.Model


class Indexes extends Backbone.Collection
    model: Index
    name: 'Indexes'
    comparator: 'index'

class Index extends Backbone.Model

class Distribution extends Backbone.Collection
    model: Shard
    name: 'Shards'

class Shard extends Backbone.Model

class ShardAssignments extends Backbone.Collection
    model: ShardAssignment
    name: 'ShardAssignment'
    comparator: (a, b) ->
        if a.get('shard_id') < b.get('shard_id')
            return -1
        else if a.get('shard_id') > b.get('shard_id')
            return 1
        else
            if a.get('start_shard') is true
                return -1
            else if b.get('start_shard') is true
                return 1
            else if a.get('end_shard') is true
                return 1
            else if b.get('end_shard') is true
                return -1
            else if a.get('director') is true and b.get('replica') is true
                return -1
            else if a.get('replica') is true and b.get('director') is true
                return 1
            else if a.get('replica') is true and  b.get('replica') is true
                if a.get('replica_position') < b.get('replica_position')
                    return -1
                else if a.get('replica_position') > b.get('replica_position')
                    return 1
                return 0

class ShardAssignment extends Backbone.Model

class Namespace extends Backbone.Model
    initialize: ->
        # Add a computed shards property for convenience and metadata
        @compute_shards()
    compute_shards: =>
        @.set 'computed_shards', new DataUtils.Shards [],@

    regex:
        string: /^S/
        number: /^N.*%23(\-?[0-9]*(\.)?[0-9]*$)/
        date: /^(PTIME).*%23\-?([0-9]*(\.)?[0-9]*$)/
    transform_key: (a) ->
        if a is null
            a_new = a
        else if a is ""
            a_new = -Infinity
        else if typeof a is "string"
            if @regex.string.test(a) is true
                a_new = a.slice(1)
            else if @regex.number.test(a) is true
                s =
                    a.slice(a.indexOf("%23")+3)
                if _.isNaN(parseFloat(s)) is false
                    a_new = parseFloat(s)
            else if @regex.date.test(a) is true
                a_new = new Date(parseFloat(a.slice(a.indexOf("%23")+3)))
        return a_new


    compare_keys: (a, b) =>
        #return 1 if a > b
        a_new = @transform_key(a)
        b_new = @transform_key(b)

        if typeof a_new is 'number' and typeof b_new is 'number'
            return a_new-b_new
        else if (typeof a_new is 'string' and typeof b_new is 'string') or (a_new instanceof Date and b_new instanceof Date)
            if a_new > b_new
                return 1
            else if a_new < b_new
                return -1
            else
                return 0
        else if typeof a_new isnt typeof b_new
            if typeof a_new is 'number' and typeof b_new is 'string'
                return -1
            else if typeof a_new is 'number' and b_new instanceof Date
                return -1
            else if a_new instanceof Date and typeof b_new is 'string'
                return -1
            else if typeof a_new is 'string' and typeof b_new is 'number'
                return 1
            else if a_new instanceof Date and typeof b_new is 'number'
                return 1
            else if typeof a_new is 'string' and b_new instanceof Date
                return 1

            else if a_new is null and b_new isnt null
                return 1
            else if b_new is null and a_new isnt null
                return -1
        return 0


    load_key_distr: =>
        $.ajax
            processData: false
            url: "ajax/distribution?namespace=#{@get('id')}&depth=2"
            type: 'GET'
            contentType: 'application/json'
            success: (distr_data) =>
                # Cache the data
                # Sort the keys and cache that too
                distr_keys = []
                for key, count of distr_data
                    distr_keys.push(key)
                distr_keys.sort(@compare_keys)

                @set('key_distr_sorted', distr_keys)
                @set('key_distr', distr_data)
                @timeout = setTimeout @load_key_distr, 5000
            error: =>
                if namespaces.get(@get('id'))?
                    @timeout = setTimeout @load_key_distr, 1000

    clear_timeout: =>
        if @timeout?
            clearTimeout @timeout

    # Some shard helpers
    compute_shard_rows_approximation: (shard) =>
        # first see if we can grab the distr data
        shard = $.parseJSON(shard)
        if not @get('key_distr')? or not @get('key_distr_sorted')?
            return null

        # some basic initialization
        start_key = shard[0]
        end_key = shard[1]

        # TODO: we should interpolate once we will have decided how to order different type of keys

        # find the first key greater than the beginning of our shard
        # and keep summing until we get past our shard boundary.
        count = 0

        for key in @get('key_distr_sorted')
            # TODO Might be unsafe when comparing string and integers. Need to be checked when the back end will have decided what to do.
            if @compare_keys(key, start_key) >= 0 or start_key is ''
                if @compare_keys(key, end_key) < 0 or end_key is null
                    if @get('key_distr')[key]?
                        count += @get('key_distr')[key]

        return count.toString() # Return string since [] == 0 return true (for Handlebars)

    # Is x between the lower and upper splitpoints (the open interval) for the given index?
    splitpoint_between: (shard_index, sp) =>
        all_sps = @.get('splitpoints')
        return (shard_index == 0 || all_sps[shard_index - 1] < sp) && (shard_index == all_sps.length || sp < all_sps[shard_index])

    # Computing btree stats based on machine stats
    get_stats: =>
        __s =
            keys_read: 0
            keys_set: 0
        _.each DataUtils.get_namespace_machines(@get('id')), (mid) =>
            _m = machines.get(mid)
            if _m?
                _s = _m.get_stats()[@get('id')]
            if _s?.serializers?
                keys_read = 0
                keys_set = 0
                for serializer_id of _s.serializers
                    serializer = _s.serializers[serializer_id]
                    if serializer['btree-primary']?
                        keys_read = parseFloat(serializer['btree-primary'].keys_read)
                        keys_set = parseFloat(serializer['btree-primary'].keys_set)
                        if not isNaN(keys_read)
                            __s.keys_read += keys_read
                        if not isNaN(keys_set)
                            __s.keys_set += keys_set
        return __s

    get_stats_for_performance: =>
        # Ops/sec stats
        __s =
            keys_read: 0
            keys_set: 0
            global_disk_space: 0
        _s = this.get_stats()
        if _s?
            __s.keys_read += _s.keys_read
            __s.keys_set += _s.keys_set
        # CPU, mem, disk
        num_machines_in_namespace = 0
        for machine in machines.models
            if machine.get('stats')? and @get('id') of machine.get('stats') and machine.is_reachable
                num_machines_in_namespace++
                __s.global_disk_space += machine.get_used_disk_space()
        return __s

    get_durability: =>
        if @get('hard_durability')?
            return @get('hard_durability')
        else
            for dc, ack of @get('ack_expectations')
                @set('hard_durability', ack.hard_durability)
                return @get('hard_durability')
            @set('hard_durability', true)
            return @get('hard_durability')

class Datacenter extends Backbone.Model
    # Compute the number of machines not used by other datacenters for one namespace
    compute_num_machines_not_used_by_other_datacenters: (namespace) =>
        max_machines = machines.length
        for datacenter_id of namespace.get('replica_affinities')
            # The second condition is to make sure that the datacenter does exist (and was not deleted)
            if datacenter_id isnt @get('id') and datacenters.get(datacenter_id)?
                max_machines -= namespace.get('replica_affinities')[datacenter_id]
        if namespace.get('primary_uuid') isnt @get('id')
            max_machines -= 1

        return max_machines


    get_machines: =>
        machines.filter (machine) => machine.get('datacenter_uuid') is @get('id')
    get_stats: =>
        stats =
            dc_disk_space: 0
        for machine in machines.models
            if machine.get('datacenter_uuid') is @get('id')
                stats.dc_disk_space += machine.get_used_disk_space()
        return stats

    get_stats_for_performance: =>
        # Ops/sec stats
        __s =
            keys_read: 0
            keys_set: 0
            global_disk_space: 0
        for namespace in namespaces.models
            if not namespace.get('blueprint')? or not namespace.get('blueprint').peers_roles?
                continue
            for machine_id of namespace.get('blueprint').peers_roles
                machine = machines.get(machine_id)
                if not machine?
                    continue
                has_data = false
                if machine.get('datacenter_uuid') is @get('id') and namespace.get('blueprint').peers_roles[machine_id]?
                    for shard_key of namespace.get('blueprint').peers_roles[machine_id]
                        if namespace.get('blueprint').peers_roles[machine_id][shard_key] isnt 'role_nothing'
                            has_data = true
                            break
                    if has_data is true
                        break
            if has_data is true
                _s = namespace.get_stats()
                if not _s?
                    continue
                __s.keys_read += _s.keys_read
                __s.keys_set += _s.keys_set

        # CPU, mem, disk
        num_machines_in_datacenter = 0
        for machine in machines.models
            if machine.get('datacenter_uuid') is @get('id') and machine.is_reachable
                num_machines_in_datacenter++
                __s.global_disk_space += machine.get_used_disk_space()
        return __s


class Machine extends Backbone.Model
    get_stats: =>
        stats = @get('stats')
        if not stats?
            stats = {}
        if not stats.proc?
            stats.proc = {}
        return stats

    get_used_disk_space: =>
        machine_disk_space = 0
        for nid, value of @get_stats()
            # Check if nid is actually a namespace id
            if namespaces.get(nid)?
                stats = value.cache
                if stats?
                    machine_disk_space += parseInt(stats.block_size) * parseInt(stats.blocks_total)
        return machine_disk_space

    get_stats_for_performance: =>
        stats_full = @get_stats()
        __s =
            keys_read: 0
            keys_set: 0
            global_disk_space: parseInt(@get_used_disk_space())

        for namespace in namespaces.models
            _s = namespace.get_stats()
            if not _s?
                continue

            if namespace.get('id') of stats_full
                __s.keys_read += _s.keys_read
                __s.keys_set += _s.keys_set
        # CPU, mem, disk
        return __s

    is_reachable: =>
        reachable = directory.get(@get('id'))?
        return reachable

    # Returns a filtered list of namespaces whose shards use this server as master
    is_master_for_namespaces: =>
        namespaces.filter (namespace) =>
            for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                if machine_uuid is @get('id')
                    for shard, role of peer_roles
                        if role is 'role_primary'
                            return true

    # Return an object with warnings if a server cannot be moved
    get_data_for_moving: =>
        data = {}
        dc_uuid = @get('datacenter_uuid')
        # If the server is in Universe, we can move it since it's always safe
        if dc_uuid isnt universe_datacenter.get('id')
            # Count the number of servers in the same datacenter as this server
            num_machines_in_dc = 0
            machines.each (machine) => num_machines_in_dc++ if dc_uuid is machine.get('datacenter_uuid')

            # Looking for critical issues (long term loss of availability)
            # (look for namespaces where the datacenter is primary and with just one machine)
            namespaces_with_critical_issue = []
            # Find the tables that won't have sufficient replicas if we unassign this server
            namespaces_with_unsatisfiable_goals = []
            for namespace in namespaces.models
                # Does the datacenter of this server have responsibilities for the namespace?
                if dc_uuid of namespace.get('replica_affinities')
                    num_replicas = namespace.get('replica_affinities')[dc_uuid]
                    # If the datacenter acts as primary for the namespace, bump the replica count by one
                    num_replicas++ if namespace.get('primary_uuid') is dc_uuid

                # There will be a unsatisfiable goals if we unassign the machine
                # We take for granted that acks < num_replicas. We might want to increase the safety.
                if num_machines_in_dc <= num_replicas
                    namespaces_with_unsatisfiable_goals.push
                        id: namespace.get('id')
                        name: namespace.get('name')

                # The last machine in the datacenter is primary so no new master can be elected if the master is moved
                if num_machines_in_dc is 1 and dc_uuid is namespace.get('primary_uuid')
                    namespaces_with_critical_issue.push
                        id: namespace.get('id')
                        name: namespace.get('name')


                # That's all, if the machine that's going to replace universe is already working for universe,
                # it will start working for the datacenter and the unassigned one will start working for universe
            if namespaces_with_unsatisfiable_goals.length > 0
                data = _.extend data,
                    has_warning: true
                    namespaces_with_unsatisfiable_goals: namespaces_with_unsatisfiable_goals
                    num_namespaces_with_unsatisfiable_goals: namespaces_with_unsatisfiable_goals.length
            if namespaces_with_critical_issue.length > 0
                data = _.extend data,
                    namespaces_with_critical_issue: namespaces_with_critical_issue
                    num_namespaces_with_critical_issue: namespaces_with_critical_issue.length

            if namespaces_with_unsatisfiable_goals.length > 0 or namespaces_with_critical_issue.length > 0
                data = _.extend data,
                    has_warning: true
                    datacenter_id: @get('datacenter_uuid')
                    datacenter_name: datacenters.get(@get('datacenter_uuid')).get('name')
                    machine_id: @get('id')
                    machine_name: @get('name')
        return data

class LogEntry extends Backbone.Model
    get_iso_8601_timestamp: => new Date(@.get('timestamp') * 1000)
    get_formatted_message: =>
        msg = @.get('message')

class Issue extends Backbone.Model

class IssueRedundancy extends Backbone.Model

class Progress extends Backbone.Model

# this is a hook into the directory
class MachineAttributes extends Backbone.Model

class ConnectionStatus extends Backbone.Model

# This is a computed model for the cluster (mainly for stats right
# now)
class ComputedCluster extends Backbone.Model
    initialize: ->
        log_initial '(initializing) computed cluster model'
        super

    get_stats: =>
        # Ops/sec stats
        __s =
            keys_read: 0
            keys_set: 0
            global_disk_space: 0
        for namespace in namespaces.models
            _s = namespace.get_stats()
            if not _s?
                continue
            __s.keys_read += _s.keys_read
            __s.keys_set += _s.keys_set
        # CPU, mem, disk
        for m in machines.models
            __s.global_disk_space += m.get_used_disk_space()

        return __s




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

class Issues extends Backbone.Collection
    model: Issue
    url: 'ajax/issues'

# We compare the directory and the blueprints to detect redundancy problems
class IssuesRedundancy extends Backbone.Collection
    model: IssueRedundancy
    num_replicas : 0
    initialize: ->
        directory.on 'all', @compute_redundancy_errors
        namespaces.on 'all', @compute_redundancy_errors



    convert_activity: (role) ->
        switch role
            when 'role_secondary' then return 'secondary_up_to_date'
            when 'role_nothing' then return 'nothing'
            when 'role_primary' then return 'primary'

    compute_redundancy_errors: =>
        issues_redundancy_new = []

        num_replicas = 0


        directory_by_namespaces = DataUtils.get_directory_activities_by_namespaces()
        for namespace in namespaces.models
            namespace_id = namespace.get('id')
            blueprint = namespace.get('blueprint').peers_roles
            for machine_id of blueprint
                if machines.get(machine_id)? and machines.get(machine_id).get('name')?
                    machine_name = machines.get(machine_id).get('name')
                else # can happen if machines is not loaded yet
                    machine_name = machine_id

                for key of blueprint[machine_id]
                    value = blueprint[machine_id][key]
                    if value is "role_primary" or value is "role_secondary"
                        num_replicas++

                        if !(directory_by_namespaces?) or !(directory_by_namespaces[namespace_id]?) or !(directory_by_namespaces[namespace_id][machine_id]?)
                            issue_redundancy_param =
                                machine_id: machine_id
                                machine_name: machine_name
                                namespace_uuid: namespace_id
                                namespace_name: namespace.get('name')
                                shard: key
                                blueprint: value
                                directory: 'not_found'
                            issue_redundancy = new IssueRedundancy issue_redundancy_param
                            issues_redundancy_new.push issue_redundancy
                        else if directory_by_namespaces[namespace_id][machine_id][key] != @convert_activity(value)
                            issue_redundancy_param =
                                machine_id: machine_id
                                machine_name: machine_name
                                namespace_uuid: namespace_id
                                namespace_name: namespace.get('name')
                                shard: key
                                blueprint: value
                                directory: directory_by_namespaces[namespace_id][machine_id][key]
                            issues_redundancy_new.push new IssueRedundancy issue_redundancy_param


        if issues_redundancy_new.length > 0 or issues_redundancy_new.length isnt @.length
            @.reset(issues_redundancy_new)

        if num_replicas isnt @num_replicas
            @num_replicas = num_replicas
            @.trigger 'reset'

class ProgressList extends Backbone.Collection
    model: Progress
    url: 'ajax/progress'

# hook into directory
class Directory extends Backbone.Collection
    model: MachineAttributes
    url: 'ajax/directory'

# This module contains utility functions that compute and massage
# commonly used data.
module 'DataUtils', ->
    # The equivalent of a database view, but for our Backbone models.
    # Our models and collections have direct representations on the server. For
    # convenience, it's useful to pick data from several of these models: these
    # are computed models (and computed collections).

    # Computed shard for a particular namespace: computed for convenience and for extra metadata
    # Arguments:
    #   shard: element of the namespace shards list: Namespace.get('shards')
    #   namespace: namespace that this shard belongs to
    # Computed attributes of a shard:
    #   shard_boundaries: JSON string representing the shard boundaries
    #   primary_uuid: machine uuid that is the master for this shard
    #   secondary_uuids: list of machine_uuids that are the secondaries for this shard
    class @Shard extends Backbone.Model
        get_primary_uuid: => DataUtils.get_shard_primary_uuid @namespace.get('id'), @shard
        get_secondary_uuids: => DataUtils.get_shard_secondary_uuids @namespace.get('id'), @shard

        initialize: (shard, namespace) =>
            @shard = shard
            @namespace = namespace

            @.set 'shard_boundaries', @shard
            @.set 'primary_uuid', @get_primary_uuid()
            @.set 'secondary_uuids',  @get_secondary_uuids()

            namespace.on 'change:blueprint', @set_uuids

        set_uuids: =>
            new_primary_uuid = @get_primary_uuid()
            new_secondary_uuids = @get_secondary_uuids()
            @.set 'primary_uuid', new_primary_uuid if @.get('primary_uuid') isnt new_primary_uuid
            @.set 'secondary_uuids', new_secondary_uuids if not _.isEqual @.get('secondary_uuids'), new_secondary_uuids

        destroy: =>
            namespace.on 'change:blueprint', @set_uuids

    # The Shards collection maintains the computed shards for a given namespace.
    # It will observe for any changes in the sharding scheme (or if the
    # namespace has just been added) and maintains the list of computed shards,
    # rebuilding if necessary.
    class @Shards extends Backbone.Collection
        model: DataUtils.Shard

        initialize: (models, namespace) =>
            @namespace = namespace
            @namespace.on 'change:shards', @compute_shards_without_args
            @namespace.on 'add', @compute_shards_without_args

        compute_shards_without_args: =>
            @compute_shards @namespace.get('shards')

        compute_shards: (shards) =>
            new_shards = []
            for shard in shards
                new_shards.push new DataUtils.Shard shard, @namespace
            @.reset new_shards

        destroy: =>
            @namespace.off 'change:shards', @compute_shards_without_args
            @namespace.off 'add', @compute_shards_without_args


    @stripslashes = (str) ->
        str=str.replace(/\\'/g,'\'')
        str=str.replace(/\\"/g,'"')
        str=str.replace(/\\0/g,"\x00")
        str=str.replace(/\\\\/g,'\\')
        return str

    @get_machine_reachability = (machine_uuid) ->
        reachable = directory.get(machine_uuid)?
        if not reachable
            _m = machines.get(machine_uuid)
            if _m?
                last_seen = _m.get('last_seen')
            else
                last_seen = null
            if last_seen
                last_seen = $.timeago(new Date(parseInt(last_seen) * 1000))
        json =
            reachable: reachable
            last_seen: last_seen
        return json

    @get_datacenter_reachability = (datacenter_uuid) ->
        total = (_.filter machines.models, (m) => m.get('datacenter_uuid') is datacenter_uuid).length
        reachable = (_.filter directory.models, (m) =>
            _m = machines.get(m.get('id'))
            if _m?
                _m.get('datacenter_uuid') is datacenter_uuid
            else
                return false
            ).length

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
            if !machines.get(machine_uuid)? # In case the machine is dead
                continue

            datacenter_uuid = machines.get(machine_uuid).get('datacenter_uuid')
            for _shard, role of peers_roles
                if shard.toString() is _shard.toString() and role is 'role_secondary'
                    if not secondaries[datacenter_uuid]?
                        secondaries[datacenter_uuid] = []
                    secondaries[datacenter_uuid][secondaries[datacenter_uuid].length] = machine_uuid
        return secondaries

    @get_ack_expectations = (namespace_uuid, datacenter_uuid) ->
        namespace = namespaces.get(namespace_uuid)
        acks = namespace?.get('ack_expectations')?[datacenter_uuid]?.expectation
        if acks?
            return acks
        else
            return 0

    @get_replica_affinities = (namespace_uuid, datacenter_uuid) ->
        namespace = namespaces.get(namespace_uuid)
        if datacenter_uuid is universe_datacenter.get('id')
            datacenter = universe_datacenter
        else
            datacenter = datacenters.get(datacenter_uuid)
        affs = namespace?.get('replica_affinities')?[datacenter?.get('id')]
        if affs?
            return affs
        else
            return 0

    @get_datacenter_machines = (datacenter_uuid) ->
        return _.filter(machines.models, (m) -> m.get('datacenter_uuid') is datacenter_uuid)

    # Return a list of machines relevant to a namespace
    @get_namespace_machines = (namespace_uuid) ->
        mids = []
        _n = namespaces.get(namespace_uuid)
        if not _n
            return []
        _n = _n.get('blueprint').peers_roles
        for mid, roles of _n
            mids.push(mid)
        return mids


    # Cache the value of the refactored directory_activities
    @directory_activities = null
    @directory_activities_by_namespaces = null
    @reset_directory_activities = ->
        @directory_activities = null
        @directory_activities_by_namespaces = null

    # Organizes the directory as a map of activity ids
    @get_directory_activities = ->
        if not @directory_activities?
            @directory_activities = {}
            for machine in directory.models
                bcards = machine.get('rdb_namespaces')['reactor_bcards']
                for namespace_id, activity_map of bcards
                    activity_map = activity_map['activity_map']
                    for activity_id, activity of activity_map
                        @directory_activities[activity_id] =
                            value: activity
                            machine_id: machine.get('id')
                            namespace_id: namespace_id
        return @directory_activities
    

    @get_directory_activities_by_namespaces = ->
        #TODO We report the status of the "last" internal hash shard as being the status of the whole shard
        #We should eventually report the status of all internal shards
        if not @directory_activities_by_namespaces?
            @directory_activities_by_namespaces = {}
            for machine in directory.models
                bcards = machine.get('rdb_namespaces')['reactor_bcards']
                for namespace_id, activity_map of bcards
                    if not @directory_activities_by_namespaces[namespace_id]?
                        @directory_activities_by_namespaces[namespace_id] = {}

                    activity_map = activity_map['activity_map']
                    for activity_id, activity of activity_map
                        if not @directory_activities_by_namespaces[namespace_id][machine.get('id')]?
                            @directory_activities_by_namespaces[namespace_id][machine.get('id')] = {}
                        @directory_activities_by_namespaces[namespace_id][machine.get('id')][activity[0]] = activity[1]['type']

        return @directory_activities_by_namespaces

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
                    machine_id:           sender.machine_id #TODO can fail
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

    # datacenter_uuid is optional and limits the information to a specific datacenter
    # If datacenter is not defined, we get the status for all datacenters
    # TODO We should clean this function. We don't need so much data most of the time
    @get_namespace_status = (namespace_uuid, datacenter_uuid) ->
        namespace = namespaces.get(namespace_uuid)
        json =
            nshards: 0
            nreplicas: 0
            nashards: 0 # Number of available shards
            nareplicas: 0 # Number of available replicas
            reachability: 'Live' # 'Live' if we can write on the table, 'Down' if we can't

        # If we can't see the namespace...
        if not namespace?
            return null

        # machine and datacenter counts
        _machines = []
        _datacenters = []

        for machine_uuid, role of namespace.get('blueprint').peers_roles
            if !machines.get(machine_uuid)? # If the machine is dead
                continue
            # We filter machines. If 
            if datacenter_uuid? and machines.get(machine_uuid)?.get('datacenter_uuid') isnt datacenter_uuid
                continue
            peer_accessible = directory.get(machine_uuid)
            machine_active_for_namespace = false
            for shard, role_name of role
                if role_name is 'role_primary'
                    machine_active_for_namespace = true
                    json.nshards += 1
                    json.nreplicas += 1
                    if peer_accessible?
                        json.nashards += 1
                        json.nareplicas += 1
                if role_name is 'role_secondary'
                    machine_active_for_namespace = true
                    json.nreplicas += 1
                    if peer_accessible?
                        json.nareplicas += 1
            if machine_active_for_namespace
                _machines.push machine_uuid
                if not datacenter_uuid? or datacenter_uuid isnt universe_datacenter.get('id') # If datacenter_uuid is defined, we don't want to count universe
                    _datacenters.push machines.get(machine_uuid).get('datacenter_uuid')

        json.nmachines = _.uniq(_machines).length
        json.ndatacenters = _.uniq(_datacenters).length

        # Getting the directoy per namespace
        directory_by_namespaces = DataUtils.get_directory_activities_by_namespaces()

        # We now want to know if the table is available or not
        # The table is available only if these two conditions are met:
        # - A write can succeed, which means that for each shard, # the number of available
        #   replicas in each datacenter should be at least the number of acks required in this datacenter
        # - The master for this shard is available

        # This is an object that will store the number of acks for each shard and datacenter
        # Ex: _shard_required = {
        #   ['', 'foo']: {
        #       universe_id: 0,
        #       datacenter_1: 3,
        #       datacenter_2: 1
        #   }
        #   ['foo', null]: {
        #       universe_id: 0,
        #       datacenter_1: 3,
        #       datacenter_2: 1
        #  }
        # }
        _shard_required = {}

        # An object that will store for each shard a boolean that represents whether its master is available or not
        # Ex: _shard_has_master = {
        #   ['', 'foo']: Boolean
        #   ['foo', null]: Boolean
        # }
        _shard_has_master = {}


        # Initialize all the two previous variable
        for shard in namespace.get('shards')
            _shard_required[shard] = {} # We will add the requirement per datacenter in the next loop
            _shard_has_master[shard] = false # We haven't seen the master yet, so we consider it not available

        # For each shard, we save the number of write required (per datacenter) to have a write acknowledge (in this datacenter)
        for datacenter, value of namespace.get('ack_expectations')
            for shard of _shard_required
                _shard_required[shard][datacenter] = value.expectation

        # Checking blueprint (goals) vs directory (real state of the cluster)
        # For each shard assignments, we are going to
        #     - Make sure there is a master
        #     - Decrement the number of remaining ack we need to be available (per shard)
        blueprint = namespace.get('blueprint').peers_roles
        for machine_id of blueprint
            if json.reachability isnt 'Live'
                break

            machine = machines.get(machine_id)
            if not machine?
                continue

            datacenter_id = machine.get('datacenter_uuid')
            machine_name = machine.get('name')

            for shard, role of blueprint[machine_id]
                if json.reachability isnt 'Live'
                    break

                if role is "role_primary" # This machine is the master for the current shard
                    if directory_by_namespaces?[namespace.get('id')]?[machine_id]?[shard] is "primary"
                        _shard_has_master[shard] = true
                        
                        if not _shard_required[shard]?[datacenter_id]?
                            _shard_required[shard]?[datacenter_id] = 0
                        _shard_required[shard]?[datacenter_id] -= 1
                    else
                        json.reachability = 'Down'
                else if role is "role_secondary" # This machine is a secondary for the current shard
                    if directory_by_namespaces?[namespace.get('id')]?[machine_id]?[shard] is "secondary_up_to_date"
                        # The shard can be defined in shards but not in the blueprint if there were not yet reprinted
                        if not _shard_required[shard]?[datacenter_id]?
                            _shard_required[shard]?[datacenter_id] = 0
                        _shard_required[shard]?[datacenter_id] -= 1

        # By default we consider a machine to hold a replica because of a per datacenter requirement
        # If a datacenter has more responsabilities than required, it's because some of these machines
        # are working for the whole cluster (universe), so we move them to universe
        for shard of _shard_required
            for datacenter of _shard_required[shard]
                if _shard_required[shard][datacenter] < 0
                    _shard_required[shard][universe_datacenter.get('id')] += _shard_required[shard][datacenter]
                    _shard_required[shard][datacenter] = 0


        # Make sure that all shards have a master
        if json.reachability = 'Live'
            for shard of _shard_has_master
                if _shard_has_master[shard] is false
                    json.reachability = 'Down'
                    break

        # Make sure that all shards have enough replica to satisfy a write
        if json.reachability = 'Live'
            for shard of _shard_required
                for datacenter of _shard_required[shard]
                    if _shard_required[shard][datacenter] > 0
                        json.reachability = 'Down'
                    break

        return json


    @is_integer = (data) ->
        return data.search(/^\d+$/) isnt -1

    # Deep copy. We do not copy prototype.
    @deep_copy = (data) ->
        if typeof data is 'boolean' or typeof data is 'number' or typeof data is 'string' or typeof data is 'number' or data is null or data is undefined
            return data
        else if typeof data is 'object' and Object.prototype.toString.call(data) is '[object Array]'
            result = []
            for value in data
                result.push @deep_copy value
            return result
        else if typeof data is 'object'
            result = {}
            for key, value of data
                result[key] = @deep_copy value
            return result

    @approximate_count = (num) ->
        # 0 => 0
        # 1 - 5 => 5
        # 5 - 10 => 10
        # 11 - 99 => Rounded to _0
        # 100 - 999 => Rounded to _00
        # 1,000 - 9,999 => _._K
        # 10,000 - 10,000 => __K
        # 100,000 - 1,000,000 => __0K
        # Millions and billions have the same behavior as thousands
        # If num>1000B, then we just print the number of billions
        if num is 0
            return '0'
        else if num <= 5
            return '5'
        else if num <= 10
            return '10'
        else
            # Approximation to 2 significant digit
            approx = Math.round(num/Math.pow(10, num.toString().length-2))*Math.pow(10, num.toString().length-2)
            if approx < 100 # We just want one digit
                return (Math.floor(approx/10)*10).toString()
            else if approx < 1000 # We just want one digit
                return (Math.floor(approx/100)*100).toString()
            else if approx < 1000000
                result = (approx/1000).toString()
                if result.length is 1 # In case we have 4 for 4000, we want 4.0
                    result = result + '.0'
                return result+'K'
            else if approx < 1000000000
                result = (approx/1000000).toString()
                if result.length is 1 # In case we have 4 for 4000, we want 4.0
                    result = result + '.0'
                return result+'M'
            else
                result = (approx/1000000000).toString()
                if result.length is 1 # In case we have 4 for 4000, we want 4.0
                    result = result + '.0'
                return result+'B'

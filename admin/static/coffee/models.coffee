# Models for Backbone.js
class Namespace extends Backbone.Model
    urlRoot: '/ajax/namespaces'
    initialize: ->
        log_initial '(initializing) namespace model'
        super
        @load_key_distr()

    # Cache key distribution info.
    load_key_distr: =>
        $.ajax
            processData: false
            url: "/ajax/distribution?namespace=#{@get('id')}&depth=2"
            type: 'GET'
            success: (distr_data) =>
                # Cache the data
                @set('key_distr', distr_data)
                # Sort the keys and cache that too
                distr_keys = []
                for key, count of distr_data
                    distr_keys.push(key)
                _.sortBy(distr_keys, _.identity)
                @set('key_distr_sorted', distr_keys)
                # TODO: magic number
                window.setTimeout @load_key_distr, 5000
    sorted_key_distr_keys: =>
        keys = @get('key_distr_sorted')
        if keys?
            return keys
        else
            return []

    # Some shard helpers
    compute_shard_rows_approximation: (shard) =>
        # first see if we can grab the distr data
        shard = $.parseJSON(shard)
        distr_data = @get('key_distr')
        if not distr_data?
            return null
        # some basic initialization
        start_key = shard[0]
        end_key = shard[1]
        distr_keys = @sorted_key_distr_keys()
        # TODO: we should probably support interpolation here, but
        # fuck it for now.

        # find the first key greater than the beginning of our shard
        # and keep summing until we get past our shard boundary.
        count = 0
        for key in distr_keys
            if key >= start_key
                count += distr_data[key]
            if end_key? and key >= end_key
                break

        return count

    # Is x between the lower and upper splitpoints (the open interval) for the given index?
    splitpoint_between: (shard_index, sp) =>
        console.log "splitpoint_between #{shard_index}, #{sp}"
        all_sps = @.get('splitpoints')
        return (shard_index == 0 || all_sps[shard_index - 1] < sp) && (shard_index == all_sps.length || sp < all_sps[shard_index])

    # Computing btree stats based on machine stats
    get_stats: =>
        __s =
            keys_read: 0
            keys_set: 0
        _.each DataUtils.get_namespace_machines(@get('id')), (mid) =>
            _m = machines.get(mid)
            _s = _m.get_stats()[@get('id')]
            if _s? and _s.btree?
                keys_read = parseFloat(_s.btree.keys_read)
                if not isNaN(keys_read)
                    __s.keys_read += keys_read
                keys_set = parseFloat(_s.btree.keys_set)
                if not isNaN(keys_set)
                    __s.keys_set += keys_set
        return __s

class Datacenter extends Backbone.Model
    get_stats: =>
        # Look boo, here's what we gonna do, okay? We gonna collect
        # stats we care about from the machines that belong to this
        # datacenter and aggregate them, okay? I don't care that
        # you're unhappy, I don't care that you don't like this, and I
        # don't care about the irritation on your bikini line. This is
        # how we're doing it, no ifs no ends no buts. The LT says we
        # go, we go.
        stats =
            global_cpu_util_avg: 0
            global_mem_total: 0
            global_mem_used: 0
            dc_disk_space: 0
        nmachines = 0
        for machine in machines.models
            if machine.get('datacenter_uuid') is @get('id')
                mstats = machine.get_stats().proc
                if mstats?
                    nmachines += 1
                    stats.global_cpu_util_avg += parseFloat(mstats.global_cpu_util_avg)
                    stats.global_mem_total += parseFloat(mstats.global_mem_total)
                    stats.global_mem_used += parseFloat(mstats.global_mem_used)
                    stats.dc_disk_space += machine.get_used_disk_space()
        stats.global_cpu_util_avg /= nmachines
        return stats

    get_stats_for_performance: =>
        # Ops/sec stats
        __s =
            keys_read: 0
            keys_set: 0
            global_cpu_util_avg: 0
            global_mem_total: 0
            global_mem_used: 0
            global_disk_space: 0
            global_net_recv_persec_avg: 0
            global_net_sent_persec_avg: 0
        for namespace in namespaces.models
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
                mstats = machine.get_stats().proc
                __s.global_cpu_util_avg += parseFloat(mstats.global_cpu_util_avg)
                __s.global_mem_total += parseInt(mstats.global_mem_total)
                __s.global_mem_used += parseInt(mstats.global_mem_used)
                __s.global_disk_space += machine.get_used_disk_space()
                __s.global_net_recv_persec_avg += parseFloat(mstats.global_net_recv_persec_avg)
                __s.global_net_sent_persec_avg += parseFloat(mstats.global_net_sent_persec_avg)
        __s.global_cpu_util_avg /= num_machines_in_datacenter
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
        mstats = stats_full.proc
        __s =
            keys_read: 0
            keys_set: 0
            global_cpu_util_avg: parseFloat(mstats.global_cpu_util_avg)
            global_mem_total: parseInt(mstats.global_mem_total)
            global_mem_used: parseInt(mstats.global_mem_used)
            global_disk_space: parseInt(@get_used_disk_space())
            global_net_recv_persec_avg: parseFloat(mstats.global_net_recv_persec_avg)
            global_net_sent_persec_avg: parseFloat(mstats.global_net_sent_persec_avg)
        
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

class LogEntry extends Backbone.Model
    get_iso_8601_timestamp: => ISODateString new Date(@.get('timestamp') * 1000)
    get_formatted_message: =>
        msg = @.get('message')
        return '' if not msg?

        index = msg.indexOf('{')
        return {formatted_message: msg} if index is -1

        str_fragment = msg.slice(0,index)
        json_fragment = $.parseJSON msg.slice(msg.indexOf('{'))

        return {formatted_message: msg} if not json_fragment?

        return {
            formatted_message: str_fragment
            json: JSON.stringify(json_fragment, undefined, 2)
        }


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
            global_cpu_util_avg: 0
            global_mem_total: 0
            global_mem_used: 0
            global_disk_space: 0
            global_net_recv_persec_avg: 0
            global_net_sent_persec_avg: 0
        for namespace in namespaces.models
            _s = namespace.get_stats()
            if not _s?
                continue
            __s.keys_read += _s.keys_read
            __s.keys_set += _s.keys_set
        # CPU, mem, disk
        for m in machines.models
            mstats = m.get_stats().proc
            __s.global_cpu_util_avg += parseFloat(mstats.global_cpu_util_avg)
            __s.global_mem_total += parseInt(mstats.global_mem_total)
            __s.global_mem_used += parseInt(mstats.global_mem_used)
            __s.global_disk_space += m.get_used_disk_space()
            __s.global_net_recv_persec_avg += parseFloat(mstats.global_net_recv_persec_avg)
            __s.global_net_sent_persec_avg += parseFloat(mstats.global_net_sent_persec_avg)
        __s.global_cpu_util_avg /= machines.models.length

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

class LogEntries extends Backbone.Collection
    model: LogEntry
    comparator: (a, b) ->
        if a.get('timestamp') < b.get('timestamp')
            return 1
        else if a.get('timestamp') > b.get('timestamp')
            return -1
        else if a.get('machine_uuid') <  b.get('machine_uuid')
            return 1
        else if a.get('machine_uuid') > b.get('machine_uuid')
            return -1
        else if a.get('message') < b.get('message')
            return 1
        else if a.get('message') > b.get('message')
            return -1
        else
            return 0
        # sort strings in reverse order (return a negated string)
        #String.fromCharCode.apply String,
        #    _.map(log_entry.get('datetime').split(''), (c) -> 0xffff - c.charCodeAt())

class Issues extends Backbone.Collection
    model: Issue
    url: '/ajax/issues'

# We compare the directory and the blueprints to detect redundancy problems
class IssuesRedundancy extends Backbone.Collection
    model: IssueRedundancy
    num_replicas : 0
    initialize: ->
        directory.on 'all', @compute_redundancy_errors
        namespaces.on 'all', @compute_redundancy_errors



    convert_activity:
        'role_secondary': 'secondary_up_to_date'
        'role_nothing': 'nothing'
        'role_primary': 'primary'

    compute_redundancy_errors: =>
        issues_redundancy = []
        @num_replicas = 0


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
                        @num_replicas++

                        if !(directory_by_namespaces?) or !(directory_by_namespaces[namespace_id]?) or !(directory_by_namespaces[namespace_id][machine_id]?)
                            issue_redundancy_param =
                                machine_id: machine_id
                                machine_name: machine_name
                                namespace_uuid: namespace_id
                                namespace_name: namespace.get('name')
                            issue_redundancy = new IssueRedundancy issue_redundancy_param
                            issues_redundancy.push issue_redundancy
                        else if directory_by_namespaces[namespace_id][machine_id][0] != key
                            issue_redundancy_param =
                                machine_id: machine_id
                                machine_name: machine_name
                                namespace_uuid: namespace_id
                                namespace_name: namespace.get('name')
                            issues_redundancy.push new IssueRedundancy issue_redundancy_param
                        else if directory_by_namespaces[namespace_id][machine_id][1].type != @convert_activity[value]
                            issue_redundancy_param =
                                machine_id: machine_id
                                machine_name: machine_name
                                namespace_uuid: namespace_id
                                namespace_name: namespace.get('name')
                            issues_redundancy.push new IssueRedundancy issue_redundancy_param
 
        if issues_redundancy.length > 0 or issues_redundancy.length isnt @.length
            @.reset(issues_redundancy)
        
        

class ProgressList extends Backbone.Collection
    model: Progress
    url: '/ajax/progress'

# hook into directory
class Directory extends Backbone.Collection
    model: MachineAttributes
    url: '/ajax/directory'

# This module contains utility functions that compute and massage
# commonly used data.
module 'DataUtils', ->
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
        _n = namespaces.get(namespace_uuid)
        if not _n
            return []
        _n = _n.get('blueprint').peers_roles
        for mid, roles of _n
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

    @get_directory_activities_by_namespaces = ->
        activities = {}
        for machine in directory.models
            bcards = machine.get('memcached_namespaces')['reactor_bcards']
            for namespace_id, activity_map of bcards
                activity_map = activity_map['activity_map']
                for activity_id, activity of activity_map
                    if !(namespace_id of activities)
                        activities[namespace_id] = {}
                    activities[namespace_id][machine.get('id')] = activity

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

        # If we can't see the namespace...
        if not namespace?
            return null

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


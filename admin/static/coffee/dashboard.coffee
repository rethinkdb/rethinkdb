# Dashboard: provides an overview and visualizations of the cluster
# Dashboard View
module 'DashboardView', ->
    # Cluster.Container
    class @Container extends Backbone.View
        template: Handlebars.compile $('#dashboard_view-template').html()
        id: 'dashboard_container'

        initialize: =>
            log_initial '(initializing) dashboard container view'

            @cluster_status_availability = new DashboardView.ClusterStatusAvailability()
            @cluster_status_redundancy = new DashboardView.ClusterStatusRedundancy()
            @cluster_status_reachability = new DashboardView.ClusterStatusReachability()
            @cluster_status_consistency = new DashboardView.ClusterStatusConsistency()

            @cluster_performance = new Vis.OpsPlot(computed_cluster.get_stats,
                width:  833             # width in pixels
                height: 300             # height in pixels
                seconds: 119            # num seconds to track
                type: 'cluster'
            )

            @logs = new DashboardView.Logs()

        render: =>
            @.$el.html @template({})
            @.$('.availability').html @cluster_status_availability.render().$el
            @.$('.redundancy').html @cluster_status_redundancy.render().$el
            @.$('.reachability').html @cluster_status_reachability.render().$el
            @.$('.consistency').html @cluster_status_consistency.render().$el

            @.$('#cluster_performance_container').html @cluster_performance.render().$el
            @.$('.recent-log-entries-container').html @logs.render().$el

            return @

        destroy: =>
            @cluster_status_availability.destroy()
            @cluster_status_redundancy.destroy()
            @cluster_status_reachability.destroy()
            @cluster_status_consistency.destroy()

            @cluster_performance.destroy()

    # TODO: dropped RAM and disk distribution in the templates, need to remove support for them here as well
    class @ClusterStatusAvailability extends Backbone.View
        className: 'cluster-status-availability'

        template: Handlebars.compile $('#cluster_status-availability-template').html()
        popup_template: Handlebars.compile $('#cluster_status-availability-popup-template').html()

        events:
            'mousedown .show_details': 'show_details'

        initialize: =>
            @data = ''
            directory.on 'all', @render
            namespaces.on 'all', @render

        convert_activity: (role) ->
            switch role
                when 'role_secondary' then return 'secondary_up_to_date'
                when 'role_nothing' then return 'nothing'
                when 'role_primary' then return 'primary'


        compute_data: =>
            num_masters = 0
            num_masters_down = 0
            namespaces_down = {}

            directory_by_namespaces = DataUtils.get_directory_activities_by_namespaces()
            for namespace in namespaces.models
                namespace_id = namespace.get('id')
                blueprint = namespace.get('blueprint').peers_roles
                for machine_id of blueprint
                    machine_name = machine_name = machines.get(machine_id)?.get('name') #TODO check later if defined
                    if not machine_name?
                        machine_name = machine_id

                    for shard of blueprint[machine_id]
                        value = blueprint[machine_id][shard]
                        if value is "role_primary"
                            num_masters++

                            if !(directory_by_namespaces?) or !(directory_by_namespaces[namespace_id]?) or !(directory_by_namespaces[namespace_id][machine_id]?)
                                num_masters_down++
                                if not namespaces_down[namespace.get('id')]?
                                    namespaces_down[namespace.get('id')] = []

                                namespaces_down[namespace.get('id')].push
                                    shard: shard
                                    namespace_id: namespace.get('id')
                                    namespace_name: namespace.get('name')
                                    machine_id: machine_id
                                    machine_name: machine_name
                                    blueprint_status: value
                                    directory_status: 'Not found'
                            else if directory_by_namespaces[namespace_id][machine_id][shard] != @convert_activity(value)
                                num_masters_down++
                                if not namespaces_down[namespace.get('id')]?
                                    namespaces_down[namespace.get('id')] = []

                                namespaces_down[namespace.get('id')].push
                                    shard: shard
                                    namespace_id: namespace.get('id')
                                    namespace_name: namespace.get('name')
                                    machine_id: machine_id
                                    machine_name: machine_name
                                    blueprint_status: value
                                    directory_status: directory_by_namespaces[namespace_id][machine_id][shard]

            if num_masters_down > 0
                namespaces_down_array = []
                for namespace_id, namespace_down of namespaces_down
                    namespaces_down_array.push namespace_down
                data =
                    status_is_ok: false
                    num_namespaces_down: namespaces_down_array.length
                    num_namespaces: namespaces.length
                    num_masters: num_masters
                    num_masters_down: num_masters_down
                    namespaces_down: namespaces_down_array
            else
                data =
                    status_is_ok: true
                    num_masters: num_masters

            return data

        render: =>
            data = @compute_data()
            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @data = data_in_json
                @.$el.html @template data
                if data.status_is_ok is true
                    @.$el.addClass 'no-problems-detected'
                else
                    @.$el.addClass 'problems-detected'

                if data.status_is_ok is false
                    @.$('.popup_container').html @popup_template data

            return @

        show_details: (event) =>
            event.preventDefault()
            @.$('.popup_container').css 'display', 'block'
            @.$('.popup_container').css 'margin', event.pageY+'px 0px 0px '+event.pageX+'px'


            @.$('.popup_container').on 'click', @stop_propagation
            @.$(event.target).on 'click', @stop_propagation
            $(window).on 'click', @hide_details

        stop_propagation: (event) ->
            event.stopPropagation()

        hide_details: (event) =>
            @.$('.popup_container').css 'display', 'none'

        destroy: =>
            directory.off 'all', @render
            namespaces.off 'all', @render


    class @ClusterStatusRedundancy extends Backbone.View
        template: Handlebars.compile $('#cluster_status-redundancy-template').html()
        className: 'cluster-status-availability'

        render: =>
            @.$el.html @template()


    class @ClusterStatusReachability extends Backbone.View
        template: Handlebars.compile $('#cluster_status-reachability-template').html()
        className: 'cluster-status-reachability'

        render: =>
            @.$el.html @template()


    class @ClusterStatusConsistency extends Backbone.View
        template: Handlebars.compile $('#cluster_status-consistency-template').html()
        className: 'cluster-status-consistency'

        render: =>
            @.$el.html @template()

    ###
        compute_status: ->
            status =
                has_availability_problems: false
                has_disk_problems: false
                has_conflicts_problems : false
                has_redundancy_problems: false
                has_ram_problems: false
                has_persistence_problems: false
                num_machines: machines.length
                num_namespaces: namespaces.length
                num_masters: 0

            masters = {} # find all the masters
            namespaces.each (namespace) ->
                for machine_uuid, role_summary of namespace.get('blueprint').peers_roles
                    for shard, role of role_summary
                        if role is 'role_primary'
                            masters[machine_uuid] =
                                name: namespace.get('name')
                                id: namespace.get('id')
                            status.num_masters++


            if issues.length != 0
                status.num_machines_with_disk_problems = 0
                status.machines_with_disk_problems = []

                status.num_masters_offline = 0
                status.masters_offline = []

                status.num_conflicts_name = 0
                status.num_conflicts_vector = 0

                for issue in issues.models
                    switch issue.get("type")
                        when "LOGFILE_WRITE_ERROR"
                            new_machine =
                                uid: issue.get('location')
                                name: machines.get(issue.get('location')).get('name')
                            status.machines_with_disk_problems.push(new_machine)
                        when "MACHINE_DOWN"
                            if masters[issue.get('victim')]?
                                status.masters_offline.push
                                    machine_id: issue.get('victim')
                                    machine_name: machines.get(issue.get('victim')).get('name')
                                    namespace_name: masters[issue.get('victim')].name
                                    namespace_id: masters[issue.get('victim')].id
                        when "NAME_CONFLICT_ISSUE"
                            status.num_conflicts_name++
                        when "VCLOCK_CONFLICT"
                            status.num_conflicts_vector++

                if status.machines_with_disk_problems.length > 0
                    status.num_machines_with_disk_problems = status.machines_with_disk_problems.length
                    status.has_persistence_problems = true
                if status.masters_offline.length > 0
                    status.num_masters_offline = status.masters_offline.length
                    status.has_availability_problems = true
                    status.has_masters_down = true
                if status.num_conflicts_name or status.num_conflicts_vector > 0
                    status.num_conflicts = status.num_conflicts_name+status.num_conflicts_vector
                    status.has_conflicts_problems = true


            # Check if a namespace doesn't have a master
            namespaces_without_masters = []
            for namespace in namespaces.models
                peers = namespace.get('blueprint').peers_roles
                if peers?
                    keys = {}
                    for peer_id of peers
                        peer = peers[peer_id]
                        for key, role of peer
                            if role is 'role_primary'
                                keys[key] = true
                            else
                                if !keys[key]? or keys[key] isnt true
                                    keys[key] = false
                    for key of keys
                        if keys[key] is false
                            namespaces_without_masters.push
                                id: namespace.get('id')
                                name: namespace.get('name')
                else
                    namespaces_without_masters.push
                        id: namespace.get('id')
                        name: namespace.get('name')

            if namespaces_without_masters.length > 0
                status.has_availability_problems = true
                status.has_namespaces_without_masters = true
                status.num_namespaces_without_masters = namespaces_without_masters.length
                status.namespaces_without_masters = namespaces_without_masters
 
            # checking for redundancy
            status.num_replicas = issues_redundancy.num_replicas
            if issues_redundancy.length > 0
                status.has_redundancy_problems = true
                status.num_replicas_offline = issues_redundancy.length
                status.replicas_offline = issues_redundancy.models


            # checking for Disk and RAM prolems
            status.machines_with_disk_full_problems = []
            status.machines_with_ram_problems = []

            for machine in machines.models
                if machine.get('stats')?.sys? and machine.get('stats').proc?
                    disk_used = machine.get('stats').sys.global_disk_space_used / machine.get('stats').sys.global_disk_space_total
                    if disk_used > @threshold_disk
                        new_machine =
                            id: machine.get('id')
                            name: machine.get('name')
                        status.machines_with_disk_full_problems.push new_machine
                    ram_used = machine.get('stats').proc.global_mem_used / machine.get('stats').proc.global_mem_total
                    if ram_used > @threshold_ram
                        new_machine =
                            id: machine.get('id')
                            name: machine.get('name')
                        status.machines_with_ram_problems.push new_machine


            if status.machines_with_disk_full_problems.length > 0
                status.has_disk_problems = true
                status.num_machines_with_disk_full_problems = status.machines_with_disk_full_problems.length
            status.threshold_disk = Math.floor(@threshold_disk*100)

            if status.machines_with_ram_problems.length > 0
                status.has_ram_problems = true
                status.num_machines_with_ram_problems = status.machines_with_ram_problems.length
            status.threshold_ram = Math.floor(@threshold_ram*100)

            return status

    ###
    class @Logs extends Backbone.View
        className: 'recent-log-entries'
        tagName: 'ul'
        min_timestamp: 0
        max_entry_logs: 5
        interval_update_log: 10000

        initialize: ->
            @fetch_log()
            @set_interval = setInterval @fetch_log, @interval_update_log
            @log_entries = []

        fetch_log: =>
            $.ajax({
                contentType: 'application/json'
                url: '/ajax/log/_?max_length='+@max_entry_logs+'&min_timestamp='+@min_timestamp
                dataType: 'json'
                success: @set_log_entries
            })

        set_log_entries: (response) =>
            need_render = false
            for machine_id, data of response
                for new_log_entry in data
                    for old_log_entry, i in @log_entries
                        if parseFloat(new_log_entry.timestamp) > parseFloat(old_log_entry.get('timestamp'))
                            entry = new LogEntry new_log_entry
                            entry.set('machine_uuid', machine_id)
                            @log_entries.splice i, 0, entry
                            need_render = true
                            break

                    if @log_entries.length < @max_entry_logs
                        entry = new LogEntry new_log_entry
                        entry.set('machine_uuid', machine_id)
                        @log_entries.push entry
                        need_render = true
                    else if @log_entries.length > @max_entry_logs
                        @log_entries.pop()

            if need_render
                @render()

            @min_timestamp = parseFloat(@log_entries[0].get('timestamp'))+1

        render: =>
            @.$el.html ''
            for log in @log_entries
                view = new LogView.LogEntry model: log
                @.$el.append view.render().$el
            return @

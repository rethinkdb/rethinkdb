# Dashboard: provides an overview and visualizations of the cluster
# Dashboard View
module 'DashboardView', ->
    # Cluster.Container
    class @Container extends Backbone.View
        template: Handlebars.compile $('#dashboard_view-template').html()
        id: 'dashboard'

        initialize: =>
            log_initial '(initializing) dashboard container view'

            @cluster_status = new DashboardView.ClusterStatus()
            @logs = new DashboardView.Logs()

        render: =>
            @.$el.html @template({})
            @cluster_performance = new Vis.OpsPlot(computed_cluster.get_stats,
                width:  833             # width in pixels
                height: 300             # height in pixels
                seconds: 119            # num seconds to track
                height_in_units:20500   # scale of the plot on the y-axis when the plot is empty
                type: 'cluster'
                bigplot: true
            )

            @.$('#cluster_status_container').html @cluster_status.render().el
            @.$('#cluster_performance_container').html @cluster_performance.render().el
            @.$('.recent-log-entries-container').html @logs.render().el

            return @

        destroy: =>
            @cluster_status.destroy()
            @cluster_performance.destroy()

    # TODO: dropped RAM and disk distribution in the templates, need to remove support for them here as well
    class @ClusterStatus extends Backbone.View
        template: Handlebars.compile $('#cluster_status-template').html()
        className: 'cluster-status'

        events:
            'click a[rel=dashboard_details]': 'show_popover'

        show_popover: (event) =>
            event.preventDefault()
            @.$(event.currentTarget).popover('show')
            $popover = $('.popover')

            $popover.on 'clickoutside', (e) ->
                if e.target isnt event.target  # so we don't remove the popover when we click on the link
                    $(e.currentTarget).remove()
            $('.popover_close').on 'click', (e) ->
                $(e.target).parent().parent().remove()
        initialize: ->
            log_initial '(initializing) dashboard cluster status'
            issues.on 'all', @render
            issues_redundancy.on 'all', @render # when issues_redundancy is reset
            machines.on 'stats_updated', @render # when the stats of the machines are updated
            directory.on 'all', @render
            namespaces.on 'all', @render

            $('.links_to_other_view').live 'click', ->
                $('.popover-inner').remove()

            @render()

        render: =>
            log_render '(rendering) cluster status view'

            @.$el.html @template(@compute_status())
            @.$('a[rel=dashboard_details]').popover
                trigger: 'manual'
            @.delegateEvents()

            return @

        destroy: ->
            issues.off 'all', @render
            issues_redundancy.off 'all', @render # when issues_redundancy is reset
            machines.off 'stats_updated', @render # when the stats of the machines are updated
            directory.off 'all', @render
            namespaces.off 'all', @render

        # We could create a model to create issues because of Disk/RAM
        threshold_disk: 0.9
        threshold_ram: 0.9

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
                if machine.get('stats')?
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
                @.$el.append view.render_small().$el
            return @


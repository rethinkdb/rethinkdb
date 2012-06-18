# Dashboard: provides an overview and visualizations of the cluster
# Dashboard View
module 'DashboardView', ->
    # Cluster.Container
    class @Container extends Backbone.View
        el: '#cluster'
        className: 'dashboard_view'
        template: Handlebars.compile $('#dashboard_view-template').html() # TODO use div instead of a table
        initialize: =>
            log_initial '(initializing) sidebar view:'

            @cluster_status = new DashboardView.ClusterStatus()

        render: =>
            @.$el.html @template({})
            @cluster_performance = new DashboardView.ClusterPerformance()

            @.$('#cluster_status_container').html @cluster_status.render().el
            @.$('#cluster_performance_panel_placeholder').html @cluster_performance.render()


    class @ClusterStatus extends Backbone.View
        className: 'dashboard-view'
        template: Handlebars.compile $('#cluster_status-template').html()

        events:->
            'click a[rel=dashboard_details]': 'show_popover'

        show_popover: (event) =>
            event.preventDefault()
            @.$(event.currentTarget).popover('show')
            $popover = $('.popover')

            $popover.on 'clickoutside', (e) ->
                if e.target isnt event.target  # so we don't remove the popover when we click on the link
                    $(e.currentTarget).remove()

        initialize: ->
            log_initial '(initializing) dashboard view'
            issues.on 'all', @render
            issues_redundancy.on 'reset', @render # when issues_redundancy is reset
            machines.on 'stats_updated', @render # when the stats of the machines are updated
            @render()


        # We could create a model to create issues because of CPU/RAM
        threshold_cpu: 0.9
        threshold_ram: 0.9

        compute_status: ->
            status =
                has_availability_problems: false
                has_cpu_problems: false
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


            # checking for CPU and RAM prolems
            status.machines_with_cpu_problems = []
            status.machines_with_ram_problems = []

            for machine in machines.models
                if machine.get('stats')?
                    cpu_used = machine.get('stats').proc.cpu_combined_avg
                    if cpu_used > @threshold_cpu
                        new_machine =
                            uid: machine.get('id')
                            name: machine.get('name')
                        status.machines_with_cpu_problems.push new_machine
                    ram_used = machine.get('stats').proc.global_mem_used / machine.get('stats').proc.global_mem_total
                    if ram_used > @threshold_ram
                        new_machine =
                            uid: machine.get('id')
                            name: machine.get('name')
                        status.machines_with_ram_problems.push new_machine


            if status.machines_with_cpu_problems.length > 0
                status.has_cpu_problems = true
                status.num_machines_with_cpu_problems = status.machines_with_cpu_problems.length
            status.threshold_cpu = Math.floor(@threshold_cpu*100)

            if status.machines_with_ram_problems.length > 0
                status.has_ram_problems = true
                status.num_machines_with_ram_problems = status.machines_with_ram_problems.length
            status.threshold_ram = Math.floor(@threshold_ram*100)


            status

        render: =>
            log_render '(rendering) cluster status view'


            @.$el.html @template(@compute_status())
            @.$('a[rel=dashboard_details]').popover
                trigger: 'manual'
            @.delegateEvents()
            return @

    class @ClusterPerformance extends Backbone.View
        className: 'dashboard-view'
        template: Handlebars.compile $('#cluster_performance-template').html()


        initialize: ->
            log_initial '(initializing) cluster performance view'
            $('#cluster_performance_container').html @template({})
            @perf_panel = new Vis.PerformancePanel(computed_cluster.get_stats)
            @render()

        render: =>
            log_render '(rendering) cluster_performance view'
            return @perf_panel.render().$el



# Dashboard: provides an overview and visualizations of the cluster
# Dashboard View
module 'DashboardView', ->
    # Cluster.Container
    

    class @Container extends Backbone.View
        el: '#cluster'
        className: 'dashboard_view'
        template: Handlebars.compile $('#dashboard_view-template').html()
        initialize: =>
            log_initial '(initializing) sidebar view:'


        render: =>
            @.$el.html @template({})
            @cluster_status = new DashboardView.ClusterStatus()
            @cluster_performance = new DashboardView.ClusterPerformance()

            @.$('#cluster_status_container').html @cluster_status.render().el
            @.$('#cluster_performance_panel_placeholder').html @cluster_performance.render()


    class @ClusterStatus extends Backbone.View
        className: 'dashboard-view'
        template: Handlebars.compile $('#cluster_status-template').html()

        events:->
            'click a[rel=dashboard_details]': 'do_nothing'

        initialize: ->
            log_initial '(initializing) dashboard view'
            issues.on 'all', @render
            issues_redundancy.on 'reset', @render
            @render()

        do_nothing: (event) -> event.preventDefault()

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

            masters = {}
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
                for issue in issues.models
                    switch issue.get("type")
                        when "PERSISTENCE_ISSUE"
                            new_machine = 
                                uid: issue.get('location')
                                name: machines.get(issue.get('location')).get('name')
                            status.machines_with_disk_problems.push(new_machine)
                        when "MACHINE_DOWN"
                            if issue.get('victim') of masters
                                status.masters_offline.push 
                                    machine_id: issue.get('victim')
                                    machine_name: machines.get(issue.get('victim')).get('name')
                                    namespace_name: masters[issue.get('victim')].name
                                    namespace_id: masters[issue.get('victim')].id

                if status.machines_with_disk_problems.length > 0
                    status.num_machines_with_disk_problems = status.machines_with_disk_problems.length
                    status.has_persistence_problems = true
                if status.masters_offline.length > 0
                    status.num_masters_offline = status.masters_offline.length
                    status.has_availability_problems = true

            # checking for redundancy
            status.num_replicas = issues_redundancy.num_replicas
            if issues_redundancy.length > 0
                status.has_redundancy_problems = true
                status.num_replicas_offline = issues_redundancy.length
            
            
            status

        isnt_empty: (data) ->
            for i of data
                return true
            return false

        render: =>
            log_render '(rendering) cluster status view'
            
            @.$el.html @template(@compute_status())
            $(".popover").remove()  
            @.$('a[rel=dashboard_details]').popover
                html: true

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




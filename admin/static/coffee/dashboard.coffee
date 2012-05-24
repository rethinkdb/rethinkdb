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

            if issues.length != 0
                status.num_machines_with_disk_problems = 0
                status.machines_with_disk_problems = []
                for issue in issues.models
                    switch issue.get("type")
                        when "PERSISTENCE_ISSUE"
                            new_machine = 
                                uid: issue.get('location')
                                name: machines.get(issue.get('location')).get('name')
                            status.machines_with_disk_problems.push(new_machine)
                        when "MACHINE_DOWN"
                            status.has_availability_problems = true

                if status.machines_with_disk_problems.length > 0
                    status.num_machines_with_disk_problems = status.machines_with_disk_problems.length
                    status.has_persistence_problems = true

            status

        render: =>
            log_render '(rendering) cluster status view'
            
            #$('#cluster_status_container').html @.$el.html @template(@compute_status())
            @.$el.html @template(@compute_status())


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




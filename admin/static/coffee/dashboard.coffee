# Dashboard: provides an overview and visualizations of the cluster
class DashboardView extends Backbone.View
    className: 'dashboard-view'
    template: Handlebars.compile $('#dashboard_view-template').html()

    initialize: ->
        log_initial '(initializing) dashboard view'
        @perf_panel = new Vis.PerformancePanel(computed_cluster.get_stats)
        issues.on 'all', @render, this


    refresh: ->
        $.getJSON('/ajax/issues', set_issues)

    update_status: ->
        @status = 
            has_availability_problems: false
            has_cpu_problems: false
            has_conflicts_problems : false
            has_redundancy_problems: false
            has_ram_problems: false
            has_persistence_problems: false
            num_machines: machines.length
            num_namespaces: namespaces.length

        if window.issues.length != 0
            num_machines_with_working_disk = machines.length
            for issue in issues.models
                if issue.get("type") == "PERSISTENCE_ISSUE"
                    @status.has_persistence_problems = true
                    num_machines_with_working_disk--
            @status.num_machines_with_working_disk = num_machines_with_working_disk



    render: ->
        log_render '(rendering) dashboard view'
        @update_status()
        @.$el.html @template(@status)
        @.$('.cluster_performance_panel_placeholder').html @perf_panel.render().$el
        return @


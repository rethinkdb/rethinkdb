# Dashboard: provides an overview and visualizations of the cluster
class DashboardView extends Backbone.View
    className: 'dashboard-view'
    template: Handlebars.compile $('#dashboard_view-template').html()

    initialize: ->
        log_initial '(initializing) dashboard view'
        @perf_panel = new Vis.PerformancePanel(computed_cluster.get_stats)

    render: ->
        log_render '(rendering) dashboard view'
        json =
            namespaces: _.map(namespaces.models, ((n)->
                            id: n.get('id')
                            name: n.get('name')))
        @.$el.html @template json
        @.$('.cluster_performance_panel_placeholder').html @perf_panel.render().$el
        @.$('.dropdown-toggle').dropdown()

        return @


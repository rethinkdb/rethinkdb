# Dashboard: provides an overview and visualizations of the cluster
class DashboardView extends Backbone.View
    className: 'dashboard-view'
    template: Handlebars.compile $('#dashboard_view-template').html()

    initialize: ->
        log_initial '(initializing) dashboard view'
        @perfgraph = new Vis.OpsPlot(computed_cluster.get_stats)

    render: ->
        log_render '(rendering) dashboard view'
        @.$el.html @template({})
        @.$el.append @perfgraph.render().$el

        return @


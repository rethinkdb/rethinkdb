# Dashboard: provides an overview and visualizations of the cluster
class DashboardView extends Backbone.View
    className: 'dashboard-view'
    template: Handlebars.compile $('#dashboard_view-template').html()

    update_data_streams: (datastreams) ->
        timestamp = new Date(Date.now())

        for stream in datastreams
           stream.update(timestamp)

    create_fake_data: ->
        data_points = new DataPoints()
        cached_data = {}
        for collection in [namespaces, datacenters, machines]
            collection.map (model, i) ->
                d = new DataPoint
                    collection: collection
                    value: (i+1) * 100
                    id: model.get('id')
                    # Assumption: every datapoint across datastreams at the time of sampling will have the same datetime stamp
                    time: new Date(Date.now())
                data_points.add d
                cached_data[model.get('id')] = [d]
        return data_points

    initialize: ->
        log_initial '(initializing) dashboard view'

        mem_usage_data = new DataStream
            name: 'mem_usage_data'
            pretty_print_name: 'memory usage'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        disk_usage_data = new DataStream
            name: 'disk_usage_data'
            pretty_print_name: 'disk usage'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        cluster_performance_total = new DataStream
            name:  'cluster_performance_total'
            pretty_print_name: 'total ops/sec'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        cluster_performance_reads = new DataStream
            name:  'cluster_performance_reads'
            pretty_print_name: 'reads/sec'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        cluster_performance_writes = new DataStream
            name:  'cluster_performance_writes'
            pretty_print_name: 'writes/sec'
            data: @create_fake_data()
            cached_data: []
            active_uuids: []

        @data_streams = [mem_usage_data, disk_usage_data, cluster_performance_total, cluster_performance_reads, cluster_performance_writes]

        setInterval (=> @update_data_streams @data_streams), 1500

        @data_picker = new Vis.DataPicker @data_streams
        color_map = @data_picker.get_color_map()

        @disk_usage = new Vis.ResourcePieChart disk_usage_data, color_map
        @mem_usage = new Vis.ResourcePieChart mem_usage_data, color_map
        @cluster_performance = new Vis.ClusterPerformanceGraph [cluster_performance_total, cluster_performance_reads, cluster_performance_writes], color_map

    render: ->
        # Updates elements tracked by our fake data streams | Should be removed when DataStreams are live from the server TODO
        for stream in @data_streams
            stream.set('data', @create_fake_data())

        log_render '(rendering) dashboard view'
        @.$el.html @template({})

        @.$('.data-picker').html @data_picker.render().el
        @.$('.disk-usage').html @disk_usage.render().el
        @.$('.mem-usage').html @mem_usage.render().el
        @.$('.chart.cluster-performance').html @cluster_performance.render().el

        return @


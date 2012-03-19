# Models for Backbone.js
class Namespace extends Backbone.Model
    urlRoot: '/ajax/namespaces'
    initialize: ->
        log_initial '(initializing) namespace model'

    # Is x between the lower and upper splitpoints (the open interval) for the given index?
    splitpoint_between: (shard_index, sp) =>
        console.log "splitpoint_between #{shard_index}, #{sp}"
        all_sps = @.get('splitpoints')
        return (shard_index == 0 || all_sps[shard_index - 1] < sp) && (shard_index == all_sps.length || sp < all_sps[shard_index])

class Datacenter extends Backbone.Model

class Machine extends Backbone.Model

class Event extends Backbone.Model

class Issue extends Backbone.Model

class ConnectionStatus extends Backbone.Model

class DataStream extends Backbone.Model
    max_cached: 250
    cache_ready: false

    initialize: ->
        @.set('cached_data': [])
        setInterval @tick, 1500

    tick: => @.get('data').each (data_point) => @update_data data_point

    update_data: (data_point) =>
        # Attributes of the data point
        uuid = data_point.get('id')
        existing_val = data_point.get('value')

        # Get a random number between -1 and 1, using a normal distribution
        random = d3.random.normal(0,0.2)
        delta = random()
        delta = -1 if delta < -1
        delta = 1 if delta > 1

        # Multiply by a constant factor to get a delta for the next walk
        delta = Math.floor(delta * 100)

        # Pop the oldest data point off the cached data (if we've filled the cache)
        cached_data = @.get('cached_data')
        if not cached_data[uuid]?
            cached_data[uuid] = []
        if cached_data[uuid].length >= @num_cached
            cached_data[uuid].shift()

        # Cache the existing value (create a copy based on the existing object)
        cached_data[uuid].push new DataPoint data_point.toJSON()

        if not @cache_ready and cached_data[uuid].length >= 2
            @cache_ready = true
            @.trigger 'cache_ready'

        # Logging (remove TODO)
        ###
        name = data_point.get('collection').get(uuid).get('name')
        if name is 'usa_1' and @.get('name') is 'mem_usage_data'
            console.log name, "in dataset #{@.get('name')} now includes the data", _.map cached_data["01f04592-e403-4abc-a845-83d43f6fd967"], (data_point) -> data_point.get('value')
        ###
        
        # Make sure the new value is non-negative
        new_val = existing_val + delta
        new_val = existing_val if new_val <= 0

        # Set the new value
        data_point.set
            value: new_val
            # Assumption: every datapoint across datastreams at the time of sampling will have the same datetime stamp
            time: new Date(Date.now())

class DataPoint extends Backbone.Model

class DataPoints extends Backbone.Collection
    model: DataPoint

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

class Events extends Backbone.Collection
    model: Event
    comparator: (event) ->
        # sort strings in reverse order (return a negated string)
        String.fromCharCode.apply String,
            _.map(event.get('datetime').split(''), (c) -> 0xffff - c.charCodeAt())

class Issues extends Backbone.Collection
    model: Issue
    url: '/ajax/issues'

class DashboardView extends Backbone.View
    className: 'dashboard-view'
    template: Handlebars.compile $('#dashboard_view-template').html()

    initialize: ->
        log_initial '(initializing) dashboard view'

        fake_data = new DataPoints()
        cached_data = {}
        for collection in [namespaces, datacenters, machines]
            collection.map (model, i) ->
                d = new DataPoint
                    collection: collection
                    value: (i+1) * 100
                    id: model.get('id')
                    # Assumption: every datapoint across datastreams at the time of sampling will have the same datetime stamp
                    time: new Date(Date.now())
                fake_data.add d
                cached_data[model.get('id')] = [d]

        mem_usage_data = new DataStream
            name: 'mem_usage_data'
            data: fake_data
            cached_data: []
            active_uuids: []

        disk_usage_data = new DataStream
            name: 'disk_usage_data'
            data: fake_data
            cached_data: []
            active_uuids: []

        cluster_performance_data = new DataStream
            name:  'cluster_performance_data'
            data: fake_data
            cached_data: []
            active_uuids: []


        data_streams = [mem_usage_data, disk_usage_data, cluster_performance_data]

        @data_picker = new Vis.DataPicker data_streams
        color_map = @data_picker.get_color_map()

        @disk_usage = new Vis.ResourcePieChart disk_usage_data, color_map
        @mem_usage = new Vis.ResourcePieChart mem_usage_data, color_map
        @cluster_performance = new Vis.ClusterPerformanceGraph cluster_performance_data, color_map

    render: ->
        log_render '(rendering) dashboard view'
        @.$el.html @template({})

        @.$('.data-picker').html @data_picker.render().el
        @.$('.disk-usage').html @disk_usage.render().el
        @.$('.mem-usage').html @mem_usage.render().el
        @.$('.chart.cluster-performance').html @cluster_performance.render().el

        return @

module 'Vis', ->
    class @DataPicker extends Backbone.View
        template: Handlebars.compile $('#data_picker-template').html()
        class: 'data-picker'


        events: =>
            'click .nav li': 'switch_filter'

        # The DataPicker takes the following arguments:
        #   data_streams: a list of DataStreams to update whenever new data is picked
        initialize: (data_streams)->
            @filters = {}
            @collections = [namespaces, datacenters, machines]

            color_scheme = d3.scale.category20()
            @color_map = {}
            for collection in @collections
                collection.map (model, i) =>
                    @color_map[model.get('id')] = color_scheme i

            # Create one filter for each type of data (machine, namespace, datacenter, etc.) we use in all of these data streams
            for collection in @collections
                @filters[collection.name] = new Vis.DataStreamFilter collection, data_streams, @color_map
            @selected_filter = @filters[@collections[0].name]

        render: =>
            @.$el.html @template
                stream_filters: _.map(@filters, (filter, collection_name) =>
                    name: collection_name
                    selected: filter is @selected_filter
                )

            @.$('.filter').html @selected_filter.render().el
            return @

        switch_filter: (event) =>
            collection_name = $(event.currentTarget).data('filter')
            @selected_filter = @filters[collection_name]

            @render()

            event.preventDefault()

        get_color_map: => @color_map

    class @DataStreamFilter extends Backbone.View
        className: 'data-stream-filter'
        template: Handlebars.compile $('#data_stream_filter-template').html()

        events: ->
            'click .model': 'select_model'

        # The DataStreamFilter takes the following arguments:
        #   collection: the collection whose models will be used for this filter
        #   data_streams: data streams to update whenever a new set of models is selected
        initialize: (collection, data_streams, color_map) ->
            @collection = collection
            @data_streams = data_streams
            @color_map = color_map

            @models_selected = {}
            for model in collection.models
                @models_selected[model.get('id')] = true

        render: =>
            @.$el.html @template
                models: _.map(@collection.models, (model) =>
                    id: model.get('id')
                    name: model.get('name')
                    selected: @models_selected[model.get('id')]
                    color: @color_map[model.get('id')]
                )

            @filter_data_sources()

            @.delegateEvents()
            return @

        select_model: (event) =>
            $model = $(event.currentTarget)
            id = $model.data('id')
            @models_selected[id] = not @models_selected[id]
            $('input', $model).attr('checked', @models_selected[id])


            @filter_data_sources()

        filter_data_sources: =>
            active_uuids = []
            _.each @models_selected, (selected, uuid) -> active_uuids.push uuid if selected

            for stream in @data_streams
                stream.set
                    'active_uuids': []
                stream.set
                    'active_uuids': active_uuids
                console.log stream

    class @ResourcePieChart extends Backbone.View
        className: 'resource-pie-chart'

        events: ->
            'click': 'update_chart'

        initialize: (data_stream, color_map) ->
            log_initial '(initializing) resource pie chart'

            # Use the given datastream to back the pie chart
            @data_stream = data_stream
            @data = data_stream.get('data')

            # Use a pre-defined color scale to pick the colors
            @color_map = color_map

            # Dimensions of the pie chart
            @width = 220
            @height = 200
            @radius = Math.floor(Math.min(@width, @height) / 2  * 0.65)
            @text_offset = 10

            # Transition duration (update frequency) in ms
            @duration = 1000

            # Function to sort the data in order of ascending names (a and b are two datapoints to be compared to one another)
            sort_data_points = (a, b)->
                name = (data_point) -> data_point.collection.get(data_point.id).get('name')
                return d3.ascending(name a, name b)

            # Use the pie layout, indicate where to get the actual value from and how to sort the data
            @donut = d3.layout.pie().sort(sort_data_points).value((d) -> d.value)
            # Define the arc's width using the built-in arc function
            @arc = d3.svg.arc().innerRadius(@radius).outerRadius(@radius - 25)

        # Utility function to get the total value of the data points based on filtering
        get_total: =>
            # Choose only data points that are among the selected sets, get their sum
            values = _.map @get_filtered_data(), (data_point) -> data_point.value
            return _.reduce values, (memo, num) -> memo + num

        # Utility function to only get a filtered subset of the data
        #   existing_data: optional argument that will keep previously
        #       calculated positional data for each datum in the json 
        #       if it isn't added or removed from the filtered data
        get_filtered_data: (existing_data) =>
            existing = {}
            # If existing data has been provided, order the json by uuid for easy lookup
            if existing_data?
                for datum in existing_data
                    existing[datum.data.id] =
                        previous: datum.data.previous

            # Filter the datastream based on active uuids
            filtered_data = _.map @data_stream.get('active_uuids'), (uuid) =>
                json = @data.get(uuid).toJSON()
                # If the uuid is already in our pie chart, attach its previously calculated positional data (previous position)
                if existing[uuid]?
                    json = _.defaults json, existing[uuid]
                return json

            return filtered_data

        render: =>
            $(@el).empty()
            # Define the base visualization layer for the pie chart
            svg = d3.select(@el).append('svg:svg')
                    .attr('width', @width)
                    .attr('height', @height)

            @groups = {}

            # Group for the pie sections, ticks, and labels
            @groups.arcs = svg.append('svg:g')
                .attr('class', 'arcs')
                .attr('transform', "translate(#{@width/2},#{@height/2})")

            # Group for text in the center
            @groups.center = svg.append('svg:g')
                .attr('class', 'center')
                .attr('transform', "translate(#{@width/2},#{@height/2})")

            # Define the center text
            #   label for just the "total" element
            total_label = @groups.center.append('svg:text')
                .attr('class','total-label')
                .attr('dy', -10)
                .attr('text-anchor', 'middle')
                .text('Total')
            #   label for the actual value
            total_value = @groups.center.append('svg:text')
                .attr('class','total-value')
                .attr('dy', 7)
                .attr('text-anchor', 'middle')
                .text(@get_total())
            #   label for the units
            total_units = @groups.center.append('svg:text')
                .attr('class','total-units')
                .attr('dy', 21)
                .attr('text-anchor', 'middle')
                .text('mb')

            # Draw arcs for the donut pie chart
            draw_pie_chart = =>
                @arcs = @groups.arcs.selectAll('g.arc').data(@donut @get_filtered_data())

                # Whenever a datum is added, create a new group that will contain the arc path, arc tick, and arc label
                group = @arcs.enter()
                    .append('svg:g')
                        .attr('class','arc')
                        # Save the current state of the arc (angles, values, etc.) for use when tweening
                        .each((d) =>
                            # Make sure we have a color
                            if not @color_map[d.data.id]?
                                console.log "No color can be found for #{d.data.id}."
                            d.data.previous =
                                endAngle: d.endAngle
                                startAngle: d.startAngle
                                value: d.value
                        )

                # Add an arc path to the group
                group.append('svg:path')
                    .attr('class','section')
                    # Fill the pie chart with the color scheme we defined
                    .attr('fill', (d,i) => @color_map[d.data.id])
                    # Set the arc we defined
                    .attr('d', @arc)

                # Add a tick to the group
                group.append('svg:line')
                        .attr('class','tick')
                        .attr('x1', 0)
                        .attr('x2', 0)
                        .attr('y1', -@radius - 7)
                        .attr('y2', -@radius - 3)
                        .attr('transform', (d) -> "rotate(#{(d.startAngle + d.endAngle)/2 * (180/Math.PI)})")

                # Add a label to each group
                @pie_label = group.append('svg:text')
                    .attr('class','label')
                    .attr('text-anchor','middle')
                    .attr('dominant-baseline','central')
                    .text((d) =>
                        percentage = (d.value/@get_total()) * 100
                        return percentage.toFixed(1) + '%'
                    )

                # Terrible hack that works: set a timeout of zero so that this is called only when the rendered view is actually added to the DOM
                # Reason this is necessary: getting the bounding box of the text node (bbox) will only work when the text box is actually rendered on the page
                setTimeout =>
                    @pie_label.attr('transform', (d) =>
                        angle = (d.startAngle + d.endAngle)/2
                        p = @pie_label_position(@pie_label, angle)
                        return "translate(#{p.x},#{p.y})"
                    )
                , 0

                # Whenever a datum is removed, just remove the group
                @arcs.exit().remove()

            draw_pie_chart()

            @data_stream.on 'change:active_uuids', -> draw_pie_chart()

            # Make the data change every few ms | faked TODO
            setInterval @update_chart, 2000

            return @

        update_chart: =>
            # Update the data, redraw the arcs, calculate the new total
            new_data = @get_filtered_data(@arcs.data()) # Include the old data so we can keep the positional data for existing elements (previous position)
            @arcs.data(@donut new_data)
            total = @get_total()

            # Use an arc tweening function to transition between arcs
            @arcs.select('path.section').transition().duration(@duration)
                .attrTween('d', (d) =>
                    # Interpolate from the previous angle to the new angle
                    i = d3.interpolate d.data.previous, d

                    # Return a function that specifies the new path data using the interpolated values
                    return (t) => @arc(i(t))
                )
                .each 'end', (d) ->
                    d.data.previous =
                        endAngle: d.endAngle
                        startAngle: d.startAngle
                        value: d.value

            # Transition between ticks
            @arcs.select('line.tick').transition().duration(@duration)
                .attrTween('transform', (d) =>
                    # Convert the previous tick angle and new tick angle, converting radians to degrees
                    previous_angle = (d.data.previous.startAngle + d.data.previous.endAngle)/2 * (180/Math.PI)
                    new_angle =  (d.startAngle + d.endAngle)/2 * (180/Math.PI)

                    # Interpolate from the previous angle to the new angle
                    i = @interpolate_degrees previous_angle, new_angle

                    # Return a string indicating the SVG translation for the tick at each point along the interpolation
                    return (t) => "rotate(#{i(t)})"
                )

            # Transition between label positions
            pie_label = @arcs.select('text.label')
            pie_label.transition().duration(@duration)
                .attrTween('transform', (d) =>
                    previous_angle = (d.data.previous.startAngle + d.data.previous.endAngle)/2
                    new_angle = (d.startAngle + d.endAngle)/2
                    i = d3.interpolate previous_angle, new_angle
                    return (t) =>
                        p = @pie_label_position pie_label, i(t)
                        return "translate(#{p.x},#{p.y})"
                )
                .text((d) =>
                    percentage = (d.value/total) * 100
                    return percentage.toFixed(1) + '%'
                )

            # Update the tracked value
            @groups.center.select('text.total-value')
                .text total

        # Calculates the x and y position for labels on each section of the pie chart
        # Takes a text box that will be used as the label, and an angle that the label should be positioned at
        pie_label_position: (text_box, angle) ->
            # Bounding box of the text label
            bbox = text_box.node().getBBox()
            # Distance the text box should be from the circle
            r = @radius + @text_offset

            # Point at which the vector from the center of the pie chart intersects with the ellipse of the text box
            intersection =
                x: r * Math.sin(angle)
                y: -r * Math.cos(angle)
            # Point where the center of the rect should be placed
            center_ellipse =
                x: intersection.x + bbox.width/2 * Math.sin(angle)
                y: intersection.y - bbox.height/2 * Math.cos(angle)
            # Return the SVG point to draw the rect from
            return p =
                x: center_ellipse.x
                y: center_ellipse.y


        # Returns an interpolator for angles (designed to move within [-180,180])
        interpolate_degrees: (start_angle, end_angle) ->
            delta = end_angle - start_angle
            delta -= 360 while delta >= 180
            delta += 360 while delta < -180

            return d3.interpolate start_angle, start_angle + delta

    class @ClusterPerformanceGraph extends Backbone.View
        className: 'cluster-performance-graph'
        template: Handlebars.compile $('#vis_loading-template').html()

        events: ->
            'click': 'update_chart'

        time_now: -> new Date(Date.now() - @duration)

        get_data: =>
            # Create a zero-filled array the size of the first data set's cached values: this array will help keep track of stacking data sets
            previous_values = _.map @data_stream.get('cached_data')[@active_uuids[0]], -> 0
            _.map @active_uuids, (uuid, i) =>
                active_data = @data_stream.get('cached_data')[uuid]
                points = _.map active_data, (data_point) -> _.extend data_point.toJSON(),
                    previous_value: previous_values[i]
                
                # The current active dataset values should be added to previous_values: this helps stack each dataset on top of one another
                active_values = _.map active_data, (datapoint) -> datapoint.get('value')
                previous_values = _.map _.zip(active_values, previous_values), (zipped) ->
                    _.reduce zipped, (t,s) -> t + s

                return {
                    uuid: uuid
                    datapoints: _.rest points, Math.max(0, points.length - @n) # Only < n points should be plotted
                }

        get_latest_time: (data) ->
            last_times = _.map data, (data_set) -> data_set.datapoints[data_set.datapoints.length-1].time
            return _.min last_times

        get_highest_value: (data) ->
            # Get an array of arrays for the values of each data set. Example: [[1,2],[3,4],[5,6]]
            active_values = _.map data, (data_set) ->
                _.map data_set.datapoints, (datum) -> return datum.value

            if active_values.length is 0
                return 0
            # Get an array that has just the sum of values in each position (peaks). Example: [9,12]
            sum_values = _.reduce active_values, (prev_values, values) -> # Reduce to one array
                _.map _.zip(prev_values, values), (zipped) -> # Sum each arrays' elements
                    _.reduce zipped, (t,s) -> t + s

            # Return the maximum value (highest peak of data)
            return d3.max(sum_values)
            
        initialize: (data_stream, color_map) ->
            log_initial '(initializing) cluster performance graph'
            # Generate fake data for visualization reasons | faked data TODO
            # Number of elements
            @n = 50
            # Transition duration (update frequency) in ms
            @duration = 1500

            # Data stream that backs this plot, and the currently active uuids
            @data_stream = data_stream
            @active_uuids = data_stream.get('active_uuids')

            # Use a pre-defined color scale to pick the colors
            @color_map = color_map

            # Dimensions and margins for the plot
            @margin =
                top: 6
                right: 0
                bottom: 40
                left: 40
            @width = 600 - @margin.right
            @height = 300 - @margin.top - @margin.bottom

        render: =>
            if @get_data.length >= 2
                @render_chart()
            else
                @data_stream.on 'cache_ready', =>
                    @.$('.loading').fadeOut 'medium', =>
                        $(@).remove()
                        @render_chart()
                @.$el.html @template()

            @data_stream.on 'change:active_uuids', => 
                @active_uuids = @data_stream.get('active_uuids')
                @render_chart()
                
            return @

        render_chart: =>
            data = @get_data()
            $(@el).empty()

            # If there is no data, don't render the chart
            return if data.length <= 0

            # Get the latest recorded time
            last_time = @get_latest_time data

            @x = d3.time.scale()
                .domain([last_time - 30000 - @duration, last_time - @duration])
                .range([0, @width])

            # The y scale should start at zero, and end at the highest sum across all data sets
            @y = d3.scale.linear()
                .domain([0, @get_highest_value(data)])
                .range([@height, 0])

            # Define the axes
            @x_axis = => d3.svg.axis().scale(@x).orient('bottom').ticks(5).tickFormat(d3.time.format('%X'))
            @y_axis = => d3.svg.axis().scale(@y).ticks(5).orient('left')

            # Define the lines for each data set
            @line = d3.svg.line()
                .interpolate('linear')
                .x((d,i) => @x(d.time)
                )
                .y((d) => @y(d.value + d.previous_value))

            # Define the fill for each data set
            @area = d3.svg.area()
                .x((d,i) => @x(d.time))
                .y0(@height-1)
                .y1((d) => @y(d.value + d.previous_value))

            @chart = {}

            # Define and add the base visualization layer for the plot
            @chart.svg = d3.select(@el).append('svg:svg')
                    .attr('width', @width + @margin.left + @margin.right)
                    .attr('height', @height + @margin.top + @margin.bottom)
                # Group that will hold all svg elements: translate it to accomodate internal margins
                .append('svg:g')
                    .attr('transform', "translate(#{@margin.left},#{@margin.top})")

            # Define the clipping path: we need to clip the line to make sure it doesn't interfere with the axes
            @chart.clipping_path = @chart.svg.append('defs').append('clipPath')
                    .attr('id', 'clip')
                .append('rect')
                    .attr('width', @width)
                    .attr('height', @height)

            # For each data set, create a group, add a clipping path, and set the data to be associated with the data set
            @chart.data_sets = {}
            # Add elements from the back, since SVG's z-order depends on the order in which elements are added
            for i in [data.length-1..0]
                data_set = data[i]
                color = @color_map[data_set.uuid]
                group = @chart.svg.append('g')
                        .attr('clip-path', 'url(#clip)')
                        .data([data_set.datapoints])

                # Add a line to each data set group
                group.append('path')
                        .attr('class','line')
                        .attr('stroke', color)
                        .attr('d', @line)

                # Add an area fill to each data set group
                group.append('path')
                        .attr('class','area')
                        .attr('fill', color)
                        .attr('d', @area)

                @chart.data_sets[data_set.uuid] = group

            # Define and add the axes
            @chart.axes =
                x: @chart.svg.append('g')
                    .attr('class', 'x-axis')
                    .attr('transform', "translate(0,#{@height})")
                    .call @x_axis()
                y: @chart.svg.append('g')
                    .attr('class', 'y-axis')
                    .call @y_axis()


            # Start the first tick
            @update_chart()

            return @

        # Update the chart (each tick)
        update_chart: =>
            data = @get_data()
            last_time = @get_latest_time data

            # Update the domains for both axes
            @x.domain([last_time - 30000 - @duration, last_time - @duration])

            # For the y-axis, the domain should only grow, not shrink, based on previous domains calculated
            existing_domain = @y.domain()
            highest_value = d3.max([@get_highest_value(data), existing_domain[1]])
            @y.domain([0, highest_value])

            # Transition the axes: slide linearly
            @chart.axes.x.transition()
               .duration(@duration)
               .ease('linear')
               .call @x_axis()

            @chart.axes.y.transition()
                .duration(@duration)
                .ease('linear')
                .call @y_axis()

            animate = (data_set) =>
                group = @chart.data_sets[data_set.uuid]

                # Update the data for each data set
                group.data([data_set.datapoints])

                slide_path_to = @x(last_time - @duration) - @x(last_time)
                # Redraw and transition the line for each data set
                group.select('path.line')
                    # Redraw the line, but don't transform anything yet (otherwise the effect is visually jarring)
                    .attr('d', @line)
                    .attr('transform', null)
                    # Slide in the newly drawn line by translating it
                    .transition()
                        .duration(@duration)
                        .ease('linear')
                        .attr('transform', "translate(#{slide_path_to})")

                # Redraw and transition the area fill for each data set
                group.select('path.area')
                    .attr('d', @area)
                    .attr('transform', null)
                    .transition()
                        .duration(@duration)
                        .ease('linear')
                        .attr('transform', "translate(#{slide_path_to})")
                    # Call the tick function again after this transition is finished (infinite loop)
                    .each 'end', => @update_chart() if data_set.uuid is @active_uuids[@active_uuids.length - 1]
            for data_set in data
                animate data_set

modal_registry = []
clear_modals = ->
    modal.hide_modal() for modal in modal_registry
    modal_registry = []
register_modal = (modal) -> modal_registry.push(modal)




# Router for Backbone.js
class BackboneCluster extends Backbone.Router
    routes:
        '': 'index_namespaces'
        'namespaces': 'index_namespaces'
        'namespaces/:id': 'namespace'
        'datacenters': 'index_datacenters'
        'datacenters/:id': 'datacenter'
        'machines': 'index_machines'
        'machines/:id': 'machine'
        'dashboard': 'dashboard'
        'resolve_issues': 'resolve_issues'
        'events': 'events'

    initialize: ->
        log_initial '(initializing) router'

        @$container = $('#cluster')

        @namespaces_cluster_view = new ClusterView.NamespacesContainer
            namespaces: namespaces

        @datacenters_cluster_view = new ClusterView.DatacentersContainer
            datacenters: datacenters

        @machines_cluster_view = new ClusterView.MachinesContainer
            machines: machines

        @dashboard_view = new DashboardView

        @status_panel_view = new StatusPanelView
            model: connection_status

        # Add and render the sidebar (visible across all views)
        @$sidebar = $('#sidebar')
        window.sidebar = new Sidebar.Container()
        @sidebar = window.sidebar
        @render_sidebar()

        # Same for the status panel
        @$status_panel = $('.sidebar-container .section.cluster-status')
        @$status_panel.prepend @status_panel_view.render().el

        @resolve_issues_view = new ResolveIssuesView.Container
        @events_view = new EventsView.Container

    render_sidebar: -> @$sidebar.html @sidebar.render().el

    index_namespaces: ->
        log_router '/index_namespaces'
        clear_modals()
        @$container.html @namespaces_cluster_view.render().el

    index_datacenters: ->
        log_router '/index_datacenters'
        clear_modals()
        @$container.html @datacenters_cluster_view.render().el

    index_machines: ->
        log_router '/index_machines'
        clear_modals()
        @$container.html @machines_cluster_view.render().el

    dashboard: ->
        log_router '/dashboard'
        clear_modals()
        @$container.html @dashboard_view.render().el

    resolve_issues: ->
        log_router '/resolve_issues'
        clear_modals()
        @$container.html @resolve_issues_view.render().el

    events: ->
        log_router '/events'
        clear_modals()
        @$container.html @events_view.render().el

    namespace: (id) ->
        log_router '/namespaces/' + id
        clear_modals()

        # Helper function to build the namespace view
        build_namespace_view = (namespace) =>
            namespace_view = new NamespaceView.Container model: namespace
            @$container.html namespace_view.render().el

        # Return the existing namespace from the collection if it exists
        return build_namespace_view namespaces.get(id) if namespaces.get(id)?

        # Otherwise, show an error message stating that the namespace does not exist
        @$container.empty().text 'Namespace '+id+' does not exist.'

    datacenter: (id) ->
        log_router '/datacenters/' + id
        clear_modals()

        # Helper function to build the datacenter view
        build_datacenter_view = (datacenter) =>
            datacenter_view = new DatacenterView.Container model: datacenter
            @$container.html datacenter_view.render().el

        # Return the existing datacenter from the collection if it exists
        return build_datacenter_view datacenters.get(id) if datacenters.get(id)?

        # Otherwise, show an error message stating that the datacenter does not exist
        @$container.empty().text 'Datacenter '+id+' does not exist.'


    machine: (id) ->
        log_router '/machines/' + id
        clear_modals()

        # Helper function to build the machine view
        build_machine_view = (machine) =>
            machine_view = new MachineView.Container model: machine
            @$container.html machine_view.render().el

        # Return the existing machine from the collection if it exists
        return build_machine_view machines.get(id) if machines.get(id)?

        # Otherwise, show an error message stating that the machine does not exist
        @$container.empty().text 'Machine '+id+' does not exist.'

updateInterval = 5000;

declare_client_connected = ->
    window.connection_status.set({client_disconnected: false})
    clearTimeout(window.apply_diffs_timer)
    window.apply_diffs_timer = setTimeout (-> window.connection_status.set({client_disconnected: true})), 2 * updateInterval

# Process updates from the server and apply the diffs to our view of the data. Used by our version of Backbone.sync and POST / PUT responses for form actions
apply_diffs = (updates) ->
    declare_client_connected()
    for collection_id, collection_data of updates
        for id, data of collection_data
            switch collection_id
                when 'dummy_namespaces'
                    collection = namespaces
                    data.protocol = "dummy"
                when 'memcached_namespaces'
                    collection = namespaces
                    data.protocol = "memcached"
                when 'datacenters' then collection = datacenters
                when 'machines' then collection = machines
                when 'me' then continue
                else
                    console.log "Unhandled element update: " + update
                    return
            if (collection.get(id))
                collection.get(id).set(data)
            else
                data.id = id
                collection.add new collection.model(data)
    return

$ ->
    bind_dev_tools()

    # Initializing the Backbone.js app
    window.datacenters = new Datacenters
    window.namespaces = new Namespaces
    window.machines = new Machines
    window.issues = new Issues
    window.events = new Events
    window.connection_status = new ConnectionStatus

    window.last_update_tstamp = 0

    # Add fake issues and events for testing
    generate_fake_events(events)
    generate_fake_issues(issues)

    # Load the data bootstrapped from the HTML template
    # reset_collections()
    reset_token()

    # Log all events fired for the namespaces collection (debugging)
    namespaces.on 'all', (event_name) ->
        console.log 'event fired: '+event_name

    # Override the default Backbone.sync behavior to allow reading diffs
    legacy_sync = Backbone.sync
    Backbone.sync = (method, model, success, error) ->
        if method is 'read'
            $.getJSON('/ajax', apply_diffs)
        else
            legacy_sync method, model, success, error


    # This object is for events triggered by views on the router; this exists because the router is unavailable when first initializing
    window.app_events = {}

    window.app = new BackboneCluster
    Backbone.history.start()

    # Signal that the router is ready to be bound to
    $(window.app_events).trigger('on_ready')

    setInterval (-> Backbone.sync 'read', null), updateInterval
    declare_client_connected()

    $.getJSON('/ajax', apply_diffs)

    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true

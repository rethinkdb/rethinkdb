module 'Explain', ->
    class @Container extends Backbone.View
        id: 'explain'
        template: Handlebars.templates['explain_view-template']
        machine_name_template: Handlebars.templates['explain-machine_position-template']
        legend_template: Handlebars.templates['explain-legend-template']

        events:
            'click .expandable_task': 'render_sub_graph'
            'click .go_up': 'go_up'

        initialize: =>
            @parents = []

        render: =>
            @$el.html @template()


            return @

        get_data: (file) =>
            @svg_width = @$('.explain_svg').width()
            that = @
            $.ajax
                url: '/js/explain'+file+'.json'
                dataType: 'json'
                contentType: 'application/json'
                success: that.render_graph
                error: -> console.log 'Could not retrieve data for explain'

        render_sub_graph: (event) =>
            @parents.push @current_task
            data = $(event.target).data('task')
            @render_graph(data)
                        
        go_up: (event) =>
            event.preventDefault()
            task = @parents.pop()
            @render_graph task

        render_graph: (current_task) =>
            # Check if the user can go up
            if @parents.length > 0
                @$('.go_up').show()
            else
                @$('.go_up').hide()
            @current_task = current_task

            # Clean some stuff
            @$('.explain_svg').empty() # Clean svg
            $(' .tooltip').remove() # Clean tooltip
            # We don't sort machines anymore
            #@$('.list_machine_positions').empty()
            @$('.legend').empty()
            @$('.query').empty()
            
            @$('.query').html current_task.query
            @$('.explain_svg').attr 'height', @svg_height # The first array just has one object (the whole query)

            @svg = d3.selectAll('.explain_svg')
        
            # Get machines
            @drawable_machines_map_position = {}
            @get_drawable_machines current_task, 0
    
            # Sort them and keep the master in the middle
            drawable_machines_array = []
            for machine of @drawable_machines_map_position
                if machine isnt current_task['machine_id'] # If it's not the main machine
                    drawable_machines_array.push machine
            drawable_machines_array.sort()
            drawable_machines_array.splice drawable_machines_array.length/2, 0,current_task['machine_id']

            # Fill the position of the machines
            @sub_width = @svg_width/drawable_machines_array.length
            for machine_id, i in drawable_machines_array
                @drawable_machines_map_position[machine_id] = Math.round i*@sub_width+@sub_width/2

            # Add legend
            ### 
            for machine, i in drawable_machines_array
                if machines.get(machine)?
                    @$('.list_machine_positions').append @machine_name_template
                        valid_link: true
                        id: machine
                        name: machines.get(machine).get('name')
                else
                    @$('.list_machine_positions').append @machine_name_template
                        name: machine
            @$('.machine_position').css 'width', @sub_width-1 # -2 for the border
            ###
            axis = []
            axis.push
                x1: 20
                x2: 20
                y1: 20
                y2: @svg_height-20
            axis.push
                x1: 10
                x2: 20
                y1: @svg_height-30
                y2: @svg_height-20
            axis.push
                x1: 30
                x2: 20
                y1: @svg_height-30
                y2: @svg_height-20


            @svg.selectAll('line-in-'+level)
                .data(axis) # Pff, let's make something clean
                .enter()
                .append('line')
                .attr('x1', (d) -> return d.x1)
                .attr('x2', (d) -> return d.x2)
                .attr('y1', (d) -> return d.y1)
                .attr('y2', (d) -> return d.y2)
                .style('stroke', '#000')
                .style('stroke-width', @stroke_width)

            
            level = 0
            step = 0
            num_simultaneous_task_drawn = 0
            @draw_component
                task: current_task
                level: level
                step: 0
                parent_position:
                    x: @drawable_machines_map_position[current_task['machine_id']]
                    y: @svg_padding
                parent_width: @svg_width
                num_simultaneous_task_drawn: num_simultaneous_task_drawn
                height_available: @svg_height-2*@svg_padding
                is_root: true

            #TODO bind to proper things
            @.$('rect').tooltip
                placement: 'right'
                trigger: 'hover'

            @.$('circle').tooltip
                trigger: 'hover'

        svg_height: 1200
        svg_padding: 50
        min_height_line: 20
        min_traffic_height: 20
        max_traffic_height: 40
        height_before_bifurcation: 20
        border_size: 4
        stroke_width: 2
        proportion_bloc: 2/5
        background_color: '#fff'
        task_color: '#eee'
        task_border_color: '#dfdfdf'
        task_border_width: 1
        more_height: 70
        more_bloc_height: 50
        dot_radius: 4

        get_drawable_machines: (task, num_simultaneous_task_drawn) =>
            if task['sub_tasks']?
                if task['sub_tasks_are_parallel'] is true
                    num_simultaneous_task_drawn++
                    if num_simultaneous_task_drawn < 2
                        for sub_task in task['sub_tasks']
                            @drawable_machines_map_position[sub_task['machine_id']] = true
                            @get_drawable_machines sub_task, num_simultaneous_task_drawn
                    #else we do not go deeper
                else
                    for sub_task in task['sub_tasks']
                        @drawable_machines_map_position[sub_task['machine_id']] = true
                        @get_drawable_machines sub_task, num_simultaneous_task_drawn
            else
                @drawable_machines_map_position[task['machine_id']] = true
                



        draw_component: (args) =>
            task = args.task
            level = args.level
            step = args.step
            parent_position = args.parent_position
            parent_width = args.parent_width
            num_simultaneous_task_drawn = args.num_simultaneous_task_drawn
            height_available = args.height_available
            execution_duration = args.execution_duration
            is_root = if args.is_root? then args.is_root else false
            draw_traffic_in = if args.draw_traffic_in? then args.draw_traffic_in else false
            draw_traffic_out = if args.draw_traffic_out? then args.draw_traffic_out else false
            scale_traffic = args.scale_traffic
            scale_execution = args.scale_execution


            sub_tasks = task.sub_tasks
            if not sub_tasks?
                # No subtasks, let's just draw the task
                #TODO Add first/last line?
                @draw_single_task
                    task: task
                    level: level
                    step: step
                    parent_position: parent_position
                    height_available: height_available
                    is_root: is_root
                    draw_traffic_in: draw_traffic_in
                    draw_traffic_out: draw_traffic_out
                    scale_traffic: scale_traffic
                    scale_execution: scale_execution
            else
                if task['sub_tasks_are_parallel'] is true
                    num_simultaneous_task_drawn++
                    if num_simultaneous_task_drawn < 2
                        @draw_tasks_in_parallel
                            task: task
                            sub_tasks: sub_tasks
                            level: level
                            parent_position: parent_position
                            num_simultaneous_task_drawn: num_simultaneous_task_drawn
                            height_available: height_available
                            is_root: is_root
                            draw_traffic_in: draw_traffic_in
                            draw_traffic_out: draw_traffic_out
                            scale_traffic: scale_traffic
                            scale_execution: scale_execution
                    else # We can not go deeper, so let's draw the task
                        @draw_single_task
                            task: task
                            level: level
                            step: step
                            parent_position: parent_position
                            height_available: height_available
                            is_root: is_root
                            draw_traffic_in: draw_traffic_in
                            draw_traffic_out: draw_traffic_out
                            scale_traffic: scale_traffic
                            scale_execution: scale_execution

                else # In series task
                    if task['sub_tasks_stats']?['count']? and task['sub_tasks_stats']['count'] > sub_tasks.length and is_root is false
                        @draw_single_task
                            task: task
                            level: level
                            step: step
                            parent_position: parent_position
                            height_available: height_available
                            is_root: is_root
                            draw_traffic_in: draw_traffic_in
                            draw_traffic_out: draw_traffic_out
                            scale_traffic: scale_traffic
                            scale_execution: scale_execution
                    else
                        @draw_tasks_in_series
                            task: task
                            sub_tasks: sub_tasks
                            level: level
                            parent_position: parent_position
                            parent_width: parent_width
                            num_simultaneous_task_drawn: num_simultaneous_task_drawn
                            height_available: height_available
                            is_root: is_root
                            draw_traffic_in: draw_traffic_in
                            draw_traffic_out: draw_traffic_out
                            scale_traffic: scale_traffic
                            scale_execution: scale_execution

        compute_duration_all_tasks_in_series: (tasks) =>
            duration = 0
            for task in tasks
                duration += task['execution_duration']
            return duration
        
        compute_sum_data_transfered_in_series: (tasks) =>
            if tasks.length is 0
                return 0
            data = tasks[0]['size_message_sent']
            for task in tasks
                data += task['size_message_sent']
            return data

        draw_single_task: (args) =>
            task = args.task
            level = args.level
            step = args.step
            parent_position = args.parent_position
            height_available = args.height_available
            scale_traffic = args.scale_traffic
            scale_execution = args.scale_execution
            is_root = args.is_root
            draw_traffic_in = args.draw_traffic_in
            draw_traffic_out = args.draw_traffic_out

            that = @

            #TODO if is_root is true, draw circles

            current_position_y = parent_position.y

            if draw_traffic_in is true
                task_element = @svg.selectAll('line-single')
                    .data([task])
                    .enter()
                    .append('line')
                    .attr('x1', parent_position.x)
                    .attr('y1', current_position_y)
                    .attr('x2', parent_position.x)
                    .attr('y2', current_position_y+scale_traffic(task['size_message_received']))
                    .attr('stroke', '#000')
                    .attr('stroke-width', @stroke_width)
                    .attr('title', 'Receiving data: '+task['size_message_received']+'B')
                current_position_y += scale_traffic(task['size_message_received'])
                
            task_element = @svg.selectAll('rect-level-'+level)
                .data([task])
                .enter()
                .append('rect')
                .attr('x', parent_position.x-@sub_width*@proportion_bloc/2)
                .attr('y', current_position_y)
                .attr('width', @sub_width*@proportion_bloc)
                .attr('height', scale_execution(task['execution_duration']))
                .attr('stroke-width', @task_border_width)
                .attr('stroke', @task_border_color)
                .style('fill', @task_color)
                .attr('title', 'Machine: '+task['machine_id']+'<br/>'+task['query'])
                .attr('class', (d) ->
                    if d.sub_tasks?
                        return 'expandable_task'
                    else
                        return 'task'
                )
                .attr('data-task', (d) ->
                    if d.sub_tasks?
                        return JSON.stringify d
                    else
                        return ''
                )

            if draw_traffic_out is true
                current_position_y += scale_execution(task['execution_duration'])
                task_element = @svg.selectAll('line-single')
                    .data([task])
                    .enter()
                    .append('line')
                    .attr('x1', parent_position.x)
                    .attr('y1', current_position_y)
                    .attr('x2', parent_position.x)
                    .attr('y2', current_position_y+scale_traffic(task['size_message_sent']))
                    .attr('stroke', '#000')
                    .attr('stroke-width', @stroke_width)
                    .attr('title', 'Receiving data: '+task['size_message_received']+'B')
                current_position_y += scale_traffic(task['size_message_received'])

        draw_tasks_in_series: (args) =>
            task = args.task
            sub_tasks = args.sub_tasks
            level = args.level
            step = args.step
            parent_position = args.parent_position
            parent_width = args.parent_width
            num_simultaneous_task_drawn = args.num_simultaneous_task_drawn
            height_available = args.height_available
            is_root = args.is_root
            that = @

            tasks_duration = @compute_duration_all_tasks_in_series sub_tasks
            traffic_duration = task['execution_duration']-tasks_duration

            tasks_height = tasks_duration/task['execution_duration']*height_available
            traffic_height = traffic_duration/task['execution_duration']*height_available

            #TODO adapt scale in case things are too unbalanced
            scale_execution = d3.scale.linear().domain([0, tasks_duration]).range([0, tasks_height])

            total_traffic = @compute_sum_data_transfered_in_series sub_tasks
            scale_traffic = d3.scale.linear().domain([0, total_traffic]).range([0, traffic_height])

            current_position_y = parent_position.y
            for sub_task, i in sub_tasks
                height_task = scale_execution(sub_task['execution_duration'])
                draw_traffic_in = false
                draw_traffic_out = false
                if i is 0
                    height_task += scale_traffic(sub_task['size_message_received'])
                    draw_traffic_in = true
                else if sub_tasks[i]['sub_tasks']? and sub_tasks[i]['sub_tasks_are_parallel'] is true
                    height_task += scale_traffic(sub_task['size_message_received'])
                    draw_traffic_in = true

                if not sub_tasks[i+1]? or not sub_tasks[i+1]['sub_tasks']? or sub_tasks[i+1]['sub_tasks_are_parallel'] is false
                    height_task += scale_traffic(sub_task['size_message_sent'])
                    draw_traffic_out = true


                @draw_component
                    task: sub_task
                    level: level
                    step: i
                    parent_position:
                        x: parent_position.x
                        y: current_position_y
                    num_simultaneous_task_drawn: num_simultaneous_task_drawn
                    height_available: height_task
                    execution_duration: sub_task['execution_duration']
                    scale_execution: scale_execution
                    scale_traffic: scale_traffic
                    draw_traffic_in: draw_traffic_in
                    draw_traffic_out: draw_traffic_out

                current_position_y = current_position_y+height_task

            if is_root is true
                circles = []
                circles.push
                    cx: @drawable_machines_map_position[task.machine_id]
                    cy: parent_position.y
                    title: 'Client received query'
                circles.push
                    cx: @drawable_machines_map_position[task.machine_id]
                    cy: parent_position.y+height_available
                    title: 'Client sent back results'

                @svg.selectAll('.client')
                    .data(circles)
                    .enter()
                    .append('circle')
                    .attr('cx', (d) -> return d.cx)
                    .attr('cy', (d) -> return d.cy)
                    .attr('r', @dot_radius)
                    .style('fill', @task_color_border)
                    .attr('title', (d) -> return d.title)

        draw_tasks_in_parallel: (args) =>
            task = args.task
            sub_tasks = args.sub_tasks
            level = args.level
            parent_position = args.parent_position
            num_simultaneous_task_drawn = args.num_simultaneous_task_drawn
            height_available = args.height_available
            is_root = args.is_root
            #TODO Keep parent's neighbors in memory
            that = @

            # TODO if is_root is true, draw circles

            if is_root is true
                height_available -= 2*@height_before_bifurcation
                parent_position.y += @height_before_bifurcation
                lines = []
                lines.push
                    x1: parent_position.x
                    x2: parent_position.x
                    y1: parent_position.y-@height_before_bifurcation
                    y2: parent_position.y
                lines.push
                    x1: parent_position.x
                    x2: parent_position.x
                    y1: parent_position.y+height_available
                    y2: parent_position.y+height_available+@height_before_bifurcation
                @svg.selectAll('line-in-constant-'+level)
                    .data(lines)
                    .enter()
                    .append('line')
                    .attr('x1', (d) -> return d.x1)
                    .attr('x2', (d) -> return d.x2)
                    .attr('y1', (d) -> return d.y1)
                    .attr('y2', (d) -> return d.y2)
                    .style('stroke', '#000')
                    .style('stroke-width', @stroke_width)

            # We suppose that every branches has the same scale for traffic
            # Let's compute the largest one that will allow our data to fit in height_available
            min_ratio = (d3.min sub_tasks, (d, i) -> return (task['execution_duration']-d['execution_duration'])/(d['size_message_sent']+d['size_message_received']))/task['execution_duration']*height_available

            scale_traffic = (message_size) ->
                return Math.round message_size*min_ratio
            scale_execution = (task_duration) ->
                return Math.round task_duration*height_available/task['execution_duration']
            scale_x = d3.scale.linear().domain([0, @svg_width]).range([-@sub_width*@proportion_bloc/2, @sub_width*@proportion_bloc/2])
            # Get the highest box
            height_first_split = d3.min sub_tasks, (d, i) -> return scale_traffic(d.size_message_received)
            height_first_split /= 2
            scale_y = d3.scale.linear().domain([0, @svg_width/2]).range([0, height_first_split])


            # Draw the top lines
            @svg.selectAll('line-in-'+level)
                .data(sub_tasks)
                .enter()
                .append('line')
                .attr('x1', (sub_task, i) ->
                    return that.drawable_machines_map_position[task.machine_id]+scale_x(that.drawable_machines_map_position[sub_task.machine_id])
                )
                .attr('x2', (sub_task, i) ->
                    return that.drawable_machines_map_position[task.machine_id]+scale_x(that.drawable_machines_map_position[sub_task.machine_id])
                )
                .attr('y1', (sub_task, i) ->
                    return parent_position.y
                )
                .attr('y2', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .style('stroke', '#000')
                .style('stroke-width', @stroke_width)
            @svg.selectAll('line-in-'+level)
                .data(sub_tasks)
                .enter()
                .append('line')
                .attr('x1', (sub_task, i) ->
                    return that.drawable_machines_map_position[task.machine_id]+scale_x(that.drawable_machines_map_position[sub_task.machine_id])
                )
                .attr('x2', (sub_task, i) ->
                    return that.drawable_machines_map_position[sub_task.machine_id]
                )
                .attr('y1', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .attr('y2', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .style('stroke', '#000')
                .style('stroke-width', @stroke_width)
            @svg.selectAll('line-in-'+level)
                .data(sub_tasks)
                .enter()
                .append('line')
                .attr('x1', (sub_task, i) ->
                    return that.drawable_machines_map_position[sub_task.machine_id]
                )
                .attr('x2', (sub_task, i) ->
                    return that.drawable_machines_map_position[sub_task.machine_id]
                )
                .attr('y1', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .attr('y2', (sub_task, i) ->
                    return parent_position.y+scale_traffic(sub_task['size_message_received'])
                )
                .style('stroke', '#000')
                .style('stroke-width', @stroke_width)



            height_second_split = d3.max sub_tasks, (d, i) -> return scale_traffic(d.size_message_received)+scale_execution(d.execution_duration)
            scale_y = d3.scale.linear().domain([0, @svg_width/2]).range([height_available, height_second_split])
            # Draw the bottom lines
            @svg.selectAll('line-in-'+level)
                .data(sub_tasks)
                .enter()
                .append('line')
                .attr('x1', (sub_task, i) ->
                    return that.drawable_machines_map_position[sub_task.machine_id]
                )
                .attr('x2', (sub_task, i) ->
                    return that.drawable_machines_map_position[sub_task.machine_id]
                )
                .attr('y1', (sub_task, i) ->
                    return parent_position.y+
                        scale_traffic(sub_task['size_message_received'])+
                        scale_execution(sub_task['execution_duration'])
                )
                .attr('y2', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .style('stroke', '#000')
                .style('stroke-width', @stroke_width)
            @svg.selectAll('line-in-'+level)
                .data(sub_tasks)
                .enter()
                .append('line')
                .attr('x1', (sub_task, i) ->
                    return that.drawable_machines_map_position[task.machine_id]+scale_x(that.drawable_machines_map_position[sub_task.machine_id])
                )
                .attr('x2', (sub_task, i) ->
                    return that.drawable_machines_map_position[sub_task.machine_id]
                )
                .attr('y1', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .attr('y2', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .style('stroke', '#000')
                .style('stroke-width', @stroke_width)
            @svg.selectAll('line-in-'+level)
                .data(sub_tasks)
                .enter()
                .append('line')
                .attr('x1', (sub_task, i) ->
                    return that.drawable_machines_map_position[task.machine_id]+scale_x(that.drawable_machines_map_position[sub_task.machine_id])
                )
                .attr('x2', (sub_task, i) ->
                    return that.drawable_machines_map_position[task.machine_id]+scale_x(that.drawable_machines_map_position[sub_task.machine_id])
                )
                .attr('y1', (sub_task, i) ->
                    if that.drawable_machines_map_position[sub_task.machine_id] > that.svg_width/2
                        y = that.svg_width-that.drawable_machines_map_position[sub_task.machine_id]
                    else
                        y = that.drawable_machines_map_position[sub_task.machine_id]
                    return parent_position.y+scale_y(y)
                )
                .attr('y2', (sub_task, i) ->
                    return parent_position.y+height_available
                )
                .style('stroke', '#000')
                .style('stroke-width', @stroke_width)




            # Draw the rectangles
            for sub_task, i in sub_tasks
                @draw_component
                    task: sub_task
                    level: level+1
                    step: i
                    parent_position:
                        x: @drawable_machines_map_position[sub_task.machine_id]
                        y: parent_position.y+
                            scale_traffic(sub_task['size_message_received'])
                    num_simultaneous_task_drawn: num_simultaneous_task_drawn
                    height_available: scale_execution(sub_task['execution_duration'])
                    scale_traffic: scale_traffic
                    scale_execution: scale_execution
                    draw_traffic_in: false
                    draw_traffic_out: false


        destroy: =>
            #TODO Implement
            console.log 'TODO'

# Copyright 2010-2012 RethinkDB, all rights reserved.
# Namespace view
module 'NamespaceView', ->
    # Hardcoded!
    MAX_SHARD_COUNT = 32

    class @Sharding extends Backbone.View
        template: Handlebars.templates['shards_container-template']
        view_template: Handlebars.templates['view_shards-template']
        edit_template: Handlebars.templates['edit_shards-template']
        data_repartition_template: Handlebars.templates['data_repartition-template']
        feedback_template: Handlebars.templates['edit_shards-feedback-template']
        error_ajax_template: Handlebars.templates['edit_shards-ajax_error-template']
        alert_shard_template: Handlebars.templates['alert_shard-template']
        reasons_cannot_shard_template: Handlebars.templates['shards-reason_cannot_shard-template']
        shard_status_template: Handlebars.templates['shard_status-template']

        className: 'shards_container'

        state:
            0: 'read'
            1: 'edit'
    
        events:
            'keypress .num-shards': 'keypress_shards_changes'
            'click .edit': 'switch_to_edit'
            'click .cancel': 'switch_to_read'
            'click .rebalance': 'shard_table'


        initialize: =>
            # Forbid changes if issues
            @can_change_shards = false
            @reasons_cannot_shard = {}
            @expected_num_shards = @model.get('shards').length
            issues.on 'all', @check_can_change_shards

            # Listeners/cache for the data repartition
            @model.on 'change:key_distr', @render_data_repartition

            # Listeners for the progress bar
            @model.on 'change:shards', @global_trigger_for_shards  #This one contains render_data_repartition too
            directory.on 'all', @render_status

            @progress_bar = new UIComponents.OperationProgressBar @shard_status_template

        keypress_shards_changes: (event) =>
            if event.which is 13
                event.preventDefault()
                @shard_table()

        check_shards_changes: =>
            new_num_shards = @.$('.num-shards').val()

            if DataUtils.is_integer(new_num_shards) is false
                error_msg = "The number of shards must be an integer."
                @display_msg error_msg
                return

            if new_num_shards < 1 or new_num_shards > MAX_SHARD_COUNT
                error_msg = "The number of shards must be beteen 1 and " + MAX_SHARD_COUNT + "."
                @display_msg error_msg
                return

            data = @model.get('key_distr')
            distr_keys = @model.get('key_distr_sorted')
            total_rows = _.reduce distr_keys, ((agg, key) => return agg + data[key]), 0
            rows_per_shard = total_rows / new_num_shards

            if not data? or not distr_keys?
                error_msg = "The distribution of keys has not been loaded yet. Please try again."
                @display_msg error_msg
                return

            if distr_keys.length < new_num_shards
                error_msg = 'There is not enough data in the database to make this number of balanced shards.'
                @display_msg error_msg
                return

            current_count = 0 # Global count to spread errors among shards
            split_points = []
            no_more_splits = false
            for key, i in distr_keys
                if i is 0
                    if key isnt '' # For safety. The empty string is always the smallest key
                        return 'Error'
                    split_points.push key
                    current_count += data[key]
                    continue

                # If we have just enough keys left, we just add all of them as split points
                # keys_left <= splits_point left to add
                if distr_keys.length-i <= new_num_shards-split_points.length # We don't add 1 because there is null
                    split_points.push key
                    continue

                # Else we check if we have enough keys
                if current_count >= rows_per_shard*split_points.length
                    split_points.push key
                current_count += data[key]

            split_points.push(null)

            shard_set = []
            for splitIndex in [0..(split_points.length - 2)]
                shard_set.push(JSON.stringify([split_points[splitIndex], split_points[splitIndex + 1]]))

            @.$('.cannot_shard-alert').slideUp 'fast'
            @shard_set = shard_set
            return true

        display_msg: (msg) =>
            @.$('.cannot_shard-alert').html @feedback_template msg

            @.$('.cannot_shard-alert').slideDown 'fast'


        shard_table: (event) =>
            if event?
                event.preventDefault()
            if @check_shards_changes() is true
                if @sending? and @sending is true
                    return ''
                @sending = true
                @.$('.rebalance').prop 'disabled', true

                @empty_master_pin = {}
                @empty_replica_pins = {}
                for shard in @shard_set
                    @empty_master_pin[shard] = null
                    @empty_replica_pins[shard] = []

                data =
                    shards: @shard_set
                    primary_pinnings: @empty_master_pin
                    secondary_pinnings: @empty_replica_pins

                @data_sent = data

                # Indicate that we're starting the sharding process-- provide the number of shards requested
                @expected_num_shards = @shard_set.length
                @render_status
                    new_value: @shard_set.length

                # Indicate that we're going to start replicating on the replication taba
                new_replicas = @model.get('replica_affinities')
                replicas_length = 1 # 1 for the master
                for datacenter_id of new_replicas
                    replicas_length += new_replicas[datacenter_id]

                window.app.current_view.replicas.render_status
                    new_value: replicas_length*@model.get('shards').length

                $.ajax
                     processData: false
                     url: "ajax/semilattice/#{@model.attributes.protocol}_namespaces/" +
                         "#{@model.get('id')}?prefer_distribution_for=#{@model.get('id')}"
                     type: 'POST'
                     contentType: 'application/json'
                     data: JSON.stringify(data)
                     success: @on_success
                     error: @on_error

        on_success: =>
            @sending = false
            @.$('.rebalance').prop 'disabled', false

            @model.set 'shards', @shard_set
            @model.set 'primary_pinnings', @empty_master_pin
            @model.set 'secondary_pinnings', @empty_replica_pins

            @switch_to_read()

            # Indicate that we've gotten a response, time to move to the next state for both tabs (shards and replication)
            @render_status
                got_response: true
            # Replicas'tab turn now
            new_replicas = @model.get('replica_affinities')
            replicas_length = 1 # 1 for the master
            for datacenter_id of new_replicas
                replicas_length += new_replicas[datacenter_id]

            window.app.current_view.replicas.render_status
                got_response: true
                replicas_length: replicas_length
                shards_length: @model.get('shards').length



        on_error: =>
            @sending = false
            @.$('.rebalance').prop 'disabled', false


            @display_msg @error_ajax_template()

            # In case the ajax request failed, we remove the progress bar.
            @progress_bar.set_none_state()
            @expected_num_shards = @model.get('shards').length
            @render_status()

        # We can add just one method on one listener, so it's just a container. Triggered when shards have been changed
        global_trigger_for_shards: =>
            if @current_state is @state[0]
                @switch_to_read() # We are already reading, but we will also refresh the value of the number of shards
                
            @render_data_repartition()
            @render_status_server_update()

        # Shards or acks have been change (probably on another window, so let's skip the start and process things
        render_status_server_update: =>
            @expected_num_shards = @model.get('shards').length

            @progress_bar.skip_to_processing()
            @render_status()

        # Render the status of sharding
        #   - pbar_info: optional argument that informs the progress bar backing this status
        render_status: (progress_bar_info) =>
            # We check for the type because the listener on directory is called with the event type (which is a string like 'reset')
            if not progress_bar_info? or typeof progress_bar_info isnt 'object' 
                progress_bar_info = {}

            # If blueprint not ready, we just skip. It shouldn't happen.
            blueprint = @model.get('blueprint').peers_roles
            if not blueprint?
                return ''

            # Initialize an object tracking if for each shards we have found the master and the machines that are not ready/used
            # We will also use it to know if a shard is eventually available or not
            shards = {}
            num_shards = @model.get('shards').length
            for shard in @model.get('shards')
                shards[shard] =
                    master: null
                    machines_not_ready: {}
                    acks_expected: _.extend {}, @model.get('ack_expectations').expectation
                if (not (shards[shard]['acks_expected'][@model.get('primary_uuid')]?)) or shards[shard]['acks_expected'][@model.get('primary_uuid')] is 0
                    shards[shard]['acks_expected'][@model.get('primary_uuid')] = 1
        
            # Let's make sure that the bluprint has been regenerated. If not we just exit
            for machine_id of blueprint
                num_shards_in_blueprint = 0
                for shard of blueprint[machine_id]
                    num_shards_in_blueprint++
                if num_shards_in_blueprint isnt @expected_num_shards
                    @.$('.shard-status').html @progress_bar.render(0, @expected_num_shards, progress_bar_info).$el
                    return ''

            # Loop over the blueprint
            for machine_id of blueprint
                for shard of blueprint[machine_id]
                    # If there is a mismatch in the blueprint, it means that the blueprint has not been regenerated.
                    if not shards[shard]?
                        @.$('.shard-status').html @progress_bar.render(0, @expected_num_shards, progress_bar_info).$el
                        return ''

                    role = blueprint[machine_id][shard]

                    # This machine is doing nothing so let's save it
                    if role is 'role_nothing'
                        shards[shard]['machines_not_ready'][machine_id] = true
                        continue

                    # Because the back end doesn't match the terms in blueprint and directory
                    if role is 'role_primary'
                        expected_status = 'primary'
                    else if role is 'role_secondary'
                        expected_status = 'secondary_up_to_date'

                    # Loop over the directory
                    activities = directory.get(machine_id)?.get(@model.get('protocol')+'_namespaces')?['reactor_bcards'][@model.get('id')]?['activity_map']
                    if activities?
                        for activity_id of activities
                            activity = activities[activity_id]
                            if activity[0] is shard
                                if activity[1]['type'] isnt expected_status # The activity doesn't match the blueprint, so the machine is not ready
                                    shards[shard]['machines_not_ready'][machine_id] = true
                                else
                                    # The activity matched and it's a primary, so let's save it
                                    if activity[1]['type'] is 'primary'
                                        shards[shard].master = machine_id

            #TODO That is approximate since we cannot make sure that a machine is working for a datacenter or for universe
            for shard of shards
                for machine in machines.models
                    machine_id = machine.get('id')

                    # We have one machine up to date
                    if not shards[shard]['machines_not_ready'][machine_id]?
                        datacenter_id = machine.get('datacenter_uuid')
                        if (not shards[shard]['acks_expected'][datacenter_id]?) or shards[shard]['acks_expected'][datacenter_id] <= 0
                            shards[shard]['acks_expected'][universe_datacenter.get('id')]--
                        else
                            shards[shard]['acks_expected'][datacenter_id]--

            # Compute the number of shards that are ready
            num_shards_not_ready = 0
            for shard of shards
                master = shards[shard]['master']
                if master is null or (shards[shard]['machines_not_ready'][master]?) # If no master found or if the master is not completly ready yet.
                    num_shards_not_ready++
                    continue
                for datacenter_id of shards[shard]['acks_expected']
                    if shards[shard]['acks_expected'][datacenter_id] > 0 # We don't meeet the number of acks, so the shard is not ready yet
                        num_shards_not_ready++
                        break

            # The blueprint has been regenerated, so let's render everything
            num_shards_ready = num_shards-num_shards_not_ready
            @.$('.shard-status').html @progress_bar.render(num_shards_ready, num_shards, progress_bar_info).$el

            return @

        check_can_change_shards: =>
            reasons_cannot_shard = {}

            can_change_shards_now = true
            for issue in issues.models
                if issue.get('type') is 'UNSATISFIABLE_GOALS' and issue.get('namespace_id') is @model.get('id') # If unsatisfiable goals, the user should not change shards
                    can_change_shards_now = false
                    reasons_cannot_shard.unsatisfiable_goals = true
                if issue.get('type') is 'MACHINE_DOWN' # If a machine connected (even with a role nothing) is down, the user should not change shards
                    can_change_shards_now = false
                    reasons_cannot_shard.machine_down = true

            if @can_change_shards is true and can_change_shards_now is false
                @.$('.critical_error').html @reasons_cannot_shard_template @reasons_cannot_shard
                @.$('.critical_error').slideDown()
                @.$('.edit').prop 'disabled', true
                @.$('.rebalance').prop 'disabled', true
            else if @can_change_shards is false and can_change_shards_now is true
                @.$('.critical_error').hide()
                @.$('.critical_error').empty()
                @.$('.edit').prop 'disabled', false
                @.$('.rebalance').prop 'disabled', false
            else if @can_change_shards is false and can_change_shards_now is false
                if not _.isEqual reasons_cannot_shard, @reasons_cannot_shard
                    @reasons_cannot_shard = reasons_cannot_shard
                    @.$('.critical_error').html @reasons_cannot_shard_template @reasons_cannot_shard
                    @.$('.critical_error').slideDown()
                # Just for safety
                @.$('.edit').prop 'disabled', true
                @.$('.rebalance').prop 'disabled', true

        render: =>
            @.$el.html @template({})
            @switch_to_read()
            @render_status()
            @check_can_change_shards()
            return @

        switch_to_read: =>

            
            @current_state = @state[0]
            @.$('.edit-shards').html @view_template
                num_shards: @model.get('shards').length

        switch_to_edit: =>
            if not @model.get('key_distr_sorted')?
                max_shards = 1
            else
                max_shards = Math.min 32, @model.get('key_distr_sorted').length
                max_shards = Math.max 1, max_shards

            @current_state = @state[1]
            @.$('.edit-shards').html @edit_template
                num_shards: @model.get('shards').length
                max_shards: max_shards
            @.$('.num-shards').focus()

        render_data_repartition: =>
            $('.tooltip').remove()

            # Compute the data we need do draw the graph
            shards = []
            total_keys = 0
            for shard in @model.get('shards')
                new_shard =
                    boundaries: shard
                    num_keys: parseInt @model.compute_shard_rows_approximation shard
                shards.push new_shard
                total_keys += new_shard.num_keys

            max_keys = d3.max shards, (d) -> return d.num_keys
            min_keys = d3.min shards, (d) -> return d.num_keys

            if @model.get('key_distr')? and max_keys? and not _.isNaN max_keys and shards.length isnt 0 # Should not happen

                @.$('.data_repartition-container').html @data_repartition_template()
                @.$('.loading_text-diagram').css 'display', 'none'

                svg_width = 328 # Width of the whole svg
                svg_height = 270 # Height of the whole svg

                margin_width = 20 # Margin on left of the y-axis
                margin_height = 20 # Margin under the x-axis
                min_margin_width_inner = 20 # Min margin on the right of the y-axis

                # We have two special cases
                if shards.length is 1
                    bar_width = 100
                    margin_bar = 20
                else if shards.length is 2
                    bar_width = 80
                    margin_bar = 20
                else #80% for the bar, 20% for the margin
                    bar_width = Math.floor(0.8*(328-margin_width*2-min_margin_width_inner*2)/shards.length)
                    margin_bar = Math.floor(0.2*(328-margin_width*2-min_margin_width_inner*2)/shards.length)

                # size of all bars and white space between bars
                width_of_all_bars = bar_width*shards.length + margin_bar*(shards.length-1)

                # Update the margin on the right of the y-axis
                margin_width_inner = Math.floor((svg_width-margin_width*2-width_of_all_bars)/2)

                # Y scale
                y = d3.scale.linear().domain([0, max_keys]).range([1, svg_height-margin_height*2.5])

                # Add svg
                svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')

                # Add rectangle
                svg.selectAll('rect').data(shards)
                    .enter()
                    .append('rect')
                    .attr('class', 'rect')
                    .attr('x', (d, i) ->
                        return margin_width+margin_width_inner+bar_width*i+margin_bar*i
                    )
                    .attr('y', (d) -> return svg_height-y(d.num_keys)-margin_height-1) #-1 not to overlap with axe
                    .attr('width', bar_width)
                    .attr( 'height', (d) -> return y(d.num_keys))
                    .attr( 'title', (d) ->
                        keys = $.parseJSON(d.boundaries)
                        for key, i in keys
                            keys[i] = pretty_key key
                            if typeof keys[i] is 'number'
                                keys[i] = keys[i].toString()
                            if typeof keys[i] is 'string'
                                if keys[i].length > 7
                                    keys[i] =keys[i].slice(0, 7)+'...'

                        result = 'Shard: '
                        result += '[ '+keys[0]+', '+keys[1]+']'
                        result += '<br />~'+d.num_keys+' keys'
                        return result
                    )


                # Create axes
                extra_data = []
                extra_data.push
                    x1: margin_width
                    x2: margin_width
                    y1: margin_height
                    y2: svg_height-margin_height

                extra_data.push
                    x1: margin_width
                    x2: svg_width-margin_width
                    y1: svg_height-margin_height
                    y2: svg_height-margin_height

                svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')
                svg.selectAll('line').data(extra_data).enter().append('line')
                    .attr('x1', (d) -> return d.x1)
                    .attr('x2', (d) -> return d.x2)
                    .attr('y1', (d) -> return d.y1)
                    .attr('y2', (d) -> return d.y2)

                # Create legend
                axe_legend = []
                axe_legend.push
                    x: margin_width
                    y: Math.floor(margin_height/2)
                    string: 'Docs'
                    anchor: 'middle'
                axe_legend.push
                    x: Math.floor(svg_width/2)
                    y: svg_height
                    string: 'Shards'
                    anchor: 'middle'

                svg.selectAll('.legend')
                    .data(axe_legend).enter().append('text')
                    .attr('x', (d) -> return d.x)
                    .attr('y', (d) -> return d.y)
                    .attr('text-anchor', (d) -> return d.anchor)
                    .text((d) -> return d.string)

                # Create ticks
                # First the tick's background
                svg.selectAll('.cache').data(y.ticks(5))
                    .enter()
                    .append('rect')
                    .attr('width', (d, i) ->
                        if i is 0
                            return 0 # We don't display 0
                        return 4
                    )
                    .attr('height',18)
                    .attr('x', margin_width-2)
                    .attr('y', (d) -> svg_height-margin_height-y(d)-14)
                    .attr('fill', '#fff')

                # Then the actual tick's value
                svg.selectAll('.rule').data(y.ticks(5))
                    .enter()
                    .append('text')
                    .attr('x', margin_width)
                    .attr('y', (d) -> svg_height-margin_height-y(d))
                    .attr('text-anchor', "middle")
                    .text((d, i) ->
                        if i isnt 0
                            return d # We don't display 0
                        else
                            return ''
                    )

                @.$('rect').tooltip
                    trigger: 'hover'

                @delegateEvents()
            return @

        destroy: =>
            issues.off 'all', @check_can_change_shards
            @model.off 'change:key_distr', @render_data_repartition
            @model.off 'change:shards', @global_trigger_for_shards
            directory.off 'all', @render_status

    # Modify replica counts and ack counts in each datacenter
    class @ChangeShardsModal extends UIComponents.AbstractModal
        ###
        template: Handlebars.templates['change_shards-modal-template']
        change_shards_success_alert_template: Handlebars.templates['change_shards-success-alert-template']
        ###
        class: 'change_shards'

        initialize: (data) =>
            @model = data.model
            @parent = data.parent
            super

        set_num_shards: (num_shards) =>
            @num_shards = num_shards

        render: =>
            super
                modal_title: "Confirm rebalancing shards"
                btn_primary_text: "Shard"
                num_shards: @num_shards
            $('.btn-primary').focus()


        display_error: (error) =>
            @.$('.alert-error').css 'display', 'none'
            @.$('.alert-error').html error
            @.$('.alert-error').slideDown 'fast'
            @reset_buttons()

        on_submit: =>
            super


        on_success: =>
            @parent.$('.user-alert-space').css 'display', 'none'
            @parent.$('.user-alert-space').html @change_shards_success_alert_template({})
            @parent.$('.user-alert-space').slideDown 'fast'
            super

            #Update data // assuming no bug on the server...
            namespaces.get(@model.get('id')).set @data_sent

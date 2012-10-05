# Namespace view
module 'NamespaceView', ->
    # Hardcoded!
    MAX_SHARD_COUNT = 32

    class @Sharding extends Backbone.View
        template: Handlebars.compile $('#shards_container-template').html()
        view_template: Handlebars.compile $('#view_shards-template').html()
        edit_template: Handlebars.compile $('#edit_shards-template').html()
        data_repartition_template: Handlebars.compile $('#data_repartition-template').html()
        feedback_template: Handlebars.compile $('#edit_shards-feedback-template').html()
        error_ajax_template: Handlebars.compile $('#edit_shards-ajax_error-template').html()
        successfuly_shard_template: Handlebars.compile $('#edit_shards-successfully_shard-template').html()
        className: 'shards_container'

        events:
            'keyup .num-shards': 'keypress_shards_changes'
            'click .edit': 'switch_to_edit'
            'click .cancel': 'switch_to_read'
            'click .rebalance': 'shard_table'

        initialize: =>
            # Forbid changes if issues
            issues.on 'all', @check_has_unsatisfiable_goals
            @check_has_unsatisfiable_goals()

            # Listeners/cache for the data repartition
            @model.on 'change:key_distr', @render_data_repartition
            @model.on 'change:shards', @render_data_repartition


        keypress_shards_changes: (event) =>
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

            if distr_keys.length < 2
                error_msg = "There is not enough data in the database to make balanced shards."
                @display_msg error_msg
                return

            current_shard_count = 0
            split_points = [""]
            no_more_splits = false
            for key in distr_keys
                if split_points.length >= new_num_shards
                    no_more_splits = true
                current_shard_count += data[key]
                if current_shard_count >= rows_per_shard and not no_more_splits
                    split_points.push(key)
                    current_shard_count = 0
            split_points.push(null)

            shard_set = []
            for splitIndex in [0..(split_points.length - 2)]
                shard_set.push(JSON.stringify([split_points[splitIndex], split_points[splitIndex + 1]]))

            if shard_set.length < new_num_shards
                error_msg = "There is only enough data to make " + shard_set.length + " balanced shards."
                @display_msg error_msg
                return

            if event?.which? and event.which is 13
                @shard_table()

            @.$('.cannot_shard-alert').slideUp 'fast'
            @shard_set = shard_set
            return true

        display_msg: (msg) =>
            @.$('.cannot_shard-alert').html @feedback_template msg

            @.$('.cannot_shard-alert').slideDown 'fast'


        shard_table: (event) =>
            if event?
                event.preventDefault()
            if @keypress_shards_changes() is true

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

                $.ajax
                     processData: false
                     url: "/ajax/semilattice/#{@model.attributes.protocol}_namespaces/#{@model.get('id')}"
                     type: 'POST'
                     contentType: 'application/json'
                     data: JSON.stringify(data)
                     success: @on_success
                     error: @on_error

        on_success: =>
            @model.set 'shards', @shard_set
            @model.set 'primary_pinnings', @empty_master_pin
            @model.set 'secondary_pinnings', @empty_replica_pins

            @model.load_key_distr_once()

            @switch_to_read()
            @display_msg @successfuly_shard_template
                name: @model.get 'name'
                num_shards: @model.get('shards').length

        on_error: =>
            @display_msg @error_ajax_template()

        check_has_unsatisfiable_goals: =>
            if @should_be_hidden
                should_be_hidden_new = false
                for issue in issues.models 
                    if issue.get('type') is 'UNSATISFIABLE_GOALS' and issue.get('namespace_id') is @model.get('id') # If unsatisfiable goals, the user should not change shards
                        should_be_hidden_new = true
                        break
                    if issue.get('type') is 'MACHINE_DOWN' # If a machine connected (even with a role nothing) is down, the user should not change shards
                        if machines.get(issue.get('victim')).get('datacenter_uuid') of @model.get('replica_affinities')
                            should_be_hidden_new = true
                            break

                if not should_be_hidden_new
                    @should_be_hidden = false
                    @render()
            else
                for issue in issues.models
                    if issue.get('type') is 'UNSATISFIABLE_GOALS' and issue.get('namespace_id') is @model.get('id')
                        @should_be_hidden = true
                        @render()
                        break
                    if issue.get('type') is 'MACHINE_DOWN'
                        if machines.get(issue.get('victim')).get('datacenter_uuid') of @model.get('replica_affinities')
                            @should_be_hidden = true
                            @render()
                            break

        render: =>
            @.$el.html @template({})
            @switch_to_read()
            return @

        switch_to_read: =>
            @.$('.edit-shards').html @view_template
                num_shards: @model.get('shards').length

        switch_to_edit: =>
            @.$('.edit-shards').html @edit_template
                num_shards: @model.get('shards').length

        render_data_repartition: =>
            $('.tooltip').remove()
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

            #TODO remove data. We don't need it anymore.
            data = {}
            if shards.length > 1
                data.has_shards = true
                if shards.length > 9
                    data.numerous_shards = true
            else
                data.has_shards = false

            if @model.get('key_distr')? and max_keys? and not _.isNaN max_keys and shards.length isnt 0 # Should not happen

                @.$('.data_repartition-container').html @data_repartition_template data
                @.$('.loading_text-diagram').css 'display', 'none'
                margin_width = 20
                margin_height = 20

                margin_width_inner = 20
                width = 28
                margin_bar = 2

                svg_height = 270

                if data.numerous_shards? and data.numerous_shards is true
                    svg_width = margin_width+margin_width_inner+(width+margin_bar)*@model.get('shards').length+margin_width_inner+margin_width
                    svg_width = Math.min svg_width, 350
                    width_and_margin = (svg_width-margin_width*2-margin_width_inner*2)/@model.get('shards').length
                    width = width_and_margin/@model.get('shards').length*(@model.get('shards').length-2)
                    margin_bar = width_and_margin/@model.get('shards').length*2
                    container_width = svg_width
                else
                    svg_width = margin_width+margin_width_inner+(width+margin_bar)*@model.get('shards').length+margin_width_inner+margin_width
                    container_width = Math.max svg_width, 350


                @.$('.shard-graph').css('width', container_width+'px')
                y = d3.scale.linear().domain([0, max_keys]).range([1, svg_height-margin_height*2.5])

                svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')
                svg.selectAll('rect').data(shards)
                    .enter()
                    .append('rect')
                    .attr('x', (d, i) ->
                        return margin_width+margin_width_inner+(width+margin_bar)*i
                    )
                    .attr('y', (d) -> return svg_height-y(d.num_keys)-margin_height-1) #-1 not to overlap with axe
                    .attr('width', width)
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
                        result += '<br />'+d.num_keys+' keys'
                        return result
                    )
            

                arrow_width = 4
                arrow_length = 7
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

                axe_legend = []
                axe_legend.push
                    x: margin_width
                    y: Math.floor(margin_height/2)
                    string: 'Keys'
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

                @.$('rect').tooltip
                    trigger: 'hover'

                @delegateEvents()
            return @

        destroy: =>
            issues.on 'all', @check_has_unsatisfiable_goals
            @model.off 'change:shards', @check_num_shards_changed
            @model.off 'change:key_distr', @render_data_repartition
            @model.off 'change:shards', @render_data_repartition




    # Modify replica counts and ack counts in each datacenter
    class @ChangeShardsModal extends UIComponents.AbstractModal
        ###
        template: Handlebars.compile $('#change_shards-modal-template').html()
        change_shards_success_alert_template: Handlebars.compile $('#change_shards-success-alert-template').html()
        ###
        class: 'change_shards'

        initialize: (data) =>
            @model = data.model
            @parent = data.parent
            super

        set_num_shards: (num_shards) =>
            @num_shards = num_shards

        render: =>
            @model.load_key_distr_once()

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

# Helper stuff
Handlebars.registerPartial 'shard_name', $('#shard_name-partial').html()

# Namespace view
module 'NamespaceView', ->
    # Hardcoded!
    MAX_SHARD_COUNT = 32

    # All of our rendering code needs a view of available shards
    compute_renderable_shards_array = (namespace_uuid, shards) ->
        ret = []
        for i in [0...shards.length]
            primary_uuid = DataUtils.get_shard_primary_uuid namespace_uuid, shards[i]
            secondary_uuids = DataUtils.get_shard_secondary_uuids namespace_uuid, shards[i]
            ret.push
                name: human_readable_shard shards[i]
                shard: shards[i]
                shard_stats:
                    rows_approx: namespaces.get(namespace_uuid).compute_shard_rows_approximation(shards[i])
                notlast: i != shards.length - 1
                index: i
                primary:
                    uuid: primary_uuid
                    # primary_uuid may be null when a new shard hasn't hit the server yet
                    name: if primary_uuid then machines.get(primary_uuid).get('name') else null
                    status: if primary_uuid then DataUtils.get_machine_reachability(primary_uuid) else null
                    backfill_progress: if primary_uuid then DataUtils.get_backfill_progress(namespace_uuid, shards[i], primary_uuid) else null
                secondaries: _.map(secondary_uuids, (machine_uuids, datacenter_uuid) ->
                    datacenter_uuid: datacenter_uuid
                    datacenter_name: datacenters.get(datacenter_uuid).get('name')
                    machines: _.map(machine_uuids, (machine_uuid) ->
                        uuid: machine_uuid
                        name: machines.get(machine_uuid).get('name')
                        status: DataUtils.get_machine_reachability(machine_uuid)
                        backfill_progress: DataUtils.get_backfill_progress(namespace_uuid, shards[i], machine_uuid)
                        )
                    )
        return ret


    # Shards view
    # The sharding view consists of several lists. Here's the hierarchy:
    #   @Sharding
    #   |-- @Shard
    #   |   `-- @ShardDatacenterList: primary and secondary replicas organized by datacenter
    #   |       `-- @ShardDatacenter
    #   |           `-- @ShardMachineList: machines with primary and secondary replicas
    #   |               `-- @ShardMachine
    class @Sharding extends UIComponents.AbstractList
        className: 'namespace-shards'
        template: Handlebars.compile $('#namespace_view-sharding-template').html()
        error_msg: Handlebars.compile $('#namespace_view-sharding_alert-template').html()

        events: ->
            'click .change-sharding-scheme': 'change_sharding_scheme'
        should_be_hidden: false

        initialize: ->
            super @model.get('computed_shards'), NamespaceView.Shard, 'table.shards tbody',
                element_args:
                    namespace: @model
            @render()

            @check_has_unsatisfiable_goals()
            issues.on 'all', @check_has_unsatisfiable_goals

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
            super()

            @.$el.toggleClass('namespace-shards-blackout', @should_be_hidden)
            @.$('.blackout').toggleClass('blackout-active', @should_be_hidden)
            @.$('.alert_for_sharding').toggle @should_be_hidden
            @.$('.alert_for_sharding').html @error_msg if @should_be_hidden

            return @

        change_sharding_scheme: (event) =>
            event.preventDefault()
            @.$('.sharding').hide()
            view = new NamespaceView.ModifyShards @model.get('id')
            @.$('.change-shards').html view.render().el

        destroy: ->
            super()
            issues.off()

    class @Shard extends Backbone.View
        tagName: 'tr'
        template: Handlebars.compile $('#namespace_view-shard-template').html()
        summary_template: Handlebars.compile $('#namespace_view-shard-summary-template').html()

        initialize: ->
            @namespace = @options.args.namespace

            @datacenter_list = new NamespaceView.ShardDatacenterList datacenters, NamespaceView.ShardDatacenter, 'div.datacenters',
                filter: (datacenter) =>
                    return true
                ###
                filter: (datacenter) =>
                    for datacenter_uuid of @model.get('secondary_uuids')
                        return true if datacenter.get('id') is datacenter_uuid
                    if @model.get('primary_uuid')
                        return true if datacenter.get('id') is machines.get(@model.get('primary_uuid')).get('datacenter_uuid')
                ###
                element_args:
                    shard: @model
                    namespace: @namespace

            @namespace.on 'change:key_distr_sorted', @render_summary
            
            @namespace.on 'change:blueprint', @reset_datacenter_list #TODO bind to peers_roles

        render: =>
            @.$el.html @template({})
            @render_summary()
            @.$('.datacenter-list').html @datacenter_list.render().el
            return @

        render_summary: =>
            @.$('.shard.summary').html @summary_template
                name: human_readable_shard @model.get('shard_boundaries')
                shard_stats:
                    rows_approx: @namespace.compute_shard_rows_approximation(@model.get('shard_boundaries'))

        reset_datacenter_list: =>
            @datacenter_list.render()

        destroy: =>
            @datacenter_list.destroy()
            @namespace.off()

    class @ShardDatacenterList extends UIComponents.AbstractList
        template: Handlebars.compile $('#namespace_view-shard_datacenter_list-template').html()

            

    class @ShardDatacenter extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#namespace_view-shard_datacenter-template').html()
        summary_template: Handlebars.compile $('#namespace_view-shard_datacenter-summary-template').html()

        initialize: ->
            super

            @namespace = @options.args.namespace

            @machine_list = new NamespaceView.ShardMachineList machines, NamespaceView.ShardMachine, 'ul.machines',
                filter: (machine) =>
                    shard = @options.args.shard
                    return false if machine.get('datacenter_uuid') isnt @model.get('id')
                    for datacenter_uuid, machine_uuids of shard.get('secondary_uuids')
                        return true if machine.get('id') in machine_uuids
                    if shard.get('primary_uuid')
                        return true if machine.get('id') is shard.get('primary_uuid')
                sort: (a, b) =>
                    a_is_master = a.model.get('id') is @options.args.shard.get('primary_uuid')
                    b_is_master = b.model.get('id') is @options.args.shard.get('primary_uuid')
                    a_name = a.model.get('name')
                    b_name = b.model.get('name')

                    return -1 if a_is_master > b_is_master
                    return 1 if a_is_master < b_is_master

                    return -1 if a_name < b_name
                    return 1 if a_name > b_name
                    return 0
                element_args:
                    shard: @options.args.shard
                    namespace: @options.args.namespace
                    datacenter: @model

            @model.on 'change', @render_summary
            directory.on 'all', @render_summary
            @namespace.on 'change:replica_affinities', @reset_list
            @namespace.on 'change:secondary_pinnings', @reset_list
            @namespace.on 'change:blueprint', @reset_list

        reset_list: =>
            @machine_list.reset_element_views()
            @machine_list.render()
            @render()

        render: =>
            if @machine_list.element_views.length > 0 # We don't display empty datacenter here.
                @.$el.html @template({})
                @render_summary()
                @.$('.machine-list').html @machine_list.render().el

            super

            return @

        render_summary: =>
            json = _.extend @model.toJSON(),
                status: DataUtils.get_datacenter_reachability(@model.get('id'))

            @.$('.datacenter.summary').html @summary_template json

        destroy: =>
            @model.off()
            directory.off()
            @namespace.off()

    class @ShardMachineList extends UIComponents.AbstractList
        template: Handlebars.compile $('#namespace_view-shard_machine_list-template').html()

        initialize: ->
            super
            @namespace = @options.element_args.namespace
            @namespace.on 'change:primary_pinnings', @render

        destroy: =>
            @machine_list.off()
            @namespace.off()

    class @ShardMachine extends Backbone.View
        template: Handlebars.compile $('#namespace_view-shard_machine-template').html()
        change_machine_popover: Handlebars.compile $('#namespace_view-shard_machine-assign_machine_popover-template').html()
        alert_tmpl: Handlebars.compile $('#edit_machines-alert-template').html()
        tagName: 'li'

        events: ->
            'click a[rel=popover]': 'show_popover'
            'click .make-master': 'make_master'

        initialize: ->
            @shard = @options.args.shard
            @namespace = @options.args.namespace
            @datacenter = @options.args.datacenter

            @shard.on 'change:primary_uuid', @render
            @shard.on 'change:secondary_uuids', @render
            @model.on 'change:name', @render
            directory.on 'all', @render
            #@namespace.on 'change:primary_pinnings', => @list.render()

        show_popover: (event) =>
            event.preventDefault()
            popover_link = @.$(event.currentTarget).popover('show')
            $popover = $('.popover')
            $popover_button = $('.btn.change-machine', $popover)

            $('.chosen-dropdown', $popover).chosen()
            $popover.on 'clickoutside', (event) -> $(event.currentTarget).remove()
            $popover_button.on 'click', (event) =>
                event.preventDefault()

                post_data = {}
                new_pinnings = _.without @get_currently_pinned(), @model.get('id')
                new_pinnings.push $('select.chosen-dropdown', $popover).val()
                post_data[@shard.get('shard_boundaries')] = new_pinnings

                $.ajax
                    processData: false
                    url: "/ajax/#{@namespace.get("protocol")}_namespaces/#{@namespace.get('id')}/secondary_pinnings"
                    type: 'POST'
                    data: JSON.stringify(post_data)
                    success: (response) =>
                        # Trigger a manual refresh of the data
                        collect_server_data =>
                            $popover.remove()
                            $('#user-alert-space').append (@alert_tmpl {})
                    #TODO: Handle error
                $popover_button.button('loading')

        make_master: (event) =>
            event.preventDefault()

            # New primary pinning for this shard should be this machine
            post_data = {}
            post_data[@shard.get('shard_boundaries')] = @model.get('id')

            # Present a confirmation dialog to make sure the user actually wants this
            confirmation_modal = new UIComponents.ConfirmationDialogModal
            confirmation_modal.render("Are you sure you want to make machine #{@model.get('name')} the master for this shard in datacenter #{@datacenter.get('name')}?",
                "/ajax/semilattice/memcached_namespaces/#{@namespace.get('id')}/primary_pinnings",
                JSON.stringify(post_data),
                (response) =>
                    # Set the link's text to a loading state
                    $link = @.$('a.make-master')
                    $link.text $link.data('loading-text')

                    clear_modals()
                    $('#user-alert-space').append (@alert_tmpl {})
                    
                    # Trigger a manual refresh of the data
                    collect_server_data(true)
            )

        get_available_machines_in_datacenter: =>
            machines_in_datacenter = DataUtils.get_datacenter_machines(@model.get('datacenter_uuid'))
            # Machines that are unused (neither primary nor secondary replicas for this shard)
            available_machines = _.filter machines_in_datacenter, (machine) =>
                return false if machine.get('id') is @shard.get('primary_uuid')
                for datacenter_uuid, machine_uuids of @shard.get('secondary_uuids')
                    return false if machine.get('id') in machine_uuids
                return true
            return _.map available_machines, (machine) ->
                name: machine.get('name')
                id: machine.get('id')

        get_currently_pinned: =>
            # Grab pinned machines for this shard
            for shard, pins of @namespace.get('secondary_pinnings')
                return pins if JSON.stringify(@shard.get('shard_boundaries')) is JSON.stringify(shard)

        render: =>
            @.$el.html @template _.extend @model.toJSON(),
                status: DataUtils.get_machine_reachability(@model.get('id'))
                is_master: @model.get('id') is @options.args.shard.get('primary_uuid')
                cannot_be_master: @options.args.datacenter.get('id') isnt @options.args.namespace.get('primary_uuid')
                backfill_progress: DataUtils.get_backfill_progress(@namespace.get('id'), @shard.get('shard_boundaries'), @model.get('id'))
                no_available_machines: @get_available_machines_in_datacenter().length is 0

            # Popover to change the machine
            @.$('a[rel=popover]').popover
                html: true
                trigger: 'manual'
                content: @change_machine_popover
                    available_machines: @get_available_machines_in_datacenter()

            @.delegateEvents()
            return @

        destroy: =>
            @shard.off()
            @shard.off()
            @model.off()
            directory.off()

    # A view for modifying the sharding plan.
    class @ModifyShards extends Backbone.View
        template: Handlebars.compile $('#modify_shards-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()
        invalid_splitpoint_msg: Handlebars.compile $('#namespace_view_invalid_splitpoint_alert-template').html()
        invalid_merge_msg: Handlebars.compile $('#namespace_view_invalid_merge_alert-template').html()
        invalid_shard_msg: Handlebars.compile $('#namespace_view_invalid_shard_alert-template').html()

        class: 'modify-shards'
        events:
            'click #suggest_shards_btn': 'suggest_shards'
            'click #cancel_shards_suggester_btn': 'cancel_shards_suggester_btn'
            'click .btn-compute-shards-suggestion': 'compute_shards_suggestion'
            'click .btn-reset': 'reset_shards'
            'click .btn-primary-commit': 'on_submit'
            'click .btn-cancel': 'cancel'

        initialize: (namespace_id, shard_set) ->
            log_initial '(initializing) view: ModifyShards'
            @namespace_id = namespace_id
            @namespace = namespaces.get(@namespace_id)
            shard_set = @namespace.get('shards')

            # Keep an unmodified copy of the shard boundaries with which we compare against when reviewing the changes.
            @original_shard_set = _.map(shard_set, _.identity)
            @shard_set = _.map(shard_set, _.identity)

            # Shard suggester business
            @suggest_shards_view = false

            # We should rerender on key distro updates
            @namespace.on 'all', @render_only_shards

            super

        cancel: (e) ->
            e.preventDefault()
            @.undelegateEvents()
            @.remove()
            $('.sharding').show()

        reset_shards: (e) ->
            e.preventDefault()
            @shard_set = _.map(@original_shard_set, _.identity)
            @render()

        reset_button_enable: ->
            @.$('.btn-reset').button('reset')

        reset_button_disable: ->
            @.$('.btn-reset').button('loading')

        suggest_shards: (e) =>
            e.preventDefault()
            @suggest_shards_view = true
            @render()


        compute_shards_suggestion: (e) =>
            # Do the bullshit event crap
            e.preventDefault()
            @.$('.btn-compute-shards-suggestion').button('loading')

            # Make sure their input aint crazy
            if DataUtils.is_integer(form_data_as_object($('form', @.el)).num_shards) is false
                @error_msg = "The number of shards must be an integer."
                @desired_shards = form_data_as_object($('form', @.el)).num_shards
                @render()
                return
            @desired_shards = parseInt(form_data_as_object($('form', @.el)).num_shards) #It's safe to use parseInt now
            if @desired_shards < 1 or @desired_shards > MAX_SHARD_COUNT
                @error_msg = "The number of shards must be beteen 1 and " + MAX_SHARD_COUNT + "."
                @render()
                return

            # grab the data
            data = @namespace.get('key_distr')
            distr_keys = @namespace.sorted_key_distr_keys(data)
            total_rows = _.reduce distr_keys, ((agg, key) => return agg + data[key]), 0
            rows_per_shard = total_rows / @desired_shards

            # Make sure there is enough data to actually suggest stuff
            if distr_keys.length < 2
                @error_msg = "There isn't enough data in the database to suggest shards."
                @render()
                return

            # Phew. Go through the keys now and compute the bitch ass split points.
            current_shard_count = 0
            split_points = [""]
            no_more_splits = false
            for key in distr_keys
                # Let's not overdo it :-D
                if split_points.length >= @desired_shards
                    no_more_splits = true
                current_shard_count += data[key]
                if current_shard_count >= rows_per_shard and not no_more_splits
                    # Hellz yeah ho, we got our split point
                    split_points.push(key)
                    current_shard_count = 0
            split_points.push(null)

            # convert split points into whatever bitch ass format we're using here
            _shard_set = []
            for splitIndex in [0..(split_points.length - 2)]
                _shard_set.push(JSON.stringify([split_points[splitIndex], split_points[splitIndex + 1]]))

            # See if we have enough shards
            if _shard_set.length < @desired_shards
                @error_msg = "There is only enough data to suggest " + _shard_set.length + " shards."
                @render()
                return
            @shard_set = _shard_set

            # All right, we be done, boi. Put those
            # motherfuckers into the dialog, reset the buttons
            # or whateva, and hop on into the sunlight.
            @.$('.btn-compute-shards-suggestion').button('reset')
            @render()

        cancel_shards_suggester_btn: (e) =>
            e.preventDefault()
            @suggest_shards_view = false
            @render()

        insert_splitpoint: (index, splitpoint) =>
            if (0 <= index || index < @shard_set.length)
                json_repr = $.parseJSON(@shard_set[index])
                if (splitpoint <= json_repr[0] || (splitpoint >= json_repr[1] && json_repr[1] != null))
                    @.$('.invalid_shard_alert').html @invalid_splitpoint_msg
                    @.$('.invalid_shard_alert').css('display', 'block')
                    return
                @shard_set.splice(index, 1, JSON.stringify([json_repr[0], splitpoint]), JSON.stringify([splitpoint, json_repr[1]]))
                @render()
            else
                @.$('.invalid_shard_alert').html @invalid_shard_msg
                @.$('.invalid_shard_alert').css('display', 'block')


        merge_shard: (index) =>
            if (index < 0 || index + 1 >= @shard_set.length)
                @.$('.invalid_shard_alert').html @invalid_splitpoint_msg
                @.$('.invalid_shard_alert').css('display', 'block')
                return

            newshard = JSON.stringify([$.parseJSON(@shard_set[index])[0], $.parseJSON(@shard_set[index+1])[1]])
            @shard_set.splice(index, 2, newshard)
            @render()

        render: =>

            log_render '(rendering) ModifyShards'

            # TODO render "touched" shards (that have been split or merged within this view) a bit differently
            user_made_changes = JSON.stringify(@original_shard_set).replace(/\s/g, '') isnt JSON.stringify(@shard_set).replace(/\s/g, '')
            json =
                namespace: @namespace.toJSON()
                shards: compute_renderable_shards_array(@namespace.get('id'), @shard_set)
                suggest_shards_view: @suggest_shards_view
                current_shards_count: @original_shard_set.length
                max_shard_count: MAX_SHARD_COUNT
                new_shard_count: if @desired_shards? then @desired_shards else @shard_set.length
                unsaved_settings: user_made_changes
                error_msg: @error_msg
            @error_msg = null

            shard_views = _.map(compute_renderable_shards_array(@namespace.get('id'), @shard_set), (shard) => new NamespaceView.ModifySingleShard @namespace, shard, @)
            @.$el.html(@template json)

            @render_only_shards

            @.$('.shards tbody').html ''
            @.$('.shards tbody').append view.render().el for view in shard_views

            # Control the suggest button
            @.$('.btn-compute-shards-suggestion').button()

            # Control the reset button, boiii
            if user_made_changes
                @reset_button_enable()
            else
                @reset_button_disable()
            $('#focus_num_shards').focus()
            return @

        render_only_shards: =>
            shard_views = _.map(compute_renderable_shards_array(@namespace.get('id'), @shard_set), (shard) => new NamespaceView.ModifySingleShard @namespace, shard, @)
            @.$('.shards tbody').html ''
            @.$('.shards tbody').append view.render().el for view in shard_views


        on_submit: (e) =>
            e.preventDefault()
            @.$('.btn-primary').button('loading')
            formdata = form_data_as_object($('form', @el))

            empty_master_pin = {}
            empty_master_pin[JSON.stringify(["", null])] = null
            empty_replica_pins = {}
            empty_replica_pins[JSON.stringify(["", null])] = []
            json =
                shards: @shard_set
                primary_pinnings: empty_master_pin
                secondary_pinnings: empty_replica_pins
            # TODO detect when there are no changes.
            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@namespace.attributes.protocol}_namespaces/#{@namespace.id}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(json)
                success: @on_success

        on_success: (response) =>
            @.$('.btn-primary').button('reset')
            namespaces.get(@namespace.id).set(response)
            @.$el.remove()
            $('.namespace-view .sharding').show()
            $('#user-alert-space').append(@alert_tmpl({}))

        destroy: =>
            @namespace.off()

    # A view for modifying a specific shard
    class @ModifySingleShard extends Backbone.View
        template: Handlebars.compile $('#modify_shards-view_shard-template').html()
        editable_tmpl: Handlebars.compile $('#modify_shards-edit_shard-template').html()

        tagName: 'tr'
        class: 'shard'

        events: ->
            'click .split': 'split'
            'click .commit_split': 'commit_split'
            'click .cancel_split': 'cancel_split'
            'click .merge': 'merge'
            'click .commit_merge': 'commit_merge'
            'click .cancel_merge': 'cancel_merge'
            'keypress .splitpoint_input': 'check_if_enter_press'

        initialize: (namespace, shard, parent_modal) ->
            @namespace = namespace
            @shard = shard
            @parent = parent_modal

        render: ->
            @.$el.html @template
                shard: @shard

            return @

        check_if_enter_press: (event) =>
            if event.which is 13
                event.preventDefault()
                @commit_split(event)

        split: (e) ->
            shard_index = $(e.target).data 'index'
            log_action "split button clicked with index #{shard_index}"

            @.$el.html @editable_tmpl
                splitting: true
                shard: @shard

            e.preventDefault()

        cancel_split: (e) ->
            @render()
            e.preventDefault()

        commit_split: (e) ->
            splitpoint = $('input[name=splitpoint]', @el).val()
            # TODO validate splitpoint
            e.preventDefault()

            @parent.insert_splitpoint(@shard.index, splitpoint)

        merge: (e) =>
            @.$el.html @editable_tmpl
                merging: true
                shard: @shard
            e.preventDefault()

        cancel_merge: (e) =>
            @render()
            e.preventDefault()

        commit_merge: (e) =>
            @parent.merge_shard(@shard.index)
            e.preventDefault()


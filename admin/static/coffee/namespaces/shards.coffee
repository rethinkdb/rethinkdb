# Namespace view
module 'NamespaceView', ->
    # All of our rendering code needs a view of available shards
    compute_renderable_shards_array = (namespace_uuid, shards) ->
        ret = []
        for i in [0...shards.length]
            primary_uuid = DataUtils.get_shard_primary_uuid namespace_uuid, shards[i]
            secondary_uuids = DataUtils.get_shard_secondary_uuids namespace_uuid, shards[i]
            ret.push
                name: human_readable_shard shards[i]
                shard: shards[i]
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

        events: ->
            'click .edit': 'modify_shards'

        modify_shards: (e) ->
            log_action 'modify shards clicked'

            modal = new NamespaceView.ModifyShardsModal(@model, @model.get('shards'))
            modal.render()
            e.preventDefault()

        initialize: ->
            log_initial '(initializing) namespace view: shards'
            #progress_list.on 'all', @render

            super @model.get('computed_shards'), NamespaceView.Shard, 'table.shards tbody',
                element_args:
                    namespace: @model

    class @Shard extends Backbone.View
        tagName: 'tr'
        template: Handlebars.compile $('#namespace_view-shard-template').html()

        initialize: ->
            @datacenter_list = new NamespaceView.ShardDatacenterList datacenters, NamespaceView.ShardDatacenter, 'div.datacenters',
                filter: (datacenter) =>
                    for datacenter_uuid of @model.get('secondary_uuids')
                        return true if datacenter.get('id') is datacenter_uuid
                    if @model.get('primary_uuid')
                        return true if datacenter.get('id') is machines.get(@model.get('primary_uuid')).get('datacenter_uuid')
                element_args:
                    shard: @model
                    namespace: @options.args.namespace

        render: =>
            @.$el.html @template
                name: human_readable_shard @model.get('shard_boundaries')

            @.$('.datacenter-list').html @datacenter_list.render().el
            return @

    class @ShardDatacenterList extends UIComponents.AbstractList
        template: Handlebars.compile $('#namespace_view-shard_datacenter_list-template').html()
        
    class @ShardDatacenter extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#namespace_view-shard_datacenter-template').html()
        summary_template: Handlebars.compile $('#namespace_view-shard_datacenter-summary-template').html()

        initialize: ->
            super

            @machine_list = new NamespaceView.ShardMachineList machines, NamespaceView.ShardMachine, 'ul.machines',
                filter: (machine) =>
                    shard = @options.args.shard
                    return false if machine.get('datacenter_uuid') isnt @model.get('id')
                    for datacenter_uuid, machine_uuids of shard.get('secondary_uuids')
                        return true if machine.get('id') in machine_uuids
                    if shard.get('primary_uuid')
                        return true if machine.get('id') is shard.get('primary_uuid')
                element_args:
                    shard: @options.args.shard
                    namespace: @options.args.namespace
                    datacenter: @model

            window.testing = @machine_list
            @model.on 'change', @render_summary
            directory.on 'all', @render_summary

        render: =>
            @.$el.html @template({})
            @render_summary()
            @.$('.machine-list').html @machine_list.render().el

            super

            return @

        render_summary: =>
            json = _.extend @model.toJSON(),
                status: DataUtils.get_datacenter_reachability(@model.get('id'))

            @.$('.datacenter.summary').html @summary_template json

    class @ShardMachineList extends UIComponents.AbstractList
        template: Handlebars.compile $('#namespace_view-shard_machine_list-template').html()

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
                        $popover.remove()
                        @namespace.set(response)
                        $('#user-alert-space').append (@alert_tmpl {})
                $popover_button.button('loading')

        make_master: (event) =>
            event.preventDefault()

            # New primary pinning for this shard should be this machine
            post_data = {}
            post_data[@shard.get('shard_boundaries')] = @model.get('id')

            # Present a confirmation dialog to make sure the user actually wants this
            confirmation_modal = new UIComponents.ConfirmationDialogModal
            confirmation_modal.render("Are you sure you want to make machine #{@model.get('name')} the master for this shard in datacenter #{@datacenter.get('name')}?",
                "/ajax/memcached_namespaces/#{@namespace.get('id')}/primary_pinnings",
                JSON.stringify(post_data),
                (response) =>
                    clear_modals()
                    @namespace.set(response)
                    $('#user-alert-space').append (@alert_tmpl {})
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
                backfill_progress: DataUtils.get_backfill_progress(@namespace.get('id'), @shard.get('shard_boundaries'), @model.get('id'))

            # Popover to change the machine
            @.$('a[rel=popover]').popover
                html: true
                trigger: 'manual'
                content: @change_machine_popover
                    available_machines: @get_available_machines_in_datacenter()
            return @

    class @ModifyShardsModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#modify_shards_modal-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()
        class: 'modify-shards'
        events: ->
            _.extend super,
                'click #suggest_shards_btn': 'suggest_shards'
                'click #cancel_shards_suggester_btn': 'cancel_shards_suggester_btn'
                'click .btn-compute-shards-suggestion': 'compute_shards_suggestion'

        initialize: (namespace, shard_set) ->
            log_initial '(initializing) modal dialog: ModifyShards'
            @namespace = namespace

            # Keep an unmodified copy of the shard boundaries with which we compare against when reviewing the changes.
            @original_shard_set = _.map(shard_set, _.identity)
            @shard_set = _.map(shard_set, _.identity)

            # Shard suggester business
            @suggest_shards_view = false

            super

            @add_custom_button "Reset", "btn-reset", "Reset", (e) =>
                e.preventDefault()
                @shard_set = _.map(@original_shard_set, _.identity)
                clear_modals()
                @render()

        reset_button_enable: ->
            @find_custom_button('btn-reset').button('reset')

        reset_button_disable: ->
            @find_custom_button('btn-reset').button('loading')

        suggest_shards: (e) =>
            e.preventDefault()
            @suggest_shards_view = true
            clear_modals()
            @render()

        compute_shards_suggestion: (e) =>
            # Do the bullshit event crap
            e.preventDefault()
            @.$('.btn-compute-shards-suggestion').button('loading')

            # grab the data ho, 'cause it's all about the data, it's
            # all about the data, the data it's all about ho, yes it
            # is.
            $.ajax
                processData: false
                url: "/ajax/distribution?namespace=#{@namespace.id}&depth=2"
                type: 'GET'
                success: (data) =>
                    # we got the data ho, we got the data. Put that
                    # shit into the array 'cause who knows if maps
                    # preserve order, god reset their souls.
                    distr_keys = []
                    total_rows = 0
                    for key, count of data
                        distr_keys.push(key)
                        total_rows += count
                    _.sortBy(distr_keys, _.identity)

                    # All right, now let's see roughly how many bitch
                    # ass rows we want per bitch ass shard.
                    formdata = form_data_as_object($('form', @.el))
                    rows_per_shard = total_rows / formdata.num_shards
                    # Phew. Go through the keys now and compute the bitch ass split points.
                    current_shard_count = 0
                    split_points = [""]
                    for key in distr_keys
                        current_shard_count += data[key]
                        if current_shard_count >= rows_per_shard
                            # Hellz yeah ho, we got our split point
                            split_points.push(key)
                            current_shard_count = 0
                    split_points.push(null)
                    # convert split points into whatever bitch ass format we're using here
                    @shard_set = []
                    for splitIndex in [0..(split_points.length - 2)]
                        @shard_set.push(JSON.stringify([split_points[splitIndex], split_points[splitIndex + 1]]))
                    # All right, we be done, boi. Put those
                    # motherfuckers into the dialog, reset the buttons
                    # or whateva, and hop on into the sunlight.
                    @.$('.btn-compute-shards-suggestion').button('reset')
                    clear_modals()
                    @render()

        cancel_shards_suggester_btn: (e) =>
            e.preventDefault()
            @suggest_shards_view = false
            clear_modals()
            @render()

        insert_splitpoint: (index, splitpoint) =>
            if (0 <= index || index < @shard_set.length)
                json_repr = $.parseJSON(@shard_set[index])
                if (splitpoint <= json_repr[0] || (splitpoint >= json_repr[1] && json_repr[1] != null))
                    throw "Error invalid splitpoint"

                @shard_set.splice(index, 1, JSON.stringify([json_repr[0], splitpoint]), JSON.stringify([splitpoint, json_repr[1]]))
                clear_modals()
                @render()
            else
                # TODO handle error

        merge_shard: (index) =>
            if (index < 0 || index + 1 >= @shard_set.length)
                throw "Error invalid index"

            newshard = JSON.stringify([$.parseJSON(@shard_set[index])[0], $.parseJSON(@shard_set[index+1])[1]])
            @shard_set.splice(index, 2, newshard)
            clear_modals()
            @render()

        render: =>
            log_render '(rendering) ModifyShards dialog'

            # TODO render "touched" shards (that have been split or merged within this modal) a bit differently
            json =
                namespace: @namespace.toJSON()
                shards: compute_renderable_shards_array(@namespace.get('id'), @shard_set)
                modal_title: 'Change sharding scheme'
                btn_primary_text: 'Commit'
                suggest_shards_view: @suggest_shards_view
                current_shards_count: @original_shard_set.length
                new_shard_count: @shard_set.length

            super json

            shard_views = _.map(compute_renderable_shards_array(@namespace.get('id'), @shard_set), (shard) => new NamespaceView.ModifyShardsModalShard @namespace, shard, @)
            @.$('.shards tbody').append view.render().el for view in shard_views

            # Control the suggest button
            @.$('.btn-compute-shards-suggestion').button()

            # Control the reset button, boiii
            if JSON.stringify(@original_shard_set).replace(/\s/g, '') is JSON.stringify(@shard_set).replace(/\s/g, '')
                @reset_button_disable()
            else
                @reset_button_enable()

        on_submit: ->
            super
            formdata = form_data_as_object($('form', @$modal))
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
                url: "/ajax/#{@namespace.attributes.protocol}_namespaces/#{@namespace.id}"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(json)
                success: @on_success

        on_success: (response) ->
            super
            namespaces.get(@namespace.id).set(response)
            $('#user-alert-space').append(@alert_tmpl({}))

    # A modal for modifying sharding plan
    class @ModifyShardsModalShard extends Backbone.View
        template: Handlebars.compile $('#modify_shards_modal-shard-template').html()
        editable_tmpl: Handlebars.compile $('#modify_shards_modal-edit_shard-template').html()

        tagName: 'tr'
        class: 'shard'

        events: ->
            'click .split': 'split'
            'click .commit_split': 'commit_split'
            'click .cancel_split': 'cancel_split'
            'click .merge': 'merge'
            'click .commit_merge': 'commit_merge'
            'click .cancel_merge': 'cancel_merge'

        initialize: (namespace, shard, parent_modal) ->
            @namespace = namespace
            @shard = shard
            @parent = parent_modal

        render: ->
            @.$el.html @template
                shard: @shard

            return @

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

            @parent.insert_splitpoint(@shard.index, splitpoint);

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


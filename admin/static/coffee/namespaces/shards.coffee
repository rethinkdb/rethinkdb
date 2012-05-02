
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
    class @Shards extends Backbone.View
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
            @model.on 'all', @render
            directory.on 'all', @render
            progress_list.on 'all', @render
            @shards = []

        bind_edit_machines: (shard) ->
            # Master assignments
            @.$('#assign_master_' + shard.index).click (e) =>
                e.preventDefault()
                modal = new NamespaceView.EditMasterMachineModal @model, shard.shard
                modal.render()

            # Fucking JS closures
            bind_shard = (shard, secondary) =>
                @.$('#assign_machines_' + shard.index + '_' + secondary.datacenter_uuid).click (e) =>
                    e.preventDefault()
                    modal = new NamespaceView.EditReplicaMachinesModal @model, _.extend secondary,
                        name: shard.name
                        shard: shard.shard
                    modal.render()

            for secondary in shard.secondaries
                bind_shard shard, secondary

        render: =>
            log_render '(rendering) namespace view: shards'

            shards_array = compute_renderable_shards_array(@model.get('id'), @model.get('shards'))
            @.$el.html @template
                shards: shards_array

            # Bind events to 'assign machine' links
            for shard in shards_array
                @bind_edit_machines(shard)

            return @

    class @ModifyShardsModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#modify_shards_modal-template').html()
        alert_tmpl: Handlebars.compile $('#modify_shards-alert-template').html()
        class: 'modify-shards'

        initialize: (namespace, shard_set) ->
            log_initial '(initializing) modal dialog: ModifyShards'
            @namespace = namespace
            @shards = compute_renderable_shards_array(namespace.get('id'), shard_set)

            # Keep an unmodified copy of the shard boundaries with which we compare against when reviewing the changes.
            @original_shard_set = _.map(shard_set, _.identity)
            @shard_set = _.map(shard_set, _.identity)
            super

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

            super json

            shard_views = _.map(compute_renderable_shards_array(@namespace.get('id'), @shard_set), (shard) => new NamespaceView.ModifyShardsModalShard @namespace, shard, @)
            @.$('.shards tbody').append view.render().el for view in shard_views

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
            console.log 'split event fired'
            shard_index = $(e.target).data 'index'
            log_action "split button clicked with index #{shard_index}"

            @.$el.html @editable_tmpl
                splitting: true
                shard: @shard

            e.preventDefault()

        cancel_split: (e) ->
            console.log 'cancel_split event fired'
            @render()
            e.preventDefault()

        commit_split: (e) ->
            console.log 'commit split event fired'
            splitpoint = $('input[name=splitpoint]', @el).val()
            # TODO validate splitpoint
            console.log 'splitpoint is', splitpoint
            e.preventDefault()

            @parent.insert_splitpoint(@shard.index, splitpoint);

        merge: (e) =>
            console.log 'merge event fired'

            shard_index = parseInt $(e.target).data 'index'
            log_action "merge button clicked with index #{shard_index}"
            shards = @namespace.get('shards')
            low_shard = human_readable_shard(shards[shard_index])
            high_shard = human_readable_shard(shards[shard_index + 1])

            console.log 'shards to be merged: ',[low_shard, high_shard, shards, shard_index]

            @.$el.html @editable_tmpl
                merging: true
                shard: @shard
                low_shard: low_shard
                high_shard: high_shard

            e.preventDefault()

        cancel_merge: (e) =>
            console.log 'cancel_merge event fired'
            @render()
            e.preventDefault()

        commit_merge: (e) =>
            console.log 'commit_merge event fired'

            @parent.merge_shard(@shard.index)

            e.preventDefault()


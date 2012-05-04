# Machine view
module 'MachineView', ->
    # Container
    class @Container extends Backbone.View
        className: 'machine-view'
        template: Handlebars.compile $('#machine_view-container-template').html()

        events: ->
            'click a.set-datacenter': 'set_datacenter'
            'click a.rename-machine': 'rename_machine'

        initialize: (id) =>
            log_initial '(initializing) machine view: container'
            @machine_uuid = id

        rename_machine: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

        wait_for_model_noop: =>
            return true

        wait_for_model: =>
            @model = machines.get(@machine_uuid)
            if not @model
                machines.off 'all', @render
                machines.on 'all', @render
                return false

            # Model is finally ready, bind necessary handlers
            machines.off 'all', @render
            @model.on 'all', @render
            directory.on 'all', @render

            # Everything has been set up, we don't need this logic any
            # more
            @wait_for_model = @wait_for_model_noop

            return true

        render_empty: =>
            @.$el.text 'Machine ' + @machine_uuid + ' is not available.'
            return @

        render: =>
            log_render '(rendering) machine view: container'

            if not @wait_for_model()
                return @render_empty()

            datacenter_uuid = @model.get('datacenter_uuid')
            json =
                name: @model.get('name')
                ip: "192.168.1.#{Math.round(Math.random() * 255)}" # Fake IP, replace with real data TODO
                datacenter_uuid: datacenter_uuid

            # If the machine is assigned to a datacenter, add relevant json
            if datacenter_uuid?
                json = _.extend json,
                    assigned_to_datacenter: datacenter_uuid
                    datacenter_name: datacenters.get(datacenter_uuid).get('name')

            # If the machine is reachable, add relevant json
            _namespaces = []
            for namespace in namespaces.models
                _shards = []
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                _shards[_shards.length] =
                                    role: role
                                    shard: shard
                                    name: human_readable_shard shard
                if _shards.length > 0
                    _namespaces[_namespaces.length] =
                        shards: _shards
                        name: namespace.get('name')
                        uuid: namespace.id

            json = _.extend json,
                data:
                    namespaces: _namespaces

            # Reachability
            _.extend json,
                status: DataUtils.get_machine_reachability(@model.get('id'))

            @.$el.html @template json

            if @model.get('log_entries')?
                @model.get('log_entries').each (log_entry) =>
                    view = new MachineView.RecentLogEntry
                        model: log_entry
                    @.$('.recent-log-entries').append view.render().el

            return @

        set_datacenter: (event) =>
            event.preventDefault()
            set_datacenter_modal = new ServerView.SetDatacenterModal
            set_datacenter_modal.render [@model]

    # MachineView.RecentLogEntry
    class @RecentLogEntry extends Backbone.View
        className: 'recent-log-entry'
        template: Handlebars.compile $('#machine_view-recent_log_entry-template').html()

        events: ->
            'click a[rel=popover]': 'do_nothing'

        do_nothing: (event) -> event.preventDefault()

        render: =>
            json = _.extend @model.toJSON(), @model.get_formatted_message()
            @.$el.html @template _.extend json,
                machine_name: machines.get(@model.get('machine_uuid')).get('name')
                timeago_timestamp: @model.get_iso_8601_timestamp()

            @.$('abbr.timeago').timeago()
            @.$('a[rel=popover]').popover
                html: true
            return @

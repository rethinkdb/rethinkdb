# Machine view
module 'MachineView', ->
    # Container
    class @Container extends Backbone.View
        className: 'machine-view'
        template: Handlebars.compile $('#machine_view-container-template').html()

        events: ->
            'click a.set-datacenter': 'set_datacenter'
            'click a.rename-machine': 'rename_machine'
            'click .close': 'close_alert'

        max_log_entries_to_render: 3

        initialize: (id) =>
            log_initial '(initializing) machine view: container'
            @machine_uuid = id


        rename_machine: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()
            @title.update()


        wait_for_model_noop: =>
            return true

        wait_for_model: =>
            @model = machines.get(@machine_uuid)
            if not @model
                machines.off 'all', @render
                machines.on 'all', @render
                return false

            # Model is finally ready, unbind unnecessary handlers
            machines.off 'all', @render

            # Everything has been set up, we don't need this logic any more
            @wait_for_model = @wait_for_model_noop

            return true

        render_empty: =>
            @.$el.text 'Machine ' + @machine_uuid + ' is not available.'
            return @

        render: =>

            log_render '(rendering) machine view: container'

            if not @wait_for_model()
                return @render_empty()

            @title = new MachineView.Title(@machine_uuid)
            @profile = new MachineView.Profile(@machine_uuid)
            @data = new MachineView.Data(@machine_uuid)
            
            stats = @model.get_stats_for_performance
            @performance_graph = new Vis.OpsPlot(stats)
            
            @stats_panel = new Vis.StatsPanel(stats)

            # create main structure
            @.$el.html @template

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # fill the profile (name, reachable + performance)
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display stats sparklines
            @.$('.machine-stats').html @stats_panel.render().$el

            # display the data on the machines
            @.$('.data').html @data.render().$el


            # log entries
            if @model.get('log_entries')?
                entries_to_render = []
                @model.get('log_entries').each (log_entry) =>
                    entries_to_render.push(new MachineView.RecentLogEntry
                        model: log_entry)
                entries_to_render = entries_to_render.slice(0, @max_log_entries_to_render)
                @.$('.recent-log-entries').append entry.render().el for entry in entries_to_render

            return @

        set_datacenter: (event) =>
            event.preventDefault()
            set_datacenter_modal = new ServerView.SetDatacenterModal
            set_datacenter_modal.render [@model]

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

    # MachineView.Title
    class @Title extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.compile $('#machine_view_title-template').html()
        initialize: (model_id) =>
            @id = model_id
            @name = machines.get(@id).get('name')
            machines.on 'all', @update
        
        update: =>
            if @name isnt machines.get(@id).get('name')
                @name = machines.get(@id).get('name')
                @render()

        render: =>
            json =
                name: @name
            @.$el.html @template(json)
            return @

    # MachineView.Profile
    class @Profile extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.compile $('#machine_view_profile-template').html()
        initialize: (model_id) =>
            @id = model_id
            @model = machines.get(model_id)
            @model.on 'all', @render
            directory.on 'all', @render
            machines.on 'all', @render

        render: =>
            datacenter_uuid = @model.get('datacenter_uuid')
            ips = if directory.get(@model.get('id'))? then directory.get(@model.get('id')).get('ips') else null
            json =
                name: @model.get('name')
                ips: ips
                nips: if ips then ips.length else 1
                uptime: $.timeago(new Date(Date.now() - @model.get_stats().proc.uptime * 1000)).slice(0, -4)
                datacenter_uuid: datacenter_uuid
                global_cpu_util: Math.floor(@model.get_stats().proc.global_cpu_util_avg * 100)
                global_mem_total: human_readable_units(@model.get_stats().proc.global_mem_total * 1024, units_space)
                global_mem_used: human_readable_units(@model.get_stats().proc.global_mem_used * 1024, units_space)
                global_net_sent: if @model.get_stats().proc.global_net_sent_persec? then human_readable_units(@model.get_stats().proc.global_net_sent_persec.avg, units_space) else 0
                global_net_recv: if @model.get_stats().proc.global_net_recv_persec? then human_readable_units(@model.get_stats().proc.global_net_recv_persec.avg, units_space) else 0
                machine_disk_space: human_readable_units(@model.get_used_disk_space(), units_space)
            
            # If the machine is assigned to a datacenter, add relevant json
            if datacenter_uuid?
                json = _.extend json,
                    assigned_to_datacenter: datacenter_uuid
                    datacenter_name: datacenters.get(datacenter_uuid).get('name')


            # Reachability
            _.extend json,
                status: DataUtils.get_machine_reachability(@model.get('id'))
            @.$el.html @template(json)



            return @

    class @Data extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.compile $('#machine_view_data-template').html()

        initialize: (model_id) =>
            @model = machines.get(model_id)

            @model.on 'all', @render
            directory.on 'all', @render

        render: =>
            json = {}
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
            
            @.$el.html @template(json)
            return @



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

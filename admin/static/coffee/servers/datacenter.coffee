# Datacenter view
module 'DatacenterView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#datacenter_view-not_found-template').html()
        initialize: (id) -> @id = id
        render: =>
            @.$el.html @template id: @id
            return @

    # Container
    class @Container extends Backbone.View
        className: 'datacenter-view'
        template: Handlebars.compile $('#datacenter_view-container-template').html()
        events: ->
            'click .tab-link': 'change_route'
            'click a.rename-datacenter': 'rename_datacenter'
            'click .display_more_machines': 'expand_profile'
            'click .close': 'close_alert'

        max_log_entries_to_render: 3

        initialize: =>
            log_initial '(initializing) datacenter view: container'

            # Panels for datacenter view
            @title = new DatacenterView.Title(model: @model)
            @profile = new DatacenterView.Profile(model: @model)
            @data = new DatacenterView.Data(model: @model)
            @stats_panel = new Vis.StatsPanel(@model.get_stats_for_performance)
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance)

            filter = {}
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('id')
                    filter[machine.get('id')] = true
            @logs = new LogView.Container
                template_header: Handlebars.compile $('#log-header-datacenter-template').html()
                filter: filter
        
        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later.
            window.router.navigate @.$(event.target).attr('href')

        rename_datacenter: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render()
            @title.update()

        render: (tab) =>
            log_render('(rendering) datacenter view: container')

            @.$el.html @template
                datacenter_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # fill the profile (name, reachable...)
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display the data on the machines
            @.$('.datacenter-stats').html @stats_panel.render().$el

            # display the data on the machines
            @.$('.data').html @data.render().$el

            # Filter all the machines for those belonging to this datacenter and append logs
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

            # log entries
            @.$('.recent-log-entries').html @logs.render().$el

            @.$('.nav-tabs').tab()

            if tab?
                @.$('.active').removeClass('active')
                switch tab
                    when 'stats'
                        @.$('#datacenter-stats').addClass('active')
                        @.$('#datacenter-stats-link').tab('show')
                    when 'data'
                        @.$('#datacenter-data').addClass('active')
                        @.$('#datacenter-data-link').tab('show')
                    when 'logs'
                        @.$('#datacenter-logs').addClass('active')
                        @.$('#datacenter-logs-link').tab('show')
                    else
                        @.$('#datacenter-stats').addClass('active')
                        @.$('#datacenter-stats-link').tab('show')

            return @

        expand_profile: (event) ->
            event.preventDefault()
            @profile.more_link_should_be_displayed = false
            @.$('.more_machines').remove()
            @.$('.profile-expandable').css('overflow', 'auto')
            @.$('.profile-expandable').css('height', 'auto')

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        
        destroy: =>
            @title.destroy()
            @profile.destroy()
            @data.destroy()
            @stats_panel.destroy()
            @performance_graph.destroy()
            @logs.destroy()

        
    # DatacenterView.Title
    class @Title extends Backbone.View
        className: 'datacenter-info-view'
        template: Handlebars.compile $('#datacenter_view_title-template').html()
        initialize: =>
            @name = @model.get('name')
            datacenters.on 'all', @update

        update: =>
            if @name isnt @model.get('name')
                @name = @model.get('name')
                @render()

        render: =>
            @.$el.html @template
                name: @name
            return @

        destroy: ->
            datacenters.off 'all', @update

    class @Profile extends Backbone.View
        className: 'datacenter-info-view'

        template: Handlebars.compile $('#datacenter_view_profile-template').html()
        initialize: =>

            @model.on 'all', @render
            machines.on 'all', @render
            directory.on 'all', @render

            @more_link_should_be_displayed = true

        render: =>
            # Filter all the machines for those belonging to this datacenter
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

            # Data residing on this lovely datacenter
            _namespaces = []
            for namespace in namespaces.models
                _shards = []
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machines.get(machine_uuid).get('datacenter_uuid') is @model.get('id')
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                _shards[_shards.length] =
                                    role: role
                                    shard: shard
                                    name: human_readable_shard shard
                if _shards.length > 0
                    # Compute number of primaries and secondaries for each shard
                    __shards = {}
                    for shard in _shards
                        shard_repr = shard.shard.toString()
                        if not __shards[shard_repr]?
                            __shards[shard_repr] =
                                shard: shard.shard
                                name: human_readable_shard shard.shard
                                nprimaries: if shard.role is 'role_primary' then 1 else 0
                                nsecondaries: if shard.role is 'role_secondary' then 1 else 0
                        else
                            if shard.role is 'role_primary'
                                __shards[shard_repr].nprimaries += 1
                            if shard.role is 'role_secondary'
                                __shards[shard_repr].nsecondaries += 1

                    # Append the final data
                    _namespaces[_namespaces.length] =
                        shards: _.map(__shards, (shard, shard_repr) -> shard)
                        name: namespace.get('name')
                        uuid: namespace.id

            # make sure than the list of machines return contains first the one not reachable
            machines_returned = []
            for machine in machines_in_datacenter
                status = DataUtils.get_machine_reachability(machine.get('id'))
                if status.reachable
                    machines_returned.push
                        name: machine.get('name')
                        id: machine.get('id')
                        status: status
                else
                    machines_returned.unshift
                        name: machine.get('name')
                        id: machine.get('id')
                        status: status

            # Generate json and render
            json =
                name: @model.get('name')
                list_machines:
                    machines: machines_returned
                    more_link_should_be_displayed: @more_link_should_be_displayed
                status: DataUtils.get_datacenter_reachability(@model.get('id'))
            stats = @model.get_stats()
            json = _.extend json,
                global_cpu_util: Math.floor(stats.global_cpu_util_avg * 100)
                global_mem_total: human_readable_units(stats.global_mem_total * 1024, units_space)
                global_mem_used: human_readable_units(stats.global_mem_used * 1024, units_space)
                dc_disk_space: human_readable_units(stats.dc_disk_space, units_space)

            @.$el.html @template(json)

            return @

        destroy: =>
            @model.off 'all', @render
            machines.off 'all', @render
            directory.off 'all', @render


    class @Data extends Backbone.View
        className: 'datacenter-data-view'

        template: Handlebars.compile $('#datacenter_view_data-template').html()

        initialize: =>
            @model.on 'all', @render
            machines.on 'all', @render
            directory.on 'all', @render

        render: =>
            # Filter all the machines for those belonging to this datacenter
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

            # Data residing on this lovely datacenter
            _namespaces = []
            for namespace in namespaces.models
                _shards = []
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machines.get(machine_uuid).get('datacenter_uuid') is @model.get('id')
                        for shard, role of peer_roles
                            if role isnt 'role_nothing'
                                _shards[_shards.length] =
                                    role: role
                                    shard: shard
                                    name: human_readable_shard shard
                if _shards.length > 0
                    # Compute number of primaries and secondaries for each shard
                    __shards = {}
                    for shard in _shards
                        shard_repr = shard.shard.toString()
                        if not __shards[shard_repr]?
                            __shards[shard_repr] =
                                shard: shard.shard
                                name: human_readable_shard shard.shard
                                nprimaries: if shard.role is 'role_primary' then 1 else 0
                                nsecondaries: if shard.role is 'role_secondary' then 1 else 0
                        else
                            if shard.role is 'role_primary'
                                __shards[shard_repr].nprimaries += 1
                            if shard.role is 'role_secondary'
                                __shards[shard_repr].nsecondaries += 1

                    # Append the final data
                    _namespaces[_namespaces.length] =
                        shards: _.map(__shards, (shard, shard_repr) -> shard)
                        name: namespace.get('name')
                        uuid: namespace.id
            # Generate json and render
            json =
                data:
                    namespaces: _namespaces
            stats = @model.get_stats()
            json = _.extend json,
                global_cpu_util: Math.floor(stats.global_cpu_util_avg * 100)
                global_mem_total: human_readable_units(stats.global_mem_total * 1024, units_space)
                global_mem_used: human_readable_units(stats.global_mem_used * 1024, units_space)
                dc_disk_space: human_readable_units(stats.dc_disk_space, units_space)

            @.$el.html @template(json)

            return @

        destroy: =>
            @model.off 'all', @render
            machines.off 'all', @render
            directory.off 'all', @render

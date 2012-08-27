# Machine view
module 'MachineView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#machine_view-not_found-template').html()
        initialize: (id) -> @id = id
        render: =>
            @.$el.html @template id: @id
            return @

    # Container
    class @Container extends Backbone.View
        className: 'machine-view'
        template: Handlebars.compile $('#machine_view-container-template').html()

        events: ->
            'click .close': 'close_alert'
            'click .tab-link': 'change_route'

        initialize: =>
            log_initial '(initializing) machine view: container'

            # Panels for machine view
            @title = new MachineView.Title model: @model
            @profile = new MachineView.Profile model: @model
            @data = new MachineView.Data model: @model
            @assignments = new MachineView.Assignments model: @model
            @operations = new MachineView.Operations model: @model
            @overview = new MachineView.Overview model: @model
            @performance_graph = new Vis.OpsPlot @model.get_stats_for_performance
            @logs = new LogView.Container
                route: "/ajax/log/"+@model.get('id')+"_?"
                template_header: Handlebars.compile $('#log-header-machine-template').html()

        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later.
            window.router.navigate @.$(event.target).attr('href')

        render: (tab) =>
            log_render '(rendering) machine view: container'

            # create main structure
            @.$el.html @template
                server_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # fill the profile (name, reachable + performance)
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display stats sparklines
            @.$('.overview').html @overview.render().$el

            # display the data on the machines
            @.$('.data').html @data.render().$el

            # display the data on the machines
            @.$('.assignments').html @assignments.render().$el

            # display the operations
            @.$('.operations').html @operations.render().$el

            # log entries
            @.$('.recent-log-entries').html @logs.render().$el
            
            if tab?
                @.$('.active').removeClass('active')
                switch tab
                    when 'overview'
                        @.$('#machine-overview').addClass('active')
                        @.$('#machine-overview-link').tab('show')
                    when 'data'
                        @.$('#machine-data').addClass('active')
                        @.$('#machine-data-link').tab('show')
                    when 'assignments'
                        @.$('#machine-assignments').addClass('active')
                        @.$('#machine-assignments-link').tab('show')
                    when 'operations'
                        @.$('#machine-operations').addClass('active')
                        @.$('#machine-operations-link').tab('show')
                    when 'logs'
                        @.$('#machine-log_entries').addClass('active')
                        @.$('#machine-log_entries-link').tab('show')
                    else
                        @.$('#machine-statistics').addClass('active')
                        @.$('#machine-statistics-link').tab('show')
            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        destroy: =>
            @model.off 'change:name', @render
            @title.destroy()
            @profile.destroy()
            @data.destroy()
            @overview.destroy()
            @performance_graph.destroy()
            @logs.destroy()

    # MachineView.Title
    class @Title extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.compile $('#machine_view_title-template').html()
        initialize: =>
            @model.on 'change:name', @render
        
        render: =>
            @.$el.html @template
                name: @model.get('name')
            return @

        destroy: =>

            @model.off 'change:name', @update

    # MachineView.Profile
    class @Profile extends Backbone.View
        className: 'machine-info-view'
        template: Handlebars.compile $('#machine_view_profile-template').html()
        initialize: =>
            directory.on 'all', @render
            @model.on 'all', @render

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
                stats_up_to_date: @model.get('stats_up_to_date')
            
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

        destroy: =>
            directory.off 'all', @render
            @model.off 'all', @render


    class @Overview extends Backbone.View
        className: 'machine-overview'
        container_template: Handlebars.compile $('#machine_overview-container-template').html()
        pie_disk_template: Handlebars.compile $('#machine_overview-pie_disk-container-template').html()
        pie_ram_template: Handlebars.compile $('#machine_overview-pie_ram-container-template').html()

        initialize: =>
            @model.on 'change:stats', @render_pie_disk
            @model.on 'change:stats', @render_pie_ram
            @disk_data = {}
            @ram_data = {}

        render: =>
            @.$el.html @container_template
                is_assigned: @model.get('datacenter_uuid')?
                machine_id: @model.get 'id'
            @render_pie_disk()
            @render_pie_ram()

            #TODO: Add network and CPU if relevant?
            return @

        render_pie_disk: =>
            if @model.get('stats')? and @model.get('stats')['sys']?
                disk_rdb = 0
                for namespace in namespaces.models
                    if @model.get('stats')[namespace.get('id')]?
                        for key of @model.get('stats')[namespace.get('id')].serializers
                            if @model.get('stats')[namespace.get('id')].serializers[key].cache?.blocks_in_memory?
                                disk_rdb += parseInt(@model.get('stats')[namespace.get('id')].serializers[key].cache.blocks_total) * parseInt(@model.get('stats')[namespace.get('id')].serializers[key].cache.block_size)

                data =
                    disk_used: human_readable_units @model.get('stats')['sys']['global_disk_space_used'], units_space
                    disk_free: human_readable_units @model.get('stats')['sys']['global_disk_space_free'], units_space
                    disk_total: human_readable_units @model.get('stats')['sys']['global_disk_space_total'], units_space
                    disk_used_percent: Math.ceil(100*@model.get('stats')['sys']['global_disk_space_used']/@model.get('stats')['sys']['global_disk_space_total'])+'%'
                    disk_rdb: human_readable_units disk_rdb, units_space
                    disk_rdb_used_percent: Math.ceil(100*disk_rdb/@model.get('stats')['sys']['global_disk_space_used'])+'%'

                need_update = false
                for key of data
                    if not @disk_data[key]? or @disk_data[key] != data[key]
                        need_update = true
                        break
        
                if need_update
                    @.$('.pie_disk-container').html @pie_disk_template data

                    r = 80
                    width = 300
                    height = 200
                    color = (i) ->
                        if i is 0
                            return '#1f77b4'
                        else
                            return '#f00'
                                    
                    data_pie = [@model.get('stats')['sys']['global_disk_space_free'], @model.get('stats')['sys']['global_disk_space_used']]
        
                    if $('.pie_chart-disk').length is 1
                        @disk_data = data

                        arc = d3.svg.arc().innerRadius(0).outerRadius(r)
                        svg = d3.select('.pie_chart-disk').attr('width', width).attr('height', height).append('svg:g').attr('transform', 'translate('+width/2+', '+height/2+')')
                        arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc)
            else
                setTimeout(@render_pie_disk, 1000)

                data =
                    disk_used: 'Loading...'
                    disk_free: 'Loading...'
                    disk_total: 'Loading...'
                    disk_used_percent: 'Loading...'
                    disk_rdb: 'Loading...'
                    disk_rdb_used_percent: 'Loading...'

                @.$('.pie_disk-container').html @pie_disk_template data

        render_pie_ram: =>
            if @model.get('stats')? and @model.get('stats').proc? and @model.get('stats').proc.global_mem_used?
                ram_rdb = 0
                for namespace in namespaces.models
                    if @model.get('stats')[namespace.get('id')]?
                        for key of @model.get('stats')[namespace.get('id')].serializers
                            if @model.get('stats')[namespace.get('id')].serializers[key].cache?.blocks_in_memory?
                                ram_rdb += parseInt(@model.get('stats')[namespace.get('id')].serializers[key].cache.blocks_in_memory) * parseInt(@model.get('stats')[namespace.get('id')].serializers[key].cache.block_size)
                data =
                    ram_used: human_readable_units @model.get('stats').proc.global_mem_used*1024, units_space
                    ram_free: human_readable_units (@model.get('stats').proc.global_mem_total-@model.get('stats').proc.global_mem_used)*1024, units_space
                    ram_total: human_readable_units @model.get('stats').proc.global_mem_total*1024, units_space
                    ram_used_percent: Math.ceil(100*@model.get('stats').proc.global_mem_used/@model.get('stats').proc.global_mem_total)+'%'
                    ram_rdb: human_readable_units ram_rdb, units_space
                    ram_rdb_used_percent: Math.ceil(100*ram_rdb/(@model.get('stats').proc.global_mem_used*1024))+'%'

                need_update = false
                for key of data
                    if not @ram_data[key]? or @ram_data[key] != data[key]
                        need_update = true
                        break
        
                if need_update
                    @.$('.pie_ram-container').html @pie_ram_template data

                    r = 80
                    width = 300
                    height = 200
                    color = (i) ->
                        if i is 0
                            return '#1f77b4'
                        else
                            return '#f00'
                                    
                    data_pie = [@model.get('stats').proc.global_mem_total-@model.get('stats').proc.global_mem_used, @model.get('stats').proc.global_mem_used]
        
                    if $('.pie_chart-ram').length is 1
                        @ram_data = data

                        arc = d3.svg.arc().innerRadius(0).outerRadius(r)
                        svg = d3.select('.pie_chart-ram').attr('width', width).attr('height', height).append('svg:g').attr('transform', 'translate('+width/2+', '+height/2+')')
                        arcs = svg.selectAll('path').data(d3.layout.pie().sort(null)(data_pie)).enter().append('svg:path').attr('fill', (d,i) -> color(i)).attr('d', arc)
            else
                setTimeout(@render_pie_ram, 1000)

                data =
                    ram_used: 'Loading...'
                    ram_free: 'Loading...'
                    ram_total: 'Loading...'
                    ram_used_percent: 'Loading...'
                    ram_rdb: 'Loading...'
                    ram_rdb_used_percent: 'Loading...'

                @.$('.pie_ram-container').html @pie_ram_template data


        destroy: =>
            @model.on 'change:stats', @render_disk



    class @Data extends Backbone.View
        className: 'machine-data-view'
        template: Handlebars.compile $('#machine_view_data-template').html()

        initialize: =>
            @namespaces_with_listeners = {}
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
                                keys = namespace.compute_shard_rows_approximation shard
                                _shards[_shards.length] =
                                    shard: shard
                                    name: human_readable_shard shard
                                    keys: keys if typeof keys is 'string'
                if _shards.length > 0
                    if not @namespaces_with_listeners[namespace.get('id')]?
                        @namespaces_with_listeners[namespace.get('id')] = true
                        namespace.load_key_distr_once()
                        namespace.on 'change:key_distr', @render

                    _namespaces.push
                        shards: _shards
                        name: namespace.get('name')
                        uuid: namespace.get('id')

            json = _.extend json,
                data:
                    namespaces: _namespaces
        
            @.$el.html @template(json)
            return @

        destroy: =>
            for namespace_id of @namespaces_with_listeners
                namespaces.get(namespace_id).off 'change:key_distr', @render
                namespaces.get(namespace_id).clear_timeout()


    class @Assignments extends Backbone.View
        className: 'machine_assignments-view'
        template: Handlebars.compile $('#machine_view-assignments-template').html()
        alert_set_server_template: Handlebars.compile $('#alert-set_server-template').html()
        outdated_data_template: Handlebars.compile $('#outdated_data-template').html()

        reason_set_secondary_need_replica_template: Handlebars.compile $('#reason-set_secondary-need_replica-template').html()
        reason_set_nothing_set_secondary_first_template: Handlebars.compile $('#reason-set_nothing-set_secondary_first-template').html()
        reason_set_master_datacenter_not_primary_template: Handlebars.compile $('#reason-set_master-datacenter_not_primary-template').html()
        reason_set_nothing_unsatisfiable_goals_template: Handlebars.compile $('#reason-set_nothing-unsatisfiable_goals-template').html()
        reason_set_primary_set_secondary_first_template: Handlebars.compile $('#reason-set_primary-set_secondary_first-template').html()
        reason_set_primary_set_datecenter_as_primary_template: Handlebars.compile $('#reason-set_primary-set_datecenter_as_primary-template').html()
        reason_set_primary_no_datacenter_template: Handlebars.compile $('#reason-set_primary-no_datacenter-template').html()
        reason_set_secondary_increase_replicas_template: Handlebars.compile $('#reason-set_secondary-increase_replicas-template').html()

        events:
            'click .make_master': 'make_master'
            'click .make_secondary': 'make_secondary'
            'click .make_nothing': 'make_nothing'

        initialize: =>
            @directory_entry = directory.get @model.get 'id'
            @directory_entry.on 'change:memcached_namespaces', @render
            namespaces.on 'add', @render
            namespaces.on 'remove', @render
            namespaces.on 'reset', @render
            @namespaces_with_listeners = {}

        render: =>
            #Update the namespaces with listners on :shards
            for namespace in namespaces.models
                if not @namespaces_with_listeners[namespace.get('id')]?
                    @namespaces_with_listeners[namespace.get('id')] = true
                    namespace.on 'change:shards', @render
                    namespace.on 'change:primary_uuid', @render
                    namespace.on 'change:primary_pinnings', @render
                    namespace.on 'change:secondary_pinnings', @render
                    namespace.on 'change:replica_affinities', @render

            json = {}
            _namespaces = []
            for namespace in namespaces.models
                _shards = []
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        for shard, role of peer_roles
                            new_shard =
                                role: role
                                shard: shard
                                name: human_readable_shard shard
                                namespace_id: namespace.get('id')
                            
                            switch role
                                when 'role_primary'
                                    new_shard.display_master = false

                                    if namespace.get('replica_affinities')[@model.get('datacenter_uuid')] > 0
                                        new_shard.display_secondary = true
                                    else
                                        new_shard.display_secondary = true
                                        new_shard.display_secondary_desactivated = true
                                        new_shard.reason_secondary = @reason_set_secondary_need_replica_template({namespace_id: namespace.get('id')})

                                    new_shard.display_nothing = true
                                    new_shard.display_nothing_desactivated = true
                                    new_shard.reason_nothing = @reason_set_nothing_set_secondary_first_template({})


                                when 'role_secondary'
                                    #TODO We could check if a secondary is up to date here too (and not just in the modal)
                                    if namespace.get('primary_uuid') is @model.get('datacenter_uuid')
                                        new_shard.display_master = true
                                    else
                                        new_shard.display_master = true
                                        new_shard.display_master_desactivated = true
                                        new_shard.reason_master = @reason_set_master_datacenter_not_primary_template({namespace_id: namespace.get('id')})

                                    new_shard.display_secondary = false

                                    num_machines_in_datacenter = 0
                                    for machine in machines.models
                                        if machine.get('datacenter_uuid') is @model.get('datacenter_uuid')
                                            num_machines_in_datacenter++
                                    num_replicas = namespace.get('replica_affinities')[@model.get('datacenter_uuid')]
                                    if @model.get('datacenter_uuid') is namespace.get('primary_uuid')
                                        num_replicas++
                                    if num_machines_in_datacenter > num_replicas
                                        new_shard.display_nothing = true
                                    else
                                        new_shard.display_nothing = true
                                        new_shard.display_nothing_desactivated = true
                                        new_shard.reason_nothing = @reason_set_nothing_unsatisfiable_goals_template({namespace_id: namespace.get('id')})

                                when 'role_nothing'
                                    new_shard.display_master = true
                                    new_shard.display_master_desactivated = true
                                    if @model.get('datacenter_uuid')?
                                        if namespace.get('primary_uuid') is @model.get('datacenter_uuid')
                                            new_shard.reason_master = @reason_set_primary_set_secondary_first_template()
                                        else
                                            new_shard.reason_master = @reason_set_primary_set_datecenter_as_primary_template({namespace_id: namespace.get('id')})
                                    else
                                        new_shard.reason_master = @reason_set_primary_no_datacenter_template()

                                    new_shard.display_secondary = true
                                    if @model.get('datacenter_uuid')?
                                        if not namespace.get('replica_affinities')[@model.get('datacenter_uuid')]? or namespace.get('replica_affinities')[@model.get('datacenter_uuid')] < 1
                                            new_shard.display_secondary_desactivated = true
                                            new_shard.reason_secondary = @reason_set_secondary_increase_replicas_template({namespace_id: namespace.get('id')})
                                    else
                                        new_shard.display_secondary_desactivated = true
                                        new_shard.reason_secondary = @reason_set_primary_no_datacenter_template()

                            _shards.push new_shard
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

        make_master: (event) =>
            event.preventDefault()
            shard = $(event.target).data('shard')
            namespace_id = $(event.target).data('namespace_id')
            namespace = namespaces.get(namespace_id)
            shard_key = JSON.stringify(shard)
            model = @model

            confirmation_modal = new UIComponents.ConfirmationDialogModal
            confirmation_modal.on_submit = ->
                @.$('.btn-primary').button('loading')
                @.$('.cancel').button('loading')

                post_data = {}
                post_data[namespace.get('protocol')+'_namespaces'] = {}
                post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')] = {}
                post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['primary_pinnings'] = namespace.get('primary_pinnings')
                post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'] = namespace.get('secondary_pinnings')


                old_master = post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['primary_pinnings'][shard_key]
                shard_key = JSON.stringify(shard)

                post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['primary_pinnings'][shard_key] = model.get('id')

                for mid, index in post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key]
                    if mid is model.get('id')
                        post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].splice(index, 1)
                        break

                num_replicas = namespace.get('replica_affinities')[model.get('datacenter_uuid')]
                if post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].length >= num_replicas
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].pop()
                if old_master?
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].push old_master
                
                $.ajax
                    processData: false
                    url: @url
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify(post_data)
                    success: @on_success
                    error: @on_error


            #TODO Double check if can become secondary
            confirmation_modal.render("Are you sure you want to make the machine <strong>#{@model.get('name')}</strong> a primary machine for the shard <strong>#{JSON.stringify(shard)}</strong> of the namespace <strong>#{namespace.get('name')}</strong>?",
                "/ajax/semilattice",
                '',
                (response) =>
                    apply_diffs(response)

                    $link = @.$('a.make-master')
                    $link.text $link.data('loading-text')

                    clear_modals()
                    $('#user-alert-space').html @alert_set_server_template
                        role: 'primary'
                        shard: shard_key
                        namespace_name: namespace.get('name')
                    @render()
            )

        make_secondary: (event) =>
            event.preventDefault()
            shard = $(event.target).data('shard')
            namespace_id = $(event.target).data('namespace_id')
            namespace = namespaces.get(namespace_id)
            shard_key = JSON.stringify(shard)
            model = @model
        
            if namespace.get('blueprint')['peers_roles'][@model.get('id')][shard_key] is 'role_nothing'
                confirmation_modal = new UIComponents.ConfirmationDialogModal
                confirmation_modal.on_submit = ->
                    @.$('.btn-primary').button('loading')
                    @.$('.cancel').button('loading')

                    post_data = {}
                    post_data[namespace.get('protocol')+'_namespaces'] = {}
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')] = {}
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'] = namespace.get('secondary_pinnings')

                    num_replicas = namespace.get('replica_affinities')[model.get('datacenter_uuid')]
                    if post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].length >= num_replicas
                        post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].pop()
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].push model.get('id')
                        
                    $.ajax
                        processData: false
                        url: @url
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify(post_data)
                        success: @on_success
                        error: @on_error

                        
                confirmation_modal.render("Are you sure you want to make the machine <strong>#{@model.get('name')}</strong> a secondary machine for the shard <strong>#{JSON.stringify(shard)}</strong> of the namespace <strong>#{namespace.get('name')}</strong>?",
                    "/ajax/semilattice",
                    '',
                    (response) =>
                        apply_diffs(response)
                        # Set the link's text to a loading state
                        $link = @.$('a.make-master')
                        $link.text $link.data('loading-text')

                        clear_modals()
                        $('#user-alert-space').html @alert_set_server_template
                            role: 'secondary'
                            shard: shard_key
                            namespace_name: namespace.get('name')
                        @render()
                )
            else if namespace.get('blueprint')['peers_roles'][@model.get('id')][shard_key] is 'role_primary'
                outdated_data_template = @outdated_data_template
                confirmation_modal = new UIComponents.ConfirmationDialogModal
                confirmation_modal.on_submit = ->
                    @.$('.btn-primary').button('loading')
                    @.$('.cancel').button('loading')

                
                    for machine_id of namespace.get('blueprint')['peers_roles']
                        if new_master?
                            break
                        if namespace.get('blueprint')['peers_roles'][machine_id][shard_key] is 'role_secondary'
                            for key of directory.get(machine_id).get(namespace.get('protocol')+'_namespaces')['reactor_bcards'][namespace.get('id')]['activity_map']
                                if directory.get(machine_id).get(namespace.get('protocol')+'_namespaces')['reactor_bcards'][namespace.get('id')]['activity_map'][key][0] is shard_key and directory.get(machine_id).get(namespace.get('protocol')+'_namespaces')['reactor_bcards'][namespace.get('id')]['activity_map'][key][1]['type'] is 'secondary_up_to_date'
                                    new_master = machine_id
                                    break

                    if not new_master?
                        @.$('.error_answer').html outdated_data_template {}

                        if @.$('.error_answer').css('display') is 'none'
                            @.$('.error_answer').slideDown('fast')
                        else
                            @.$('.error_answer').css('display', 'none')
                            @.$('.error_answer').fadeIn()

                        @reset_buttons()
                        return

                    post_data = {}
                    post_data[namespace.get('protocol')+'_namespaces'] = {}
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')] = {}
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['primary_pinnings'] = namespace.get('primary_pinnings')
                    post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['primary_pinnings'][shard_key] = new_master

                    $.ajax
                        processData: false
                        url: @url
                        type: 'POST'
                        contentType: 'application/json'
                        data: JSON.stringify(post_data)
                        success: @on_success
                        error: @on_error

                        
                confirmation_modal.render("Are you sure you want to make the machine <strong>#{@model.get('name')}</strong> a secondary machine for the shard <strong>#{JSON.stringify(shard)}</strong> of the namespace <strong>#{namespace.get('name')}</strong>?",
                    "/ajax/semilattice",
                    '',
                    (response) =>
                        apply_diffs(response)
                        # Set the link's text to a loading state
                        $link = @.$('a.make-master')
                        $link.text $link.data('loading-text')

                        clear_modals()
                        $('#user-alert-space').html @alert_set_server_template
                            role: 'secondary'
                            shard: shard_key
                            namespace_name: namespace.get('name')
                        @render()
                )

        make_nothing: (event) =>
            event.preventDefault()
            shard = $(event.target).data('shard')
            namespace_id = $(event.target).data('namespace_id')
            namespace = namespaces.get(namespace_id)
            shard_key = JSON.stringify(shard)
            model = @model

            confirmation_modal = new UIComponents.ConfirmationDialogModal
            confirmation_modal.on_submit = ->
                @.$('.btn-primary').button('loading')
                @.$('.cancel').button('loading')

                post_data = {}
                post_data[namespace.get('protocol')+'_namespaces'] = {}
                post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')] = {}
                post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'] = namespace.get('secondary_pinnings')

                replica_requirement = namespace.get('replica_affinities')[model.get('datacenter_uuid')]
                if namespace.get('primary_uuid') is model.get('datacenter_uuid')
                    replica_requirement--

                for machine_id, i in post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key]
                    if machine_id is model.get('id')
                        post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].splice(i, 1)
                        break

                for machine in machines.models
                    # Skip the machine we are changing role
                    if machine.get('id') is model.get('id')
                        continue

                    if post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'].length >= replica_requirement
                        break

                    if machine.get('datacenter_uuid') is model.get('datacenter_uuid') # Check if in the same datacenter
                        already_secondary = false
                        for secondary_id in post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key]
                            if machine.get('id') is secondary_id
                                already_secondary = true
                                break

                        if already_secondary is false
                            post_data[namespace.get('protocol')+'_namespaces'][namespace.get('id')]['secondary_pinnings'][shard_key].push machine.get('id')

                $.ajax
                    processData: false
                    url: @url
                    type: 'POST'
                    contentType: 'application/json'
                    data: JSON.stringify(post_data)
                    success: @on_success
                    error: @on_error


            #TODO Double check if can become secondary
            confirmation_modal.render("Are you sure you want to make the machine <strong>#{@model.get('name')}</strong> a primary machine for the shard <strong>#{JSON.stringify(shard)}</strong> of the namespace <strong>#{namespace.get('name')}</strong>?",
                "/ajax/semilattice",
                '',
                (response) =>
                    apply_diffs(response)

                    $link = @.$('a.make-master')
                    $link.text $link.data('loading-text')

                    clear_modals()
                    $('#user-alert-space').html @alert_set_server_template
                        role: 'nothing'
                        shard: shard_key
                        namespace_name: namespace.get('name')
                    @render()
            )

        destroy: =>
            @directory_entry.on 'change:memcached_namespaces', @render
            for namespace_id of @namespaces_with_listeners
                namespace = namespaces.get namespace_id
                namespace.off 'change:shards', @render
                namespace.off 'change:primary_uuid', @render
                namespace.off 'change:primary_pinnings', @render
                namespace.off 'change:secondary_pinnings', @render
                namespace.off 'change:replica_affinities', @render


    class @Operations extends Backbone.View
        className: 'namespace-other'

        template: Handlebars.compile $('#machine_view-operations-template').html()
        still_master_template: Handlebars.compile $('#reason-cannot_unassign-master-template').html()
        unsatisfiable_goals_template: Handlebars.compile $('#reason-cannot_unassign-goals-template').html()

        events: ->
            'click .rename_server-button': 'rename_server'
            'click .change_datacenter-button': 'change_datacenter'
            'click .unassign_datacenter-button': 'unassign_datacenter'
            'click .to_assignments-link': 'to_assignments'

        initialize: =>
            @data = {}
            machines.on 'all', @render
            namespaces.on 'all', @render

        rename_server: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'machine'
            rename_modal.render()

        change_datacenter: (event) =>
            event.preventDefault()
            set_datacenter_modal = new ServerView.SetDatacenterModal
            set_datacenter_modal.render [@model]

        unassign_datacenter: (event) =>
            event.preventDefault()
            unassign_dialog = new MachineView.UnassignModal
            machine_to_unassign = @model
            unassign_dialog.render @model

        to_assignments: (event) ->
            event.preventDefault()
            $('#machine-assignments').addClass('active')
            $('#machine-assignments-link').tab('show')
            window.router.navigate @.$(event.target).attr('href')

        need_update: (old_data, new_data) ->
            for key of old_data
                if not new_data[key]?
                    return true
                if new_data[key] isnt old_data[key]
                    return true
            for key of new_data
                if not old_data[key]?
                    return true
                if new_data[key] isnt old_data[key]
                    return true
            return false

        render: =>
            data = {}
            if not @model.get('datacenter_uuid')?
                data.can_assign = true
                data.can_unassign = false
                data.unassign_reason = 'This server is not part of any datacenter'
                if @need_update(@data, data)
                    @.$el.html @template data
                    @data = data
                return @

            for namespace in namespaces.models
                for machine_uuid, peer_roles of namespace.get('blueprint').peers_roles
                    if machine_uuid is @model.get('id')
                        for shard, role of peer_roles
                            if role is 'role_primary'
                                data.can_assign = false
                                data.can_unassign = false
                                data.unassign_reason = @still_master_template
                                    server_id: @model.get 'id'
                                data.assign_reason = data.unassign_reason
                                if @need_update(@data, data)
                                    @.$el.html @template data
                                    @data = data
                                return @

            num_machines_in_datacenter = 0
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('datacenter_uuid')
                    num_machines_in_datacenter++

            namespaces_need_less_replicas = []
            for namespace in namespaces.models
                if @model.get('datacenter_uuid') of namespace.get('replica_affinities') # If the datacenter has responsabilities
                    num_replica = namespace.get('replica_affinities')[@model.get('datacenter_uuid')]
                    if namespace.get('primary_uuid') is @model.get('datacenter_uuid')
                        num_replica++

                    if num_machines_in_datacenter <= num_replica
                        namespaces_need_less_replicas.push
                            id: namespace.get('id')
                            name: namespace.get('name')
            if namespaces_need_less_replicas.length > 0
                data.can_unassign = false
                data.unassign_reason = @unsatisfiable_goals_template
                    namespaces_need_less_replicas: namespaces_need_less_replicas
                if @need_update(@data, data)
                    @.$el.html @template data
                    @data = data
                return @

            data.can_unassign = true
            @.$el.html @template data
            return @


    class @UnassignModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#unassign-modal-template').html()
        alert_tmpl: Handlebars.compile $('#unassign-alert-template').html()
        class: 'unassign-dialog'

        initialize: ->
            super

        render: (_machine_to_unassign) ->
            @machine_to_unassign = _machine_to_unassign

            super
                modal_title: 'Remove datacenter'
                btn_primary_text: 'Remove'
                id: _machine_to_unassign.get('id')
                name: _machine_to_unassign.get('name')

            @.$('.btn-primary').focus()

        on_submit: =>
            super

            # For when /ajax will handle post request
            data = {}
            data['machines'] = {}
            for machine in machines.models
                data['machines'][machine.get('id')] = {}
                data['machines'][machine.get('id')]['name'] = machine.get('name')
                data['machines'][machine.get('id')]['datacenter_uuid'] = machine.get('datacenter_uuid')

            data['machines'][@machine_to_unassign.get('id')]['datacenter_uuid'] = null

            $.ajax
                url: "/ajax/semilattice"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                dataType: 'json',
                success: @on_success,
                error: @on_error

        on_success: (response) =>
            machines.get(@machine_to_unassign.get('id')).set('datacenter_uuid', null)
            clear_modals()
            
        on_error: (response) =>
            #TODO implement


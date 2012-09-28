# Datacenter view
module 'DatacenterView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.compile $('#element_view-not_found-template').html()
        initialize: (id) ->
            @id = id
        render: =>
            @.$el.html @template
                id: @id
                type: 'datacenter'
                type_url: 'datacenters'
                type_all_url: 'servers'
            return @

    # Container
    class @Container extends Backbone.View
        className: 'datacenter-view'
        template: Handlebars.compile $('#datacenter_view-container-template').html()
        events: ->
            'click .tab-link': 'change_route'
            'click .display_more_machines': 'expand_profile'
            'click .close': 'close_alert'

        max_log_entries_to_render: 3

        initialize: =>
            log_initial '(initializing) datacenter view: container'

            # Panels for datacenter view
            @title = new DatacenterView.Title model: @model
            @profile = new DatacenterView.Profile model: @model
            @data = new DatacenterView.Data model: @model
            @operations = new DatacenterView.Operations model: @model
            @overview = new DatacenterView.Overview model: @model
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance,
                width:  390             # width in pixels
                height: 300             # height in pixels
                seconds: 65             # num seconds to track
                type: 'datacenter'
            )

            filter = {}
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('id')
                    filter[machine.get('id')] = true

            @logs = new LogView.Container
                template_header: Handlebars.compile $('#log-header-datacenter-template').html()
                filter: filter

            datacenters.on 'remove', @check_if_still_exists
        
        check_if_still_exists: =>
            exist = false
            for datacenter in datacenters.models
                if datacenter.get('id') is @model.get('id')
                    exist = true
                    break
            if exist is false
                window.router.navigate '#servers'
                window.app.index_servers
                    alert_message: "The datacenter <a href=\"#datacenters/#{@model.get('id')}\">#{@model.get('name')}</a> could not be found and was probably deleted."


        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later.
            window.router.navigate @.$(event.target).attr('href')



        render: (tab) =>
            log_render('(rendering) datacenter view: container')

            @.$el.html @template
                datacenter_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # fill the profile (name, reachable...)
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # display the data on the datacenter
            @.$('.overview').html @overview.render().$el

            # display the data on the datacenter
            @.$('.data').html @data.render().$el

            # display the operations
            @.$('.operations').html @operations.render().$el

            @.$('.performance-graph').html @performance_graph.render().$el

            # Filter all the machines for those belonging to this datacenter and append logs
            machines_in_datacenter = machines.filter (machine) => return machine.get('datacenter_uuid') is @model.get('id')

            # log entries
            @.$('.recent-log-entries').html @logs.render().$el

            @.$('.nav-tabs').tab()

            if tab?
                @.$('.active').removeClass('active')
                switch tab
                    when 'overview'
                        @.$('#datacenter-overview').addClass('active')
                        @.$('#datacenter-overview-link').tab('show')
                    when 'data'
                        @.$('#datacenter-data').addClass('active')
                        @.$('#datacenter-data-link').tab('show')
                    when 'operations'
                        @.$('#datacenter-operations').addClass('active')
                        @.$('#datacenter-operations-link').tab('show')
                    when 'logs'
                        @.$('#datacenter-logs').addClass('active')
                        @.$('#datacenter-logs-link').tab('show')
                    else
                        @.$('#datacenter-overview').addClass('active')
                        @.$('#datacenter-overview-link').tab('show')

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
            @overview.destroy()
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

            # Check if data are up to date
            stats_up_to_date = true
            for machine in machines_in_datacenter
                if machine.get('stats_up_to_date') is false
                    stats_up_to_date = false
                    break

 

            # Generate json and render
            json =
                name: @model.get('name')
                list_machines:
                    machines: machines_returned
                    more_link_should_be_displayed: @more_link_should_be_displayed
                status: DataUtils.get_datacenter_reachability(@model.get('id'))
                stats_up_to_date: stats_up_to_date
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



    class @Overview extends Backbone.View
        className: 'datacenter-overview'

        container_template: Handlebars.compile $('#datacenter_overview-container-template').html()
        ram_repartition_template: Handlebars.compile $('#datacenter-ram_repartition-template').html()
        disk_repartition_template: Handlebars.compile $('#datacenter-disk_repartition-template').html()

        initialize: =>
            machines.on 'all', @render_ram_repartition
            machines.on 'all', @render_disk_repartition

        events:
            'click rect': 'redirect_to_server'
            'mouseenter rect': 'show_tooltip'
            'mouseleave rect': 'hide_tooltip'


        redirect_to_server: (event) ->
            $('.tooltip').remove()
            window.app.navigate '#servers/'+event.target.dataset.id, {trigger: true}
            $('body').scrollTop 0

        show_tooltip: (event) ->
            $('.tooltip').remove()
            @.$('#'+event.target.dataset.target).tooltip 'show'

        hide_tooltip: ->
            @.$('#'+event.target.dataset.target).tooltip 'hide'
            $('.tooltip').remove()
            
        render: =>
            @.$el.html @container_template {}
            return @
        
        render_ram_repartition: =>
            machines_in_datacenter = []
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('id')
                    machines_in_datacenter.push machine

            test_data = (d) ->
                return d.get('stats')?.proc?.global_mem_total? and d.get('stats')?.proc?.global_mem_used?

            get_total = (d) ->
                return d.get('stats').proc.global_mem_total

            get_used = (d) ->
                return d.get('stats').proc.global_mem_used

            @render_stacked_bars(machines_in_datacenter, test_data, get_total, get_used, '.ram_repartition-container', '.ram_repartition-diagram', '.ram_repartition-graph', 'ram_free_bar', 'ram_used_bar', '.loading_text-ram-diagram', @ram_repartition_template, @render_ram_repartition, 'Servers', 'RAM')


        render_disk_repartition: =>
            machines_in_datacenter = []
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('id')
                    machines_in_datacenter.push machine


            test_data = (d) ->
                return d.get('stats')?.sys?.global_disk_space_total?


            get_total = (d) ->
                return d.get('stats').sys.global_disk_space_total

            get_used = (d) ->
                return d.get('stats').sys.global_disk_space_used

            @render_stacked_bars(machines_in_datacenter, test_data, get_total, get_used, '.disk_repartition-container', '.disk_repartition-diagram', '.disk_repartition-graph', 'disk_free_bar', 'disk_used_bar', '.loading_text-disk-diagram', @disk_repartition_template, @render_disk_repartition, 'Servers', 'Disk')

        render_stacked_bars: (machines_in_datacenter, test_data, get_total, get_used, class_container, class_diagram, class_graph, class_free_bar, class_used_bar, loading_text_class, template_container, this_callback, x_title, y_title) =>
            if machines_in_datacenter.length is 0
                @.$(class_container).html template_container
                    few_machines: true
                    no_machine: true
                return @

            @.$(class_container).html template_container
                few_machines: machines_in_datacenter.length < 17

            data_is_defined = true
            for machine in machines_in_datacenter
                if not test_data(machine)
                    data_is_defined = false
                    break

            if $(class_diagram).length is 0 or data_is_defined is false
                setTimeout(this_callback, 1000)
                return
            else
                @.$(loading_text_class).css 'display', 'none'

            max_value = d3.max machines_in_datacenter, get_total
            margin_width = 20
            margin_height = 20

            margin_width_inner = 20
            width = 28
            margin_bar = 2

            # Compute the width of the graph
            if machines_in_datacenter.length < 17
                svg_width = margin_width+margin_width_inner+(width+margin_bar)*machines_in_datacenter.length+margin_width_inner+margin_width
                svg_height = 270
                container_width = Math.max svg_width, 350
            else
                if margin_width+margin_width_inner+(width+margin_bar)*machines_in_datacenter.length+margin_width_inner+margin_width > 700
                    svg_width = 700
                    width_and_margin = (700-margin_width*2-margin_width_inner*2)/machines_in_datacenter.length
                    width = width_and_margin/30*28
                    margin_bar = width_and_margin/30*2
                else
                    svg_width = margin_width+margin_width_inner+(width+margin_bar)*machines_in_datacenter.length+margin_width_inner+margin_width
                svg_height = 350
                container_width = svg_width

            # Set graph's container width
            @.$(class_graph).css('width', container_width+'px')

            y = d3.scale.linear().domain([0, max_value]).range([1, svg_height-margin_height*2.5])

            svg = d3.select(class_diagram).attr('width', svg_width).attr('height', svg_height).append('svg:g')
            svg.selectAll(class_free_bar).data(machines_in_datacenter)
                .enter()
                .append('rect')
                .attr('class', class_free_bar)
                .attr('x', (d, i) ->
                    return margin_width+margin_width_inner+(width+margin_bar)*i
                )
                .attr('y', (d) ->
                    return svg_height-y(get_total(d))-margin_height-1
                ) #-1 not to overlap with axe
                .attr('width', width)
                .attr( 'height', (d) ->
                    return y(get_total(d) - get_used(d))
                )
                .attr('data-id', (d) -> return d.get('id'))
                .attr('id', (d) -> return y_title+'-'+d.get('id'))
                .attr('data-target', (d) -> return y_title+'-'+d.get('id'))
                .attr('title', (d) -> 
                    return machines.get(d.get('id')).get('name') + '<br/>' + human_readable_units(get_used(d), units_space) + '/' + human_readable_units(get_total(d), units_space)
                )

            svg.selectAll(class_used_bar).data(machines_in_datacenter)
                .enter()
                .append('rect')
                .attr('class', class_used_bar)
                .attr('x', (d, i) ->
                    return margin_width+margin_width_inner+(width+margin_bar)*i
                )
                .attr('y', (d) ->
                    return svg_height-y(get_used(d))-margin_height-1
                ) #-1 not to overlap with axe
                .attr('width', width)
                .attr( 'height', (d) ->
                    return y(get_used(d))
                )
                .attr('data-id', (d) -> return d.get('id'))
                .attr('data-target', (d) -> return y_title+'-'+d.get('id'))
       

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
                x2: margin_width+margin_width_inner+(width+margin_bar)*machines_in_datacenter.length+margin_width_inner
                y1: svg_height-margin_height
                y2: svg_height-margin_height

            svg = d3.select(class_diagram).attr('width', svg_width).attr('height', svg_height).append('svg:g')
            svg.selectAll('line').data(extra_data).enter().append('line')
                .attr('x1', (d) -> return d.x1)
                .attr('x2', (d) -> return d.x2)
                .attr('y1', (d) -> return d.y1)
                .attr('y2', (d) -> return d.y2)
                .style('stroke', '#000')

            axe_legend = []
            axe_legend.push
                x: margin_width
                y: Math.floor(margin_height/2)
                anchor: 'middle'
                string: y_title
            axe_legend.push
                x: Math.floor(svg_width/2)
                y: svg_height
                anchor: 'middle'
                string: x_title

            svg.selectAll('.legend')
                .data(axe_legend).enter().append('text')
                .attr('class', 'legend')
                .attr('x', (d) -> return d.x)
                .attr('y', (d) -> return d.y)
                .attr('text-anchor', (d) -> return d.anchor)
                .text((d) -> return d.string)

            @.$('rect').tooltip
                trigger: 'manual'

        destroy: =>
            machines.off 'all', @render_ram_repartition
            machines.off 'all', @render_disk_repartition

    class @Data extends Backbone.View
        className: 'datacenter-data-view'

        template: Handlebars.compile $('#datacenter_view_data-template').html()

        initialize: =>
            @namespaces_with_listeners = {}

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
                    if not @namespaces_with_listeners[namespace.get('id')]?
                        @namespaces_with_listeners[namespace.get('id')] = true
                        namespace.load_key_distr_once()
                        namespace.on 'change:key_distr', @render

                    # Compute number of primaries and secondaries for each shard
                    __shards = {}
                    for shard in _shards
                        shard_repr = shard.shard.toString()
                        if not __shards[shard_repr]?
                            keys = namespace.compute_shard_rows_approximation shard.shard
                            __shards[shard_repr] =
                                shard: shard.shard
                                name: human_readable_shard shard.shard
                                nprimaries: if shard.role is 'role_primary' then 1 else 0
                                nsecondaries: if shard.role is 'role_secondary' then 1 else 0
                                keys: keys if typeof keys is 'string'
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

 
    class @Data extends Backbone.View
        className: 'datacenter-data-view'

        template: Handlebars.compile $('#datacenter_view_data-template').html()

        initialize: =>
            @namespaces_with_listeners = {}

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
                    if not @namespaces_with_listeners[namespace.get('id')]?
                        @namespaces_with_listeners[namespace.get('id')] = true
                        namespace.load_key_distr_once()
                        namespace.on 'change:key_distr', @render

                    # Compute number of primaries and secondaries for each shard
                    __shards = {}
                    for shard in _shards
                        shard_repr = shard.shard.toString()
                        if not __shards[shard_repr]?
                            keys = namespace.compute_shard_rows_approximation shard.shard
                            __shards[shard_repr] =
                                shard: shard.shard
                                name: human_readable_shard shard.shard
                                nprimaries: if shard.role is 'role_primary' then 1 else 0
                                nsecondaries: if shard.role is 'role_secondary' then 1 else 0
                                keys: keys if typeof keys is 'string'
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
            for namespace_id of @namespaces_with_listeners
                namespaces.get(namespace_id).off 'change:key_distr', @render
                namespaces.get(namespace_id).clear_timeout()


    class @Operations extends Backbone.View
        className: 'datacenter-other'

        template: Handlebars.compile $('#datacenter_view-operations-template').html()
        still_primary_template: Handlebars.compile $('#reason-cannot_delete-goals-template').html()

        events: ->
            'click .rename_datacenter-button': 'rename_datacenter'
            'click .delete_datacenter-button': 'delete_datacenter'

        initialize: =>
            @data = {}
            machines.on 'all', @render
            namespaces.on 'all', @render


        rename_datacenter: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'datacenter'
            rename_modal.render(@model)
            @title.update()

        delete_datacenter: (event) ->
            event.preventDefault()
            remove_datacenter_dialog = new DatacenterView.RemoveDatacenterModal
            remove_datacenter_dialog.render @model

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
            namespaces_where_primary = []
            for namespace in namespaces.models
                if namespace.get('primary_uuid') is @model.get('id')
                    namespaces_where_primary.push
                        id: namespace.get('id')
                        name: namespace.get('name')
            if namespaces_where_primary.length > 0
                data.can_delete = false
                data.delete_reason = @still_primary_template
                    namespaces_where_primary: namespaces_where_primary
            else
                data.can_delete = true

            @.$el.html @template data
            return @


    class @RemoveDatacenterModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#remove_datacenter-modal-template').html()
        class: 'remove_datacenter-dialog'

        initialize: ->
            super

        render: (_datacenter_to_delete) ->
            @datacenter_to_delete = _datacenter_to_delete

            super
                modal_title: 'Remove datacenter'
                btn_primary_text: 'Remove'
                id: _datacenter_to_delete.get('id')
                name: _datacenter_to_delete.get('name')

            @.$('.btn-primary').focus()

        on_submit: =>
            super

            # That creates huge logs. Should post specific data multiple times instead?
            data = {}
            data['datacenters'] = {}
            for datacenter in datacenters.models
                data['datacenters'][datacenter.get('id')] = {}
                data['datacenters'][datacenter.get('id')]['name'] = datacenter.get('name')
            data['datacenters'][@datacenter_to_delete.get('id')] = null
            
            data['machines'] = {}
            for machine in machines.models
                data['machines'][machine.get('id')] = {}
                data['machines'][machine.get('id')]['name'] = machine.get('name')
                data['machines'][machine.get('id')]['datacenter_uuid'] = if machine.get('datacenter_uuid') is @datacenter_to_delete.get('id') then null else machine.get('datacenter_uuid')

            $.ajax
                url: "/ajax/semilattice"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify data
                dataType: 'json'
                success: @on_success
                error: @on_error

        on_success: (response) =>
            name = datacenters.get(@datacenter_to_delete.get('id')).get 'name'
            datacenters.remove @datacenter_to_delete.get('id')
            for machine in machines.models
                if machine.get('datacenter_uuid') is @datacenter_to_delete.get('id')
                    machine.set('datacenter_uuid', null)
            datacenters.trigger 'remove'

            window.router.navigate '#servers'
            window.app.index_servers
                alert_message: "The datacenter #{name} was successfully deleted."

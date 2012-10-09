# Dashboard: provides an overview and visualizations of the cluster
# Dashboard View
module 'DashboardView', ->
    # Cluster.Container
    class @Container extends Backbone.View
        template: Handlebars.compile $('#dashboard_view-template').html()
        id: 'dashboard_container'

        initialize: =>
            log_initial '(initializing) dashboard container view'

            @cluster_status_availability = new DashboardView.ClusterStatusAvailability()
            @cluster_status_redundancy = new DashboardView.ClusterStatusRedundancy()
            @cluster_status_reachability = new DashboardView.ClusterStatusReachability()
            @cluster_status_consistency = new DashboardView.ClusterStatusConsistency()

            @cluster_performance = new Vis.OpsPlot(computed_cluster.get_stats,
                width:  833             # width in pixels
                height: 300             # height in pixels
                seconds: 119            # num seconds to track
                type: 'cluster'
            )

            @logs = new DashboardView.Logs()

        render: =>
            @.$el.html @template({})
            @.$('.availability').html @cluster_status_availability.render().$el
            @.$('.redundancy').html @cluster_status_redundancy.render().$el
            @.$('.reachability').html @cluster_status_reachability.render().$el
            @.$('.consistency').html @cluster_status_consistency.render().$el

            @.$('#cluster_performance_container').html @cluster_performance.render().$el
            @.$('.recent-log-entries-container').html @logs.render().$el

            return @

        destroy: =>
            @cluster_status_availability.destroy()
            @cluster_status_redundancy.destroy()
            @cluster_status_reachability.destroy()
            @cluster_status_consistency.destroy()

            @cluster_performance.destroy()

    # TODO: dropped RAM and disk distribution in the templates, need to remove support for them here as well
    class @ClusterStatusAvailability extends Backbone.View
        className: 'cluster-status-availability '

        template: Handlebars.compile $('#cluster_status-container-template').html()
        status_template: Handlebars.compile $('#cluster_status-availability_status-template').html()
        popup_template: Handlebars.compile $('#cluster_status-availability-popup-template').html()

        events:
            'mousedown .show_details': 'show_details'
            'mousedown .close': 'hide_details'

        initialize: =>
            @data = ''
            directory.on 'all', @render_status
            namespaces.on 'all', @render_status

        convert_activity: (role) ->
            switch role
                when 'role_secondary' then return 'secondary_up_to_date'
                when 'role_nothing' then return 'nothing'
                when 'role_primary' then return 'primary'


        compute_data: =>
            num_masters = 0
            num_masters_down = 0
            namespaces_down = {}

            directory_by_namespaces = DataUtils.get_directory_activities_by_namespaces()
            for namespace in namespaces.models
                namespace_id = namespace.get('id')
                blueprint = namespace.get('blueprint').peers_roles
                for machine_id of blueprint
                    machine_name = machine_name = machines.get(machine_id)?.get('name') #TODO check later if defined
                    if not machine_name?
                        machine_name = machine_id

                    for shard of blueprint[machine_id]
                        value = blueprint[machine_id][shard]
                        if value is "role_primary"
                            num_masters++

                            if !(directory_by_namespaces?) or !(directory_by_namespaces[namespace_id]?) or !(directory_by_namespaces[namespace_id][machine_id]?)
                                num_masters_down++
                                if not namespaces_down[namespace.get('id')]?
                                    namespaces_down[namespace.get('id')] = []

                                namespaces_down[namespace.get('id')].push
                                    shard: human_readable_shard shard
                                    namespace_id: namespace.get('id')
                                    namespace_name: namespace.get('name')
                                    machine_id: machine_id
                                    machine_name: machine_name
                                    blueprint_status: value
                                    directory_status: 'Not found'
                            else if directory_by_namespaces[namespace_id][machine_id][shard] != @convert_activity(value)
                                num_masters_down++
                                if not namespaces_down[namespace.get('id')]?
                                    namespaces_down[namespace.get('id')] = []

                                namespaces_down[namespace.get('id')].push
                                    shard: human_readable_shard shard
                                    namespace_id: namespace.get('id')
                                    namespace_name: namespace.get('name')
                                    machine_id: machine_id
                                    machine_name: machine_name
                                    blueprint_status: value
                                    directory_status: directory_by_namespaces[namespace_id][machine_id][shard]

            if num_masters_down > 0
                namespaces_down_array = []
                for namespace_id, namespace_down of namespaces_down
                    namespaces_down_array.push
                        namespace_id: namespace_id
                        namespace_name: namespaces.get(namespace_id).get('name')
                        namespaces_down: namespace_down
                data =
                    status_is_ok: false
                    num_namespaces_down: namespaces_down_array.length
                    has_namespaces_down: namespaces_down_array.length>0
                    num_namespaces: namespaces.length
                    num_masters: num_masters
                    num_masters_down: num_masters_down
                    namespaces_down: namespaces_down_array if namespaces_down_array.length > 0
            else
                data =
                    status_is_ok: true
                    num_masters: num_masters

            return data

        render: =>
            @.$el.html @template()
            @render_status()

        # So we don't blow avay the popup
        render_status: =>
            data = @compute_data()
            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @data = data_in_json
                @.$('.status').html @status_template data
                if data.status_is_ok is true
                    @.$('.status').addClass 'no-problems-detected'
                    @.$('.status').removeClass 'problems-detected'
                else
                    @.$('.status').addClass 'problems-detected'
                    @.$('.status').removeClass 'no-problems-detected'

                if data.status_is_ok is false
                    @.$('.popup_container').html @popup_template data
                else
                    @.$('.popup_container').html @popup_template
                        has_namespaces_down: false


            return @

        clean_dom_listeners: =>
            if @link_clicked?
                @link_clicked.off 'click', @stop_propagation
            @.$('.popup_container').off 'click', @stop_propagation
            $(window).off 'click', @hide_details

        show_details: (event) =>
            event.preventDefault()
            @clean_dom_listeners()

            @.$('.popup_container').css 'display', 'block'
            margin_top = event.pageY-60-13
            margin_left= event.pageX+12
            @.$('.popup_container').css 'margin', margin_top+'px 0px 0px '+margin_left+'px'

            @link_clicked = @.$(event.target)
            @link_clicked.on 'click', @stop_propagation
            @.$('.popup_container').on 'click', @stop_propagation
            $(window).on 'click', @hide_details

        stop_propagation: (event) ->
            event.stopPropagation()

        hide_details: (event) =>
            @.$('.popup_container').css 'display', 'none'
            @clean_dom_listeners()

        destroy: =>
            directory.off 'all', @render_status
            namespaces.off 'all', @render_status
            @clean_dom_listeners() # Optional since Jquery should be cleaning listeners itself

    class @ClusterStatusRedundancy extends Backbone.View
        className: 'cluster-status-redundancy'

        template: Handlebars.compile $('#cluster_status-container-template').html()
        status_template: Handlebars.compile $('#cluster_status-redundancy_status-template').html()
        popup_template: Handlebars.compile $('#cluster_status-redundancy-popup-template').html()

        events:
            'mousedown .show_details': 'show_details'
            'mousedown .close': 'hide_details'

        initialize: =>
            @data = ''
            directory.on 'all', @render_status
            namespaces.on 'all', @render_status

        convert_activity: (role) ->
            switch role
                when 'role_secondary' then return 'secondary_up_to_date'
                when 'role_nothing' then return 'nothing'
                when 'role_primary' then return 'primary'


        compute_data: =>
            num_replicas = 0
            num_replicas_down = 0
            namespaces_down = {}

            directory_by_namespaces = DataUtils.get_directory_activities_by_namespaces()
            for namespace in namespaces.models
                namespace_id = namespace.get('id')
                blueprint = namespace.get('blueprint').peers_roles
                for machine_id of blueprint
                    machine_name = machine_name = machines.get(machine_id)?.get('name') #TODO check later if defined
                    if not machine_name?
                        machine_name = machine_id

                    for shard of blueprint[machine_id]
                        value = blueprint[machine_id][shard]
                        if value isnt "role_nothing"
                            num_replicas++

                            if !(directory_by_namespaces?) or !(directory_by_namespaces[namespace_id]?) or !(directory_by_namespaces[namespace_id][machine_id]?)
                                num_replicas_down++
                                if not namespaces_down[namespace.get('id')]?
                                    namespaces_down[namespace.get('id')] = []

                                namespaces_down[namespace.get('id')].push
                                    shard: human_readable_shard shard
                                    namespace_id: namespace.get('id')
                                    namespace_name: namespace.get('name')
                                    machine_id: machine_id
                                    machine_name: machine_name
                                    blueprint_status: value
                                    directory_status: 'Not found'
                            else if directory_by_namespaces[namespace_id][machine_id][shard] != @convert_activity(value)
                                num_replicas_down++
                                if not namespaces_down[namespace.get('id')]?
                                    namespaces_down[namespace.get('id')] = []

                                namespaces_down[namespace.get('id')].push
                                    shard: human_readable_shard shard
                                    namespace_id: namespace.get('id')
                                    namespace_name: namespace.get('name')
                                    machine_id: machine_id
                                    machine_name: machine_name
                                    blueprint_status: value
                                    directory_status: directory_by_namespaces[namespace_id][machine_id][shard]

            num_unsatisfiable_goals = 0
            namespaces_with_unsatisfiable_goals = []
            for issue in issues.models
                if issue.get('type') is 'UNSATISFIABLE_GOALS'
                    num_unsatisfiable_goals++
                    datacenters_with_issues = []
                    for datacenter_id of issue.get('replica_affinities')
                        num_replicas = issue.get('replica_affinities')[datacenter_id]
                        if issue.get('primary_datacenter') is datacenter_id
                            num_replicas++
                        if num_replicas > issue.get('actual_machines_in_datacenters')[datacenter_id]
                            if datacenter_id is universe_datacenter.get('id')
                                datacenter_name = universe_datacenter.get('name')
                            else
                                datacenter_name = datacenters.get(datacenter_id)?.get('name')
                            datacenters_with_issues.push
                                datacenter_id: datacenter_id
                                datacenter_name: datacenter_name
                                num_replicas: num_replicas
                    namespaces_with_unsatisfiable_goals.push
                        namespace_id: issue.get('namespace_id')
                        namespace_name: namespaces.get(issue.get('namespace_id')).get('name')
                        datacenters_with_issues: datacenters_with_issues
                        

            if num_replicas_down > 0
                namespaces_down_array = []
                for namespace_id, namespace_down of namespaces_down
                    namespaces_down_array.push
                        namespace_id: namespace_id
                        namespace_name: namespaces.get(namespace_id).get('name')
                        namespaces_down: namespace_down
                data =
                    status_is_ok: false
                    num_namespaces_down: namespaces_down_array.length
                    has_namespaces_down: namespaces_down_array.length>0
                    num_namespaces: namespaces.length
                    num_replicas: num_replicas
                    num_replicas_down: num_replicas_down
                    namespaces_down: namespaces_down_array if namespaces_down_array.length > 0
                    has_unsatisfiable_goals: num_unsatisfiable_goals > 0
                    num_unsatisfiable_goals: num_unsatisfiable_goals
                    namespaces_with_unsatisfiable_goals: namespaces_with_unsatisfiable_goals
                
            else
                data =
                    status_is_ok: num_unsatisfiable_goals is 0
                    num_replicas: num_replicas
                    has_unsatisfiable_goals: num_unsatisfiable_goals > 0
                    num_unsatisfiable_goals: num_unsatisfiable_goals
                    namespaces_with_unsatisfiable_goals: namespaces_with_unsatisfiable_goals

            return data

        render: =>
            @.$el.html @template()
            @render_status()

        # So we don't blow avay the popup
        render_status: =>
            data = @compute_data()
            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @data = data_in_json
                @.$('.status').html @status_template data
                if data.status_is_ok is true
                    @.$('.status').addClass 'no-problems-detected'
                    @.$('.status').removeClass 'problems-detected'
                else
                    @.$('.status').addClass 'problems-detected'
                    @.$('.status').removeClass 'no-problems-detected'

                if data.status_is_ok is false
                    @.$('.popup_container').html @popup_template data
                else
                    @.$('.popup_container').html @popup_template
                        has_namespaces_down: false


            return @

        clean_dom_listeners: =>
            if @link_clicked?
                @link_clicked.off 'click', @stop_propagation
            @.$('.popup_container').off 'click', @stop_propagation
            $(window).off 'click', @hide_details

        show_details: (event) =>
            event.preventDefault()
            @clean_dom_listeners()

            @.$('.popup_container').css 'display', 'block'
            margin_top = event.pageY-60-13
            margin_left= event.pageX+12
            @.$('.popup_container').css 'margin', margin_top+'px 0px 0px '+margin_left+'px'


            @.$('.popup_container').on 'click', @stop_propagation
            @link_clicked = @.$(event.target)
            @link_clicked.on 'click', @stop_propagation

            $(window).on 'click', @hide_details

        stop_propagation: (event) ->
            event.stopPropagation()

        hide_details: (event) =>
            @.$('.popup_container').css 'display', 'none'
            @clean_dom_listeners()

        destroy: =>
            directory.off 'all', @render_status
            namespaces.off 'all', @render_status
            @clean_dom_listeners()

    class @ClusterStatusReachability extends Backbone.View
        className: 'cluster-status-redundancy'

        template: Handlebars.compile $('#cluster_status-container-template').html()
        status_template: Handlebars.compile $('#cluster_status-reachability_status-template').html()
        popup_template: Handlebars.compile $('#cluster_status-reachability-popup-template').html()

        events:
            'mousedown .show_details': 'show_details'
            'mousedown .close': 'hide_details'

        initialize: =>
            @data = ''
            directory.on 'all', @render_status
            machines.on 'all', @render_status


        compute_data: =>
            num_replicas =  machines.length
            machines_down = {}
            for machine in machines.models
                machines_down[machine.get('id')] = true

            for machine in directory.models
                if directory.get(machine.get('id'))? # clean ghost
                    machines_down[machine.get('id')] = false

            machines_down_array = []
            for machine_id of machines_down
                if machines_down[machine_id] is false
                    continue

                machines_down_array.push
                    machine_id: machine_id
                    machine_name: machines.get(machine_id).get('name')


            data =
                has_machines_down: machines_down_array.length > 0
                num_machines_down: machines_down_array.length
                num_machines: machines.length
                machines_down: machines_down_array
        
            return data

        render: =>
            @.$el.html @template()
            @render_status()

        # So we don't blow avay the popup
        render_status: =>
            data = @compute_data()
            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @data = data_in_json
                @.$('.status').html @status_template data
                if data.has_machines_down is false
                    @.$('.status').addClass 'no-problems-detected'
                    @.$('.status').removeClass 'problems-detected'
                else
                    @.$('.status').addClass 'problems-detected'
                    @.$('.status').removeClass 'no-problems-detected'

                @.$('.popup_container').html @popup_template data

            return @

        clean_dom_listeners: =>
            if @link_clicked?
                @link_clicked.off 'click', @stop_propagation
            @.$('.popup_container').off 'click', @stop_propagation
            $(window).off 'click', @hide_details

        show_details: (event) =>
            event.preventDefault()
            @clean_dom_listeners()

            @.$('.popup_container').css 'display', 'block'
            margin_top = event.pageY-60-13
            margin_left= event.pageX-12-470
            @.$('.popup_container').css 'margin', margin_top+'px 0px 0px '+margin_left+'px'


            @.$('.popup_container').on 'click', @stop_propagation
            @link_clicked = @.$(event.target)
            @link_clicked.on 'click', @stop_propagation
            $(window).on 'click', @hide_details

        stop_propagation: (event) ->
            event.stopPropagation()

        hide_details: (event) =>
            @.$('.popup_container').css 'display', 'none'
            @clean_dom_listeners()

        destroy: =>
            directory.off 'all', @render_status
            machines.off 'all', @render_status
            @clean_dom_listeners()

    class @ClusterStatusConsistency extends Backbone.View
        className: 'cluster-status-consistency'

        template: Handlebars.compile $('#cluster_status-container-template').html()
        status_template: Handlebars.compile $('#cluster_status-consistency_status-template').html()
        popup_template: Handlebars.compile $('#cluster_status-consistency-popup-template').html()

        events:
            'mousedown .show_details': 'show_details'
            'mousedown .close': 'hide_details'

        initialize: =>
            @data = ''
            issues.on 'all', @render_status

        compute_data: =>
            conflicts = []
            for issue in issues.models
                if issue.get('type') is 'VCLOCK_CONFLICT'
                    type = issue.get('object_type')
                    switch type
                        when 'namespace'
                            type = 'table'
                            if namespaces.get(issue.get('object_id'))?
                                name = namespaces.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found table'
                        when 'database'
                            type = 'database'
                            if databases.get(issue.get('object_id'))
                                name = databases.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found database'
                        when 'datacenter'
                            type = 'datacenter'
                            if issue.get('object_id') is universe_datacenter.get('id')
                                name = universe_datacenter.get('name')
                            else if datacenters.get(issue.get('object_id'))?
                                name = datacenters.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found datacenter'
                        when 'machine'
                            type = 'server'
                            if machines.get(issue.get('object_id'))
                                name = machines.get(issue.get('object_id')).get('name')
                            else
                                name = 'Not found server'

                    conflicts.push
                        id: issue.get('object_id')
                        type: type
                        name: name
                        field: issue.get('field')


            types = {}
            num_types_conflicts = 0
            for conflict in conflicts
                if not types[conflict.type]?
                    types[conflict.type] = 1
                    num_types_conflicts++
                else
                    types[conflict.type]++
                
            has_conflicts: conflicts.length > 0
            conflicts: conflicts
            has_multiple_types: num_types_conflicts > 1
            num_types_conflicts:num_types_conflicts
            types: type if num_types_conflicts is 1
            type: type if type?
            num_conflicts: conflicts.length
            num_namespaces_conflicting: conflicts.length
            num_namespaces: namespaces.length

        render: =>
            @.$el.html @template()
            @render_status()

        render_status: =>
            data = @compute_data()
            data_in_json = JSON.stringify data
            if @data isnt data_in_json
                @data = data_in_json
                @.$('.status').html @status_template data
                if data.has_conflicts is false
                    @.$('.status').addClass 'no-problems-detected'
                    @.$('.status').removeClass 'problems-detected'
                else
                    @.$('.status').addClass 'problems-detected'
                    @.$('.status').removeClass 'no-problems-detected'

                @.$('.popup_container').html @popup_template data
            return @

        clean_dom_listeners: =>
            if @link_clicked?
                @link_clicked.off 'click', @stop_propagation
            @.$('.popup_container').off 'click', @stop_propagation
            $(window).off 'click', @hide_details

        show_details: (event) =>
            event.preventDefault()
            @clean_dom_listeners()
            @.$('.popup_container').css 'display', 'block'
            margin_top = event.pageY-60-13
            margin_left= event.pageX-12-470
            @.$('.popup_container').css 'margin', margin_top+'px 0px 0px '+margin_left+'px'


            @.$('.popup_container').on 'click', @stop_propagation
            @link_clicked = @.$(event.target)
            @link_clicked.on 'click', @stop_propagation
            $(window).on 'click', @hide_details

        stop_propagation: (event) ->
            event.stopPropagation()

        hide_details: (event) =>
            @.$('.popup_container').css 'display', 'none'
            @clean_dom_listeners()

        destroy: =>
            directory.off 'all', @render_status
            machines.off 'all', @render_status
            @clean_dom_listeners()


    class @Logs extends Backbone.View
        className: 'recent-log-entries'
        tagName: 'ul'
        min_timestamp: 0
        max_entry_logs: 5
        interval_update_log: 10000

        initialize: ->
            @fetch_log()
            @set_interval = setInterval @fetch_log, @interval_update_log
            @log_entries = []

        fetch_log: =>
            $.ajax({
                contentType: 'application/json'
                url: '/ajax/log/_?max_length='+@max_entry_logs+'&min_timestamp='+@min_timestamp
                dataType: 'json'
                success: @set_log_entries
            })

        set_log_entries: (response) =>
            need_render = false
            for machine_id, data of response
                for new_log_entry in data
                    for old_log_entry, i in @log_entries
                        if parseFloat(new_log_entry.timestamp) > parseFloat(old_log_entry.get('timestamp'))
                            entry = new LogEntry new_log_entry
                            entry.set('machine_uuid', machine_id)
                            @log_entries.splice i, 0, entry
                            need_render = true
                            break

                    if @log_entries.length < @max_entry_logs
                        entry = new LogEntry new_log_entry
                        entry.set('machine_uuid', machine_id)
                        @log_entries.push entry
                        need_render = true
                    else if @log_entries.length > @max_entry_logs
                        @log_entries.pop()

            if need_render
                @render()

            @min_timestamp = parseFloat(@log_entries[0].get('timestamp'))+1

        render: =>
            @.$el.html ''
            for log in @log_entries
                view = new LogView.LogEntry model: log
                @.$el.append view.render().$el
            return @

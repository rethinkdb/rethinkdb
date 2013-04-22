# Copyright 2010-2012 RethinkDB, all rights reserved.
# Log view
module 'LogView', ->
    # LogView.Container
    class @Container extends Backbone.View
        className: 'log-view'
        template: Handlebars.templates['log-container-template']
        header_template: Handlebars.templates['log-header-template']
        type: 'general'
        max_log_entries: 20
        interval_update_log: 10000

        route: "/ajax/log/_?"

        displayed_logs: 0
        max_timestamp: 0
        compact: false

        log_route: '#logs'

        events: ->
            'click .next-log-entries': 'next_entries'
            'click .update-log-entries': 'update_log_entries'

        initialize: ->
            log_initial '(initializing) events view: container'

            if @options.route?
                @route = @options.route
            if @options.type?
                @type = @options.type
            if @options.filter?
                @filter = @options.filter
            if @options.compact?
                @compact = @options.compact

            @current_logs = []
            
            @set_interval = setInterval @check_for_new_updates, @interval_update_log

        render: =>
            @.$el.html @template()
            # Initially, hide the message indicating no more entries are available. This will change depending on the results of the @fetch_log_entries call
            @.$('.no-more-entries').hide()

            @fetch_log_entries
                max_length: @max_log_entries
            @render_header()
            @.delegateEvents()

            return @

        fetch_log_entries: (url_params) =>
            route = @route
            for param, value of url_params
                route+="&#{param}=#{value}"
            $.getJSON route, @parse_log
            
        parse_log: (log_data_from_server) =>
            @max_timestamp = 0
            for machine_uuid, log_entries of log_data_from_server
                if @filter? and not @filter[machine_uuid]?
                    continue

                if log_entries.length > 0
                    @max_timestamp = parseFloat(log_entries[log_entries.length-1].timestamp) if @max_timestamp < parseFloat(log_entries[log_entries.length-1].timestamp)

                for json in log_entries # For each new log
                    log_saved = false
                    # Looking if it has been already added
                    for log, i in @current_logs
                        is_same_log = true
                        for attribute of log.attributes
                            if not json.attribute? or log.get(attribute) isnt json.attribute
                                is_same_log = false
                        if is_same_log
                            log_saved = true
                            break

                    # If it wasn't saved before, look if its place is IN the list 
                    if log_saved is false
                        for log, i in @current_logs
                            if parseFloat(json.timestamp) > parseFloat(log.get('timestamp'))
                                entry = new LogEntry json
                                entry.set('machine_uuid',machine_uuid)
                                @current_logs.splice i, 0, entry
                                log_saved = true
                                break

                    # If it's not, just add it ad the end
                    if log_saved is false
                        entry = new LogEntry json
                        entry.set('machine_uuid',machine_uuid)
                        @current_logs.push entry

            if @current_logs.length <= @displayed_logs
                @.$('.no-more-entries').show()
                @.$('.next-log-entries').hide()
            else
                for i in [0..@max_log_entries-1]
                    if @current_logs[@displayed_logs]?
                        view = new LogView.LogEntry
                            model: @current_logs[@displayed_logs]
                        @.$('.log-entries').append view.render(@compact).el
                        @displayed_logs++
                    else
                        @.$('.no-more-entries').show()
                        @.$('.next-log-entries-container').hide()
 
            @render_header()

        next_entries: (event) =>
            event.preventDefault()

            @fetch_log_entries
                max_length: @max_log_entries
                max_timestamp: @max_timestamp


        check_for_new_updates: =>
            min_timestamp = @current_logs[0]?.get('timestamp')
            if min_timestamp?
                route = @route+"&min_timestamp=#{min_timestamp}"

                @num_new_entries = 0
                $.getJSON route, (log_data_from_server) =>
                    for machine_uuid, log_entries of log_data_from_server
                        if @filter? and not @filter[machine_uuid]?
                            continue
                        @num_new_entries += log_entries.length
                    @render_header()

        render_header: =>
            @.$('.header').html @header_template
                new_entries: @num_new_entries > 0
                num_new_entries: @num_new_entries
                too_many_new_entries: @num_new_entries > @max_log_entries
                max_log_entries: @max_log_entries
                has_logs: @current_logs.length > 0
                from_date: new XDate(@current_logs[0]?.get('timestamp')*1000).toString("MMMM M, yyyy 'at' HH:mm:ss")
                to_date: new XDate(@current_logs[@displayed_logs-1]?.get('timestamp')*1000).toString("MMMM M, yyyy 'at' HH:mm:ss")
                logs_for:
                    general:    @type is 'general'
                    machine:    @type is 'machine'
                    datacenter: @type is 'datacenter'

        update_log_entries: (event) =>
            event.preventDefault()
            if @num_new_entries > @max_log_entries
                @render()
            else
                route = "/ajax/log/_?min_timestamp="+ @current_logs[0].get('timestamp')

                $.getJSON route, @parse_new_log

        parse_new_log: (log_data_from_server) =>
            for machine_uuid, log_entries of log_data_from_server
                if @filter? and not @filter[machine_uuid]?
                    continue


                for json in log_entries # For each new log
                    log_saved = false
                    for log, i in @current_logs
                        if parseFloat(json.timestamp) > parseFloat(log.get('timestamp'))
                            entry = new LogEntry json
                            entry.set('machine_uuid',machine_uuid)
                            @current_logs.splice i, 0, entry
                            log_saved = true
                            break

                    if log_saved is false
                        entry = new LogEntry json
                        entry.set('machine_uuid',machine_uuid)
                        @current_logs.push entry

            for i in [@num_new_entries-1..0]
                view = new LogView.LogEntry
                    model: @current_logs[i]
                rendered_view = $(view.render().el)
                rendered_view.css('background-color','#F8EEB1')
                @.$('.log-entries').prepend view.render().el
                rendered_view.animate
                    backgroundColor: '#FFF'
                , 2000
                @displayed_logs++

            @render_header()
            @.$('.header .alert').remove()

        destroy: =>
            clearInterval @set_interval

    class @LogEntry extends Backbone.View
        className: 'log-entry'
        classNameSmall: 'log-entry'
        tagName: 'li'
        template: Handlebars.templates['log-entry-template']
        template_small: Handlebars.templates['log-entry-small_template']

        log_single_value_template: Handlebars.templates['log-single_value-template']
        log_machines_values_template: Handlebars.templates['log-machines_values-template']
        log_datacenters_values_template: Handlebars.templates['log-datacenters_values-template']
        log_shards_values_template: Handlebars.templates['log-shards_values-template']
        log_shards_list_values_template: Handlebars.templates['log-shards_list_values-template']
        log_shard_names_values_template: Handlebars.templates['log-shard_names_values-template']
        log_new_namespace_template: Handlebars.templates['log-new_namespace-template']
        log_new_datacenter_template: Handlebars.templates['log-new_datacenter-template']
        log_new_database_template: Handlebars.templates['log-new_database-template']
        log_delete_something_template: Handlebars.templates['log-delete_something-template']
        log_namespace_new_name_template: Handlebars.templates['log-namespace-new_name-template']
        log_server_new_name_template: Handlebars.templates['log-server-new_name-template']
        log_server_set_datacenter_template: Handlebars.templates['log-server-set_datacenter-template']
        log_datacenter_new_name_template: Handlebars.templates['log-datacenter-new_name-template']
        log_database_new_name_template: Handlebars.templates['log-database-new_name-template']

        log_new_something_small_template: Handlebars.templates['log-new_something-small_template']
        log_new_datacenter_small_template: Handlebars.templates['log-new_datacenter-small_template']
        log_new_database_small_template: Handlebars.templates['log-new_database-small_template']
        log_namespace_value_small_template: Handlebars.templates['log-namespace_value-small_template']
        log_machine_value_small_template: Handlebars.templates['log-machine_value-small_template']
        log_datacenter_value_small_template: Handlebars.templates['log-datacenter_value-small_template']
        log_database_value_small_template: Handlebars.templates['log-database_value-small_template']

        events:
            'click .more-details-link': 'display_details'

        display_details: (event) =>
            event.preventDefault()
            if @.$(event.target).html() is 'More details'
                @.$(event.target).html 'Hide details'
                @.$(event.target).parent().parent().next().next().slideDown 'fast'
                
                # Check if we can show other details
                found_more_details_link = false
                $('.more-details-link').each ->
                    if $(this).html() is 'More details'
                        found_more_details_link = true
                        return false
                if found_more_details_link is false
                    $('.expand_all_link').html 'Hide all details'
            else
                @.$(event.target).html 'More details'
                @.$(event.target).parent().parent().next().next().slideUp 'fast'

                # Check if we can show other details
                found_hide_details_link = false
                $('.more-details-link').each ->
                    if $(this).html() is 'Hide details'
                        found_hide_details_link = true
                        return false
                if found_hide_details_link is false
                    $('.expand_all_link').html 'Show all details'

        # TODO This is bad code. The old version had two render functions:
        # render and render_small-- they were nearly identical.  They were
        # collapsed into one render function with a flag (e.g. "compact"). This
        # was done in a not-so-great way, more refactoring should take place.
        # Each template (template vs. template_small) should be consolidated
        # into one template, with a flag passed to the template that defines
        # whether it should be compact or not.  We have at least 2x the
        # necessary code to do this task.
        render: (compact) =>
            if compact
                json = _.extend @model.toJSON(), @format_msg_small(@model)
            else
                json = _.extend @model.toJSON(), @format_msg(@model)

            json = _.extend json,
                machine_name: if machines.get(@model.get('machine_uuid'))? then machines.get(@model.get('machine_uuid')).get('name') else 'Unknown machine'
                datetime: new XDate(@model.get('timestamp')*1000).toString("HH:mm - MMMM dd, yyyy")

            if compact
                @.$el.html @template_small json
                @.$el.attr('class', @classNameSmall)
            else
                @.$el.html @template json
            
            @delegateEvents()
            return @

        pattern_applying_data: /^(Applying data {)/

        format_msg: (model) =>
            msg = model.get('message')
            
            if @pattern_applying_data.test(msg)
                data = msg.slice 14
                data = DataUtils.stripslashes data
                msg = ''
                json_data = $.parseJSON data
                for group of json_data
                    if group is 'rdb_namespaces'
                        for namespace_id of json_data[group]
                            if namespace_id is 'new'
                                #TODO datacenter name
                                if datacenters.get(json_data[group]['new']['primary_uuid'])?
                                    datacenter_name = datacenters.get(json_data[group]['new']['primary_uuid']).get 'name'
                                else if json_data[group]['new']['primary_uuid'] is universe_datacenter.get('id')
                                    datacenter_name = universe_datacenter.get('name')
                                else
                                    datacenter_name = 'a removed datacenter'
                                msg += @log_new_namespace_template
                                    namespace_name: json_data[group]['new']['name']
                                    datacenter_id: json_data[group]['new']['primary_uuid']
                                    datacenter_name: datacenter_name
                                    is_universe: json_data[group]['new']['primary_uuid'] is universe_datacenter.get('id')
                            else
                                if json_data[group][namespace_id] is null
                                    msg += @log_delete_something_template
                                        type: 'table'
                                        id: namespace_id
                                else
                                    attributes = []
                                    for attribute of json_data[group][namespace_id]
                                        if attribute is 'ack_expectations' or attribute is 'replica_affinities'
                                            _datacenters = []
                                            for datacenter_id of json_data[group][namespace_id][attribute]
                                                if datacenter_id is universe_datacenter.get('id')
                                                    datacenter_name = universe_datacenter.get 'name'
                                                else if datacenters.get(datacenter_id)?.get('name')?
                                                    datacenter_name = datacenters.get(datacenter_id).get 'name'
                                                else
                                                    datacenter_name = 'a removed datacenter'
                                                if attribute is 'ack_expectations'
                                                    value = json_data[group][namespace_id][attribute][datacenter_id].expectation
                                                else
                                                    value = json_data[group][namespace_id][attribute][datacenter_id]
                                                data =
                                                    is_universe: datacenter_id is universe_datacenter.get('id')
                                                    datacenter_id: datacenter_id
                                                    datacenter_name: datacenter_name
                                                    value: value
                                                _datacenters.push data
                                            msg += @log_datacenters_values_template
                                                namespace_id: namespace_id
                                                namespace_name: if namespaces.get(namespace_id)? then namespaces.get(namespace_id).get 'name' else '[table no longer exists]'
                                                namespace_exists: namespaces.get(namespace_id)?
                                                attribute: attribute
                                                datacenters: _datacenters
                                        else if attribute is 'name'
                                            msg += @log_namespace_new_name_template
                                                name: json_data[group][namespace_id][attribute]
                                                namespace_id: namespace_id
                                                namespace_id_trunked: namespace_id.slice 24

                                                namespace_exists: namespaces.get(namespace_id)?
                                                attribute: attribute
                                                value: json_data[group][namespace_id][attribute]
                                        else if attribute is 'primary_uuid'
                                            datacenter_id = json_data[group][namespace_id][attribute]
                                            if datacenter_id is universe_datacenter.get('id')
                                                datacenter_name = universe_datacenter.get 'name'
                                            else if datacenters.get(datacenter_id)?
                                                datacenter_name = datacenters.get(datacenter_id).get 'name'
                                            else
                                                datacenter_name = 'a removed datacenter'

                                            msg += @log_single_value_template
                                                namespace_id: namespace_id
                                                namespace_name: if namespaces.get(namespace_id)? then namespaces.get(namespace_id).get 'name' else '[table no longer exists]'
                                                namespace_exists: namespaces.get(namespace_id)?
                                                attribute: 'primary datacenter'
                                                datacenter_id: datacenter_id
                                                datacenter_name: datacenter_name
                                        else if attribute is 'primary_pinnings'
                                            shards = []
                                            for shard of json_data[group][namespace_id][attribute]
                                                if json_data[group][namespace_id][attribute][shard]?
                                                    shards.push
                                                        shard: shard
                                                        machine_id:  json_data[group][namespace_id][attribute][shard]
                                                        machine_name: if machines.get(json_data[group][namespace_id][attribute][shard])? then  machines.get(json_data[group][namespace_id][attribute][shard]).get('name') else '[machine no longer exists]'
                                                        machine_exists: machines.get(json_data[group][namespace_id][attribute][shard])?
                                                else
                                                    shards.push
                                                        shard: shard
                                                        is_null: true

                                            msg += @log_shards_values_template
                                                namespace_id: namespace_id
                                                namespace_name: if namespaces.get(namespace_id)? then namespaces.get(namespace_id).get 'name' else '[table no longer exists]'
                                                namespace_exists: namespaces.get(namespace_id)?
                                                attribute: attribute
                                                shards: shards
                                        else if attribute is 'secondary_pinnings'
                                            shards = []
                                            for shard of json_data[group][namespace_id][attribute]
                                                _machines = []
                                                for machine_id in json_data[group][namespace_id][attribute][shard]
                                                    if machine_id?
                                                        _machines.push
                                                            id: machine_id
                                                            name: if machines.get(machine_id)? then machines.get(machine_id).get 'name' else '[machine no longer exists]'
                                                            exists: machines.get(machine_id)?
                                                shards.push
                                                    shard: shard
                                                    machines: _machines
                                            msg += @log_shards_list_values_template
                                                namespace_id: namespace_id
                                                namespace_name: if namespaces.get(namespace_id)? then namespaces.get(namespace_id).get 'name' else '[table no longer exists]'
                                                namespace_exists: namespaces.get(namespace_id)?
                                                attribute: attribute
                                                shards: shards
                                        else if attribute is 'shards'
                                            value = [] # We could make thing faster later.
                                            for shard_string in json_data[group][namespace_id][attribute]
                                                value.push JSON.parse shard_string
                                            msg += @log_shard_names_values_template
                                                namespace_id: namespace_id
                                                namespace_name: if namespaces.get(namespace_id)? then namespaces.get(namespace_id).get 'name' else '[table no longer exists]'
                                                namespace_exists: namespaces.get(namespace_id)? 
                                                attribute: attribute
                                                shards: _.map(value, (shard) -> JSON.stringify(shard).replace(/\\"/g,'"'))
                                                value: JSON.stringify(value).replace(/\\"/g,'"')
                    else if group is 'machines'
                        for machine_id of json_data[group]
                            if json_data[group][machine_id] is null
                                msg += @log_delete_something_template
                                    type: 'machine'
                                    id: machine_id
                            else
                                for attribute of json_data[group][machine_id]
                                    if attribute is 'name'
                                        msg += @log_server_new_name_template
                                            name: json_data[group][machine_id][attribute]
                                            machine_id: machine_id
                                            machine_id_trunked: machine_id.slice 24
                                    else if attribute is 'datacenter_uuid'
                                        datacenter_id = json_data[group][machine_id][attribute]
                                        if datacenter_id is universe_datacenter.get('id')
                                            datacenter_name = universe_datacenter.get 'name'
                                        else if datacenters.get(datacenter_id)?
                                            datacenter_name = datacenters.get(datacenter_id).get 'name'
                                        else
                                            datacenter_name = 'a removed datacenter'
                                        msg += @log_server_set_datacenter_template
                                            machine_id: machine_id
                                            machine_name: if machines.get(machine_id)? then machines.get(machine_id).get('name') else '[machine no longer exists]'
                                            machine_exists: machines.get(machine_id)?
                                            datacenter_id: datacenter_id
                                            datacenter_name: datacenter_name
                    else if group is 'datacenters'
                        for datacenter_id of json_data[group]
                            if json_data[group][datacenter_id] is null
                                msg += @log_delete_something_template
                                    type: 'datacenter'
                                    id: datacenter_id
                            else if datacenter_id is 'new'
                                msg += @log_new_datacenter_template
                                    datacenter_name: json_data[group][datacenter_id]['name']

                            else
                                for attribute of json_data[group][datacenter_id]
                                    if attribute is 'name'
                                        msg += @log_datacenter_new_name_template
                                            name: json_data[group][datacenter_id][attribute]
                                            datacenter_id: datacenter_id
                                            datacenter_id_trunked: datacenter_id.slice 24
                    else if group is 'databases'
                        for database_id of json_data[group]
                            if json_data[group][database_id] is null
                                msg += @log_delete_something_template
                                    type: 'database'
                                    id: database_id
                            else if database_id is 'new'
                                msg += @log_new_database_template
                                    database_name: json_data[group][database_id]['name']
                            else
                                for attribute of json_data[group][database_id]
                                    if attribute is 'name'
                                        msg += @log_database_new_name_template
                                            name: json_data[group][database_id][attribute]
                                            database_id: database_id
                                            database_id_trunked: database_id.slice 24

                    else
                        msg += "We were unable to parse this log. Click on 'More details' to see the raw log data."
                        raw_data = JSON.stringify $.parseJSON(data), undefined, 2
                return {
                    msg: msg
                    raw_data: (raw_data if raw_data?)
                }
            else
                return {
                    msg: msg
                }

        # TODO this should be merged with the format_msg function and simplified / commented. This code is incomprehensible
        # and should either provide a complete set of info for messages (compact or regular) to discard as needed, or should
        # be clearly commented to show the compact and non-compact representations. Copy-pasting this amount of code is unacceptable.
        format_msg_small: (model) =>
            msg = model.get('message')
            
            if @pattern_applying_data.test(msg)
                data = msg.slice 14
                data = DataUtils.stripslashes data
                msg = ''
                json_data = $.parseJSON data
                for group of json_data
                    if group is 'rdb_namespaces'
                        for namespace_id of json_data[group]
                            if json_data[group][namespace_id] is null
                                msg += @log_delete_something_template
                                    type: 'table'
                                    id: namespace_id
                            else if namespace_id is 'new'
                                msg += @log_new_something_small_template
                                    type: 'table'
                            else
                                attributes = []
                                for attribute of json_data[group][namespace_id]
                                    attributes.push attribute

                                value = ''
                                for attribute, i in attributes
                                    value += attribute
                                    if i isnt attributes.length-1
                                        value += ', '

                                msg += @log_namespace_value_small_template
                                    namespace_id: namespace_id
                                    namespace_name: if namespaces.get(namespace_id)? then namespaces.get(namespace_id).get 'name' else '[table no longer exists]'
                                    namespace_exists: namespaces.get(namespace_id)?
                                    attribute: value
                    else if group is 'machines'
                        for machine_id of json_data[group]
                            for attribute of json_data[group][machine_id]
                                attributes = []
                                for attribute of json_data[group][machine_id]
                                    attributes.push attribute

                                value = ''
                                for attribute, i in attributes
                                    value += attribute
                                    if i isnt attributes.length-1
                                        value += ', '

                                msg += @log_machine_value_small_template
                                    machine_id: machine_id
                                    machine_name: if machines.get(machine_id)? then machines.get(machine_id).get 'name' else '[machine no longer exists]'
                                    machine_exists: machines.get(machine_id)?
                                    attribute: value

                    else if group is 'datacenters'
                        for datacenter_id of json_data[group]
                            if json_data[group][datacenter_id] is null
                                msg += @log_delete_something_template
                                    type: 'datacenter'
                                    id: datacenter_id
                            else if datacenter_id is 'new'
                                msg += @log_new_something_small_template
                                    type: 'datacenter'
                            else
                                for attribute of json_data[group][datacenter_id]
                                    attributes = []
                                    for attribute of json_data[group][datacenter_id]
                                        attributes.push attribute

                                    value = ''
                                    for attribute, i in attributes
                                        value += attribute
                                        if i isnt attributes.length-1
                                            value += ', '

                                    if datacenter_id is universe_datacenter.get('id')
                                        datacenter_name = universe_datacenter.get 'name'
                                    else if datacenters.get(datacenter_id)?
                                        datacenter_name = datacenters.get(datacenter_id).get 'name'
                                    else
                                        datacenter_name = 'a removed datacenter'
                                    msg += @log_datacenter_value_small_template
                                        datacenter_id: datacenter_id
                                        datacenter_name: datacenter_name
                                        attribute: value
                    else if group is 'databases'
                        for database_id of json_data[group]
                            if json_data[group][database_id] is null
                                msg += @log_delete_something_template
                                    type: 'database'
                                    id: database_id
                            else if database_id is 'new'
                                msg += @log_new_something_small_template
                                    type: 'database'
                            else
                                for attribute of json_data[group][database_id]
                                    attributes = []
                                    for attribute of json_data[group][database_id]
                                        attributes.push attribute

                                    value = ''
                                    for attribute, i in attributes
                                        value += attribute
                                        if i isnt attributes.length-1
                                            value += ', '

                                    msg += @log_database_value_small_template
                                        database_id: database_id
                                        database_name: if databases.get(database_id)? then databases.get(database_id).get 'name' else 'removed database'
                                        database_exists: databases.get(database_id)?
                                        attribute: value

                    else
                        msg += "We were unable to parse this log. Click on 'More details' to see the raw log"
                return {
                    msg: msg
                    raw_data: JSON.stringify $.parseJSON(data), undefined, 2
                    timeago_timestamp: @model.get_iso_8601_timestamp()
                }
            else
                return {
                    msg: msg
                    timeago_timestamp: @model.get_iso_8601_timestamp()
                }

# Log view
module 'LogView', ->
    # LogView.Container
    class @Container extends Backbone.View
        className: 'log-view'
        template: Handlebars.compile $('#log-container-template').html()
        header_template: Handlebars.compile $('#log-header-template').html()
        max_log_entries: 10

        current_logs: []
        displayed_logs: 0
        max_timestamp: 0

        log_route: '#logs'

        events: ->
            'click .next-log-entries': 'next_entries'
            'click .update-log-entries': 'update_log_entries'

        initialize: ->
            log_initial '(initializing) events view: container'
            
           #@set_interval = setInterval @check_for_new_updates, updateInterval

        render: =>
            @.$el.html @template({})

            @fetch_log_entries
                max_length: @max_log_entries

            @.delegateEvents()

            return @

        fetch_log_entries: (url_params) =>
            route = "/ajax/log/_?"
            for param, value of url_params
                route+="&#{param}=#{value}"
            
            $.getJSON route, @parse_log
            
        parse_log: (log_data_from_server) =>
            @max_timestamp = 0
            for machine_uuid, log_entries of log_data_from_server

                if log_entries.length > 0
                    @max_timestamp = parseFloat(log_entries[log_entries.length-1].timestamp) if @max_timestamp < parseFloat(log_entries[log_entries.length-1].timestamp)

                last_position = 0
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
                            console.log 'break'
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
                return
            else
                for i in [0..@max_log_entries-1]
                    if @current_logs[@displayed_logs]?
                        view = new LogView.LogEntry
                            model: @current_logs[@displayed_logs]
                        @.$('.log-entries').append view.render().el
                        @displayed_logs++
                    else
                        @.$('.no-more-entries').show()
                        @.$('.next-log-entries').hide()
 


        next_entries: (event) =>
            event.preventDefault()

            @fetch_log_entries
                max_length: @max_log_entries
                max_timestamp: @max_timestamp

        update_log_entries: (event) =>
            event.preventDefault()
            if @num_new_entries > @max_log_entries
                @render()
            else
                @fetch_log_entries(
                    min_timestamp: @log_entries.at(0).get('timestamp')
                    , (new_log_entries) =>
                        @log_entries.add new_log_entries.models
                        @render_header()
                        @.$('.header .alert').remove()

                        new_log_entries.each (entry) =>
                            view = new LogView.LogEntry
                                model: entry

                            # Flash the new entries (highlighted background color that fades away)
                            $view = $(view.render().el)
                            $view.css('background-color','#F8EEB1')
                            @$log_entries.prepend($view)
                            $view.animate
                                backgroundColor: '#FFF'
                            , 2000
                )

        check_for_new_updates: =>
            if window.location.hash is @log_route and @log_entries.length > 0
                min_timestamp = @log_entries.at(0).get('timestamp')
                route = "/ajax/log/_?min_timestamp=#{min_timestamp}" if min_timestamp?

                @num_new_entries = 0
                $.getJSON route, (log_data_from_server) =>
                    for machine_uuid, log_entries of log_data_from_server
                        @num_new_entries += log_entries.length
                    @render_header()


        render_header: =>
            @.$('.header').html @header_template
                new_entries: @num_new_entries > 0
                num_new_entries: @num_new_entries
                too_many_new_entries: @num_new_entries > @max_log_entries
                max_log_entries: @max_log_entries
                from_date: new XDate(@log_entries.at(0).get('timestamp')*1000).toString("MMMM M, yyyy 'at' HH:mm:ss")
                to_date: new XDate(@log_entries.at(@log_entries.length-1).get('timestamp')*1000).toString("MMMM M, yyyy 'at' HH:mm:ss")


        destroy: =>
            clearInterval @set_interval

    class @LogEntry extends Backbone.View
        className: 'log-entry'
        tagName: 'li'
        template: Handlebars.compile $('#log-entry-template').html()

        events:
            'click .more-details-link': 'display_details'

        display_details: (event) =>
            event.preventDefault()
            if @.$(event.target).html() is 'More details'
                @.$(event.target).html 'Hide details'
                @.$(event.target).parent().parent().next().slideDown 'fast'
            else
                @.$(event.target).html 'More details'
                @.$(event.target).parent().parent().next().slideUp 'fast'

        render: =>
            json = _.extend @model.toJSON(), @model.get_formatted_message()
            json = _.extend json,
                machine_name: machines.get(@model.get('machine_uuid')).get('name')
                datetime: new XDate(@model.get('timestamp')*1000).toString("MMMM dd, yyyy 'at' HH:mm:ss")
        
            json_data = $.parseJSON json.json
            if json_data?
                for group of json_data
                    switch group
                        when 'memcached_namespaces'
                            json.memcached_namespaces = []
                            for key of json_data[group]
                                if namespaces.get(key)?
                                    json.memcached_namespaces.push
                                        id: key
                                        name: namespaces.get(key).get('name')
                                    json.concerns_memcached_namespaces = true
                        when 'machines'
                            json.machines = []
                            for key of json_data[group]
                                if machines.get(key)?
                                    json.machines.push
                                        id: key
                                        name: machines.get(key).get('name')
                                    json.concerns_machines = true
                        when 'datacenters'
                            json.datacenters = []
                            for key of json_data[group]
                                if datacenters.get(key)?
                                    json.datacenters.push
                                        id: key
                                        name: datacenters.get(key).get('name')
                                    json.concerns_datacenters = true   


             if json.formatted_message is 'Applying data ' and (json.concerns_memcached_namespaces or json.concerns_machines or json.concerns_datacenters)
                json.formatted_message = ' Applying data for'

   
            @.$el.html @template json 
            
            @delegateEvents()
            return @

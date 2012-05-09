# Log view
module 'LogView', ->
    # LogView.Container
    class @Container extends Backbone.View
        className: 'log-view'
        template: Handlebars.compile $('#log-container-template').html()
        header_template: Handlebars.compile $('#log-header-template').html()
        max_log_entries: 20
        log_route: '#logs'

        events: ->
            'click .next-log-entries': 'next_entries'
            'click .update-log-entries': 'update_log_entries'

        initialize: ->
            log_initial '(initializing) events view: container'
            @log_entries = new LogEntries

            setInterval @check_for_new_updates, updateInterval
            , updateInterval

        check_for_new_updates: =>
            if window.location.hash is @log_route and @log_entries.length > 0
                min_timestamp = @log_entries.at(0).get('timestamp') + 1
                route = "/ajax/log/_?min_timestamp=#{min_timestamp}" if min_timestamp?

                @num_new_entries = 0
                $.getJSON route, (log_data_from_server) =>
                    for machine_uuid, log_entries of log_data_from_server
                        @num_new_entries += log_entries.length
                    @render_header()

        render: =>
            @.$el.html @template({})
            @$log_entries = @.$('ul.log-entries')
            @fetch_log_entries(
                {}, (new_log_entries) =>
                    @log_entries.add new_log_entries.models[0...@max_log_entries]
                    @render_header()

                    # Render each log entry (append it to the list)
                    @log_entries.each (entry) =>
                        view = new LogView.LogEntry
                            model: entry
                        @$log_entries.append view.render().el
            )

            @.delegateEvents()

            return @

        next_entries: (event) =>
            event.preventDefault()
            # Ensure we have older entries (older than the oldest timestamp we're displaying)
            @fetch_log_entries(
                max_timestamp: @log_entries.at(@log_entries.length-1).get('timestamp')-1
                , (new_log_entries) =>
                    last_rendered_entry_index = @log_entries.length - 1
                    @log_entries.add new_log_entries.models[0...@max_log_entries]
                    @render_header()

                    # Render each log entry (append it to the list)
                    for entry in @log_entries.models[last_rendered_entry_index...@log_entries.length]
                        view = new LogView.LogEntry
                            model: entry
                        @$log_entries.append view.render().el
            )

        update_log_entries: (event) =>
            event.preventDefault()
            if @num_new_entries > @max_log_entries
                @render()
            else
                @fetch_log_entries(
                    min_timestamp: @log_entries.at(0).get('timestamp') + 1
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

        render_header: =>
            @.$('.header').html @header_template
                new_entries: @num_new_entries > 0
                num_new_entries: @num_new_entries
                too_many_new_entries: @num_new_entries > @max_log_entries
                max_log_entries: @max_log_entries
                from_date: new XDate(@log_entries.at(0).get('timestamp')*1000).toString("MMMM M, yyyy 'at' HH:mm:ss")
                to_date: new XDate(@log_entries.at(@log_entries.length-1).get('timestamp')*1000).toString("MMMM M, yyyy 'at' HH:mm:ss")

        fetch_log_entries: (url_params, callback) =>
            route = "/ajax/log/_?&max_length=#{@max_log_entries * machines.length}"

            for param, value of url_params
                route+="&#{param}=#{value}"

            $.getJSON route, (log_data_from_server) =>
                new_log_entries = new LogEntries
                for machine_uuid, log_entries of log_data_from_server
                    # Otherwise, keep processing log entry json and prepare to render it
                    for json in log_entries
                        entry = new LogEntry json
                        entry.set('machine_uuid',machine_uuid)
                        new_log_entries.add entry

                # Don't render anything if there are no new log entries.
                if new_log_entries.length is 0
                    @.$('.no-more-entries').show()
                    @.$('.next-log-entries').hide()
                    return

                callback(new_log_entries)

    class @LogEntry extends Backbone.View
        className: 'log-entry'
        tagName: 'li'
        template: Handlebars.compile $('#log-entry-template').html()

        events: ->
            'click a[rel=popover]': 'do_nothing'

        do_nothing: (event) -> event.preventDefault()

        render: =>
            json = _.extend @model.toJSON(), @model.get_formatted_message()
            @.$el.html @template _.extend json,
                machine_name: machines.get(@model.get('machine_uuid')).get('name')
                datetime: new XDate(@model.get('timestamp')*1000).toString("MMMM MM, yyyy 'at' HH:mm:ss")

            @.$('a[rel=popover]').popover
                html: true

            return @

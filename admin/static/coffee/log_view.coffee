# Log view
module 'LogView', ->
    # LogView.Container
    class @Container extends Backbone.View
        className: 'log-view'
        template: Handlebars.compile $('#log-container-template').html()
        header_template: Handlebars.compile $('#log-header-template').html()
        max_log_entries: 50
        log_route: '#logs'

        events: ->
            'click .next-log-entries': 'next_entries'
            'click .update-log-entries': 'update_log_entries'

        initialize: ->
            log_initial '(initializing) events view: container'
            @log_entries = new LogEntries

        render: =>
            @.$el.html @template({})

            @fetch_log_entries()

            setInterval =>
                if window.location.hash is @log_route
                    min_timestamp = @log_entries.at(0).get('timestamp') + 1
                    route = "/ajax/log/_?max_length=#{@max_log_entries}"
                    route += "&min_timestamp=#{min_timestamp}" if min_timestamp?

                    @num_new_entries = 0
                    $.getJSON route, (log_data_from_server) =>
                        for machine_uuid, log_entries of log_data_from_server
                            console.log log_entries.length
                            @num_new_entries += log_entries.length
                        @render_header()

            , updateInterval

            return @

        render_header: =>
            @.$('.header').html @header_template
                new_entries: @num_new_entries > 0
                num_new_entries: @num_new_entries
                from_date: @log_entries.at(@log_entries.length-1).get('timestamp')
                to_date: @log_entries.at(0).get('timestamp')

        render_log_entries: =>
            max_entries = if @log_entries.length > @max_log_entries then @max_log_entries else @log_entries.length
            for entry in @log_entries.models[0...max_entries]
                view = new LogView.LogEntry
                    model: entry
                @.$('tbody.log-table').append view.render().el

            return @

        fetch_log_entries: (min_timestamp) =>
            route = "/ajax/log/_?max_length=#{@max_log_entries}"
            route += "&min_timestamp=#{min_timestamp}" if min_timestamp?
            $.getJSON route, (log_data_from_server) =>
                new_log_entries = []
                for machine_uuid, log_entries of log_data_from_server

                    # If the machine referenced in the log entry isn't among known machines in the collections, we're probably just starting the app up.
                    # Bind to the global event that will indicate when collections are fully populated, and rerender the logs then.
                    if not machines.get(machine_uuid)?
                        $(window.app_events).on 'collections_ready', =>
                            @fetch_log_entries()
                        return
                    # Otherwise, keep processing log entry json and prepare to render it
                    for json in log_entries
                        entry = new LogEntry json
                        entry.set('machine_uuid',machine_uuid)
                        new_log_entries.push entry

                # Don't render anything if there are no new log entries.
                return if new_log_entries.length is 0

                @log_entries.reset new_log_entries
                @render_header()
                @render_log_entries()

        next_entries: (event) =>
            # Ensure we have new entries (oldest timestamp should greater than any existing timestamp)
            @fetch_log_entries @log_entries.at(0).get('timestamp')+1
            event.preventDefault()

        update_log_entries: (event) =>
            @.$('tbody.log-table').empty()
            @fetch_log_entries()
            event.preventDefault()

    class @LogEntry extends Backbone.View
        className: 'log-entry'
        tagName: 'tr'
        template: Handlebars.compile $('#log-entry-template').html()

        render: =>
            @.$el.html @template _.extend @model.toJSON(),
                machine_name: machines.get(@model.get('machine_uuid')).get('name')
                datetime: @model.get_iso_8601_timestamp()
            return @

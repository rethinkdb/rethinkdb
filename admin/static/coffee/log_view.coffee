# Copyright 2010-2012 RethinkDB, all rights reserved.

app = require('./app.coffee')
driver = app.driver
models = require('./models.coffee')

class LogsContainer extends Backbone.View
    template:
        main: Handlebars.templates['logs_container-template']
        error: Handlebars.templates['error-query-template']
        alert_message: Handlebars.templates['alert_message-template']

    events:
        'click .next-log-entries': 'increase_limit'

    initialize: (obj = {}) =>
        @limit = obj.limit or 20
        @current_limit = @limit
        @server_id = obj.server_id
        @container_rendered = false
        @query = obj.query or driver.queries.all_logs
        @logs = new models.Logs()
        @logs_list = new LogsListView
            collection: @logs
        @fetch_data()

    increase_limit: (e) =>
        e.preventDefault()
        @current_limit += @limit
        driver.stop_timer(@timer)
        @fetch_data()

    fetch_data: =>

        @timer = driver.run(@query(@current_limit, @server_id),
            5000, (error, result) =>
                if error?
                    if @error?.msg isnt error.msg
                        @error = error
                        @$el.html @template.error
                            url: '#logs'
                            error: error.message
                else
                    for log, index in result
                        @logs.add(new models.Log(log), merge: true, index: index)
                    @render()
        )

    render: =>
        if not @container_rendered
            @$el.html @template.main
            @$('.log-entries-wrapper').html(@logs_list.render().$el)
            @container_rendered = true
        else
            @logs_list.render()
        @

    remove: =>
        driver.stop_timer @timer
        super()

class LogsListView extends Backbone.View
    tagName: 'ul'
    className: 'log-entries'
    initialize: =>
        @log_view_cache = {}

        if @collection.length is 0
            @$('.no-logs').show()
        else
            @$('.no-logs').hide()

        @listenTo @collection, 'add', (log) =>
            view = @log_view_cache[log.get('id')]
            if not view?
                view = new LogView
                    model: log
                @log_view_cache[log.get('id')]

            position = @collection.indexOf log
            if @collection.length is 1
                @$el.html(view.render().$el)
            else if position is 0
                @$('.log-entry').prepend(view.render().$el)
            else
                @$('.log-entry').eq(position-1).after(view.render().$el)

            @$('.no-logs').hide()

        @listenTo @collection, 'remove', (log) =>
            log_view = log_view_cache[log.get('id')]
            log_view.$el.slideUp 'fast', =>
                log_view.remove()
            delete log_view[log.get('id')]
            if @collection.length is 0
                @$('.no-logs').show()
    render: =>
        @

class LogView extends Backbone.View
    tagName: 'li'
    className: 'log-entry'
    template: Handlebars.templates['log-entry']
    initialize: =>
        @model.on 'change', @render

    render: =>
        template_model = @model.toJSON()
        template_model['iso'] = template_model.timestamp.toISOString()
        @$el.html @template template_model
        @$('time.timeago').timeago()
        @

    remove: =>
        @stopListening()
        super()

module.exports =
    LogsContainer: LogsContainer
    LogsListView: LogsListView
    LogView: LogView

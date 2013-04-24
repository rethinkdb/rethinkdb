# Copyright 2010-2012 RethinkDB, all rights reserved.
render_body = ->
    template = Handlebars.templates['body-structure-template']

    $('body').html(template())

    alert_settings_view = new AlertSettings
    $('.updates_container').html alert_settings_view.render().$el

    settings_view = new Settings
        alert_view: alert_settings_view


    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true

    # Set actions on developer tools
    $('#dev-tools #pause-application').on 'click', (event) -> debugger

    $('.settings_link').click (event) ->
        event.preventDefault()
        $('.settings_container').html settings_view.render().$el
        $('.settings_container').show()

class AlertSettings extends Backbone.View
    alert_settings_template: Handlebars.templates['alert_settings-template']
    has_update_template: Handlebars.templates['has_update-template']

    className: 'settings alert'
    events:
        'click .no_update_btn': 'set_settings'
        'click .ok_update_btn': 'set_settings'
        'click .close': 'close'

    initialize: =>
        if window.localStorage?.check_updates?
            @check_updates = JSON.parse window.localStorage.check_updates
            if @check_updates is true
                @check()
        else
            @check_updates = null

    close: (event) =>
        event.preventDefault()
        window.localStorage.ignore_version = JSON.stringify @next_version
        @$el.slideUp 'fast'

    hide: =>
        @$el.slideUp 'fast'

    check: =>
        # If it's fail, it's fine

        version = '1.4.2' #TODO See with Etienne how to set it up during build time
        $.getJSON "http://newton:5000/update_for/#{version}?callback=?", @render_updates

    render_updates: (data) =>
        if data.status is 'need_update'
            if (not window.localStorage.ignore_version?) or @compare_version(JSON.parse(window.localStorage.ignore_version), data.last_version) < 0
                @next_version = data.last_version # Save it so users can ignore the update
                @$el.html @has_update_template
                    last_version: data.last_version
                    changelog: data.changelog
                @$el.slideDown 'fast'
                @delegateEvents()


    compare_version: (v1, v2) =>
        v1_array_str = v1.split('.')
        v2_array_str = v2.split('.')
        v1_array = []
        for value in v1_array_str
            v1_array.push parseInt value
        v2_array = []
        for value in v2_array_str
            v2_array.push parseInt value

        for value, index in v1_array
            if value < v2_array[index]
                return -1
            else if value > v2_array[index]
                return 1
        return 0

   
    render: =>
        if @check_updates is null
            @$el.html @alert_settings_template()
            @$el.show()
        return @

    set_settings: (event) ->
        update = @$(event.target).data('value')
        if update is 'yes'
            @check_updates = true
            @$el.slideUp 'fast'
            if window.localStorage?
                window.localStorage.check_updates = JSON.stringify true
            @check()
        else if update is 'no'
            @check_updates = false
            @$el.slideUp 'fast'
            if window.localStorage?
                window.localStorage.check_updates = JSON.stringify false

class Settings extends Backbone.View
    settings_template: Handlebars.templates['settings-template']
    events:
        'click .check_updates_btn': 'change_settings'
        'click .close': 'close'

    close: (event) =>
        event.preventDefault()
        @$el.parent().hide()
        @$el.remove()

    initialize: (args) =>
        @alert_view = args.alert_view
        if window.localStorage?.check_updates?
            @check_updates = JSON.parse window.localStorage.check_updates
        else
            @check_updates = false


    change_settings: (event) =>
        update = @$(event.target).data('update')
        if update is 'on'
            @check_updates = true
            if window.localStorage?
                window.localStorage.check_updates = JSON.stringify true
            @alert_view.check()
        else if update is 'off'
            @check_updates = false
            @alert_view.hide()
            if window.localStorage?
                window.localStorage.check_updates = JSON.stringify false
                window.localStorage.removeItem('ignore_version')
        @render()

    render: =>
        @$el.html @settings_template
            check_value: if @check_updates then 'off' else 'on'
        @delegateEvents()
        return @
 

class IsDisconnected extends Backbone.View
    el: 'body'
    className: 'is_disconnected_view'
    template: Handlebars.templates['is_disconnected-template']
    message: Handlebars.templates['is_disconnected_message-template']
    initialize: =>
        log_initial '(initializing) sidebar view:'
        @render()

    render: =>
        @.$('#modal-dialog > .modal').css('z-index', '1')
        @.$('.modal-backdrop').remove()
        @.$el.append @template
        @.$('.is_disconnected').modal
            'show': true
            'backdrop': 'static'
        @animate_loading()

    animate_loading: =>
        if @.$('.three_dots_connecting')
            if @.$('.three_dots_connecting').html() is '...'
                @.$('.three_dots_connecting').html ''
            else
                @.$('.three_dots_connecting').append '.'
            setTimeout(@animate_loading, 300)

    display_fail: =>
        @.$('.animation_state').fadeOut 'slow', =>
            $('.reconnecting_state').html(@message)
            $('.animation_state').fadeIn('slow')

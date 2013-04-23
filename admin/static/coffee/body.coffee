# Copyright 2010-2012 RethinkDB, all rights reserved.
render_body = ->
    template = Handlebars.templates['body-structure-template']

    $('body').html(template())

    alert_settings_view = new AlertSettings
    $('.updates_container').html alert_settings_view.render().$el

    settings_view = new Settings


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

    initialize: =>
        if window.localStorage?.check_updates?
            @check_updates = JSON.parse window.localStorage.check_updates
            if @check_updates is true
                @check()
        else
            @check_updates = null

    check: =>
        # If it's fail, it's fine
        $.getJSON "http://update.rethinkdb.com/version?callback=?", @render_updates

    render_updates: (data) =>
        if local_version < data.current_version
            @$el.html @has_update_template
                features: data.features

   
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

    initialize: =>
        if window.localStorage?.check_updates?
            @check_updates = JSON.parse window.localStorage.check_updates
        else
            @check_updates = false


    change_settings: (event) =>
        update = @$(event.target).data('update')
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

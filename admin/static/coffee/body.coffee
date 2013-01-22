# Copyright 2010-2012 RethinkDB, all rights reserved.
render_body = ->
    template = Handlebars.templates['body-structure-template']
    $('body').html(template())
    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true

    # Set actions on developer tools
    $('#dev-tools #pause-application').on 'click', (event) -> debugger

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

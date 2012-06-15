render_body = ->
    template = Handlebars.compile $('#body-structure-template').html()
    $('body').html(template())
    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true

    # Set actions on developer tools
    $('#dev-tools #show-walkthrough-popup').on 'click', (event) ->
        event.preventDefault()
        $('.walkthrough-popup').html (new Walkthrough.Popup).render().el
    


class @IsDisconnected extends Backbone.View
    el: 'body'
    className: 'is_disconnected_view'
    template: Handlebars.compile $('#is_disconnected-template').html()
    message: Handlebars.compile $('#is_disconnected_message-template').html()
    initialize: =>
        log_initial '(initializing) sidebar view:'
        @render()

    render: =>
        @.$('#modal-dialog > .modal').css('z-index', '1')
        @.$('.modal-backdrop').remove()
        @.$el.append @template
        @.$('.is_disconnected').modal(
            'show': true
            'backdrop': 'static'
        )
        @animate_loading()

    animate_loading: =>
        if @.$('.three_dots_connecting')
            if @.$('.three_dots_connecting').html() is '...'
                @.$('.three_dots_connecting').html ''
            else
                @.$('.three_dots_connecting').append '.'
            setTimeout(@animate_loading, 300)

    display_fail: =>
        @.$('.animation_state').fadeOut('slow', =>
            $('.reconnecting_state').html(@message)
            $('.animation_state').fadeIn('slow')
        )

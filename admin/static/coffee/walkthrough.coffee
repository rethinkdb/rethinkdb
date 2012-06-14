module 'Walkthrough', ->
    class @Popup extends Backbone.View
        template: Handlebars.compile $('#walkthrough_popup-template').html()

        events: ->
            '.show-walkthrough-popup a': 'show_walkthrough'
            '.hide-walkthrough a': 'hide_popup'

        render: =>
            @.$el.html @template
            return @

        hide_popup: (event) =>
            event.preventDefault()
            @.$el.remove()

        show_walkthrough: (event) =>
            event.preventDefault()

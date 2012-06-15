module 'Walkthrough', ->
    class @Popup extends Backbone.View
        template: Handlebars.compile $('#walkthrough_popup-template').html()

        events: ->
            'click .show-walkthrough a': 'show_walkthrough'
            'click .hide-popup a': 'hide_popup'

        render: =>
            @.$el.html @template
            return @

        hide_popup: (event) =>
            event.preventDefault()
            @.$el.remove()

        show_walkthrough: (event) =>
            event.preventDefault()
            @.$el.remove()
            $('#walkthrough').html (new Walkthrough.Carousel).render().el

    class @Carousel extends Backbone.View
        template: Handlebars.compile $('#walkthrough_carousel-template').html()

        render: =>
            @.$el.html @template
            @.$('.carousel').carousel()
            return @

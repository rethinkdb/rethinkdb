
render_body = ->
    template = Handlebars.compile $('#body-structure-template').html()
    $('body').html(template())
    # Set up common DOM behavior
    $('.modal').modal
        backdrop: true
        keyboard: true


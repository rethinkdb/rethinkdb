
render_loading = ->
    template = Handlebars.compile $('#loading-page-template').html()
    $('body').html(template())


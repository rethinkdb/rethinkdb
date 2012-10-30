# Copyright 2010-2012 RethinkDB, all rights reserved.

render_loading = ->
    template = Handlebars.compile $('#loading-page-template').html()
    $('body').html(template())


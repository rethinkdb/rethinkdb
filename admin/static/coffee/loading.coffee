# Copyright 2010-2012 RethinkDB, all rights reserved.

render_loading = ->
    template = Handlebars.templates['loading-page-template']
    $('body').html(template())


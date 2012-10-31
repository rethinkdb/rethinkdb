# Safe string
Handlebars.registerHelper 'print_safe', (str) ->
    if str?
        return new Handlebars.SafeString(str)
    else
        return ""

Handlebars.registerHelper 'pluralize_noun', (noun, num, capitalize) ->
    ends_with_y = noun.substr(-1) is 'y'
    if num is 1
        result = noun
    else
        if ends_with_y and (noun isnt 'key')
            result = noun.slice(0, noun.length - 1) + "ies"
        else if noun.substr(-1) is 's'
            result = noun + "es"
        else
            result = noun + "s"
    if capitalize is true
        result = result.charAt(0).toUpperCase() + result.slice(1)
    return result


@module = (names, fn) ->
    names = names.split '.' if typeof names is 'string'
    space = @[names.shift()] ||= {}
    space.module ||= @module
    if names.length
        space.module names, fn
    else
        fn.call space

$(window).load ->
    @current_view = new DataExplorerView.Container
        can_extend: false
        local_connect: true
    $('#dataexplorer_container').html @current_view.render().el
    @current_view.call_codemirror()
   
    js_server.local_server['test'] =
        'test':
            data: {}
            options:
                cache_size: 1073741824
                datacenter: null
                primary_key: 'id'
    #r.dbCreate('test').run()
    

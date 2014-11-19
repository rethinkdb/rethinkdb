# Copyright 2010-2012 RethinkDB, all rights reserved.
# Prettifies a date given in Unix time (ms since epoch)

# Returns a comma-separated list of the provided array
Handlebars.registerHelper 'comma_separated', (context, block) ->
    out = ""
    for i in [0...context.length]
        out += block context[i]
        out += ", " if i isnt context.length-1
    return out

# Returns a comma-separated list of the provided array without the need of a transformation
Handlebars.registerHelper 'comma_separated_simple', (context) ->
    out = ""
    for i in [0...context.length]
        out += context[i]
        out += ", " if i isnt context.length-1
    return out

# Returns a list to links to servers
Handlebars.registerHelper 'links_to_servers', (servers, safety) ->
    out = ""
    for i in [0...servers.length]
        if servers[i].exists
            out += '<a href="#servers/'+servers[i].id+'" class="links_to_other_view">'+servers[i].name+'</a>'
        else
            out += servers[i].name
        out += ", " if i isnt servers.length-1
    if safety? and safety is false
        return out
    return new Handlebars.SafeString(out)

#Returns a list of links to datacenters on one line
Handlebars.registerHelper 'links_to_datacenters_inline_for_replica', (datacenters) ->
    out = ""
    for i in [0...datacenters.length]
        out += '<strong>'+datacenters[i].name+'</strong>'
        out += ", " if i isnt datacenters.length-1
    return new Handlebars.SafeString(out)

# Helpers for pluralization of nouns and verbs
Handlebars.registerHelper 'pluralize_noun', (noun, num, capitalize) ->
    return 'NOUN_NULL' unless noun?
    ends_with_y = noun.substr(-1) is 'y'
    if num is 1
        result = noun
    else
        if ends_with_y and (noun isnt 'key')
            result = noun.slice(0, noun.length - 1) + "ies"
        else if noun.substr(-1) is 's'
            result = noun + "es"
        else if noun.substr(-1) is 'x'
            result = noun + "es"
        else
            result = noun + "s"
    if capitalize is true
        result = result.charAt(0).toUpperCase() + result.slice(1)
    return result

Handlebars.registerHelper 'pluralize_verb_to_have', (num) -> if num is 1 then 'has' else 'have'
Handlebars.registerHelper 'pluralize_verb', (verb, num) -> if num is 1 then verb+'s' else verb
#
# Helpers for capitalization
capitalize = (str) ->
    if str?
        str.charAt(0).toUpperCase() + str.slice(1)
    else
        "NULL"
Handlebars.registerHelper 'capitalize', capitalize

# Helpers for shortening uuids
Handlebars.registerHelper 'humanize_uuid', (str) ->
    if str?
        str.substr(0, 6)
    else
        "NULL"

# Helpers for printing reachability
Handlebars.registerHelper 'humanize_server_reachability', (status) ->
    if not status?
        status = 'N/A'
    success = if status == 'available' then 'success' else 'failure'
    reachability = "<span class='label label-#{success}'>#{capitalize(status)}</span>"
    return new Handlebars.SafeString(reachability)

Handlebars.registerHelper 'humanize_table_readiness', (status) ->
    if status is 'all_replicas_ready'
        label = 'success'
        value = 'Ready'
    else if status is 'not_ready'
        label = 'failure'
        value = 'Unavailable'
    else
        label = 'unknown'
        if status is 'ready_for_writes'
            value = 'Writable'
        else if status is 'ready_for_reads'
            value = 'Readable'
        else if status is 'ready_for_outdated_reads'
            value = 'Outdated reads only'
        else
            value = status
    return new Handlebars.SafeString(
        "<div class='status label label-#{label}'>#{value}</div>")


# Safe string
Handlebars.registerHelper 'print_safe', (str) ->
    if str?
        return new Handlebars.SafeString(str)
    else
        return ""

# Register some useful partials
Handlebars.registerPartial 'backfill_progress_summary', $('#backfill_progress_summary-partial').html()
Handlebars.registerPartial 'backfill_progress_details', $('#backfill_progress_details-partial').html()

# Extract form data as an object
form_data_as_object = (form) ->
    formarray = form.serializeArray()
    formdata = {}
    for x in formarray
        formdata[x.name] = x.value
    return formdata


###
    Taken from "Namespacing and modules with Coffeescript"
    https://github.com/jashkenas/coffee-script/wiki/Easy-modules-with-coffeescript

    Introduces module function that allows namespaces by enclosing classes in anonymous functions.

    Usage:
    ------------------------------
        @module "foo", ->
          @module "bar", ->
            class @Amazing
              toString: "ain't it"
    ------------------------------

    Or, more simply:
    ------------------------------
        @module "foo.bar", ->
          class @Amazing
            toString: "ain't it"
    ------------------------------

    Which can then be accessed with:
    ------------------------------
        x = new foo.bar.Amazing
    ------------------------------
###

@module = (names, fn) ->
    names = names.split '.' if typeof names is 'string'
    space = @[names.shift()] ||= {}
    space.module ||= @module
    if names.length
        space.module names, fn
    else
        fn.call space

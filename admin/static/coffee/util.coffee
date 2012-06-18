Handlebars.registerHelper 'debug', (inputs..., options) ->
    console.log @, "Current Context"
    console.log options, 'Options'
    if input
        for input in inputs
            console.log input, 'Input ##{_i}'

# Prettifies a date given in ISO 8601 format
Handlebars.registerHelper 'prettify_date', (date) ->
    (Date.parse date).toString 'MMMM dS, H:mm:ss' if date?

# Returns a comma-separated list of the provided array
Handlebars.registerHelper 'comma_separated', (context, block) ->
    out = ""
    for i in [0...context.length]
        out += block context[i]
        out += ", " if i isnt context.length-1
    return out

# Returns an html list
Handlebars.registerHelper 'html_list', (context) ->
    out = "<ul>"
    for i in [0...context.length]
        out += '<li>'+context[i]+'</li>'
    out += '</ul>'
    return new Handlebars.SafeString(out)

# Returns a list to links to machine
Handlebars.registerHelper 'links_to_machines', (machines) ->
    out = ""
    for i in [0...machines.length]
        out += '<a href="#machines/'+machines[i].uid+'">'+machines[i].name+'</a>'
        out += ", " if i isnt machines.length-1
    return out

#Returns a list of links to namespaces
Handlebars.registerHelper 'links_to_namespaces', (namespaces) ->
    out = ""
    for i in [0...namespaces.length]
        out += '<p><a href="#namespaces/'+namespaces[i].id+'">'+namespaces[i].name+'</a></p>'
        out += ", " if i isnt namespaces.length-1
    return out

#Returns a list of links to machines and namespaces
Handlebars.registerHelper 'links_to_masters_and_namespaces', (machines) ->
    out = ""
    for i in [0...machines.length]
        out += '<p><a href="#namespaces/'+machines[i].namespace_uuid+'">'+machines[i].namespace_name+'</a> (<a href="#machines/'+machines[i].machine_id+'">'+machines[i].machine_name+'</a>)</p>'
        out += ", " if i isnt machines.length-1
    return out


#Returns a list of links to machines and namespaces
Handlebars.registerHelper 'links_to_replicas_and_namespaces', (machines) ->
    out = ""
    for i in [0...machines.length]
        out += '<p><a href="#namespaces/'+machines[i].get('namespace_uuid')+'">'+machines[i].get('namespace_name')+'</a> (<a href="#machines/'+machines[i].get('machine_id')+'">'+machines[i].get('machine_name')+'</a>)</p>'
        out += ", " if i isnt machines.length-1
    return out

# If the two arguments are equal, show the inner block; else block is available
Handlebars.registerHelper 'ifequal', (val_a, val_b, if_block, else_block) ->
    if val_a is val_b
        if_block()
    else if else_block?
        else_block()

# Helpers for pluralization of nouns and verbs
Handlebars.registerHelper 'pluralize_noun', (noun, num, capitalize) ->
    if num is 1 or num is 0
        result = noun
    else
        if noun.substr(-1) is 'y' and (noun isnt 'key')
            result = noun.slice(0, noun.length - 1) + "ies"
        else if noun.substr(-1) is 's'
            result = noun + "es"
        else
            result = noun + "s"
    if capitalize is true
        result = result.charAt(0).toUpperCase() + result.slice(1)
    return result

Handlebars.registerHelper 'pluralize_verb_to_be', (num) -> if num is 1 then 'is' else 'are'
Handlebars.registerHelper 'pluralize_verb_to_have', (num) -> if num is 1 then 'has' else 'have'

# Helpers for capitalization
Handlebars.registerHelper 'capitalize', (str) -> str.charAt(0).toUpperCase() + str.slice(1)

# Helpers for shortening uuids
Handlebars.registerHelper 'humanize_uuid', (str) -> str.substr(0, 6)

# Helpers for printing roles
Handlebars.registerHelper 'humanize_role', (role) ->
    if role is 'role_primary' then return 'master'
    if role is 'role_secondary' then return 'replica'
    return role

# Helpers for printing reachability
Handlebars.registerHelper 'humanize_machine_reachability', (status) ->
    if not status?
        result = 'N/A'
    else
        if status.reachable
            result = "<span class='label label-success'>Reachable</span>"
        else
            _last_seen = if status.last_seen? then status.last_seen else 'unknown'
            result = "<span class='label label-important'>Unreachable</span>
                <span class='timeago' title='#{_last_seen}'>since #{_last_seen}</span>"
    return new Handlebars.SafeString(result)

Handlebars.registerHelper 'humanize_datacenter_reachability', (status) ->
    if status.reachable > 0
        result = "<span class='label label-success'>Live</span>"
    else
        if status.total > 0
            result = "<span class='label label-important'>Down</span>"
        else
            result = "<span class='label'>Empty</span>"
    if status.reachable == 0 and status.total > 0
        result += "<br/><span class='timeago' title='#{status.last_seen}'>since #{status.last_seen}</abbr>"

    return new Handlebars.SafeString(result)

Handlebars.registerHelper 'humanize_namespace_reachability', (reachability) ->
    if reachability
        result = "<span class='label label-success'>Live</span>"
    else
        result = "<span class='label label-important'>Down</span>"

    return new Handlebars.SafeString(result)


Handlebars.registerHelper 'display_datacenter_in_namespace', (datacenter, role) ->
    result = '<tr>'
    result += '<td class="role">'+role+'</td>'
    result += '<td class="datacenter_name"><a href="#datacenters/'+datacenter.id+'">'+datacenter.name+'</a></td>'
    result += '<td>Replicas: '+datacenter.replicas+'</td>'
    result += '<td>/</td>'
    result += '<td>Machines '+datacenter.total_machines+'</td>'
    result += '</tr>'
    return new Handlebars.SafeString(result)



Handlebars.registerHelper 'display_primary_and_secondaries', (primary, secondaries) ->
    result = '<table class="datacenter_list">'
    if primary.id? and primary.id isnt ''
        result += new Handlebars.helpers.display_datacenter_in_namespace(primary, "Primary")
    else
        result += '<tr><td class="role" colspan="5">No primary was found</td></tr>'
    display_role = true
    for secondary in secondaries
        if display_role
            display_role = false
            result += new Handlebars.helpers.display_datacenter_in_namespace(secondary, "Secondaries")
        else
            result += new Handlebars.helpers.display_datacenter_in_namespace(secondary, "")

    result += '</table>'

    return new Handlebars.SafeString(result)

Handlebars.registerHelper 'display_truncated_machines', (data) ->
    machines = data.machines
    out = ''
    num_displayed_machine = 0
    more_link_should_be_displayed = data.more_link_should_be_displayed
    for machine in machines
        out += '<li><a href="#machines/'+machine.id+'">'+machine.name+'</a>'+Handlebars.helpers.humanize_machine_reachability(machine.status)+'</li>'
        num_displayed_machine++
        if machine.status.reachable isnt true
            num_displayed_machine++

        if more_link_should_be_displayed is true and num_displayed_machine > 6
            more_link_should_be_displayed = false
            out += '<li class="more_machines"><a href="#" class="display_more_machines">» More</a></li>'


    return new Handlebars.SafeString(out)

# Safe string
Handlebars.registerHelper 'print_safe', (str) ->
    if str?
        return new Handlebars.SafeString(str)
    else
        return ""


# Register some useful partials
Handlebars.registerPartial 'backfill_progress_summary', $('#backfill_progress_summary-partial').html()
Handlebars.registerPartial 'backfill_progress_details', $('#backfill_progress_details-partial').html()

# Dev utility functions and variables
window.pause_live_data = false
window.log_initial = (msg) -> #console.log msg
window.log_render = (msg) ->  #console.log msg
window.log_action = (msg) ->  #console.log msg
window.log_router = (msg) ->  #console.log '-- router -- ' + msg
window.log_binding = (msg) -> #console.log msg
window.log_ajax = (msg) -> #console.log msg
window.class_name = (obj) ->  obj.__proto__.constructor.name

# Date utility functions
# -------------------------------------------
# Taken from the Mozilla Developer Center, quick function to generate ISO 8601 dates in Javascript for these sample alerts
ISODateString = (d) ->
    pad = (n) -> if n<10 then '0'+n else n
    d.getUTCFullYear() + '-' +
        pad(d.getUTCMonth()+1)+'-' +
        pad(d.getUTCDate())+'T' +
        pad(d.getUTCHours())+':' +
        pad(d.getUTCMinutes())+':' +
        pad(d.getUTCSeconds())+'Z'

# Choose a random model from the given collection
# -----------------------------------------------
random_model_from = (collection) ->_.shuffle(collection.models)[0]

# Generate ISO 8601 timestamps from Unix timestamps
iso_date_from_unix_time = (unix_time) -> ISODateString new Date(unix_time * 1000)

# Extract form data as an object
form_data_as_object = (form) ->
    formarray = form.serializeArray()
    formdata = {}
    for x in formarray
        formdata[x.name] = x.value
    return formdata

# TODO: This should be a handlebars helper, please only use for templates!
# Shards aren't pretty to print, let's change that
human_readable_shard = (shard) ->
    # Shard may be null, in which case we need to return the empty
    # string. Please do not fuck with this code, Michael, as you
    # removed it once already and it broke shit.
    if not shard?
        return ""
    json_shard = $.parseJSON(shard)
    res = ""
    res += if json_shard[0] == "" then "&minus;&infin;" else json_shard[0]
    res += " to "
    res += if json_shard[1] == null then "+&infin;" else json_shard[1]
    return res

# Utils to print units
units_space = ["B", "KB", "MB", "GB", "TB", "PB"]
human_readable_units = (num, units) ->
    if not num?
        return "N/A"
    index = 0
    loop
        _tmp = num / 1024
        if _tmp < 1
            break
        else
            index += 1
            num /= 1024
        if index > units_space.length - 1
            return "N/A"
    num_str = num.toFixed(1)
    if ("" + num_str)[num_str.length - 1] is '0'
        num_str = num.toFixed(0)

    return "" + num_str + units_space[index]

# Binds actions to the dev tools (accessible through alt+d)
bind_dev_tools = ->
    # Development tools
    $('body').bind 'keydown', 'alt+d', ->
        $('#dev-tools').toggle()

    $('#live-data').click (e) ->
        $(this).text if $(this).text() == 'Pause live data feed' then 'Resume live data feed' else 'Pause live data feed'
        pause_live_data = not pause_live_data
        return false

    $('#reset-simulation-data').click (e) ->
        $.ajax
            url: '/ajax/reset_data',
            success: ->
                console.log 'Reset simulation data.'
        return false

    $('#reset-session').click (e) ->
        $.ajax
            url: '/ajax/reset_session',
            success: ->
                console.log 'Reset session.'
        return false

    $('#fetch-backbone-data').click (e) ->
        collection.fetch() for collection in [datacenters, namespaces, machines]

    $('#make-diff').click (e) ->
        $.ajax
            url: '/ajax/make_diff',
            success: ->
                console.log 'Made diff to simulation data.'
        return false

    $('#visit_bad_route').click (e) ->
        $.get('/fakeurl')
        return false

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

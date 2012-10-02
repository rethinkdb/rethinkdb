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

# Returns a comma-separated list of the provided array without the need of a transformation
Handlebars.registerHelper 'comma_separated_simple', (context) ->
    out = ""
    for i in [0...context.length]
        out += context[i]
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
Handlebars.registerHelper 'links_to_machines', (machines, safety) ->
    out = ""
    for i in [0...machines.length]
        out += '<a href="#servers/'+machines[i].id+'" class="links_to_other_view">'+machines[i].name+'</a>'
        out += ", " if i isnt machines.length-1
    if safety? and safety is false
        return out
    return new Handlebars.SafeString(out)

Handlebars.registerHelper 'links_to_machines_inline', (machines) ->
    out = ""
    for i in [0...machines.length]
        out += '<a href="#servers/'+machines[i].uid+'" class="links_to_other_view">'+machines[i].name+'</a>'
        out += ", " if i isnt machines.length-1
    return new Handlebars.SafeString(out)

#Returns a list of links to namespaces
Handlebars.registerHelper 'links_to_namespaces', (namespaces) ->
    out = ""
    for i in [0...namespaces.length]
        out += '<p><a href="#tables/'+namespaces[i].id+'" class="links_to_other_view">'+namespaces[i].name+'</a></p>'
    return out

#Returns a list of links to namespaces on one line
Handlebars.registerHelper 'links_to_namespaces_inline', (namespaces, tab) ->
    out = ""
    for i in [0...namespaces.length]
        if tab?
            out += '<a href="#tables/'+namespaces[i].id+'/'+tab+'" class="links_to_other_view">'+namespaces[i].name+'</a>'
        else
            out += '<a href="#tables/'+namespaces[i].id+'" class="links_to_other_view">'+namespaces[i].name+'</a>'
        out += ", " if i isnt namespaces.length-1
    return new Handlebars.SafeString(out)

#Returns a list of links to datacenters on one line
Handlebars.registerHelper 'links_to_datacenters_inline', (datacenters) ->
    out = ""
    for i in [0...datacenters.length]
        out += '<a href="#datacenters/'+datacenters[i].id+'" class="links_to_other_view">'+datacenters[i].name+'</a>'
        out += ", " if i isnt datacenters.length-1
    return new Handlebars.SafeString(out)

#Returns a list of links to machines and namespaces
Handlebars.registerHelper 'links_to_masters_and_namespaces', (machines) ->
    out = ""
    for i in [0...machines.length]
        out += '<p><a href="#tables/'+machines[i].namespace_id+'" class="links_to_other_view">'+machines[i].namespace_name+'</a> (<a href="#servers/'+machines[i].machine_id+'" class="links_to_other_view">'+machines[i].machine_name+'</a>)</p>'
    return out


#Returns a list of links to machines and namespace
#TODO make thinkgs prettier
Handlebars.registerHelper 'links_to_replicas_and_namespaces', (machines) ->
    out = ""
    for i in [0...machines.length]
        out += '<p><a href="#tables/'+machines[i].get('namespace_uuid')+'" class="links_to_other_view">'+machines[i].get('namespace_name')+'</a> (<a href="#servers/'+machines[i].get('machine_id')+'" class="links_to_other_view">'+machines[i].get('machine_name')+'</a>)</p>'
        out += '<ul><li>Shard: '+machines[i].get('shard')+'</li>'
        out += '<li>Blueprint: '+machines[i].get('blueprint')+'</li>'
        out += '<li>Directory: '+machines[i].get('directory')+'</li></ul>'
    return out

Handlebars.registerHelper 'display_reasons_cannot_move', (reasons) ->
    out = ""
    for machine_id of reasons
        if reasons[machine_id]['master']?.length > 0
            out += '<li>The server <a href="#servers/'+machine_id+'">'+machines.get(machine_id).get('name')+'</a> is master for some shards of the tables '
            is_first = true
            for reason in reasons[machine_id]['master']
                namespace_id = reason.namespace_id
                namespace_name = namespaces.get(namespace_id).get('name')
                if is_first
                    out += '<a href="#tables/'+namespace_id+'">'+namespace_name+'</a>'
                    is_first = false
                else
                    out += ', <a href="#tables/'+namespace_id+'">'+namespace_name+'</a>'
            out += '</li>'
        if reasons[machine_id]['goals']?.length > 0
            out += '<li>Moving the table <a href="#servers/'+machine_id+'">'+machines.get(machine_id).get('name')+'</a> will result in unsatisfiable goals for the tables '
            is_first = true
            for reason in reasons[machine_id]['goals']
                namespace_id = reason.namespace_id
                namespace_name = namespaces.get(namespace_id).get('name')
                if is_first
                    out += '<a href="#tables/'+namespace_id+'">'+namespace_name+'</a>'
                    is_first = false
                else
                    out += ', <a href="#tables/'+namespace_id+'">'+namespace_name+'</a>'
            out += '.</li>'


                
            
    return new Handlebars.SafeString(out)

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
Handlebars.registerHelper 'pluralize_verb', (verb, num) -> if num is 1 then verb+'s' else verb
Handlebars.registerHelper 'pluralize_its', (num) -> if num is 1 then 'its' else 'their'
# Helpers for capitalization
Handlebars.registerHelper 'capitalize', (str) -> str.charAt(0).toUpperCase() + str.slice(1)

# Helpers for shortening uuids
Handlebars.registerHelper 'humanize_uuid', (str) -> str.substr(0, 6)

# Helpers for printing roles
Handlebars.registerHelper 'humanize_role', (role) ->
    if role is 'role_primary'
        return new Handlebars.SafeString('<span class="master responsability master">Master</span>')
    if role is 'role_secondary'
        return new Handlebars.SafeString('<span class="secondary responsability secondary">Secondary</span>')
    if role is 'role_nothing'
        return new Handlebars.SafeString('<span class="secondary responsability nothing">Nothing</span>')
 
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
        out += '<li><a href="#servers/'+machine.id+'">'+machine.name+'</a>'+Handlebars.helpers.humanize_machine_reachability(machine.status)+'</li>'
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


# Awful things because /ajax/distribution is bad. We should feel bad too.
pretty_key = (s) ->
    if s is null
        return "+∞"
    else if s is ""
        return "+∞"
    else if typeof s is "string" and s[0]? and s[0] is 'N'
        s = s.slice(s.indexOf("%23")+3)
        if _.isNaN(parseFloat(s)) is false
            return parseFloat(s)
        else
            return NaN
    else if typeof s is "string" and s[0]? and s[0] is 'S'
        s = s.slice(1)
        s = s.replace(/%21/g, '!')
        s = s.replace(/%22/g, '"')
        s = s.replace(/%23/g, '#')
        s = s.replace(/%24/g, '$')
        s = s.replace(/%25/g, '%')
        s = s.replace(/%26/g, '&')
        s = s.replace(/%27/g, '\'')
        s = s.replace(/%28/g, '(')
        s = s.replace(/%29/g, ')')
        s = s.replace(/%2A/g, '*')
        s = s.replace(/%2B/g, '+')
        s = s.replace(/%2C/g, ',')
        s = s.replace(/%2D/g, '-')
        s = s.replace(/%2E/g, '.')
        s = s.replace(/%2F/g, '/')
        s = s.replace(/%3A/g, ':')
        s = s.replace(/%3B/g, ';')
        s = s.replace(/%3C/g, '<')
        s = s.replace(/%3D/g, '=')
        s = s.replace(/%3E/g, '>')
        s = s.replace(/%3F/g, '?')
        s = s.replace(/%40/g, '@')
        s = s.replace(/%5B/g, '[')
        s = s.replace(/%5C/g, '\\')
        s = s.replace(/%5D/g, ']')
        s = s.replace(/%60/g, '`')
        s = s.replace(/%7B/g, '{')
        s = s.replace(/%7C/g, '|')
        s = s.replace(/%7D/g, '}')
        s = s.replace(/%7E/g, '}')
        s = s.replace(/%7F/g, '}')
        return s
    else
        return s

human_readable_shard = (shard) ->
    json_shard = $.parseJSON(shard)
    res = ""
    res += pretty_key(json_shard[0])
    res += " to "
    res += pretty_key(json_shard[1])
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
            contentType: 'application/json'
            url: '/ajax/reset_data',
            success: ->
                console.log 'Reset simulation data.'
        return false

    $('#reset-session').click (e) ->
        $.ajax
            contentType: 'application/json'
            url: '/ajax/reset_session',
            success: ->
                console.log 'Reset session.'
        return false

    $('#fetch-backbone-data').click (e) ->
        collection.fetch() for collection in [datacenters, namespaces, machines]

    $('#make-diff').click (e) ->
        $.ajax
            contentType: 'application/json'
            url: '/ajax/make_diff',
            success: ->
                console.log 'Made diff to simulation data.'
        return false

    $('#visit_bad_route').click (e) ->
        $.get('/fakeurl')
        return false

objects_are_equal = (a, b) ->
    for key of a
        if not key of b
            return false
    for key of b
        if not key of a
            return false

    # They have the same set of keys
    for key of a
        if not key of b
            return false
        else
            if typeof a[key] isnt typeof b[key]
                return false
            else if a[key].constructor? and a[key].constructor is Array
                if b[key].constructor? and b[key].constructor is Array
                    if a[key].length isnt b[key].length
                        return false
                    else
                        for i in [0..a[key].length-1]
                            if a[key][i] isnt b[key][i]
                                return false
                else
                    return false
            else if typeof a[key] is 'object'
                if objects_are_equal(a[key], b[key]) is false
                    return false
            else if a[key] isnt b[key]
                return false
    return true


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

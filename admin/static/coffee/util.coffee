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

# If the two arguments are equal, show the inner block; else block is available
Handlebars.registerHelper 'ifequal', (val_a, val_b, if_block, else_block) ->
    if val_a is val_b
        if_block()
    else if else_block?
        else_block()

# Helpers for pluralization of nouns and verbs
Handlebars.registerHelper 'pluralize_noun', (num) -> if num is 1 then '' else 's'
Handlebars.registerHelper 'pluralize_verb_to_be', (num) -> if num is 1 then 'is' else 'are'

# Helpers for capitalization
Handlebars.registerHelper 'capitalize', (str) -> str.charAt(0).toUpperCase() + str.slice(1)

# Helpers for shortening uuids
Handlebars.registerHelper 'humanize_uuid', (str) -> str.substr(0, 6)

# Dev utility functions and variables
window.pause_live_data = false
window.log_initial = (msg) -> #console.log msg
window.log_render = (msg) ->  #console.log msg
window.log_action = (msg) ->  #console.log msg
window.log_router = (msg) ->  #console.log '-- router -- ' + msg
window.log_binding = (msg) -> #console.log msg
window.log_ajax = (msg) -> #console.log msg
window.class_name = (obj) ->  obj.__proto__.constructor.name

# Date utility functions (helps with testing)
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

iso_date_from_unix_time = (unix_time) -> ISODateString new Date(unix_time * 1000)

# Choose a random model from the given collection
# -----------------------------------------------
random_model_from = (collection) ->_.shuffle(collection.models)[0]

# Generate a date/time stamp in the past for testing
# --------------------------------------------------
time_travel = (minutes) ->
    d = new Date()
    d.setMinutes(d.getMinutes() - minutes)
    return d
# -------------------------------------------
# Create a set of fake events
generate_fake_events = (events) ->
    events.reset [
            priority: 'medium'
            message: 'Machine riddler was disconnected.'
            datetime: ISODateString time_travel 43
        ,
            priority: 'medium'
            message: 'Machine puzzler was disconnected.'
            datetime: ISODateString time_travel 20
        ,
            priority: 'low'
            message: 'Cluster started successfully.'
            datetime: ISODateString time_travel 55
    ]

# Create a set of fake issues
generate_fake_issues = (issues) ->
    issues.reset [
            critical: true
            type: 'master_down'
            datetime: ISODateString time_travel 15
        ,
            critical: true
            type: 'master_down'
            datetime: ISODateString time_travel 24
        ,
            critical: true
            type: 'metadata_conflict'
            datetime: ISODateString time_travel 40
        ,
            critical: false
            type: 'namespace_missing_replicas'
            datetime: ISODateString time_travel 29
        ,
            critical: false
            type: 'datacenter_inaccessible'
            datetime: ISODateString time_travel 59
    ]

# Extract form data as an object
form_data_as_object = (form) ->
    formarray = form.serializeArray()
    formdata = {}
    for x in formarray 
        formdata[x.name] = x.value
    return formdata

# Shards aren't pretty to print, let's change that
human_readable_shard = (shard) ->
    json_shard = $.parseJSON(shard)
    res = ""
    res += if json_shard[0] == "" then "&minus;&infin;" else json_shard[0]
    res += " to "
    res += if json_shard[1] == null then "+&infin;" else json_shard[1]
    return res

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

    $('#random-machine-failure').click (e) ->
        machine = random_model_from machines
        events.add new Event
            priority: 'medium'
            message: "Machine #{machine.get('name')} was disconnected."
            datetime: ISODateString new Date()
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

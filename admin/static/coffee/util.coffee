# Copyright 2010-2012 RethinkDB, all rights reserved.
# Prettifies a date given in Unix time (ms since epoch)

Handlebars = require('hbsfy/runtime');

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
pluralize_noun = (noun, num, capitalize) ->
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

Handlebars.registerHelper 'pluralize_noun', pluralize_noun
Handlebars.registerHelper 'pluralize_verb_to_have', (num) -> if num is 1 then 'has' else 'have'
pluralize_verb = (verb, num) -> if num is 1 then "#{verb}s" else verb
Handlebars.registerHelper 'pluralize_verb', pluralize_verb
#
# Helpers for capitalization
capitalize = (str) ->
    if str?
        str.charAt(0).toUpperCase() + str.slice(1)
    else
        "NULL"
Handlebars.registerHelper 'capitalize', capitalize

# Helpers for shortening uuids
humanize_uuid = (str) -> if str? then str.substr(0, 6) else "NULL"
Handlebars.registerHelper 'humanize_uuid', humanize_uuid

# Helpers for printing connectivity
Handlebars.registerHelper 'humanize_server_connectivity', (status) ->
    if not status?
        status = 'N/A'
    success = if status == 'connected' then 'success' else 'failure'
    connectivity = "<span class='label label-#{success}'>#{capitalize(status)}</span>"
    return new Handlebars.SafeString(connectivity)

humanize_table_status = (status) ->
    if not status
        ""
    else if status.all_replicas_ready or status.ready_for_writes
        "Ready"
    else if status.ready_for_reads
        'Reads only'
    else if status.ready_for_outdated_reads
        'Outdated reads'
    else
        'Unavailable'

Handlebars.registerHelper 'humanize_table_readiness', (status, num, denom) ->
    if status is undefined
        label = 'failure'
        value = 'unknown'
    else if status.all_replicas_ready
        label = 'success'
        value = "#{humanize_table_status(status)} #{num}/#{denom}"
    else if status.ready_for_writes
        label = 'partial-success'
        value = "#{humanize_table_status(status)} #{num}/#{denom}"
    else
        label = 'failure'
        value = humanize_table_status(status)
    return new Handlebars.SafeString(
        "<div class='status label label-#{label}'>#{value}</div>")

Handlebars.registerHelper 'humanize_table_status', humanize_table_status

approximate_count = (num) ->
    # 0 => 0
    # 1 - 5 => 5
    # 5 - 10 => 10
    # 11 - 99 => Rounded to _0
    # 100 - 999 => Rounded to _00
    # 1,000 - 9,999 => _._K
    # 10,000 - 10,000 => __K
    # 100,000 - 1,000,000 => __0K
    # Millions and billions have the same behavior as thousands
    # If num>1000B, then we just print the number of billions
    if num is 0
        return '0'
    else if num <= 5
        return '5'
    else if num <= 10
        return '10'
    else
        # Approximation to 2 significant digit
        approx = Math.round(num/Math.pow(10, num.toString().length-2)) *
            Math.pow(10, num.toString().length-2);
        if approx < 100 # We just want one digit
            return (Math.floor(approx/10)*10).toString()
        else if approx < 1000 # We just want one digit
            return (Math.floor(approx/100)*100).toString()
        else if approx < 1000000
            result = (approx/1000).toString()
            if result.length is 1 # In case we have 4 for 4000, we want 4.0
                result = result + '.0'
            return result+'K'
        else if approx < 1000000000
            result = (approx/1000000).toString()
            if result.length is 1 # In case we have 4 for 4000, we want 4.0
                result = result + '.0'
            return result+'M'
        else
            result = (approx/1000000000).toString()
            if result.length is 1 # In case we have 4 for 4000, we want 4.0
                result = result + '.0'
            return result+'B'

Handlebars.registerHelper 'approximate_count', approximate_count

# Safe string
Handlebars.registerHelper 'print_safe', (str) ->
    if str?
        return new Handlebars.SafeString(str)
    else
        return ""

# Increment a number
Handlebars.registerHelper 'inc', (num) -> num + 1

# if-like block to check whether a value is defined (i.e. not undefined).
Handlebars.registerHelper 'if_defined', (condition, options) ->
    if typeof condition != 'undefined' then return options.fn(this) else return options.inverse(this)

# Extract form data as an object
form_data_as_object = (form) ->
    formarray = form.serializeArray()
    formdata = {}
    for x in formarray
        formdata[x.name] = x.value
    return formdata


stripslashes = (str) ->
    str=str.replace(/\\'/g,'\'')
    str=str.replace(/\\"/g,'"')
    str=str.replace(/\\0/g,"\x00")
    str=str.replace(/\\\\/g,'\\')
    return str

is_integer = (data) ->
    return data.search(/^\d+$/) isnt -1

# Deep copy. We do not copy prototype.
deep_copy = (data) ->
    if typeof data is 'boolean' or typeof data is 'number' or typeof data is 'string' or typeof data is 'number' or data is null or data is undefined
        return data
    else if typeof data is 'object' and Object.prototype.toString.call(data) is '[object Array]'
        result = []
        for value in data
            result.push deep_copy value
        return result
    else if typeof data is 'object'
        result = {}
        for key, value of data
            result[key] = deep_copy value
        return result


# JavaScript doesn't let us set a timezone
# So we create a date shifted of the timezone difference
# Then replace the timezone of the JS date with the one from the ReQL object
date_to_string = (date) ->
    timezone = date.timezone

    # Extract data from the timezone
    timezone_array = date.timezone.split(':')
    sign = timezone_array[0][0] # Keep the sign
    timezone_array[0] = timezone_array[0].slice(1) # Remove the sign

    # Save the timezone in minutes
    timezone_int = (parseInt(timezone_array[0])*60+parseInt(timezone_array[1]))*60
    if sign is '-'
        timezone_int = -1*timezone_int

    # d = real date with user's timezone
    d = new Date(date.epoch_time*1000)

    # Add the user local timezone
    timezone_int += d.getTimezoneOffset()*60

    # d_shifted = date shifted with the difference between the two timezones
    # (user's one and the one in the ReQL object)
    d_shifted = new Date((date.epoch_time+timezone_int)*1000)

    # If the timezone between the two dates is not the same,
    # it means that we changed time between (e.g because of daylight savings)
    if d.getTimezoneOffset() isnt d_shifted.getTimezoneOffset()
        # d_shifted_bis = date shifted with the timezone of d_shifted and not d
        d_shifted_bis = new Date((date.epoch_time+timezone_int-(d.getTimezoneOffset()-d_shifted.getTimezoneOffset())*60)*1000)

        if d_shifted.getTimezoneOffset() isnt d_shifted_bis.getTimezoneOffset()
            # We moved the clock forward -- and therefore cannot generate the appropriate time with JS
            # Let's create the date outselves...
            str_pieces = d_shifted_bis.toString().match(/([^ ]* )([^ ]* )([^ ]* )([^ ]* )(\d{2})(.*)/)
            hours = parseInt(str_pieces[5])
            hours++
            if hours.toString().length is 1
                hours = "0"+hours.toString()
            else
                hours = hours.toString()
            #Note str_pieces[0] is the whole string
            raw_date_str = str_pieces[1]+" "+str_pieces[2]+" "+str_pieces[3]+" "+str_pieces[4]+" "+hours+str_pieces[6]
        else
            raw_date_str = d_shifted_bis.toString()
    else
        raw_date_str = d_shifted.toString()

    # Remove the timezone and replace it with the good one
    return raw_date_str.slice(0, raw_date_str.indexOf('GMT')+3)+timezone

prettify_duration = (duration) ->
    if duration < 1
        return '<1ms'
    else if duration < 1000
        return duration.toFixed(0)+"ms"
    else if duration < 60*1000
        return (duration/1000).toFixed(2)+"s"
    else # We do not expect query to last one hour.
        minutes = Math.floor(duration/(60*1000))
        return minutes+"min "+((duration-minutes*60*1000)/1000).toFixed(2)+"s"

binary_to_string = (bin) ->
    # We print the size of the binary, not the size of the base 64 string
    # We suppose something stronger than the RFC 2045:
    # We suppose that there is ONLY one CRLF every 76 characters
    blocks_of_76 = Math.floor(bin.data.length/78) # 78 to count \r\n
    leftover = bin.data.length-blocks_of_76*78

    base64_digits = 76*blocks_of_76+leftover

    blocks_of_4 = Math.floor(base64_digits/4)

    end = bin.data.slice(-2)
    if end is '=='
        number_of_equals = 2
    else if end.slice(-1) is '='
        number_of_equals = 1
    else
        number_of_equals = 0

    size = 3*blocks_of_4-number_of_equals

    if size >= 1073741824
        sizeStr = (size/1073741824).toFixed(1)+'GB'
    else if size >= 1048576
        sizeStr = (size/1048576).toFixed(1)+'MB'
    else if size >= 1024
        sizeStr = (size/1024).toFixed(1)+'KB'
    else if size is 1
        sizeStr = size+' byte'
    else
        sizeStr = size+' bytes'


    # Compute a snippet and return the <binary, size, snippet> result
    if size is 0
        return "<binary, #{sizeStr}>"
    else
        str = atob bin.data.slice(0, 8)
        snippet = ''
        for char, i  in str
            next = str.charCodeAt(i).toString(16)
            if next.length is 1
                next = "0" + next
            snippet += next

            if i isnt str.length-1
                snippet += " "
        if size > str.length
            snippet += "..."

        return "<binary, #{sizeStr}, \"#{snippet}\">"

# Render a datum as a pretty tree
json_to_node = do ->
    template =
        span: require('../handlebars/dataexplorer_result_json_tree_span.hbs')
        span_with_quotes: require('../handlebars/dataexplorer_result_json_tree_span_with_quotes.hbs')
        url: require('../handlebars/dataexplorer_result_json_tree_url.hbs')
        email: require('../handlebars/dataexplorer_result_json_tree_email.hbs')
        object: require('../handlebars/dataexplorer_result_json_tree_object.hbs')
        array: require('../handlebars/dataexplorer_result_json_tree_array.hbs')

    # We build the tree in a recursive way
    return json_to_node = (value) ->
        value_type = typeof value

        output = ''
        if value is null
            return template.span
                classname: 'jt_null'
                value: 'null'
        else if Object::toString.call(value) is '[object Array]'
            if value.length is 0
                return '[ ]'
            else
                sub_values = []
                for element in value
                    sub_values.push
                        value: json_to_node element
                    if typeof element is 'string' and (/^(http|https):\/\/[^\s]+$/i.test(element) or  /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(element))
                        sub_values[sub_values.length-1]['no_comma'] = true


                sub_values[sub_values.length-1]['no_comma'] = true
                return template.array
                    values: sub_values
        else if Object::toString.call(value) is '[object Object]' and value.$reql_type$ is 'TIME'
            return template.span
                classname: 'jt_date'
                value: date_to_string(value)
        else if Object::toString.call(value) is '[object Object]' and value.$reql_type$ is 'BINARY'
            return template.span
                classname: 'jt_bin'
                value: binary_to_string(value)

        else if value_type is 'object'
            sub_keys = []
            for key of value
                sub_keys.push key
            sub_keys.sort()

            sub_values = []
            for key in sub_keys
                last_key = key
                sub_values.push
                    key: key
                    value: json_to_node value[key]
                # We don't add a coma for url and emails, because we put it in value (value = url, >>)
                if typeof value[key] is 'string' and ((/^(http|https):\/\/[^\s]+$/i.test(value[key]) or /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(value[key])))
                    sub_values[sub_values.length-1]['no_comma'] = true

            if sub_values.length isnt 0
                sub_values[sub_values.length-1]['no_comma'] = true

            data =
                no_values: false
                values: sub_values

            if sub_values.length is 0
                data.no_value = true

            return template.object data
        else if value_type is 'number'
            return template.span
                classname: 'jt_num'
                value: value
        else if value_type is 'string'
            if /^(http|https):\/\/[^\s]+$/i.test(value)
                return template.url
                    url: value
            else if /^[-0-9a-z.+_]+@[-0-9a-z.+_]+\.[a-z]{2,4}/i.test(value) # We don't handle .museum extension and special characters
                return template.email
                    email: value
            else
                return template.span_with_quotes
                    classname: 'jt_string'
                    value: value
        else if value_type is 'boolean'
            return template.span
                classname: 'jt_bool'
                value: if value then 'true' else 'false'


class BackboneViewWidget
    # An adapter class to allow a Backbone View to act as a
    # virtual-dom Widget. This allows the view to update itself and
    # not have to worry about being destroyed and reinserted, causing
    # loss of listeners etc.
    type: "Widget"  # required by virtual-dom to detect this as a widget.
    constructor: (thunk) ->
        # The constructor doesn't create the view or cause a render
        # because many many widgets will be created, and they need to
        # be very lightweight.
        @thunk = thunk
        @view = null
    init: () ->
        # init is called when virtual-dom first notices a widget. This
        # will initialize the view with the given options, render the
        # view, and return its DOM element for virtual-dom to place in
        # the document
        @view = @thunk()
        @view.render()
        @view.el  # init needs to return a raw dom element
    update: (previous, domNode) ->
        # Every time the vdom is updated, a new widget will be
        # created, which will be passed the old widget (previous). We
        # need to copy the state from the old widget over and return
        # null to indicate the DOM element should be re-used and not
        # replaced.
        @view = previous.view
        return null
    destroy: (domNode) ->
        # This is called when virtual-dom detects the widget should be
        # removed from the tree. It just calls `.remove()` on the
        # underlying view so listeners etc can be cleaned up.
        @view?.remove()


exports.capitalize = capitalize
exports.humanize_table_status = humanize_table_status
exports.form_data_as_object = form_data_as_object
exports.stripslashes = stripslashes
exports.is_integer = is_integer
exports.deep_copy = deep_copy
exports.date_to_string = date_to_string
exports.prettify_duration = prettify_duration
exports.binary_to_string = binary_to_string
exports.json_to_node = json_to_node
exports.approximate_count = approximate_count
exports.pluralize_noun = pluralize_noun
exports.pluralize_verb = pluralize_verb
exports.humanize_uuid = humanize_uuid
exports.BackboneViewWidget = BackboneViewWidget

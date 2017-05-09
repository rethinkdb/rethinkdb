// Copyright 2010-2012 RethinkDB, all rights reserved.
// Prettifies a date given in Unix time (ms since epoch)

const Handlebars = require('hbsfy/runtime')

// Returns a comma-separated list of the provided array
Handlebars.registerHelper('comma_separated', (context, block) => {
  let out = ''
  for (let i = 0, end = context.length, asc = end >= 0; asc ? i < end : i > end; asc ? i++ : i--) {
    out += block(context[i])
    if (i !== context.length - 1) {
      out += ', '
    }
  }
  return out
})

// Returns a comma-separated list of the provided array without the need of a transformation
Handlebars.registerHelper('comma_separated_simple', context => {
  let out = ''
  for (let i = 0, end = context.length, asc = end >= 0; asc ? i < end : i > end; asc ? i++ : i--) {
    out += context[i]
    if (i !== context.length - 1) {
      out += ', '
    }
  }
  return out
})

// Returns a list to links to servers
Handlebars.registerHelper('links_to_servers', (servers, safety) => {
  let out = ''
  for (let i = 0, end = servers.length, asc = end >= 0; asc ? i < end : i > end; asc ? i++ : i--) {
    if (servers[i].exists) {
      out += `<a href="#servers/${servers[i].id}" class="links_to_other_view">${servers[
        i
      ].name}</a>`
    } else {
      out += servers[i].name
    }
    if (i !== servers.length - 1) {
      out += ', '
    }
  }
  if (safety != null && safety === false) {
    return out
  }
  return new Handlebars.SafeString(out)
})

// Returns a list of links to datacenters on one line
Handlebars.registerHelper('links_to_datacenters_inline_for_replica', datacenters => {
  let out = ''
  for (
    let i = 0, end = datacenters.length, asc = end >= 0;
    asc ? i < end : i > end;
    asc ? i++ : i--
  ) {
    out += `<strong>${datacenters[i].name}</strong>`
    if (i !== datacenters.length - 1) {
      out += ', '
    }
  }
  return new Handlebars.SafeString(out)
})

// Helpers for pluralization of nouns and verbs
const pluralize_noun = (noun, num, capitalize) => {
  let result
  if (noun == null) {
    return 'NOUN_NULL'
  }
  const ends_with_y = noun.substr(-1) === 'y'
  if (num === 1) {
    result = noun
  } else {
    if (ends_with_y && noun !== 'key') {
      result = `${noun.slice(0, noun.length - 1)}ies`
    } else if (noun.substr(-1) === 's') {
      result = `${noun}es`
    } else if (noun.substr(-1) === 'x') {
      result = `${noun}es`
    } else {
      result = `${noun}s`
    }
  }
  if (capitalize === true) {
    result = result.charAt(0).toUpperCase() + result.slice(1)
  }
  return result
}

Handlebars.registerHelper('pluralize_noun', pluralize_noun)
Handlebars.registerHelper('pluralize_verb_to_have', num => {
  if (num === 1) {
    return 'has'
  } else {
    return 'have'
  }
})
const pluralize_verb = (verb, num) => {
  if (num === 1) {
    return `${verb}s`
  } else {
    return verb
  }
}
Handlebars.registerHelper('pluralize_verb', pluralize_verb)
//
// Helpers for capitalization
const capitalize = str => {
  if (str != null) {
    return str.charAt(0).toUpperCase() + str.slice(1)
  } else {
    return 'NULL'
  }
}
Handlebars.registerHelper('capitalize', capitalize)

// Helpers for shortening uuids
const humanize_uuid = str => {
  if (str != null) {
    return str.substr(0, 6)
  } else {
    return 'NULL'
  }
}
Handlebars.registerHelper('humanize_uuid', humanize_uuid)

// Helpers for printing connectivity
Handlebars.registerHelper('humanize_server_connectivity', status => {
  if (status == null) {
    status = 'N/A'
  }
  const success = status === 'connected' ? 'success' : 'failure'
  const connectivity = `<span class='label label-${success}'>${capitalize(status)}</span>`
  return new Handlebars.SafeString(connectivity)
})

const humanize_table_status = status => {
  if (!status) {
    return ''
  } else if (status.all_replicas_ready || status.ready_for_writes) {
    return 'Ready'
  } else if (status.ready_for_reads) {
    return 'Reads only'
  } else if (status.ready_for_outdated_reads) {
    return 'Outdated reads'
  } else {
    return 'Unavailable'
  }
}

Handlebars.registerHelper('humanize_table_readiness', (status, num, denom) => {
  let label
  let value
  if (status === undefined) {
    label = 'failure'
    value = 'unknown'
  } else if (status.all_replicas_ready) {
    label = 'success'
    value = `${humanize_table_status(status)} ${num}/${denom}`
  } else if (status.ready_for_writes) {
    label = 'partial-success'
    value = `${humanize_table_status(status)} ${num}/${denom}`
  } else {
    label = 'failure'
    value = humanize_table_status(status)
  }
  return new Handlebars.SafeString(`<div class='status label label-${label}'>${value}</div>`)
})

Handlebars.registerHelper('humanize_table_status', humanize_table_status)

const approximate_count = num => {
  // 0 => 0
  // 1 - 5 => 5
  // 5 - 10 => 10
  // 11 - 99 => Rounded to _0
  // 100 - 999 => Rounded to _00
  // 1,000 - 9,999 => _._K
  // 10,000 - 10,000 => __K
  // 100,000 - 1,000,000 => __0K
  // Millions and billions have the same behavior as thousands
  // If num>1000B, then we just print the number of billions
  if (num === 0) {
    return '0'
  } else if (num <= 5) {
    return '5'
  } else if (num <= 10) {
    return '10'
  } else {
    // Approximation to 2 significant digit
    let result
    const approx = Math.round(num / Math.pow(10, num.toString().length - 2)) *
      Math.pow(10, num.toString().length - 2)
    if (approx < 100) {
      // We just want one digit
      return (Math.floor(approx / 10) * 10).toString()
    } else if (approx < 1000) {
      // We just want one digit
      return (Math.floor(approx / 100) * 100).toString()
    } else if (approx < 1000000) {
      result = (approx / 1000).toString()
      if (result.length === 1) {
        // In case we have 4 for 4000, we want 4.0
        result = `${result}.0`
      }
      return `${result}K`
    } else if (approx < 1000000000) {
      result = (approx / 1000000).toString()
      if (result.length === 1) {
        // In case we have 4 for 4000, we want 4.0
        result = `${result}.0`
      }
      return `${result}M`
    } else {
      result = (approx / 1000000000).toString()
      if (result.length === 1) {
        // In case we have 4 for 4000, we want 4.0
        result = `${result}.0`
      }
      return `${result}B`
    }
  }
}

Handlebars.registerHelper('approximate_count', approximate_count)

// formatBytes
// from http://stackoverflow.com/a/18650828/180718
const format_bytes = (bytes, decimals = 1) => {
  if (bytes === 0) {
    return '0 Bytes'
  }
  const k = 1024
  const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return `${(bytes / Math.pow(k, i)).toFixed(decimals)} ${sizes[i]}`
}

// Safe string
Handlebars.registerHelper('print_safe', str => {
  if (str != null) {
    return new Handlebars.SafeString(str)
  } else {
    return ''
  }
})

// Increment a number
Handlebars.registerHelper('inc', num => num + 1)

// if-like block to check whether a value is defined (i.e. not undefined).
Handlebars.registerHelper('if_defined', function (condition, options) {
  if (typeof condition !== 'undefined') {
    return options.fn(this)
  } else {
    return options.inverse(this)
  }
})

// Extract form data as an object
const form_data_as_object = form => {
  const formarray = form.serializeArray()
  const formdata = {}
  for (let x of Array.from(formarray)) {
    formdata[x.name] = x.value
  }
  return formdata
}

const stripslashes = str => {
  str = str.replace(/\\'/g, "'")
  str = str.replace(/\\"/g, '"')
  str = str.replace(/\\0/g, '\x00')
  str = str.replace(/\\\\/g, '\\')
  return str
}

const is_integer = data => data.search(/^\d+$/) !== -1

// Deep copy. We do not copy prototype.
const deep_copy = data => {
  let result
  let value
  if (
    typeof data === 'boolean' ||
    typeof data === 'number' ||
    typeof data === 'string' ||
    typeof data === 'number' ||
    data === null ||
    data === undefined
  ) {
    return data
  } else if (
    typeof data === 'object' && Object.prototype.toString.call(data) === '[object Array]'
  ) {
    result = []
    for (value of Array.from(data)) {
      result.push(deep_copy(value))
    }
    return result
  } else if (typeof data === 'object') {
    result = {}
    for (let key in data) {
      value = data[key]
      result[key] = deep_copy(value)
    }
    return result
  }
}

// JavaScript doesn't let us set a timezone
// So we create a date shifted of the timezone difference
// Then replace the timezone of the JS date with the one from the ReQL object
const date_to_string = date => {
  let raw_date_str
  const { timezone } = date

  // Extract data from the timezone
  const timezone_array = date.timezone.split(':')
  const sign = timezone_array[0][0] // Keep the sign
  timezone_array[0] = timezone_array[0].slice(1) // Remove the sign

  // Save the timezone in minutes
  let timezone_int = (parseInt(timezone_array[0]) * 60 + parseInt(timezone_array[1])) * 60
  if (sign === '-') {
    timezone_int = (-1) * timezone_int
  }

  // d = real date with user's timezone
  const d = new Date(date.epoch_time * 1000)

  // Add the user local timezone
  timezone_int += d.getTimezoneOffset() * 60

  // d_shifted = date shifted with the difference between the two timezones
  // (user's one and the one in the ReQL object)
  const d_shifted = new Date((date.epoch_time + timezone_int) * 1000)

  // If the timezone between the two dates is not the same,
  // it means that we changed time between (e.g because of daylight savings)
  if (d.getTimezoneOffset() !== d_shifted.getTimezoneOffset()) {
    // d_shifted_bis = date shifted with the timezone of d_shifted and not d
    const d_shifted_bis = new Date(
      (date.epoch_time +
        timezone_int -
        (d.getTimezoneOffset() - d_shifted.getTimezoneOffset()) * 60) *
        1000
    )

    if (d_shifted.getTimezoneOffset() !== d_shifted_bis.getTimezoneOffset()) {
      // We moved the clock forward -- and therefore cannot generate the appropriate time with JS
      // Let's create the date outselves...
      const str_pieces = d_shifted_bis
        .toString()
        .match(/([^ ]* )([^ ]* )([^ ]* )([^ ]* )(\d{2})(.*)/)
      let hours = parseInt(str_pieces[5])
      hours++
      if (hours.toString().length === 1) {
        hours = `0${hours.toString()}`
      } else {
        hours = hours.toString()
      }
      // Note str_pieces[0] is the whole string
      raw_date_str = `${str_pieces[1]} ${str_pieces[2]} ${str_pieces[3]} ${str_pieces[
        4
      ]} ${hours}${str_pieces[6]}`
    } else {
      raw_date_str = d_shifted_bis.toString()
    }
  } else {
    raw_date_str = d_shifted.toString()
  }

  // Remove the timezone and replace it with the good one
  return raw_date_str.slice(0, raw_date_str.indexOf('GMT') + 3) + timezone
}

const prettify_duration = duration => {
  if (duration === null) {
    return ''
  } else if (duration < 1) {
    return '<1ms'
  } else if (duration < 1000) {
    return `${duration.toFixed(0)}ms`
  } else if (duration < 60 * 1000) {
    return `${(duration / 1000).toFixed(2)}s`
  } else {
    // We do not expect query to last one hour.
    const minutes = Math.floor(duration / (60 * 1000))
    return `${minutes}min ${((duration - minutes * 60 * 1000) / 1000).toFixed(2)}s`
  }
}

const binary_to_string = ({ data }) => {
  // We print the size of the binary, not the size of the base 64 string
  // We suppose something stronger than the RFC 2045:
  // We suppose that there is ONLY one CRLF every 76 characters
  let number_of_equals

  let sizeStr
  const blocks_of_76 = Math.floor(data.length / 78) // 78 to count \r\n
  const leftover = data.length - blocks_of_76 * 78

  const base64_digits = 76 * blocks_of_76 + leftover

  const blocks_of_4 = Math.floor(base64_digits / 4)

  const end = data.slice(-2)
  if (end === '==') {
    number_of_equals = 2
  } else if (end.slice(-1) === '=') {
    number_of_equals = 1
  } else {
    number_of_equals = 0
  }

  const size = 3 * blocks_of_4 - number_of_equals

  if (size >= 1073741824) {
    sizeStr = `${(size / 1073741824).toFixed(1)}GB`
  } else if (size >= 1048576) {
    sizeStr = `${(size / 1048576).toFixed(1)}MB`
  } else if (size >= 1024) {
    sizeStr = `${(size / 1024).toFixed(1)}KB`
  } else if (size === 1) {
    sizeStr = `${size} byte`
  } else {
    sizeStr = `${size} bytes`
  }

  // Compute a snippet and return the <binary, size, snippet> result
  if (size === 0) {
    return `<binary, ${sizeStr}>`
  } else {
    const str = atob(data.slice(0, 8))
    let snippet = ''

    str.forEach((char, i) => {
      let next = str.charCodeAt(i).toString(16)
      if (next.length === 1) {
        next = `0${next}`
      }
      snippet += next

      if (i !== str.length - 1) {
        snippet += ' '
      }
    })

    if (size > str.length) {
      snippet += '...'
    }

    return `<binary, ${sizeStr}, \"${snippet}\">`
  }
}

// Render a datum as a pretty tree
let json_to_node = (() => {
  const template = {
    span            : require('../handlebars/dataexplorer_result_json_tree_span.hbs'),
    span_with_quotes: require('../handlebars/dataexplorer_result_json_tree_span_with_quotes.hbs'),
    url             : require('../handlebars/dataexplorer_result_json_tree_url.hbs'),
    email           : require('../handlebars/dataexplorer_result_json_tree_email.hbs'),
    object          : require('../handlebars/dataexplorer_result_json_tree_object.hbs'),
    array           : require('../handlebars/dataexplorer_result_json_tree_array.hbs'),
  }

  // We build the tree in a recursive way
  return json_to_node = value => {
    let sub_values
    const value_type = typeof value

    const output = ''
    if (value === null) {
      return template.span({
        classname: 'jt_null',
        value    : 'null',
      })
    } else if (Object.prototype.toString.call(value) === '[object Array]') {
      if (value.length === 0) {
        return '[ ]'
      } else {
        sub_values = []
        for (let element of Array.from(value)) {
          sub_values.push({
            value: json_to_node(element),
          })
          if (
            typeof element === 'string' &&
            (/^(http|https):\/\/[^\s]+$/i.test(element) ||
              /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(element))
          ) {
            sub_values[sub_values.length - 1]['no_comma'] = true
          }
        }

        sub_values[sub_values.length - 1]['no_comma'] = true
        return template.array({
          values: sub_values,
        })
      }
    } else if (
      Object.prototype.toString.call(value) === '[object Object]' && value.$reql_type$ === 'TIME'
    ) {
      return template.span({
        classname: 'jt_date',
        value    : date_to_string(value),
      })
    } else if (
      Object.prototype.toString.call(value) === '[object Object]' && value.$reql_type$ === 'BINARY'
    ) {
      return template.span({
        classname: 'jt_bin',
        value    : binary_to_string(value),
      })
    } else if (value_type === 'object') {
      const sub_keys = []
      for (var key in value) {
        sub_keys.push(key)
      }
      sub_keys.sort()

      sub_values = []
      for (key of Array.from(sub_keys)) {
        const last_key = key
        sub_values.push({
          key,
          value: json_to_node(value[key]),
        })
        // We don't add a coma for url and emails, because we put it in value (value = url, >>)
        if (
          typeof value[key] === 'string' &&
          (/^(http|https):\/\/[^\s]+$/i.test(value[key]) ||
            /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(value[key]))
        ) {
          sub_values[sub_values.length - 1]['no_comma'] = true
        }
      }

      if (sub_values.length !== 0) {
        sub_values[sub_values.length - 1]['no_comma'] = true
      }

      const data = {
        no_values: false,
        values   : sub_values,
      }

      if (sub_values.length === 0) {
        data.no_value = true
      }

      return template.object(data)
    } else if (value_type === 'number') {
      return template.span({
        classname: 'jt_num',
        value,
      })
    } else if (value_type === 'string') {
      if (/^(http|https):\/\/[^\s]+$/i.test(value)) {
        return template.url({
          url: value,
        })
      } else if (/^[-0-9a-z.+_]+@[-0-9a-z.+_]+\.[a-z]{2,4}/i.test(value)) {
        // We don't handle .museum extension and special characters
        return template.email({
          email: value,
        })
      } else {
        return template.span_with_quotes({
          classname: 'jt_string',
          value,
        })
      }
    } else if (value_type === 'boolean') {
      return template.span({
        classname: 'jt_bool',
        value    : value ? 'true' : 'false',
      })
    }
  }
})()

const replica_rolename = (
  {
    configured_primary: configured,
    currently_primary: currently,
    nonvoting,
  }
) => {
  if (configured && currently) {
    return 'Primary replica'
  } else if (configured && !currently) {
    return 'Goal primary replica'
  } else if (!configured && currently) {
    return 'Acting primary replica'
  } else if (nonvoting) {
    return 'Non-voting secondary replica'
  } else {
    return 'Secondary replica'
  }
}

const replica_roleclass = (
  {
    configured_primary: configured,
    currently_primary: currently,
  }
) => {
  if (configured && currently) {
    return 'primary'
  } else if (configured && !currently) {
    return 'goal.primary'
  } else if (!configured && currently) {
    return 'acting.primary'
  } else {
    return 'secondary'
  }
}

const state_color = state => {
  switch (state) {
    case 'ready':
      return 'green'
    case 'disconnected':
      return 'red'
    default:
      return 'yellow'
  }
}

const humanize_state_string = state_string => state_string.replace(/_/g, ' ')

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
exports.replica_rolename = replica_rolename
exports.replica_roleclass = replica_roleclass
exports.state_color = state_color
exports.humanize_state_string = humanize_state_string
exports.format_bytes = format_bytes

goog.provide('rethinkdb.server.Utils')

# Class of static helper functions
class Utils
    @checkName: (name) ->
        if not /^[A-Za-z0-9_\-]+$/.test name
            throw new BadQuery "Invalid name '#{name}'.  (Use A-Za-z0-9_ only.)."

    # We have this to match the JSON.stringify implementation on the server
    # If we can make the tests whitespace independent then we can replace this
    # with JSON.stringify
    @stringify: (doc, level) ->
        if not level?
            level = 0
        result = ''
        if typeof doc is 'object'
            if doc is null
                result += 'null'
            else if Object.prototype.toString.call(doc) is '[object Array]'
                result += '['
                for element, i in doc
                    result += Utils.stringify(element, level+1)
                    if i isnt doc.length-1
                        result += ', '
                result += ']'
            else
                result = '{'
                is_first = true
                for own key, value of doc
                    if is_first is true
                        is_first = false
                    else
                        result += ','
                    result += '\n'
                    if level > 0
                        for i in [0..level]
                            result += '\t'
                    else
                        result += '\t'
                    result += JSON.stringify(key)+':\t'+Utils.stringify(value, level+1)
                result += '\n'
                if level > 0
                    for i in [0..level-1]
                        result += '\t'
                result += '}'
        else
            result += JSON.stringify doc

        return result

    

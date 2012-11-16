# Helpers
class Helpers
    generate_uuid: =>
        # That should be good enough for the client server
        result = (Math.random()+'').slice(2)+(Math.random()+'').slice(2)
        result = CryptoJS.SHA1(result)+''
        result = result.slice(0, 8)+'-'+result.slice(8,12)+'-'+result.slice(12, 16)+'-'+result.slice(16, 20)+'-'+result.slice(20, 32)
        return result

    are_equal: (a, b) =>
        if typeof a isnt typeof b
            return false
        else
            if typeof a is 'object' and typeof b is 'object'
                if Object.prototype.toString.call(a) is '[object Array]' and Object.prototype.toString.call(b) is '[object Array]'
                    if a.length isnt b.length
                        return false
                    else
                        for a_value, i in a
                            if @are_equal(a[i], b[i]) is false
                                return false
                        return true
                else if a is null and b isnt null
                    return false
                else if b is null and a isnt null
                    return false
                else
                    for key of a
                        if @are_equal(a[key], b[key]) is false
                            return false
                    for key of b
                        if @are_equal(a[key], b[key]) is false
                            return false
                    return true
            else
                return a is b

    compare: (a, b) =>
        # Returns a-b, so a > b
        value =
            array: 1
            boolean: 2
            null: 3
            number: 4
            object: 5
            string: 6
        if typeof a is typeof b
            switch typeof a
                when 'array'
                    for value, i in a
                        result = @compare(a[i], b[i])
                        if result isnt 0
                            return result
                    return len(a)-len(b)
                when 'boolean' # false < true
                    if a is b
                        return 0
                    else if a is false
                        return -1
                    else
                        return 1
                when 'object'
                    if a is null and b is null
                        return 0
                    else
                        return { error: true }
                when 'number'
                    return a-b
                when 'string'
                    if a > b
                        return 1
                    else if a < b
                        return -1
                    else
                        return 0
        else
            type_a = typeof a
            if type_a is 'object' and a isnt null
                return {error: true}
            type_b = typeof b
            if type_b is 'object' and b isnt null
                return {error: true}

            return value[type_a] - value[type_b]



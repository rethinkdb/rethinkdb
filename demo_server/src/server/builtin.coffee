class Builtin
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        builtin_args = args.builtin_args
        context = args.context
        mapping = args.mapping
        var_args = args.var_args
        order_by_keys = args.order_by_keys
        order_by_asc = args.order_by_asc
        skip_value = args.skip_value
        limit_value = args.limit_value


        type = @data.getType()
        switch type
            when 1 # Not
                response = new Response
                if builtin_args[0].getType() isnt 9
                    response.setStatusCode 103
                    response.setErrorMessage 'Not can only be called on a boolean'
                    #TODO Backtrace
                    return response
                
                response.setStatusCode 1
                response.addResponse not builtin_args[0].getValuebool()
                return response

            when 2 # Getattr
                term = new Term builtin_args[0]
                evaluation = term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                original_object = JSON.parse evaluation.getResponse()

                attr = @data.getAttr()

                response = new Response
                if typeof attr isnt 'string'
                    response.setStatusCode 103
                    response.addResponse JSON.stringify 'TypeError: Function argument '+JSON.strinfigy yattr+' must be of the type string'
                    # That's a poor answer, especially if attr is an object. But I'm doing the same thing as our actual server has for now.
                else if not original_object[attr]?
                    response.setStatusCode 103
                    response.addResponse JSON.stringify 'Runtime Error: Object:'+ JSON.stringify(original_object, undefined, 4)+'is missing attribute '+JSON.stringify attr
                    #TODO backtrace
                else
                    response.setStatusCode 1
                    response.addResponse JSON.stringify original_object[attr]
                return response
            #when 3 # Implicit getattr
            when 4 # hasattr
                term = new Term builtin_args[0]
                evaluation = term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                original_object = JSON.parse evaluation.getResponse()

                attr = @data.getAttr()

                response = new Response
                if typeof attr isnt 'string'
                    response.setStatusCode 103
                    response.addResponse JSON.stringify 'TypeError: Function argument '+JSON.strinfigy yattr+' must be of the type string'
                    # That's a poor answer, especially if attr is an object. But I'm doing the same thing as our actual server has for now.
                else if original_object[attr] is undefined # Coffeescript ternary opertor check for null too.
                    response.setStatusCode 1
                    response.addResponse JSON.stringify false
                else
                    response.setStatusCode 1
                    response.addResponse JSON.stringify true
                return response
            #when 5 # implicit hasattr
            when 6 # pickattrs
                new_term = new Term builtin_args[0]
                evaluation = new_term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                original_object = JSON.parse evaluation.getResponse()
                attrs = @data.attrsArray()
                new_object = {}
                for attr in attrs
                    new_object[attr] = original_object[attr]

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify new_object
                return response
            #when 7 # implicit pickattrs
            when 8 # map merge
                #TODO Handle errors?
                result_term = new Term builtin_args[0]
                evaluation = result_term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value
                result_object = JSON.parse evaluation.getResponse()

                extra_term = new Term builtin_args[1]
                
                evaluation = extra_term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value
                extra_object = JSON.parse evaluation.getResponse()
               
                for key, value of extra_object
                    result_object[key] = value

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify result_object
                return response
            when 9 # array append
                #TODO Handle errors?
                result_term = new Term builtin_args[0]
                evaluation = result_term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value

                result_array = JSON.parse evaluation.getResponse()

                new_term = new Term builtin_args[1]
                evaluation = new_term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value

                new_term_value = JSON.parse evaluation.getResponse()
                
                result_array.push new_term_value

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify result_array
                return response
            when 11 # slice
                term = new Term builtin_args[0]
                new_skip_value = new Term builtin_args[1]
                new_limit_value = new Term builtin_args[2]

                if new_skip_value?
                    new_skip_value = JSON.parse new_skip_value.evaluate(args).getResponse()
                if new_limit_value?
                    new_limit_value = JSON.parse new_limit_value.evaluate(args).getResponse()
                if new_limit_value is null
                    new_limit_value = Infinity

                # I'm not sure that merging the new skip/limit with the old ones is always safe.


                evaluation = term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: new_skip_value
                    limit_value: new_limit_value

                response = new Response
                response.setStatusCode 3
                results = evaluation.responseArray()

                skipped = 0
                added = 0
                for result in results
                    if added >= new_limit_value
                        break
                    if skipped < new_skip_value
                        skipped++
                        continue

                    response.addResponse result
                    added++
                return response

            when 14 # Add
                response = new Response
                term1 = new Term builtin_args[0]
                term2 = new Term builtin_args[1]

                evaluation1 = term1.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value
                evaluation2 = term2.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value

                response1 = JSON.parse evaluation1.getResponse()
                response2 = JSON.parse evaluation2.getResponse()
                
                if (typeof response1 isnt typeof response2) and (typeof response1 isnt 'number' and Object.prototype.toString.call(response1) isnt '[object Array]') and (typeof response2 isnt 'number' and Object.prototype.toString.call(response2) isnt '[object Array]')
                    response.setStatusCode 103
                    response.setErrorMessage 'Can only ADD numbers with number and arrays with arrays'
                    #TODO Backtrace
                    return response

                if typeof response1 is 'number'
                    result = response1+response2
                else # We have two arrays
                    result = response1.concat response2
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 15 # Substract
                response = new Response
                if builtin_args[0].getType() isnt 6 or builtin_args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to SUBSTRACT must be numbers'
                    #TODO Backtrace
                    return response

                result = builtin_args[0].getNumber() - builtin_args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 16 # Multiply
                response = new Response
                if builtin_args[0].getType() isnt 6 or builtin_args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to MULTIPLY must be numbers'
                    #TODO Backtrace
                    return response

                result = builtin_args[0].getNumber() * builtin_args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 17 # Divide
                response = new Response
                if builtin_args[0].getType() isnt 6 or builtin_args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to DIVIDE  must be numbers'
                    #TODO Backtrace
                    return response

                result = builtin_args[0].getNumber() / builtin_args[1].getNumber()
                result = parseFloat result.toFixed 6
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 18 # Modulo
                response = new Response
                if builtin_args[0].getType() isnt 6 or builtin_args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to DIVIDE  must be numbers'
                    #TODO Backtrace
                    return response

                result = builtin_args[0].getNumber()%builtin_args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response
            when 19 # Compare
                term1 = new Term builtin_args[0]
                evaluation = term1.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                term1_value = JSON.parse evaluation.getResponse()
                term2 = new Term builtin_args[1]
                evaluation = term2.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                term2_value = JSON.parse evaluation.getResponse()


                response = new Response()
                comparison = @data.getComparison()
                switch comparison
                    when 1 # eq
                        result = Helpers.prototype.are_equal term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result is true)
                    when 2 # ne
                        result = Helpers.prototype.are_equal term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result is false)
                    when 3 # lt
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result < 0)
                    when 4 #le
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result <= 0)
                    when 5 #gt
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result > 0)
                    when 6 #ge
                        result = Helpers.prototype.compare term1_value, term2_value
                        response.setStatusCode 1
                        response.addResponse JSON.stringify(result >= 0)
                return response
                    
            when 20 # Filter
                ###
                terms = []
                for term_data in builtin_args
                    terms.push new Term term_data
                ###
                term = new Term builtin_args[0]
                builtin_filter = new BuiltinFilter @data.getFilter()
                return builtin_filter.evaluate
                    server: server
                    term: term
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value

            when 21 # Map
                term = new Term builtin_args[0]
                builtin_map = new BuiltinMap @data.getMap()
                return builtin_map.evaluate
                    server: server
                    term: term
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value

            when 22 # Concatmap
                term = new Term builtin_args[0]
                builtin_concat_map = new BuiltinConcatMap @data.getConcatMap()
                return builtin_concat_map.evaluate
                    server: server
                    term: term
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value

            when 23 # Orderby
                term = new Term builtin_args[0]
                order_by_args = @data.orderByArray()

                order_by_keys = []
                order_by_asc = []
                #order_by is a Builtin.OrderBy object (cf. proto)
                for order_by in @data.orderByArray()
                    order_by_keys.push order_by.getAttr()
                    order_by_asc.push order_by.getAscending()

                evaluation = term.evaluate
                    server: server
                    term: term
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                results = evaluation.responseArray()
                debugger
                array_to_sort = []
                for result in results

                    array_to_sort.push JSON.parse result

                try
                    Helpers.prototype.sort array_to_sort, order_by_keys, order_by_asc
                catch error
                    debugger
                    response.setStatusCode 103
                    response.setErrorMessage error
                    return response

                results = []
                for result in array_to_sort
                    results.push result

                response = new Response
                for result in results
                    response.addResponse JSON.stringify result
                response.setStatusCode 3
                return response
            #when 24 # Distinct
            #when 26 # Length
            #when 27 # Union
            #when 28 # Nth
            #when 29 # Stream to array
            #when 30 # Array to strean
            #when 31 # Reduce
            #when 32 # Group mapreduce
            when 35 # Any
                term1 = new Term builtin_args[0]
                evaluation = term1.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                term1_value = JSON.parse evaluation.getResponse()
                term2 = new Term builtin_args[1]
                evaluation = term2.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                term2_value = JSON.parse evaluation.getResponse()

                response = new Response()
                response.setStatusCode 1
                response.addResponse JSON.stringify(term1_value or term2_value)
                return response

            when 36 # All
                term1 = new Term builtin_args[0]
                evaluation = term1.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                term1_value = JSON.parse evaluation.getResponse()
                term2 = new Term builtin_args[1]
                evaluation = term2.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    skip_value: skip_value
                    limit_value: limit_value
                term2_value = JSON.parse evaluation.getResponse()

                response = new Response()
                response.setStatusCode 1
                response.addResponse JSON.stringify(term1_value and term2_value)
                return response
            when 37 # Range
                term = new Term builtin_args[0]
                builtin_range = new BuiltinRange @data.getRange()
                return builtin_range.evaluate
                    server: server
                    term: term
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value
            #when 38 # Implicit without
            when 39 # Without
                new_term = new Term builtin_args[0]
                
                evaluation = new_term.evaluate
                    server: server
                    context: context
                    mapping: mapping
                    var_args: var_args
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value
                original_object = JSON.parse evaluation.getResponse()
                new_object = {}
                for key, value of original_object
                    new_object[key] = value
                attrs = @data.attrsArray()
                for attr in attrs
                    delete new_object[attr]

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify new_object
                return response
    range: (args) ->
        #TODO Hum, I lost the context somewhere on my way here.
        server = args.server
        builtin_args = args.builtin_args
        attr_name = args.attr_name
        lower_bound = args.lower_bound
        upper_bound = args.upper_bound
        context = args.context
        order_by_keys = args.order_by_keys
        order_by_asc = args.order_by_asc
        skip_value = args.skip_value
        limit_value = args.limit_value


        type = @data.getType()
        switch type
            when 20 # Filter
                term = new Term builtin_args[0] # TODO Is it safe?
                builtin_filter = new BuiltinFilter @data.getFilter()
                return builtin_filter.range
                    server: server
                    attr_name: attr_name
                    lower_bound: lower_bound
                    upper_bound: upper_bound
                    term: term
                    context: context
                    order_by_keys: order_by_keys
                    order_by_asc: order_by_asc
                    skip_value: skip_value
                    limit_value: limit_value

    delete: (args) ->
        server = args.server
        mapping = args.mapping
        builtin_args = args.builtin_args
        context = args.context
        type = @data.getType()
        switch type
            when 20 # Filter
                term = new Term builtin_args[0] # TODO Is it safe?
                builtin_filter = new BuiltinFilter @data.getFilter()
                return builtin_filter.delete
                    server: server
                    term: term
                    context: context

    update: (args) ->
        server = args.server
        mapping = args.mapping
        builtin_args = args.builtin_args
        context = args.context
        type = @data.getType()
        switch type
            when 20 # Filter
                term = new Term builtin_args[0] # TODO Is it safe?
                builtin_filter = new BuiltinFilter @data.getFilter()
                return builtin_filter.update
                    server: server
                    mapping: mapping
                    term: term
                    context: context

    mutate: (args) ->
        server = args.server
        mapping = args.mapping
        builtin_args = args.builtin_args
        context = args.context
        type = @data.getType()
        switch type
            when 20 # Filter
                term = new Term builtin_args[0] # TODO Is it safe?
                builtin_filter = new BuiltinFilter @data.getFilter()
                return builtin_filter.mutate
                    server: server
                    mapping: mapping
                    term: term
                    context: context

class BuiltinFilter
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        term = args.term
        attr_name = args.attr_name
        lower_bound = args.lower_bound
        upper_bound = args.upper_bound
        context = args.context
        skip_value = skip_value
        limit_value = limit_value


        predicate = new Predicate @data.getPredicate()
        # Can we have more than one term? Let's suppose not for the time being
        return term.filter
            server: server
            predicate: predicate
            attr_name: attr_name
            lower_bound: lower_bound
            upper_bound: upper_bound
            context: context
            skip_value: skip_value
            limit_value: limit_value

    delete: (args) ->
        server = args.server
        term = args.term
        context = args.context
        predicate = new Predicate @data.getPredicate()

        # Can we have more than one term? Let's suppose not for the time being
        return term.delete
            server: server
            predicate: predicate
            context: context

    range: (args) ->
        #TODO Hum, I lost the context somewhere on my way here.
        server = args.server
        term = args.term
        attr_name = args.attr_name
        lower_bound = args.lower_bound
        upper_bound = args.upper_bound
        context = args.context
        skip_value = args.skip_value
        limit_value = args.limit_value

        predicate = new Predicate @data.getPredicate()
        return term.range
            server: server
            predicate: predicate
            attr_name: attr_name
            lower_bound: lower_bound
            upper_bound: upper_bound
            term: term
            context: context
            skip_value: skip_value
            limit_value: limit_value

    mutate: (args) ->
        server = args.server
        mapping = args.mapping
        term = args.term
        context = args.context
        predicate = new Predicate @data.getPredicate()

        # Can we have more than one term? Let's suppose not for the time being
        return term.mutate
            server: server
            mapping: mapping
            predicate: predicate
            context: context


    update: (args) ->
        server = args.server
        mapping = args.mapping
        term = args.term
        context = args.context
        predicate = new Predicate @data.getPredicate()

        # Can we have more than one term? Let's suppose not for the time being
        return term.update
            server: server
            mapping: mapping
            predicate: predicate
            context: context


class BuiltinMap
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        term = args.term
        var_args = args.var_args
        skip_value = args.skip_value
        limit_value = args.limit_value

        mapping = new Mapping @data.getMapping()
        return term.evaluate
            server: server
            mapping: mapping
            var_args: var_args
            skip_value: skip_value
            limit_value: limit_value

class BuiltinConcatMap
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        term = args.term
        var_args = args.var_args
        skip_value = args.skip_value
        limit_value = args.limit_value

        mapping = new Mapping @data.getMapping()
        return term.concat
            server: server
            mapping: mapping
            var_args: var_args
            skip_value: skip_value
            limit_value: limit_value

class BuiltinOrderBy
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        term = args.term
        var_args = args.var_args
        skip_value = args.skip_value
        limit_value = args.limit_value


        return term.evaluate
            server: server
            var_args: var_args
            order_by_keys: @data.getAttr()
            order_by_asc: @data.getAscending()
            skip_value: skip_value
            limit_value: limit_value



class BuiltinRange
    constructor: (data) ->
        @data = data

    evaluate: (args) ->
        server = args.server
        term = args.term
        var_args = args.var_args
        skip_value = args.skip_value
        limit_value = args.limit_value

        attr_name = @data.getAttrname()
        lower_bound = new Term @data.getLowerbound()
        upper_bound = new Term @data.getUpperbound()

        return term.range
            server: server
            attr_name: attr_name
            lower_bound: lower_bound
            upper_bound: upper_bound
            var_args: var_args
            skip_value: skip_value
            limit_value: limit_value



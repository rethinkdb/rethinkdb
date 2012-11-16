class Builtin
    constructor: (data) ->
        @data = data

    evaluate: (server, args, context) ->
        type = @data.getType()
        switch type
            when 1 # Not
                response = new Response
                if args[0].getType() isnt 9
                    response.setStatusCode 103
                    response.setErrorMessage 'Not can only be called on a boolean'
                    #TODO Backtrace
                    return response
                
                response.setStatusCode 1
                response.addResponse not args[0].getValuebool()
                return response

            when 2 # Getattr
                term = new Term args[0]
                original_object = JSON.parse term.evaluate(server, context).getResponse()

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
                term = new Term args[0]
                original_object = JSON.parse term.evaluate().getResponse()

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
                new_term = new Term args[0]
                original_object = JSON.parse new_term.evaluate().getResponse()
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
                result_term = new Term args[0]
                result_object = JSON.parse result_term.evaluate().getResponse()

                extra_term = new Term args[1]
                extra_object = JSON.parse extra_term.evaluate().getResponse()
               
                for key, value of extra_object
                    result_object[key] = value

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify result_object
                return response
            when 9 # array append
                #TODO Handle errors?
                result_term = new Term args[0]
                result_array = JSON.parse result_term.evaluate().getResponse()

                new_term = new Term args[1]
                new_term_value = JSON.parse new_term.evaluate().getResponse()
                
                result_array.push new_term_value

                response = new Response
                response.setStatusCode 1
                response.addResponse JSON.stringify result_array
                return response
            #when 11 # slice
            when 14 # Add
                response = new Response
                if (args[0].getType() isnt 6 and args[0].getType() isnt 10) or (args[1].getType() isnt 6 and args[1].getType() isnt 10) or (args[0].getType() isnt args[1].getType())
                    response.setStatusCode 103
                    response.setErrorMessage 'Can only ADD numbers with number and arrays with arrays'
                    #TODO Backtrace
                    return response

                if args[0].getType() is 6
                    result = args[0].getNumber() + args[1].getNumber()
                else if args[0].getType() is 10
                    result = []
                    for term_raw in args[0].arrayArray()
                        term = new Term term_raw
                        result.push JSON.parse term.evaluate(server).getResponse()
                    for term_raw in args[1].arrayArray()
                        term = new Term term_raw
                        result.push JSON.parse term.evaluate(server).getResponse()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 15 # Substract
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to SUBSTRACT must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber() - args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 16 # Multiply
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to MULTIPLY must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber() * args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 17 # Divide
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to DIVIDE  must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber() / args[1].getNumber()
                result = parseFloat result.toFixed 6
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response

            when 18 # Modulo
                response = new Response
                if args[0].getType() isnt 6 or args[1].getType() isnt 6
                    response.setStatusCode 103
                    response.setErrorMessage 'All operands to DIVIDE  must be numbers'
                    #TODO Backtrace
                    return response

                result = args[0].getNumber()%args[1].getNumber()
                response.setStatusCode 1
                response.addResponse JSON.stringify result
                return response
            when 19 # Compare
                term1 = new Term args[0]
                term1_value = JSON.parse term1.evaluate(server, context).getResponse()
                term2 = new Term args[1]
                term2_value = JSON.parse term2.evaluate(server, context).getResponse()


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
                for term_data in args
                    terms.push new Term term_data
                ###
                term = new Term args[0]
                builtin_filter = new BuiltinFilter @data.getFilter()
                return builtin_filter.evaluate server, term
            #when 21 # Map
            #when 22 # Concatmap
            #when 23 # Orderby
            #when 24 # Distinct
            #when 26 # Length
            #when 27 # Union
            #when 28 # Nth
            #when 29 # Stream to array
            #when 30 # Array to strean
            #when 31 # Reduce
            #when 32 # Group mapreduce
            when 35 # Any
                term1 = new Term args[0]
                term1_value = JSON.parse term1.evaluate(server).getResponse()
                term2 = new Term args[1]
                term2_value = JSON.parse term2.evaluate(server).getResponse()

                response = new Response()
                response.setStatusCode 1
                response.addResponse JSON.stringify(term1_value or term2_value)
                return response

            when 36 # All
                term1 = new Term args[0]
                term1_value = JSON.parse term1.evaluate(server).getResponse()
                term2 = new Term args[1]
                term2_value = JSON.parse term2.evaluate(server).getResponse()

                response = new Response()
                response.setStatusCode 1
                response.addResponse JSON.stringify(term1_value and term2_value)
                return response
            when 37 # Range
                term = new Term args[0]
                builtin_range = new BuiltinRange @data.getRange()
                return builtin_range.evaluate server, term
            #when 38 # Implicit without
            when 39 # Without
                new_term = new Term args[0]
                original_object = JSON.parse new_term.evaluate().getResponse()
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

class BuiltinFilter
    constructor: (data) ->
        @data = data
    evaluate: (server, term) ->
        predicate = new Predicate @data.getPredicate()
        # Can we have more than one term? Let's suppose not for the time being
        return term.filter server, predicate
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

class BuiltinRange
    constructor: (data) ->
        @data = data
    evaluate: (server, term) ->
        attr_name = @data.getAttrname()
        lower_bound = new Term @data.getLowerbound()
        upper_bound = new Term @data.getUpperbound()

        return term.range server, attr_name, lower_bound, upper_bound




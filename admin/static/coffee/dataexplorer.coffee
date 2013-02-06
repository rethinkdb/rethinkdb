# Copyright 2010-2012 RethinkDB, all rights reserved.
# TODO: We changed so many times the behavior of the suggestions, the code is a mess. We should refactor soon.
module 'DataExplorerView', ->
    class @Container extends Backbone.View
        id: 'dataexplorer'
        template: Handlebars.templates['dataexplorer_view-template']
        input_query_template: Handlebars.templates['dataexplorer_input_query-template']
        description_template: Handlebars.templates['dataexplorer-description-template']
        template_suggestion_name: Handlebars.templates['dataexplorer_suggestion_name_li-template']
        description_with_example_template: Handlebars.templates['dataexplorer-description_with_example-template']
        alert_connection_fail_template: Handlebars.templates['alert-connection_fail-template']
        alert_reconnection_success_template: Handlebars.templates['alert-reconnection_success-template']
        databases_suggestions_template: Handlebars.templates['dataexplorer-databases_suggestions-template']
        namespaces_suggestions_template: Handlebars.templates['dataexplorer-namespaces_suggestions-template']
        reason_dataexplorer_broken_template: Handlebars.templates['dataexplorer-reason_broken-template']

        # That's all the thing we want to store so we can display the view as it was (when the user left the data explorer)
        saved_data: {}

        # Constants
        limit: 40 # How many results we display per page // Final for now
        line_height: 13 # Define the height of a line (used for a line is too long)

        events:
            'click .CodeMirror': 'handle_keypress'
            'mousedown .suggestion_name_li': 'select_suggestion' # Keep mousedown to compete with blur on .input_query
            'mouseover .suggestion_name_li' : 'mouseover_suggestion'
            'mouseout .suggestion_name_li' : 'mouseout_suggestion'
            'click .clear_query': 'clear_query'
            'click .execute_query': 'execute_query'
            'click .change_size': 'toggle_size'
            'click #reconnect': 'reconnect'
            'click .more_results': 'show_more_results'
            'click .close': 'close_alert'
            #TODO show warning

        displaying_full_view: false # Boolean for the full view (true if full view)

        # Method to close an alert/warning/arror
        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        # Build the suggestions
        map_state: # Map function -> state
            '': ''
        descriptions: {}
        suggestions: {} # Suggestions[state] = function for this state

        set_doc_description: (command, tag, suggestions) =>
            if tag is 'run' # run is a special case, we don't want use to pass a callback, so we change a little the body
                full_tag = tag+'(' # full tag is the name plus a parenthesis (we will match the parenthesis too)
                @descriptions[full_tag] =
                    name: tag
                    args: '( )'
                    description: @description_with_example_template
                        description: command['description']
                        examples: command['langs']['js']['examples']
            else
                if command['langs']['js']['dont_need_parenthesis'] is true
                    full_tag = tag # Here full_tag is just the name of the tag
                else
                    full_tag = tag+'(' # full tag is the name plus a parenthesis (we will match the parenthesis too)

                @descriptions[full_tag] =
                    name: tag
                    dont_need_parenthesis: command['langs']['js']['dont_need_parenthesis']
                    args: '( '+command['langs']['js']['body']+' )'
                    description: @description_with_example_template
                        description: command['description']
                        examples: command['langs']['js']['examples']

            parent = command['parent']
            if tag is 'run'
                parent = 'query'

            if parent is null
                parent = ''
            if not suggestions[parent]?
                suggestions[parent] = []
            if tag isnt '' # We don't want to save ()
                suggestions[parent].push full_tag

            @map_state[full_tag] = command['returns'] # We use full_tag because we need to differentiate between r. and r(

        # Method called on the content of reql_docs.json
        # Load the suggestions in @suggestions, @map_state, @descriptions
        set_docs: (data) =>
            # Create the suggestions table
            suggestions = {}
            for group in data['sections']
                for command in group['commands']
                    tag = command['langs']['js']['name']
                    if tag is 'runp' # TODO add connection stuff
                        continue
                    if tag is 'row' # We want r.row and r.row(
                        new_command = DataUtils.deep_copy command
                        new_command['langs']['js']['dont_need_parenthesis'] = false
                        new_command['langs']['js']['body'] = 'attr'
                        new_command['description'] = 'Return the attribute of the currently visited document'
                        @set_doc_description new_command, tag, suggestions
                    if tag is '()'
                        tag = ''
                    @set_doc_description command, tag, suggestions

            # Deep copy of suggestions
            for group of suggestions
                @suggestions[group] = []
                for suggestion in suggestions[group]
                    @suggestions[group].push suggestion
            
            relations = data['types']

            # For each element, we add its parent's functions to it suggestions
            for element of relations
                if not @suggestions[element]?
                    @suggestions[element] = []
                parent = relations[element]['parent']
                while parent? and suggestions[parent]
                    for suggestion in suggestions[parent]
                        @suggestions[element].push suggestion
                    parent = relations[parent]['parent']

            for state of @suggestions
                @suggestions[state].sort()


        save_data_in_localstorage: =>
            if window.localStorage?
                window.localStorage.rethinkdb_dataexplorer = JSON.stringify @saved_data

        initialize: (args) =>
            @connect_driver = args.connect_driver

            if not DataExplorerView.Container.prototype.saved_data?
                if window.localStorage? and window.localStorage.rethinkdb_dataexplorer?
                    try
                        DataExplorerView.Container.prototype.saved_data = window.localStorage.rethinkdb_dataexplorer
                    catch err
                        DataExplorerView.Container.prototype.saved_data =
                            query: null # Last value @codemirror.getValue()
                            last_query: null # Last executed query
                            last_results: null # Last results
                            last_cursor: null # Last cursor
                            last_metadata: null # Last metadata
                            cursor_timed_out: false # Whether the cursor timed out or not (ie. we reconnected)
                else
                    DataExplorerView.Container.prototype.saved_data =
                        query: null
                        last_query: null
                        last_results: null
                        last_cursor: null
                        last_metadata: null
                        cursor_timed_out: false
            # Else the user created a data explorer view before

            # Let's have a shortcut
            @saved_data = DataExplorerView.Container.prototype.saved_data
            @save_data_in_localstorage()

            # We escape the last function because we are building a regex on top of it.
            @unsafe_to_safe_regexstr = []
            @unsafe_to_safe_regexstr.push # This one has to be firest
                pattern: /\\/g
                replacement: '\\\\'
            @unsafe_to_safe_regexstr.push
                pattern: /\(/g
                replacement: '\\('
            @unsafe_to_safe_regexstr.push
                pattern: /\)/g
                replacement: '\\)'
            @unsafe_to_safe_regexstr.push
                pattern: /\^/g
                replacement: '\\^'
            @unsafe_to_safe_regexstr.push
                pattern: /\$/g
                replacement: '\\$'
            @unsafe_to_safe_regexstr.push
                pattern: /\*/g
                replacement: '\\*'
            @unsafe_to_safe_regexstr.push
                pattern: /\+/g
                replacement: '\\+'
            @unsafe_to_safe_regexstr.push
                pattern: /\?/g
                replacement: '\\?'
            @unsafe_to_safe_regexstr.push
                pattern: /\./g
                replacement: '\\.'
            @unsafe_to_safe_regexstr.push
                pattern: /\|/g
                replacement: '\\|'
            @unsafe_to_safe_regexstr.push
                pattern: /\{/g
                replacement: '\\{'
            @unsafe_to_safe_regexstr.push
                pattern: /\}/g
                replacement: '\\}'
            @unsafe_to_safe_regexstr.push
                pattern: /\[/g
                replacement: '\\['

            @results_view = new DataExplorerView.ResultView
                container: @
                limit: @limit


            @driver_handler = new DataExplorerView.DriverHandler
                container: @

            @render()

            # Connect the driver
            @interval = setInterval @connect_driver, 3*1000

        render: =>
            @.$el.html @template()
            @.$('.input_query_full_container').html @input_query_template()
            
            # Check if the browser supports the JavaScript driver
            # We do not support internet explorer (even IE 10) and old browsers.
            if navigator?.appName is 'Microsoft Internet Explorer'
                @.$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                    is_internet_explorer: true
                @.$('.reason_dataexplorer_broken').slideDown 'fast'
                @.$('.button_query').prop 'disabled', 'disabled'
            else if (not DataView?) or (not Uint8Array?) # The main two components that the javascript driver requires.
                @.$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                @.$('.reason_dataexplorer_broken').slideDown 'fast'
                @.$('.button_query').prop 'disabled', 'disabled'
            else if not window.r? # In case the javascript driver is not found (if build from source for example)
                @.$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                    no_driver: true
                @.$('.reason_dataexplorer_broken').slideDown 'fast'
                @.$('.button_query').prop 'disabled', 'disabled'

            # Let's bring back the data explorer to its old state (if there was)
            if @saved_data?.last_query? and @saved_data?.last_results? and @saved_data?.last_metadata?
                @results_view.render_result
                    query: @saved_data.last_query
                    results: @saved_data.last_results
                @results_view.render_metadata @saved_data.last_metadata
                # The query in code mirror is set in call_codemirror (because we can't set it now)
            else
                @.$('.results_container').html @results_view.render_default().$el

            # If driver not conneced
            if @driver_connected is false
                @error_on_connect()

            return @

        # Create a code mirror instance
        # This method has to be called AFTER the el element has been inserted in the DOM tree.
        call_codemirror: =>
            @codemirror = CodeMirror.fromTextArea document.getElementById('input_query'),
                mode:
                    name: 'javascript'
                    json: true
                onKeyEvent: @handle_keypress
                onBlur: @hide_suggestion
                onGutterClick: @handle_gutter_click
                lineNumbers: true
                lineWrapping: true
                matchBrackets: true

            @codemirror.setSize '100%', 'auto'
            if @saved_data.query?
                @codemirror.setValue @saved_data.query
            @codemirror.focus() # Give focus

        # We have to keep track of a lot of things because web-kit browsers handle the events keydown, keyup, blur etc... in a strange way.
        current_suggestions: []
        current_highlighted_suggestion: -1
        current_conpleted_query: ''
        query_first_part: ''
        query_last_part: ''

        # Core of the suggestions' system: We have to parse the query
        # Return true if we want code mirror to ignore the event
        handle_keypress: (editor, event) =>
            # Save the last query (even incomplete)
            # TODO clean that proto thing
            @saved_data.query = @codemirror.getValue()

            if @codemirror.getSelection() isnt ''
                @hide_suggestion()
                @hide_description()
                return false

            if event?.which?
                if event.which is 27 # ESC
                    @hide_suggestion()
                    @hide_description()
                    return true
                # If the user hit tab, we switch the highlighted suggestion
                else if event.which is 9
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return true
                    # Switch throught the suggestions
                    if event.shiftKey
                        @current_highlighted_suggestion--
                        if @current_highlighted_suggestion < 0
                            @current_highlighted_suggestion = @current_suggestions.length-1
                    else
                        @current_highlighted_suggestion++
                        if @current_highlighted_suggestion >= @current_suggestions.length
                            @current_highlighted_suggestion = 0

                    if @current_suggestions[@current_highlighted_suggestion]?
                        @highlight_suggestion @current_highlighted_suggestion # Highlight the current suggestion
                        @write_suggestion
                            suggestion_to_write: @current_suggestions[@current_highlighted_suggestion] # Auto complete with the highlighted suggestion

                    if @current_suggestions.length is 0
                        query_lines = @codemirror.getValue().split '\n'

                        # Get query before the cursor
                        # TODO That's a strange behavior. Slava wanted this, let's ask him if it's still true
                        query_before_cursor = ''
                        if @codemirror.getCursor().line > 0
                            for i in [0..@codemirror.getCursor().line-1]
                                query_before_cursor += query_lines[i] + '\n'
                        query_before_cursor += query_lines[@codemirror.getCursor().line].slice 0, @codemirror.getCursor().ch
                        if query_before_cursor[query_before_cursor.length-1] isnt '('
                            return false
                    return true
                else if event.which is 13 and (event.shiftKey or event.ctrlKey) # If the user hit enter and (Ctrl or Shift)
                    @hide_suggestion()
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return true
                    @show_or_hide_arrow()
                    @execute_query()
                else if event.ctrlKey and event.which is 86 and event.type is 'keydown' # Ctrl + V
                    @last_action_is_paste = true
                    @num_released_keys = 0 # We want to know when the user release Ctrl AND V
                    @hide_suggestion()
                    return true
                else if event.type is 'keyup' and @last_action_is_paste is true and event.which is 17 # When the user release Ctrl after a ctrl + V
                    @num_released_keys++
                    if @num_released_keys is 2
                        @last_action_is_paste = false
                    @hide_suggestion()
                    return true
                else if event.type is 'keyup' and @last_action_is_paste is true and event.which is 86 # When the user release V after a ctrl + V
                    @num_released_keys++
                    if @num_released_keys is 2
                        @last_action_is_paste = false
                    @hide_suggestion()
                    return true
                else if @codemirror.getSelection() isnt '' # If the user select something, we don't show any suggestion
                    @hide_suggestion()
                    return false
             
            # The user just hit a normal key
            @cursor_for_auto_completion = @codemirror.getCursor()

            # We just look at key up so we don't fire the call 3 times
            # TODO Make it flawless. I'm not sure that's the desired behavior
            if event?.type? and event.type isnt 'keyup' or (event?.which? and event.which is 16) # We don't do anything for shift
                return false

            @current_highlighted_suggestion = -1
            @.$('.suggestion_name_list').empty()

            # Codemirror return a position given by line/char.
            # We need to retrieve the lines first
            query_lines = @codemirror.getValue().split '\n'
            # Then let's get query before the cursor
            query_before_cursor = ''
            if @codemirror.getCursor().line > 0
                for i in [0..@codemirror.getCursor().line-1]
                    query_before_cursor += query_lines[i] + '\n'
            query_before_cursor += query_lines[@codemirror.getCursor().line].slice 0, @codemirror.getCursor().ch

            # Get query after the cursor
            query_after_cursor = query_lines[@codemirror.getCursor().line].slice @codemirror.getCursor().ch
            if query_lines.length > @codemirror.getCursor().line+1
                query_after_cursor += '\n'
                for i in [@codemirror.getCursor().line+1..query_lines.length-1]
                    if i isnt query_lines.length-1
                        query_after_cursor += query_lines[i] + '\n'
                    else
                        query_after_cursor += query_lines[i]
            @query_last_part = query_after_cursor

            # We could perform better, but that would make the code harder to read, so let's not work too hard
            stack = @extract_data_from_query
                query: query_before_cursor
                position: 0
            #console.log JSON.stringify stack, null, 2


            result =
                status: null
                #to_complete: undefine#d
                #to_describe: undefined
                
            result_non_white_char_after_cursor = @regex.get_first_non_white_char.exec(query_after_cursor)
            #console.log 'DEBUG: '+JSON.stringify result_non_white_char_after_cursor[1][0]
            if result_non_white_char_after_cursor isnt null and (result_non_white_char_after_cursor[1]?[0] isnt '.' and result_non_white_char_after_cursor[1]?[0] isnt '}' and result_non_white_char_after_cursor[1]?[0] isnt ')' and result_non_white_char_after_cursor[1]?[0] isnt ',')
                ###
                and result_non_white_char_after_cursor[1]?[0] isnt '\''
                and result_non_white_char_after_cursor[1]?[0] isnt '"')
                ###
                console.log 'REMOVE SUGGESTIONS 1'
                result.status = 'break_and_look_for_description'
                @hide_suggestion()
            else
                result_last_char_is_white = @regex.last_char_is_white.exec(query_before_cursor)
                if result_last_char_is_white isnt null
                    console.log 'REMOVE SUGGESTIONS 2'
                    result.status = 'break_and_look_for_description'
                    @hide_suggestion()

            @create_suggestion
                stack: stack
                query: query_before_cursor
                result: result

            @current_suggestions = []
            if result.suggestions?.length > 0
                for suggestion, i in result.suggestions
                    @current_suggestions.push suggestion
                    @.$('.suggestion_name_list').append @template_suggestion_name
                        id: i
                        suggestion: suggestion
                @show_suggestion()
                @hide_description()
            else if result.description?
                @hide_suggestion()
                @show_description result.description
            else
                @hide_suggestion()
                @hide_description()




            
        # Extract information from the current query
        regex:
            anonymous:/^(\s)*function\(([a-zA-Z0-9,\s]*)\)(\s)*{/
            method: /^(\s)*([a-zA-Z]+)\(/ # forEach( merge( filter(
            method_var: /^(\s)*([a-zA-Z]+)\./ # r. r.row. (r.count will be caught later)
            return : /^(\s)*return(\s)*/
            object: /^(\s)*{(\s)*/
            array: /^(\s)*\[(\s)*/
            white: /^(\s)+$/
            white_replace: /\s/g
            white_start: /^(\s)+/
            comma: /^(\s)*,(\s)*/
            number: /^[0-9]+\.?[0-9]*/
            get_first_non_white_char: /\s*(\S+)/
            last_char_is_white: /.*(\s+)$/
        stop_char:
            opening:
                '(': true
                '{': true
                '[': true
            closing:
                ')': '('
                '}': '{'
                ']': '['


        ### 
        element.type in ['string', 'function', 'var', 'separator', 'anonymous_function', 'object']
        TODO Collapse the query.slice(...)
        TODO Pass position
        
        ###
        extract_data_from_query: (args) =>
            query = args.query
            context = if args.context? then DataUtils.deep_copy(args.context) else {}
            position = args.position

            # query_after_cursor = args.query_before_cursor
            
            stack = []
            element =
                type: null
                context: context
                complete: false
            start = 0

            is_parsing_string = false
            to_skip = 0

            for char, i in query
                if to_skip > 0 # Because we cannot mess with the iterator in coffee-script
                    to_skip--
                    continue

                if is_parsing_string is true
                    if char is string_delimiter # End of the string, we can work again?
                        is_parsing_string = false # Else we just keep parsing the string
                        if element.type is 'string'
                            element.name = query.slice start, i+1
                            element.complete = true
                            stack.push element
                            element =
                                type: null
                                context: context
                                complete: false
                            start = i+1
                else # if element.is_parsing_string is false
                    if char is '\'' or char is '"' # So we get a string here
                        is_parsing_string = true
                        string_delimiter = char
                        if element.type is null
                            element.type = 'string'
                            start = i
                        continue

                   
                    if element.type is null
                        if start is i
                            result_white = @regex.white_start.exec query.slice i
                            if result_white?
                                to_skip = result_white[0].length-1
                                start += result_white[0].length
                                continue

                            result_regex = @regex.anonymous.exec query.slice i # Check for anonymouse function
                            if result_regex isnt null
                                element.type = 'anonymous_function'
                                list_args = result_regex[2]?.split(',')
                                element.args = list_args
                                new_context = DataUtils.deep_copy context
                                for arg in list_args
                                    arg = arg.replace(/(^\s*)|(\s*$)/gi,"") # Removing leading/trailing spaces
                                    new_context[arg] = true
                                element.context = new_context
                                to_skip = result_regex[0].length
                                body_start = i+result_regex[0].length
                                stack_stop_char = ['{']
                                continue

                            result_regex = @regex.return.exec query.slice i # Check for return
                            if result_regex isnt null
                                # I'm not sure we need to keep track of return, but let's keep it for now
                                element.type = 'return'
                                element.complete = true
                                to_skip = result_regex[0].length-1
                                stack.push element
                                element =
                                    type: null
                                    context: context
                                    complete: false

                                start = i+result_regex[0].length
                                continue

                            result_regex = @regex.object.exec query.slice i # Check for object
                            if result_regex isnt null
                                element.type = 'object'
                                element.next_key = null
                                element.body = [] # We need to keep tracker of the order of pairs
                                element.current_key_start = i+result_regex[0].length
                                to_skip = result_regex[0].length-1
                                stack_stop_char = ['{']
                                continue

                            result_regex = @regex.array.exec query.slice i # Check for array
                            if result_regex isnt null
                                element.type = 'array'
                                element.next_key = null
                                element.body = []
                                entry_start = i+result_regex[0].length
                                to_skip = result_regex[0].length-1
                                stack_stop_char = ['[']
                                continue


                            if char is '.'
                                new_start = i+1
                            else
                                new_start = i

                            result_regex = @regex.method.exec query.slice new_start # Check for a standard method
                            if result_regex isnt null
                                if stack[stack.length-1]?.type is 'function' or stack[stack.length-1]?.type is 'var' # We want the query to start with r. or arg.
                                    element.type = 'function'
                                    element.name = result_regex[0]
                                    element.position = position+new_start
                                    start += new_start-i
                                    to_skip = result_regex[0].length-1+new_start-i
                                    stack_stop_char = ['(']
                                    continue
                                else
                                    position_opening_parenthesis = result_regex[0].indexOf('(')
                                    if position_opening_parenthesis isnt -1 and result_regex[0].slice(0, position_opening_parenthesis) of context
                                        # Save the var
                                        # TODO Look for do() to get real_type
                                        element.real_type = 'json'
                                        element.type = 'var'
                                        element.name = result_regex[0].slice(0, position_opening_parenthesis)
                                        stack.push element
                                        element =
                                            type: 'function'
                                            name: '('
                                            position: position+position_opening_parenthesis+1
                                            context: context
                                            complete: 'false'
                                        stack_stop_char = ['(']
                                        start = position_opening_parenthesis+1
                                        to_skip = result_regex[0].length-1


                            result_regex = @regex.method_var.exec query.slice new_start # Check for method without parenthesis r., r.row., doc.
                            if result_regex isnt null
                                if result_regex[0].slice(0, result_regex[0].length-1) of context
                                    element.type = 'var'
                                    element.real_type = 'json'
                                else
                                    element.type = 'function'
                                element.position = position+new_start
                                element.name = result_regex[0].slice(0, result_regex[0].length-1).replace(/\s/, '')
                                start += new_start-i
                                element.complete = true
                                to_skip = element.name.length-1+new_start-i
                                stack.push element
                                element =
                                    type: null
                                    context: context
                                    complete: false
                                start = new_start+to_skip+1
                                continue

                            # Look for a comma
                            result_regex = @regex.comma.exec query.slice i
                            if result_regex isnt null
                                # element should have been pushed in stack. If not, the query is malformed
                                element.complete = true
                                stack.push
                                    type: 'separator'
                                    complete: true
                                    name: query.slice i, result_regex[0].length
                                element =
                                    type: null
                                    context: context
                                    complete: false
                                start = i+result_regex[0].length-1+1
                                to_skip = result_regex[0].length-1
                                continue

                        #else # if element.start isnt i
                            # Skip white spaces
                            # TODO

                    else # element.type isnt null
                        # White spaces can means a new start: r.table(...).eqJoin('id', r.table(...), 'other_id')
                        # Is that even possible?
                        result_regex = @regex.comma.exec(query.slice(i))
                        if result_regex isnt null and stack_stop_char.length < 1
                            # element should have been pushed in stack. If not, the query is malformed
                            stack.push
                                type: 'separator'
                                complete: true
                                name: query.slice i, result_regex[0].length
                                position: position+i
                            element =
                                type: null
                                context: context
                                complete: false
                            start = i+result_regex[0].length-1
                            to_skip = result_regex[0].length-1
                            continue


                        # Hum, if we reach here, the query is probably malformed of there's a bug here
                        else if element.type is 'anonymous_function'
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        element.body = @extract_data_from_query
                                            query: query.slice body_start, i
                                            context: element.context
                                            position: position+body_start
                                        element.complete = true
                                        stack.push element
                                        element =
                                            type: null
                                            context: context
                                        start = i+1
                                #else something is broken here.
                                #TODO Default behavior? The user forgot to close something?
                                #@get_error_from_query is going to report this issue
                        else if element.type is 'function'
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        element.body = @extract_data_from_query
                                            query: query.slice start+element.name.length, i
                                            context: element.context
                                            position: position+start+element.name.length
                                        element.complete = true
                                        stack.push element
                                        element =
                                            type: null
                                            context: context
                                        start = i+1
                        else if element.type is 'object'
                            # Since we are sure that we are not in a string, we can just look for colon and comma
                            # Still, we need to check the stack_stop_char since we can have { key: { inner: 'test, 'other_inner'}, other_key: 'other_value'}
                            keys_values = []
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        # We just reach a }, it's the end of the object
                                        if element.next_key?
                                            new_element =
                                                type: 'object_key'
                                                key: element.next_key
                                                key_complete: true
                                                complete: false
                                                body: @extract_data_from_query
                                                    query: query.slice element.current_value_start, i
                                                    context: element.context
                                                    position: position+element.current_value_start
                                            element.body[element.body.length-1] = new_element
                                        element.next_key = null # No more next_key
                                        element.complete = true
                                        #else the next key is not defined
                                        #TODO Check that the next_key is just white spaces, else "throw" an error
                                        #if not element.next_key?
                                        #    element.key_complete = false
                                        #    element.next_key = query.slice element.current_key_start

                                        stack.push element
                                        element =
                                            type: null
                                            context: context
                                        start = i+1
                                        continue

                            if not element.next_key?
                                if stack_stop_char.length is 1 and char is ':'
                                    new_element =
                                        type: 'object_key'
                                        key: query.slice element.current_key_start, i
                                        key_complete: true
                                    if element.body.length is 0
                                        element.body.push new_element
                                    else
                                        element.body[element.body.length-1] = new_element
                                    element.next_key = query.slice element.current_key_start, i
                                    element.current_value_start = i+1
                            else
                                result_regex = @regex.comma.exec query.slice i
                                if stack_stop_char.length is 1 and result_regex isnt null #We reached the end of a value
                                    new_element =
                                        type: 'object_key'
                                        key:  element.next_key
                                        key_complete: true
                                        body: @extract_data_from_query
                                            query: query.slice element.current_value_start, i
                                            context: element.context
                                            position: element.current_value_start
                                    element.body[element.body.length-1] = new_element
                                    to_skip = result_regex[0].length-1
                                    element.next_key = null
                                    element.current_key_start = i+result_regex[0].length
                        else if element.type is 'array'
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        # We just reach a ], it's the end of the object
                                        new_element =
                                            type: 'array_entry'
                                            complete: true
                                            body: @extract_data_from_query
                                                query: query.slice entry_start, i
                                                context: element.context
                                                position: position+entry_start
                                        if new_element.body.length > 0
                                            element.body.push new_element
                                        continue

                            if stack_stop_char.length is 1 and char is ','
                                new_element =
                                    type: 'array_entry'
                                    complete: true
                                    body: @extract_data_from_query
                                        query: query.slice entry_start, i
                                        context: element.context
                                        position: position+entry_start
                                if new_element.body.length > 0
                                    element.body.push new_element
                                entry_start = i+1

            if element.type isnt null
                element.complete = false
                if element.type is 'function'
                    element.body = @extract_data_from_query
                        query: query.slice start+element.name.length
                        context: element.context
                        position: position+start+element.name.length
                else if element.type is 'anonymous_function'
                    element.body = @extract_data_from_query
                        query: query.slice body_start
                        context: element.context
                        position: position+body_start
                else if element.type is 'string'
                    element.name = query.slice start
                else if element.type is 'object'
                    if not element.next_key? # Key not defined yet
                        new_element =
                            type: 'object_key'
                            key: query.slice element.current_key_start
                            key_complete: false
                            complete: false
                        element.body.push new_element # They key was not defined, so we add a new element
                        element.next_key = query.slice element.current_key_start
                    else
                        new_element =
                            type: 'object_key'
                            key: element.next_key
                            key_complete: true
                            complete: false
                            body: @extract_data_from_query
                                query: query.slice element.current_value_start
                                context: element.context
                                position: position+element.current_value_start
                        element.body[element.body.length-1] = new_element
                        element.next_key = null # No more next_key
                else if element.type is 'array'
                    new_element =
                        type: 'array_entry'
                        complete: false
                        body: @extract_data_from_query
                            query: query.slice entry_start
                            context: element.context
                            position: position+entry_start
                    if new_element.body.length > 0
                        element.body.push new_element
                stack.push element
            else if start isnt i
                if query.slice(start) of element.context
                    element.name = query.slice start
                    element.type = 'var'
                    element.real_type = 'json'
                    element.complete = true
                else if @regex.number.test(query.slice(start)) is true
                    element.type = 'number'
                    element.name = query.slice start
                    element.complete = true
                else if query[start] is '.'
                    #TODO add check for [a-zA-Z]?
                    element.type = 'function'
                    element.position = position+start
                    element.name = query.slice start+1
                    element.complete = false
                else
                    element.name = query.slice start
                    element.position = position+start
                    element.complete = false
                #if @regex.white.test(element.name) is false # If its type is null and the name is just white spaces, we ignore the element
                stack.push element
            return stack


        # TODO return type
        create_suggestion: (args) =>
            stack = args.stack
            query = args.query # We don't need it anymore. Remove it later if it's really the case
            result = args.result

            # No stack, ie an empty query
            if result.status is null and stack.length is 0
                result.suggestions = []
                result.status = 'done'
                @query_first_part = ''
                for suggestion in @suggestions['']
                    result.suggestions.push suggestion

            for i in [stack.length-1..0] by -1
                element = stack[i]
                if element.body? and element.body.length > 0 and element.complete is false
                    @create_suggestion
                        stack: element.body
                        query: args.query
                        result: args.result

                if result.status is 'done'
                    continue


                if result.status is null
                    # Top of the stack
                    if element.complete is true
                        ###
                        if (element.type is 'function' and @map_state[element.name] is 'json') or (element.type is 'var' and element.real_type is 'json')
                            console.log "It's working"
                            result.suggestions = ['(']
                            result.state = 'json'
                            result.description = null
                            result.status = 'done'
                            break
                        ###
                        if element.type is 'function'
                            if element.complete is true or element.name is ''
                                result.suggestions = null
                                result.status = 'look_for_description'
                                break
                            else
                                result.suggestions = null
                                result.description = element.name
                                result.status = 'done'
                        else if element.type is 'anonymous_function' or element.type is 'separator' or element.type is 'object' or element.type is 'object_key' or element.type is 'return' or 'element.type' is 'array'
                            # element.type === 'object' is impossible I think with the current implementation of extract_data_from_query
                            result.suggestions = null
                            result.status = 'look_for_description'
                            break # We want to look in the upper levels
                        #else type cannot be null (because not complete)

                    else # if element.complete is false
                        if element.type is 'function'
                            if element.body? # It means that element.body.length === 0
                                # We just opened a new function, so let's just show the description
                                result.suggestions = null
                                result.description = element.name # That means we are going to describe the function named element.name
                                result.status = 'done'
                                break
                            else
                                # function not complete, need suggestion
                                result.suggestions = []
                                result.suggestions_regex = @create_safe_regex element.name # That means we are going to give all the suggestions that match element.name and that are in the good group (not yet defined)
                                result.description = null
                                @query_first_part = query.slice 0, element.position+1
                                @cursor_for_auto_completion.ch -= element.name.length
                                @current_query
                                if i isnt 0
                                    result.status = 'look_for_state'
                                else
                                    result.state = ''
                        else if element.type is 'anonymous_function' or element.type is 'object_key' or element.type is 'string' or element.type is 'separator' or element.type is 'array'
                            result.suggestions = null
                            result.status = 'look_for_description'
                            break
                        #else if element.type is 'object' # Not possible
                        #else if element.type is 'var' # Not possible because we require a . or ( to asssess that it's a var
                        else if element.type is null
                            result.suggestions = []
                            result.status = 'look_for_description' # We'll look for a description. If we can't find one, we show the current suggestions assuming an empty state
                            result.suggestions_regex = @create_safe_regex element.name
                            @query_first_part = query.slice 0, element.position
                            @cursor_for_auto_completion.ch -= element.name.length
                            for suggestion in @suggestions['']
                                if result.suggestions_regex.test(suggestion) is true
                                    result.suggestions.push suggestion
                            
                            ### For this state "r.expr( r" we just want the description of expr
                            # This code is if we want to get the suggestion for "r"
                            result.suggestions = []
                            regex = @create_safe_regex element.name
                            for variable of element.context
                                if regex.test(variable) is true
                                    result.suggestions.push variable
                                    break
                            for suggestion of @suggestions
                                if regex.test(suggestion) is true
                                    result.suggestions.push variable
                            result.status = 'done'
                            ###
                else if result.status is 'look_for_description'
                    if element.type is 'function'
                        result.description = element.name
                        result.suggestions = null
                        result.status = 'done'
                    else
                        break
                if result.status is 'break_and_look_for_description'
                    if element.type is 'function' and element.complete is false and element.name.indexOf('(') isnt -1
                        result.description = element.name
                        result.suggestions = null
                        result.status = 'done'
                    else
                        if element.type isnt 'function'
                            break
                        else
                            result.status = 'look_for_description'
                            break
                else if result.status is 'look_for_state'
                    if element.type is 'function' and element.complete is true
                        result.state = element.name
                        if @suggestions[@map_state[element.name]]?
                            for suggestion in @suggestions[@map_state[element.name]]
                                if result.suggestions_regex.test(suggestion) is true
                                    result.suggestions.push suggestion
                        else
                            #TODO show warning
                            console.log 'The last function is not valid'
                        result.status = 'done'
                    else if element.type is 'var' and element.complete is true
                        result.state = element.real_type
                        for suggestion in @suggestions[result.state]
                            if result.suggestions_regex.test(suggestion) is true
                                result.suggestions.push suggestion
                        result.status = 'done'
                    #else # Is that possible? A function can only be preceded by a function (except for r)

        create_safe_regex: (str) =>
            for char in @unsafe_to_safe_regexstr
                str = str.replace char.pattern, char.replacement
            return new RegExp('^('+str+')', 'i')


        ###
        # Current refactoring ends here for Container
        ###





        # Return the first unmatched closing parenthesis/square bracket/curly bracket
        # Returns
        #   error: true
        #   char: the character used
        #   position: the faulty char
        # or returns null
        # TODO Check if a key is used more than once in an object
        get_errors_from_query: (query) =>
            is_string = false
            char_used = ''

            stack = [] # Stack of opening parenthesis/square brackets/curly brackets
            for char, i in query
                if is_string is false
                    if char is '"' or char is '\''
                        is_string = true
                        char_used = char
                        position_string = i
                    else if char is '('
                        stack.push char
                    else if char is ')'
                        if stack.pop() isnt char
                            return {
                                error: true
                                char: char
                                position: i
                            }
                    else if char is '['
                        stack.push char
                    else if char is ']'
                        if stack.pop() isnt char
                            return {
                                error: true
                                char: char
                                position: i
                            }
                     else if char is '{'
                        stack.push char
                    else if char is '}'
                        if stack.pop() isnt char
                            return {
                                error: true
                                char: char
                                position: i
                            }
                else
                    if char is char_used and query[i-1] isnt '\\' # It's safe to get query[i-1] because is_string cannot be true for the first char
                        is_string = false
            if is_string is true
                return {
                    error: true
                    char: char_used
                    position: i
                }
            else
                return null

        show_suggestion: =>
            @move_suggestion()
            margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'
            @.$('.suggestion_full_container').css 'margin-top', margin
            @.$('.arrow').css 'margin-top', margin

            @.$('.suggestion_name_list').show()
            @.$('.arrow').show()

        show_description: (fn) =>
            if @descriptions[fn]?
                margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'

                @.$('.suggestion_full_container').css 'margin-top', margin
                @.$('.arrow').css 'margin-top', margin

                @.$('.suggestion_description').html @description_template @extend_description fn

                @.$('.suggestion_description').show()
                @move_suggestion()
                @show_or_hide_arrow()
            else
                @hide_description()

        hide_suggestion: =>
            @.$('.suggestion_name_list').hide()
            @show_or_hide_arrow()
        hide_description: =>
            @.$('.suggestion_description').hide()
            @show_or_hide_arrow()

        show_or_hide_arrow: =>
            if @.$('.suggestion_name_list').css('display') is 'none' and @.$('.suggestion_description').css('display') is 'none'
                @.$('.arrow').hide()
            else
                @.$('.arrow').show()


        move_suggestion: =>
            margin_left = parseInt(@.$('.CodeMirror-cursor').css('left').replace('px', ''))+27

            @.$('.arrow').css 'margin-left', margin_left

            if margin_left < 200
                @.$('.suggestion_full_container').css 'left', '0px'
            else
                 max_margin = @.$('.CodeMirror-scroll').width()-418

                if margin_left > max_margin
                    @.$('.suggestion_full_container').css 'left', (max_margin-28)+'px'
                else
                    margin_left = Math.min max_margin, Math.floor(margin_left/100)*100
                    @.$('.suggestion_full_container').css 'left', (margin_left-100)+'px'

           


            ###
            errors = @get_errors_from_query @codemirror.getValue()

            # Check if we are in a string or in the middle of a word, if we are in a string, we just show the description
            is_in_string = @is_in_string query_before_cursor
            if is_in_string.boolean is true
                @hide_suggestion()
                last_function_for_description = @extract_last_function_for_description
                    is_string: true
                    char_used: is_in_string.char_used
                    query: query_before_cursor
                if last_function_for_description isnt ''
                    @add_description last_function_for_description
                return ''
            
            # We do not suggest things if in the middle of a word
            index_next_character = 0
            while query_after_cursor[index_next_character]?
                if /\s/.test(query_after_cursor[index_next_character]) is false
                    next_non_white_character = query_after_cursor[index_next_character]
                    break
                index_next_character++
            if next_non_white_character? and next_non_white_character isnt '.' and next_non_white_character isnt ',' and next_non_white_character isnt ')' and next_non_white_character isnt ';'
                @hide_suggestion()
                last_function_for_description = @extract_last_function_for_description(query_before_cursor)
                if last_function_for_description isnt ''
                    @add_description last_function_for_description
                return ''

            # Extract the current query (the inner query)
            slice_index = @extract_query_first_part query_before_cursor
            query = query_before_cursor.slice slice_index
            
            # Store data to reconstruct when a suggestion is selected
            @query_first_part = query_before_cursor.slice 0, slice_index
            @query_last_part = query_after_cursor

            # Get the last completed function for description and suggestion
            last_function = @extract_last_function(query)
            just_description = false # Boolean to know if we want to display just the description and not the suggestions
            # Hack in case a new parenthesis is opened. Ex: r.table('foo').filter( // We want to describe filter, not suggest r(
            if /^\s*$/.test(query) is true or last_function is '' # We have an empty cursor
                last_function_full = @extract_last_function_for_description(query_before_cursor)
                if last_function_full isnt ''
                    last_function = last_function_full
                    just_description = true

            # Hack because last_function returns 'r' if the query is 'r'. and r isn't a function
            if last_function is query and last_function is 'r'
                last_function = ''

            if @map_state[last_function]? and @suggestions[@map_state[last_function]]?
                if not @suggestions[@map_state[last_function]]? or @suggestions[@map_state[last_function]].length is 0 or just_description is true
                    @hide_suggestion()
                    last_function_for_description = @extract_last_function_for_description(query_before_cursor)

                    if last_function_for_description isnt ''
                        @add_description last_function_for_description
                else
                    could_append = @append_suggestion(query, @suggestions[@map_state[last_function]])
                    if could_append is false # We couldn't find any suggestion that matched. We are probably at the end of a function (closing parenthesis)
                        last_function_for_description = @extract_last_function_for_description(query_before_cursor)

                        if last_function_for_description isnt ''
                            @add_description last_function_for_description
            else
                @hide_suggestion()
                last_function_for_description = @extract_last_function_for_description(query_before_cursor)

                if last_function_for_description isnt ''
                    @add_description last_function_for_description
            ###
            return false

        # Check if the end of the query is in the middle of a string. 
        is_in_string: (query) ->
            is_string = false
            char_used = ''
            
            for char, i in query
                if is_string is false
                    if char is '"' or char is '\''
                        is_string = true
                        char_used = char


                else
                    if char is char_used and query[i-1] isnt '\\' # It's safe to get query[i-1] because is_string cannot be true for the first char
                        is_string = false

            boolean: is_string
            char_used: char_used
        # Hipsters stuff
        
        #Highlight suggestion. Method called when the user hit tab or mouseover
        highlight_suggestion: (id) =>
            @.$('.suggestion_name_li').removeClass 'suggestion_name_li_hl'
            @.$('.suggestion_name_li').eq(id).addClass 'suggestion_name_li_hl'
            @.$('.suggestion_description').html @description_template @extend_description @current_suggestions[id]
            @show_suggestion_description()

        # Show the description of the suggestion
        show_suggestion_description: =>
            margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'
            @.$('.suggestion_full_container').css 'margin-top', margin
            @.$('.arrow').css 'margin-top', margin

            @.$('.suggestion_description').show()
            @.$('.arrow').show()

        # Write the suggestion
        write_suggestion: (args) =>
            suggestion_to_write = args.suggestion_to_write
            @codemirror.setValue @query_first_part+suggestion_to_write+@query_last_part
    
            @codemirror.focus() # Useful if the user used the mouse to select a suggestion
            @codemirror.setCursor
                line: @cursor_for_auto_completion.line
                ch: @cursor_for_auto_completion.ch+suggestion_to_write.length


            # TODO save data

        # Select the suggestion highlighted
        select_suggestion: (event) =>
            suggestion_to_write = @.$(event.target).html()
            @write_suggestion
                suggestion_to_write: suggestion_to_write

            # Give back focus to code mirror
            @hide_suggestion()

            @handle_keypress() # That's going to describe the function the user just selected

            @codemirror.focus() # Useful if the user used the mouse to select a suggestion


        # Highlight a suggestion in case of a mouseover
        mouseover_suggestion: (event) =>
            @highlight_suggestion event.target.dataset.id

        # Hide suggestion in case of a mouse out
        mouseout_suggestion: (event) =>
            @hide_suggestion_description()



        # Hide the description
        hide_suggestion_description: =>
            @.$('.suggestion_description').html ''
            @.$('.suggestion_description').css 'display', 'none'
            @.$('.arrow').hide()


        #TODO refactor show_suggestion, show_suggestion_description, add_description


        # Extend description for db() and table() with a list of databases or namespaces
        extend_description: (fn) =>
            if @options?.can_extend? and @options?.can_extend is false
                return @descriptions[fn]

            if fn is 'db('
                description = _.extend {}, @descriptions[fn]
                if databases.length is 0
                    data =
                        no_database: true
                else
                    data =
                        no_database: false
                        databases_available: _.map(databases.models, (database) -> return database.get('name'))
                description.description += @databases_suggestions_template data
            else if fn is 'table('
                # Look for the argument of the previous db()
                database_used = @extract_database_used()

                description = _.extend {}, @descriptions[fn]
                if database_used.error is false
                    namespaces_available = []
                    for namespace in namespaces.models
                        if database_used.db_found is false or namespace.get('database') is database_used.id
                            namespaces_available.push namespace.get('name')
                    data =
                        namespaces_available: namespaces_available
                        no_namespace: namespaces_available.length is 0

                    if database_used.name?
                        data.database_name = database_used.name
                else
                    data =
                        error: database_used.error

                description.description += @namespaces_suggestions_template data
            else
                description = @descriptions[fn]
            return description

        extract_database_used: =>
            query_lines = @codemirror.getValue().split '\n'
            query_before_cursor = ''
            if @codemirror.getCursor().line > 0
                for i in [0..@codemirror.getCursor().line-1]
                    query_before_cursor += query_lines[i] + '\n'
            query_before_cursor += query_lines[@codemirror.getCursor().line].slice 0, @codemirror.getCursor().ch
            # TODO: check that there is not a string containing db( before...
            last_db_position = query_before_cursor.lastIndexOf('.db(')
            if last_db_position is -1
                for database in databases.models
                    if database.get('name') is 'test'
                        database_test_id = database.get('id')
                        break
                if database_test_id?
                    return {
                        db_found: true
                        error: false
                        id: database_test_id
                        name: 'test'
                    }
                else
                    return {
                        db_found: false
                        error: true
                    }
            else
                arg = query_before_cursor.slice last_db_position+5 # +4 for .db(
                char = query_before_cursor.slice last_db_position+4, last_db_position+5 # ' or " used for the argument of db()
                end_arg_position = arg.indexOf char # Check for the next quote or apostrophe
                if end_arg_position is -1
                    return {
                        db_found: false
                        error: true
                    }
                db_name = arg.slice 0, end_arg_position
                for database in databases.models
                    if database.get('name') is db_name
                        return {
                            db_found: true
                            error: false
                            id: database.get('id')
                            name: db_name
                        }
                return {
                    db_found: false
                    error: true
                }
            
        add_description: (fn) =>
            if @descriptions[fn]?
                margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'

                @.$('.suggestion_full_container').css 'margin-top', margin
                @.$('.arrow').css 'margin-top', margin

                @.$('.suggestion_description').html @description_template @extend_description fn

                @.$('.suggestion_description').css 'display', 'block'
                @move_suggestion()
                @show_or_hide_arrow()

        # Expand the textarea of the raw view
        expand_textarea: (event) =>
            if @.$('.input_query').length is 1
                @.$('.input_query').height 0
                height = @.$('.input_query').prop('scrollHeight') # We should have -8 but Firefox doesn't add padding in scrollHeight... Maybe we should start adding hacks...
                @.$('.input_query').css 'height', height if @.$('.input_query').height() isnt height


        # Extract the last function to give a description
        # r.table(...).filter( function(doc) { => filter
        # r.table(...).filter( function(doc) { r( => r
        # r.table(...).filter( function(doc) { doc('test => (
        # r.table(...).filter( function(doc) { r.row('key').add( => add
        # r.table(...).filter( function(doc) { r.row('key').add('fdsf => add
        extract_last_function_for_description: (args) =>
            is_string = if args.is_string? then args.is_string else false
            char_used = if args.char_used? then args.char_used else true
            query = args.query

            num_not_open_parenthesis = 0

            for i in [query.length-1..0] by -1
                char = query[i]
                if is_string is false
                    if char is '"' or char is '\''
                        is_string = true
                        char_used = char
                    else if query[i] is '(' and num_not_open_parenthesis >= 0
                        if @is_in_string(query.slice(0, i)) is false
                            num_not_open_parenthesis--
                            end = i+1
                    else if query[i] is '(' and num_not_open_parenthesis < 0
                        return query.slice i+1, end
                    else if query[i] is ')'
                        num_not_open_parenthesis++
                    else if query[i] is '.' and num_not_open_parenthesis < 0
                        return query.slice i+1, end
                else
                    if char is char_used and ((not query[i-1]?) or query[i-1] isnt '\\') # It's safe to get query[i-1] because is_string cannot be true for the first char
                        is_string = false



                if query[i] is '(' and num_not_open_parenthesis >= 0
                    if @is_in_string(query.slice(0, i)) is false
                        num_not_open_parenthesis--
                        end = i+1
                else if query[i] is '(' and num_not_open_parenthesis < 0
                    return query.slice i+1, end
                else if query[i] is ')'
                    if @is_in_string(query.slice(0, i)) is false
                        num_not_open_parenthesis++
                else if query[i] is '.' and num_not_open_parenthesis < 0
                    return query.slice i+1, end
            if end?
                return query.slice 0, end

            return ''



        # Extract the last completed function of the current line, taking in account if we are in a string
        # Ex: r.d => r
        # Ex: r.db('test').table( => db(
        # Ex: r.db( => r
        extract_last_function: (query) =>
            count_dot = 0
            first_dot_position = 0
            second_dot_position = query.length

            num_not_open_parenthesis = 0

            is_string = false
            char_used = ''
            for i in [query.length-1..0] by -1
                if is_string is false
                    if (query[i] is '"' or query[i] is '\'')
                        is_string = true
                        char_used = query[i]
                    else if query[i] is ',' and num_not_open_parenthesis<=0
                        if first_dot_position is 0
                            first_dot_position = i+1
                        break
                    else if query[i] is '('
                        num_not_open_parenthesis--
                    else if query[i] is ')'
                        num_not_open_parenthesis++
                    else if query[i] is '.' and num_not_open_parenthesis <= 0
                        count_dot++
                        if count_dot is 1
                            second_dot_position = i
                        else if count_dot is 2
                            first_dot_position = i+1
                            break
                else if is_string is true
                    if query[i] is char_used
                        if query[i-1]? and query[i-1] is '\\'
                            continue
                        else
                            is_string = false

            parenthesis_position = query.indexOf('(', first_dot_position+1)
            if parenthesis_position is -1
                parenthesis_position = query.length
            else
                parenthesis_position++

            if parenthesis_position is 0
                end = second_dot_position
            else if parenthesis_position < second_dot_position
                end = parenthesis_position
            else
                end = second_dot_position
        

            return query.slice(first_dot_position, end).replace(/\s/g, '')



        # Return the position of the beggining of the first subquery
        extract_query_first_part: (query) ->
            is_string = false
            char_used = ""
            count_opening_parenthesis = 0
            for i in [query.length-1..0] by -1
                if is_string is false
                    if (query[i] is '"' or query[i] is '\'')
                        is_string = true
                        char_used = query[i]
                    else if query[i] is ';'
                        k = 0 # Strip white char
                        while query[i+1+k]? and /\s/.test(query[i+1+k])
                            k++
                        return i+1+k
                    else if query[i] is ',' and count_opening_parenthesis >= 0
                            k = 0 # Strip white char
                            while query[i+1+k]? and /\s/.test(query[i+1+k])
                                k++
                            return i+1+k
                    else if query[i] is '('
                        count_opening_parenthesis++
                        if count_opening_parenthesis > 0
                            k = 0 # Strip white char
                            while query[i+1+k]? and /\s/.test(query[i+1+k])
                                k++
                            return i+1+k
                    else if query[i] is ')'
                        count_opening_parenthesis--

                else if is_string is true
                    if query[i] is char_used
                        if query[i-1]? and query[i-1] is '\\'
                            continue
                        else
                            is_string = false
            return 0

        # Append suggestion and display them or hide them if suggestions is empty
        # Returns true if we found some suggestion, else false
        append_suggestion: (query, suggestions) =>
            @hide_suggestion()
            splitdata = query.split('.')
            @current_completed_query = ''
            if splitdata.length>1
                for i in [0..splitdata.length-2]
                    @current_completed_query += splitdata[i]+'.'
            if splitdata.length > 1
                element_currently_written = splitdata[splitdata.length-1]
            else
                element_currently_written = query


            for char in @unsafe_to_safe_regexstr
                element_currently_written = element_currently_written.replace char.pattern, char.replacement
            found_suggestion = false
            pattern = new RegExp('^('+element_currently_written+')', 'i')
            @current_suggestions = []
            for suggestion, i in suggestions
                if pattern.test(suggestion)
                    found_suggestion = true
                    @current_suggestions.push suggestion
                    @.$('.suggestion_name_list').append @template_suggestion_name
                        id: i
                        suggestion: suggestion

            if found_suggestion
                @show_suggestion()
                return true
            else
                @hide_suggestion()
                return false

        # Callback used by the cursor when the user hit 'more results'
        callback_query: (data) =>
            # If we get a run time error
            if data instanceof rethinkdb.errors.RuntimeError or data instanceof rethinkdb.errors.BadQuery or data instanceof rethinkdb.errors.ClientError or data instanceof rethinkdb.errors.ClientError
                @.$('.loading_query_img').css 'display', 'none'
                @.results_view.render_error(@query, data)
                return false
            
            # if it's a valid result and we have not reach the maximum of results displayed
            # A valid result can be null, so we don't use the coffescript's existential operator
            if data isnt undefined and  @current_results.length < @limit
                @current_results.push data
                return true
            else
                # Else we are going to display what we have
                @.$('.loading_query_img').css 'display', 'none'

                # Save the last executed query and the last displayed results
                @saved_data.last_query = @query
                @saved_data.last_results = @current_results
                @results_view.render_result
                    query: null
                    results: @current_results # The first parameter null is the query because we don't want to display it.

                execution_time = new Date() - @start_time
                metadata =
                    limit_value: @current_results.length
                    skip_value: @skip_value
                    execution_time: execution_time
                    query: @query
                    has_more_data: (true if data?) # if data is undefined, it means that there is no more data
                @saved_data.last_metadata = metadata # Let's save the metadata too.

                @results_view.render_metadata metadata

                if data isnt undefined #there is nore data
                    @skip_value += @current_results.length
                    @current_results = []
                    @current_results.push data
                return false

        # Function triggered when the user click on 'more results'
        show_more_results: (event) =>
            if @last_results isnt null
                @results_view.update_start_record @last_results.length
            try
                event.preventDefault()
                @current_results = []
                @start_time = new Date()
                @last_cursor.next(@callback_query)
                $(window).scrollTop(@.$('.results_container').offset().top)
            catch err
                @.$('.loading_query_img').css 'display', 'none'
                @results_view.render_error(@query, err)

        callback_multiple_queries: (data) =>
            # Check if the data sent by the server is an error
            if data instanceof rethinkdb.errors.RuntimeError or data instanceof rethinkdb.errors.BadQuery or data instanceof rethinkdb.errors.ClientError or data instanceof rethinkdb.errors.ClientError
                @.$('.loading_query_img').css 'display', 'none'
                @.results_view.render_error(@query, data)
                return false

            # Look for the next query
            @current_query_index++

            # If we are dealing with the last query
            if @current_query_index >= @queries.length
                # If we get a run time error
                if data instanceof rethinkdb.errors.RuntimeError
                    @.$('.loading_query_img').css 'display', 'none'
                    @.results_view.render_error(@query, data)
                    return false
                
                # if it's a valid result and we have not reach the maximum of results displayed
                if data isnt undefined and  @current_results.length < @limit
                    @current_results.push data
                    return true
                else
                    # Else we are going to display what we have
                    @.$('.loading_query_img').css 'display', 'none'

                    # Save the last executed query and the last displayed results
                    @saved_data.last_query = @query
                    @saved_data.last_results = @current_results
                    @results_view.render_result
                        query: null
                        results: @current_results # The first parameter is null ( = query, so we don't display it)

                    execution_time = new Date() - @start_time
                    metadata =
                        limit_value: @current_results.length
                        skip_value: @skip_value
                        execution_time: execution_time
                        query: @query
                        has_more_data: (true if data?) # if data is undefined, it means that there is no more data
                    @saved_data.last_metadata = metadata # Saving the metadata

                    @results_view.render_metadata metadata

                    if data isnt undefined #there is nore data
                        @skip_value += @current_results.length
                        @current_results = []
                        @current_results.push data
                    return false
            else #  Else if it's not the last query, we just execute the next query
                try
                    #For safety only.
                    if @cursor?.close?
                        @cursor.close()

                    @current_results = []
                    @skip_value = 0

                    @cursor = eval(@queries[@current_query_index])
                    @saved_data.last_cursor = @cursor # Saving the cursor so the user can call more results later
                    @cursor.next(@callback_multiple_queries)
                catch err
                    @.$('.loading_query_img').css 'display', 'none'
                    @results_view.render_error(@query, err)
            return false

        # Function that execute the query
        # Current behavior for user
        # - User execute q1
        # - User execute q2 (we stop listening to q1)
        # - User retrieve results for q2
        #
        # In case of multiple queries
        # - User execute q1_1, q1_2
        # - User execute q2
        # - We don't execute q1_3
        # - User retrieve results for q2
        execute_query: =>
            @results_view.set_start_record 1
            # The user just executed a query, so we reset cursor_timed_out to false
            @saved_data.cursor_timed_out = false

            # postpone reconnection
            @driver_handler.postpone_reconnection()

            @query = @codemirror.getValue()

            # Replace new lines with \n so the query is not splitted.
            is_string = false
            char_used = ''
            i = 0
            while i < @query.length
                if is_string is true
                    if @query[i] is char_used
                        if @query[i-1]? and @query[i-1] isnt '\\'
                            @query = @query.slice(0, start_string) + @query.slice(start_string, i).replace('\n', '\\n') + @query.slice(i)
                            is_string = false
                else if is_string is false
                    if @query[i] is '\'' or @query[i] is '"'
                        is_string = true
                        start_string = i
                        char_used = @query[i]
                i++
            
            # Display the loading gif
            @.$('.loading_query_img').css 'display', 'block'

            # Execute the query
            try
                @queries = @separate_queries @query
                @start_time = new Date()

                @current_results = []
                @skip_value = 0
                @current_query_index = 0

                if @cursor?.close? # So if a old query was running, we won't listen to it.
                    @cursor.close()

                @cursor = eval(@queries[@current_query_index])
                # Save the last cursor to fetch more results later
                @saved_data.last_cursor = @cursor
                @cursor.next(@callback_multiple_queries)

            catch err
                @.$('.loading_query_img').css 'display', 'none'
                @results_view.render_error(@query, err)

        # Separate the queries so we can execute them in a synchronous order. We use .run()\s*; to separate queries (and we make sure that the separator is not in a string)
        separate_queries: (query) =>
            start = 0
            count_dot = 0

            is_string = false
            char_used = ''
            queries = []

            # Again because of strings, we cannot know use a pretty regex
            is_parsing_function = false # Track if we are parsing a function (between a dot and a opening parenthesis)
            is_parsing_args = false # Track if we are parsing the arguments of a function so we can match for .run( ) but not for .ru n()
            last_function = '' # The last function used with its arguments 
            for i in [0..query.length-1]
                if is_string is false
                    if (query[i] is '"' or query[i] is '\'')
                        is_string = true
                        char_used = query[i]
                    else if query[i] is ';'
                        if last_function is 'run()' # If the last function is run(), we have one query
                            queries.push query.slice start, i
                            start = i+1

                    # Keep track of the last function used
                    if query[i] is '.' # New function detected, let's reset last_function and switch on is_parsing_function
                        last_function = ''
                        is_parsing_function = true
                    else if is_parsing_function is true
                        if is_parsing_args is false or /\s/.test(query[i]) is false # If we are parsing arguments, we are not interested in white space
                            last_function += query[i]

                        if query[i] is '(' # We are going to parse arguments now
                            is_parsing_args = true
                        else if query[i] is ')' # End of the function
                            is_parsing_function = false
                            is_parsing_args = false


                else if is_string is true
                    if query[i] is char_used
                        if query[i-1]? and query[i-1] is '\\'
                            continue
                        else
                            is_string = false

            last_query = query.slice start, query.length
            if /^\s*$/.test(last_query) is false
                queries.push query.slice start, query.length
            return queries

        # Clear the input
        clear_query: =>
            @codemirror.setValue ''
            @codemirror.focus()

        # Called if the driver could connect
        success_on_connect: =>
            @results_view.check_if_cursor_timed_out()
            # If the we were disconnected because of an error, we say that we did reconnect
            if window.driver_connected_previous_state is false
                @.$('#user-alert-space').hide()
                @.$('#user-alert-space').html @alert_reconnection_success_template()
                @.$('#user-alert-space').slideDown 'fast'
            @reconnecting = false
            @driver_connected = 'connected'

        # Called if the driver could not connect
        error_on_connect: =>
            @results_view.check_if_cursor_timed_out()
            # We fail to connect, so we display a message except if we were already disconnected and we are not trying to manually reconnect
            # So if the user fails to reconnect after a failure, the alert will still flash
            if window.driver_connected_previous_state isnt false or @reconnecting is true
                @.$('#user-alert-space').hide()
                @.$('#user-alert-space').html @alert_connection_fail_template({})
                @.$('#user-alert-space').slideDown 'fast'
            @reconnecting = false
            @driver_connected = 'error'

        # Reconnect, function triggered if the user click on reconnect
        reconnect: (event) =>
            @reconnecting = true
            event.preventDefault()
            clearTimeout window.timeout_driver_connect
            window.driver_connect()




        handle_gutter_click: (editor, line) =>
            start =
                line: line
                ch: 0
            end =
                line: line
                ch: @codemirror.getLine(line).length
            @codemirror.setSelection start, end

        # Switch between full view and normal view
        toggle_size: =>
            if @displaying_full_view is true
                @display_normal()
                $(window).off 'resize', @display_full
                @displaying_full_view = false
            else
                @display_full()
                $(window).on 'resize', @display_full
                @displaying_full_view = true
            @results_view.set_scrollbar()

        display_normal: =>
            $('#cluster').addClass 'container'
            $('#cluster').removeClass 'cluster_with_margin'
            @.$('.wrapper_scrollbar').css 'width', '888px'

        display_full: =>
            $('#cluster').removeClass 'container'
            $('#cluster').addClass 'cluster_with_margin'
            @.$('.wrapper_scrollbar').css 'width', ($(window).width()-92)+'px'

        destroy: =>
            @display_normal()
            $(window).off 'resize', @display_full
            @results_view.destroy()
            clearTimeout @timeout_driver_connect
            # We do not destroy the cursor, because the user might come back and use it.
    
    class @ResultView extends Backbone.View
        className: 'result_view'
        template: Handlebars.templates['dataexplorer_result_container-template']
        default_template: Handlebars.templates['dataexplorer_default_result_container-template']
        metadata_template: Handlebars.templates['dataexplorer-metadata-template']
        option_template: Handlebars.templates['dataexplorer-option_page-template']
        error_template: Handlebars.templates['dataexplorer-error-template']
        template_no_result: Handlebars.templates['dataexplorer_result_empty-template']
        template_json_tree:
            'container' : Handlebars.templates['dataexplorer_result_json_tree_container-template']
            'span': Handlebars.templates['dataexplorer_result_json_tree_span-template']
            'span_with_quotes': Handlebars.templates['dataexplorer_result_json_tree_span_with_quotes-template']
            'url': Handlebars.templates['dataexplorer_result_json_tree_url-template']
            'email': Handlebars.templates['dataexplorer_result_json_tree_email-template']
            'object': Handlebars.templates['dataexplorer_result_json_tree_object-template']
            'array': Handlebars.templates['dataexplorer_result_json_tree_array-template']

        template_json_table:
            'container' : Handlebars.templates['dataexplorer_result_json_table_container-template']
            'tr_attr': Handlebars.templates['dataexplorer_result_json_table_tr_attr-template']
            'td_attr': Handlebars.templates['dataexplorer_result_json_table_td_attr-template']
            'tr_value': Handlebars.templates['dataexplorer_result_json_table_tr_value-template']
            'td_value': Handlebars.templates['dataexplorer_result_json_table_td_value-template']
            'td_value_content': Handlebars.templates['dataexplorer_result_json_table_td_value_content-template']
            'data_inline': Handlebars.templates['dataexplorer_result_json_table_data_inline-template']
        cursor_timed_out_template: Handlebars.templates['dataexplorer-cursor_timed_out-template']

        events:
            # For Tree view
            'click .jt_arrow': 'toggle_collapse'
            # For Table view
            'mousedown td': 'handle_mousedown'
            'click .jta_arrow_v': 'expand_tree_in_table'
            'click .jta_arrow_h': 'expand_table_in_table'

            'click .link_to_tree_view': 'show_tree'
            'click .link_to_table_view': 'show_table'
            'click .link_to_raw_view': 'show_raw'

        current_result: []

        initialize: (args) =>
            @container = args.container
            @set_limit args.limit
            @set_skip 0
            @prototype = DataExplorerView.ResultView.prototype
            if not @prototype.view?
                @prototype.view = 'tree'

            $(window).mousemove @handle_mousemove
            $(window).mouseup @handle_mouseup

            @last_keys = [] # Arrays of the last keys displayed
            @last_columns_size = {} # Size of the columns displayed. Undefined if a column has the default size

        set_limit: (limit) =>
            @limit = limit
        set_skip: (skip) =>
            @skip = skip

        show_tree: (event) =>
            event.preventDefault()
            @prototype.view = 'tree'
            @render_result()
        show_table: (event) =>
            event.preventDefault()
            @prototype.view = 'table'
            @render_result()
        show_raw: (event) =>
            event.preventDefault()
            @prototype.view = 'ram'
            @render_result()

        set_start_record: (start) =>
            @start_record = start
            @prototype.start_record = @start_record
        update_start_record: (to_add) =>
            @start_record += to_add
            @prototype.start_record = @start_record

        render_error: (query, err) =>
            @.$el.html @error_template
                query: query
                error: err.toString()
                forgot_run: (err.type? and err.type is 'undefined_method' and err['arguments']?[0]? and err['arguments'][0] is 'next') # Check if next is undefined, in which case the user probably forgot to append .run()
            return @

        json_to_tree: (result) =>
            return @template_json_tree.container
                tree: @json_to_node(result)

        #TODO catch RangeError: Maximum call stack size exceeded?
        #TODO what to do with new line?
        # We build the tree in a recursive way
        json_to_node: (value) =>
            value_type = typeof value
            
            output = ''
            if value is null
                return @template_json_tree.span
                    classname: 'jt_null'
                    value: 'null'
            else if value.constructor? and value.constructor is Array
                if value.length is 0
                    return '[ ]'
                else
                    sub_values = []
                    for element in value
                        sub_values.push
                            value: @json_to_node element
                        if typeof element is 'string' and (/^(http|https):\/\/[^\s]+$/i.test(element) or  /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(element))
                            sub_values[sub_values.length-1]['no_comma'] = true


                    sub_values[sub_values.length-1]['no_comma'] = true
                    return @template_json_tree.array
                        values: sub_values
            else if value_type is 'object'
                sub_values = []
                for key of value
                    last_key = key
                    sub_values.push
                        key: key
                        value: @json_to_node value[key]
                    #TODO Remove this check by sending whether the comma should be add or not
                    if typeof value[key] is 'string' and (/^(http|https):\/\/[^\s]+$/i.test(value[key]) or  /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(value[key]))
                        sub_values[sub_values.length-1]['no_comma'] = true

                if sub_values.length isnt 0
                    sub_values[sub_values.length-1]['no_comma'] = true

                data =
                    no_values: false
                    values: sub_values

                if sub_values.length is 0
                    data.no_value = true

                return @template_json_tree.object data
            else if value_type is 'number'
                return @template_json_tree.span
                    classname: 'jt_num'
                    value: value
            else if value_type is 'string'
                if /^(http|https):\/\/[^\s]+$/i.test(value)
                    return @template_json_tree.url
                        url: value
                else if /^[a-z0-9]+@[a-z0-9]+.[a-z0-9]{2,4}/i.test(value) # We don't handle .museum extension and special characters
                    return @template_json_tree.email
                        email: value
                else
                    return @template_json_tree.span_with_quotes
                        classname: 'jt_string'
                        value: value
            else if value_type is 'boolean'
                return @template_json_tree.span
                    classname: 'jt_bool'
                    value: if value then 'true' else 'false'
 

        # Build the table
        # We order by the most frequent keys then by alphabetic order
        json_to_table: (result) =>
            if not (result.constructor? and result.constructor is Array)
                result = [result]

            map = {}
            for element in result
                if jQuery.isPlainObject(element)
                    for key of element
                        if map[key]?
                            map[key]++
                        else
                            map[key] = 1
                else
                    map['_anonymous object'] = Infinity

            keys_sorted = []
            for key of map
                keys_sorted.push [key, map[key]]

            keys_sorted.sort((a, b) ->
                if a[1] < b[1]
                    return 1
                else if a[1] > b[1]
                    return -1
                else
                    if a[0] < b[0]
                        return -1
                    else if a[0] > b[0]
                        return 1
                    else return 0
            )

            @last_keys = _.union(['record'], keys_sorted.map( (key) -> return key[0] ))

            return @template_json_table.container
                table_attr: @json_to_table_get_attr keys_sorted
                table_data: @json_to_table_get_values result, keys_sorted

        json_to_table_get_attr: (keys_sorted) =>
            attr = []
            for element, col in keys_sorted
                attr.push
                    key: element[0]
                    col: col
 
            return @template_json_table.tr_attr
                attr: attr

        json_to_table_get_values: (result, keys_stored) =>
            if not @start_record?
                @start_record = @prototype.start_record
            document_list = []
            for element, i in result
                new_document = {}
                new_document.cells = []
                for key_container, col in keys_stored
                    key = key_container[0]
                    if key is '_anonymous object'
                        value = element
                    else
                        value = element[key]

                    new_document.cells.push @json_to_table_get_td_value value, col
                new_document.record = @start_record + i
                document_list.push new_document
            return @template_json_table.tr_value
                document: document_list

        json_to_table_get_td_value: (value, col) =>
            data = @compute_data_for_type(value, col)

            return @template_json_table.td_value
                col: col
                cell_content: @template_json_table.td_value_content data
            
        compute_data_for_type: (value,  col) =>
            data =
                value: value
                class_value: 'value-'+col

            value_type = typeof value
            if value is null
                data['value'] = 'null'
                data['classname'] = 'jta_null'
            else if value is undefined
                data['value'] = 'undefined'
                data['classname'] = 'jta_undefined'
            else if value.constructor? and value.constructor is Array
                if value.length is 0
                    data['value'] = '[ ]'
                    data['classname'] = 'empty array'
                else
                    #TODO Build preview
                    #TODO Add arrows for attributes
                    data['value'] = '[ ... ]'
                    data['data_to_expand'] = JSON.stringify(value)
            else if value_type is 'object'
                data['value'] = '{ ... }'
                data['data_to_expand'] = JSON.stringify(value)
            else if value_type is 'number'
                data['classname'] = 'jta_num'
            else if value_type is 'string'
                if /^(http|https):\/\/[^\s]+$/i.test(value)
                    data['classname'] = 'jta_url'
                else if /^[a-z0-9]+@[a-z0-9]+.[a-z0-9]{2,4}/i.test(value) # We don't handle .museum extension and special characters
                    data['classname'] = 'jta_email'
                else
                    data['classname'] = 'jta_string'
            else if value_type is 'boolean'
                data['classname'] = 'jta_bool'

            return data

        # Expand a JSON object in a table. We just call the @json_to_tree
        expand_tree_in_table: (event) =>
            dom_element = @.$(event.target).parent()
            data = dom_element.data('json_data')
            result = @json_to_tree data
            dom_element.html result
            classname_to_change = dom_element.parent().attr('class').split(' ')[0] #TODO Use a Regex
            $('.'+classname_to_change).css 'max-width', 'none'
            classname_to_change = dom_element.parent().parent().attr('class')
            $('.'+classname_to_change).css 'max-width', 'none'
 
            @.$(event.target).parent().css 'max-width', 'none'
            @.$(event.target).remove() #TODO Fix this trick
            @set_scrollbar()


        # Expand the table with new columns (with the attributes of the expanded object)
        expand_table_in_table: (event) =>
            dom_element = @.$(event.target).parent()
            parent = dom_element.parent()
            classname = dom_element.parent().attr('class').split(' ')[0] #TODO Use a regex
            data = dom_element.data('json_data')
            if data.constructor? and data.constructor is Array
                classcolumn = dom_element.parent().parent().attr('class')
                $('.'+classcolumn).css 'max-width', 'none'
                join_table = @join_table
                $('.'+classname).each ->
                    $(this).children('.jta_arrow_v').remove()
                    new_data = $(this).children('.jta_object').data('json_data')
                    if new_data? and new_data.constructor? and new_data.constructor is Array
                        $(this).children('.jta_object').html join_table(new_data)
                        $(this).children('.jta_arrow_h').remove()
                    $(this).css 'max-width', 'none'
                        
            else if typeof data is 'object'
                classcolumn = dom_element.parent().parent().attr('class')
                map = {}
                $('.'+classname).each ->
                    new_data = $(this).children('.jta_object').data('json_data')
                    if new_data? and typeof new_data is 'object'
                        for key of new_data
                            if map[key]?
                                map[key]++
                            else
                                map[key] = 1
                    $(this).css 'max-width', 'none'
                keys_sorted = []
                for key of map
                    keys_sorted.push [key, map[key]]

                # TODO Use a stable sort, Mozilla doesn't use a stable sort for .sort
                keys_sorted.sort (a, b) ->
                    if a[1] < b[1]
                        return 1
                    else if a[1] > b[1]
                        return -1
                    else
                        if a[0] < b[0]
                            return -1
                        else if a[0] > b[0]
                            return 1
                        else return 0
                for i in [keys_sorted.length-1..0] by -1
                    key = keys_sorted[i]
                    collection = $('.'+classcolumn)

                    is_description = true
                    template_json_table_td_attr = @template_json_table.td_attr
                    json_to_table_get_td_value = @json_to_table_get_td_value

                    collection.each ->
                        if is_description
                            is_description = false
                            prefix = $(this).children('.jta_attr').html()
                            $(this).after template_json_table_td_attr
                                classtd: classcolumn+'-'+i
                                key: prefix+'.'+key[0]
                                col: $(this).data('col')+'-'+i
                        else
                            new_data = $(this).children().children('.jta_object').data('json_data')
                            if new_data? and new_data[key[0]]?
                                value = new_data[key[0]]
                                    
                            full_class = classname+'-'+i
                            col = full_class.slice(full_class.indexOf('-')+1) #TODO Replace this trick

                            $(this).after json_to_table_get_td_value(value, col)
                            
                        return true

                $('.'+classcolumn) .remove()
            @set_scrollbar()

        # Helper for expanding a table when showing an object (creating new columns)
        join_table: (data) =>
            result = ''
            for value, i in data
                data_cell = @compute_data_for_type(value, 'float')
                data_cell['is_inline'] = true
                if i isnt data.length-1
                    data_cell['need_comma'] = true

                result += @template_json_table.data_inline data_cell
                 
            return result

        #TODO change cursor
        mouse_down: false
        handle_mousedown: (event) =>
            if event.target.nodeName is 'TD' and event.which is 1
                @event_target = event.target
                @col_resizing = event.target.dataset.col
                @start_width = @.$(event.target).width()
                @start_x = event.pageX
                @mouse_down = true
                @.$('.json_table').toggleClass('resizing', true)

        #TODO Handle when last column is resized or when table expands too much
        handle_mousemove: (event) =>
            if @mouse_down
                @resize_column @col_resizing, @start_width-@start_x+event.pageX # Resize
                @last_columns_size[@col_resizing] = @start_width-@start_x+event.pageX # Save the personalized size

        resize_column: (col, size) =>
            $('.col-'+col).css 'max-width', size
            $('.value-'+col).css 'max-width', size-20
            $('.col-'+col).css 'width', size
            $('.value-'+col).css 'width', size-20

        handle_mouseup: (event) =>
            @mouse_down = false
            @.$('.json_table').toggleClass('resizing', false)
            @set_scrollbar()

        default_size_column: 310 # max-width value of a cell of a table (as defined in the css file)

        render_result: (args) =>
            if args.query?
                @query = args.query
            if args.results?
                @result = args.results
            @.$el.html @template
                query: @query

            switch @prototype.view
                when 'tree'
                    @.$('.tree_view').html @json_to_tree @result
                    @$('.results').hide()
                    @$('.tree_view_container').show()
                    @.$('.link_to_tree_view').addClass 'active'
                    @.$('.link_to_tree_view').parent().addClass 'active'
                when 'table'
                    previous_keys = @last_keys # Save previous keys. @last_keys will be updated in @json_to_table
                    @.$('.table_view').html @json_to_table @result
                    @$('.results').hide()
                    @$('.table_view_container').show()
                    @.$('.link_to_table_view').addClass 'active'
                    @.$('.link_to_table_view').parent().addClass 'active'

                    # Check if the keys are the same
                    if @last_keys.length isnt previous_keys.length
                        same_keys = false
                    else
                        same_keys = true
                        for keys, index in @last_keys
                            if @last_keys[index] isnt previous_keys[index]
                                same_keys = false

                    # If the keys are the same, we are going to resize the columns as they were before
                    if same_keys is true
                        for col, value of @last_columns_size
                            @resize_column col, value
                    else
                        # Reinitialize @last_columns_size
                        @last_column_size = {}

                    # Let's try to expand as much as we can
                    extra_size_table = @$('.json_table_container').width()-@$('.json_table').width()
                    if extra_size_table > 0 # The table doesn't take the full width
                        expandable_columns = []
                        for index in [0..@last_keys.length-2] # We skip the column record
                            real_size = 0
                            @$('.col-'+index).children().children().each((i, bloc) ->
                                $bloc = $(bloc)
                                if real_size<$bloc.width()
                                    real_size = $bloc.width()
                            )
                            if real_size? and real_size is real_size and real_size > @default_size_column
                                expandable_columns.push
                                    col: index
                                    size: real_size+20 # 20 for padding
                        while expandable_columns.length > 0
                            expandable_columns.sort (a, b) ->
                                return a.size-b.size
                            if expandable_columns[0].size-@$('.col-'+expandable_columns[0].col).width() < extra_size_table/expandable_columns.length
                                extra_size_table = extra_size_table-(expandable_columns[0]['size']-@$('.col-'+expandable_columns[0].col).width())

                                @$('.col-'+expandable_columns[0]['col']).css 'max-width', expandable_columns[0]['size']
                                @$('.value-'+expandable_columns[0]['col']).css 'max-width', expandable_columns[0]['size']-20
                                expandable_columns.shift()
                            else
                                max_size = extra_size_table/expandable_columns.length
                                for column in expandable_columns
                                    current_size = @$('.col-'+expandable_columns[0].col).width()
                                    @$('.col-'+expandable_columns[0]['col']).css 'max-width', current_size+max_size
                                    @$('.value-'+expandable_columns[0]['col']).css 'max-width', current_size+max_size-20
                                expandable_columns = []
                when 'raw'
                    @.$('.raw_view_textarea').html JSON.stringify @result
                    @$('.results').hide()
                    @$('.raw_view_container').show()
                    @expand_raw_textarea()
                    @.$('.link_to_raw_view').addClass 'active'
                    @.$('.link_to_raw_view').parent().addClass 'active'

            @set_scrollbar()
            if (not query?) and (not result?)
                @render_metadata()
 
        set_scrollbar: =>
            if @view is 'table'
                content_name = '.json_table'
                content_container = '.table_view_container'
            else if @view is 'tree'
                content_name = '.json_tree'
                content_container = '.tree_view_container'
            else if @view is 'raw'
                @$('.wrapper_scrollbar').hide()
                # There is no scrolbar with the raw view
                return

            # Set the floating scrollbar
            width_value = @$(content_name).innerWidth() # Include padding
            if width_value < @$(content_container).width()
                # If there is no need for scrollbar, we hide the one on the top
                @$('.wrapper_scrollbar').hide()
                $(window).unbind 'scroll'
            else
                # Else we set the fake_content to the same width as the table that contains data and links the two scrollbars
                @$('.wrapper_scrollbar').show()
                @$('.scrollbar_fake_content').width width_value

                $(".wrapper_scrollbar").scroll ->
                    $(content_container).scrollLeft($(".wrapper_scrollbar").scrollLeft())
                $(content_container).scroll ->
                    $(".wrapper_scrollbar").scrollLeft($(content_container).scrollLeft())

                position_scrollbar = ->
                    if $(content_container).offset()?
                        # Sometimes we don't have to display the scrollbar (when the results are not shown because the query is too big)
                        if $(window).scrollTop()+$(window).height() < $(content_container).offset().top+20 # bottom of the window < beginning of $('.json_table_container') // 20 pixels is the approximate height of the scrollbar (so we don't show JUST the scrollbar)
                            that.$('.wrapper_scrollbar').hide()
                        # We show the scrollbar and stick it to the bottom of the window because there is more content below
                        else if $(window).scrollTop()+$(window).height() < $(content_container).offset().top+$(content_container).height() # bottom of the window < end of $('.json_table_container')
                            that.$('.wrapper_scrollbar').show()
                            that.$('.wrapper_scrollbar').css 'overflow', 'auto'
                            that.$('.wrapper_scrollbar').css 'margin-bottom', '0px'
                        # And sometimes we "hide" it
                        else
                            # We can not hide .wrapper_scrollbar because it would break the binding between wrapper_scrollbar and content_container
                            that.$('.wrapper_scrollbar').css 'overflow', 'hidden'

                that = @
                position_scrollbar()
                $(window).scroll ->
                    position_scrollbar()
                $(window).resize ->
                    position_scrollbar()



        # Render the metadata of an the results
        render_metadata: (data) =>
            if data?
                @metadata = data
            else # If we have called render_metadata() without arguments, it's because we just switched view
                data = @metadata

            limit_value = data.limit_value
            skip_value = data.skip_value
            execution_time = data.execution_time
            query = data.query
            has_more_data = data.has_more_data

            if execution_time?
                if execution_time < 1000
                    execution_time_pretty = execution_time+"ms"
                else if execution_time < 60*1000
                    execution_time_pretty = (execution_time/1000).toFixed(2)+"s"
                else # We do not expect query to last one hour.
                    minutes = Math.floor(execution_time/(60*1000))
                    execution_time_pretty = minutes+"min "+((execution_time-minutes*60*1000)/1000).toFixed(2)+"s"

            data =
                skip_value: skip_value
                limit_value: limit_value
                execution_time: (execution_time_pretty if execution_time_pretty?)

            @.$('.metadata').html @metadata_template data

            # TODO Since we removed pagination and iter, this thing should be moved in render_results or another method.
            #render pagination
            if has_more_data? and has_more_data is true
                @check_if_cursor_timed_out()
                @.$('.more_results').show()
            else
                @.$('.more_results').hide()
            
        # Check if the cursor timed out. If yes, make sure that the user cannot fetch more results
        check_if_cursor_timed_out: =>

            if @prototype.cursor_timed_out is true
                @.$('.more_results_paragraph').html @cursor_timed_out_template()


        render: =>
            @delegateEvents()
            return @

        render_default: =>
            @.$el.html @default_template()
            return @

        toggle_collapse: (event) =>
            @.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed')
            @.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed')
            @.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed')
            @.$(event.target).toggleClass('jt_arrow_hidden')
            @set_scrollbar()

        #TODO Fix it for Firefox
        expand_raw_textarea: =>
            setTimeout(@bootstrap_hack, 0) #TODO remove this trick when we will remove bootstrap's tab 
        bootstrap_hack: =>
            @expand_textarea 'raw_view_textarea'
            return @

        expand_textarea: (classname) =>
            if $('.'+classname).length > 0
                height = $('.'+classname)[0].scrollHeight
                $('.'+classname).height(height)

        destroy: =>
            $(document).unbind 'mousemove', @handle_mousemove
            $(document).unbind 'mouseup', @handle_mouseup
            $(window).unbind 'scroll'
            $(window).unbind 'resize'


    class @DriverHandler
        # I don't want that thing in window
        constructor: (args) ->
            @container = args.container
            @on_success = args.on_success
            @on_fail = args.on_fail

            if window.location.port is ''
                if window.location.protocol is 'https:'
                    port = 443
                else
                    port = 80
            else
                port = parseInt window.location.port
            @server =
                host: window.location.hostname
                port: port
                protocol: if window.location.protocol is 'https:' then 'https' else 'http'

            @connect()



        connect: =>
            # Whether we are going to reconnect or not, the cursor might have timed out.
            @container.saved_data.cursor_timed_out = true

            if @connection?
                if @driver_status is 'connected'
                    try
                        @connection.close()
                    catch err
                        # Nothing bad here, let's just not pollute the console

                window.conn = r.connect window.server, @on_successs, @on_fail
            @connection = r.connect @server, @on_success, @on_fail
            @timeout = setTimeout @connect, 5*60*1000
    
        postpone_reconnection: =>
            clearTimeout @timeout
            @timeout = setTimeout @connect, 5*60*1000

        destroy: =>
            try
                @connection.close()
            catch err
                # Nothing bad here, let's just not pollute the console
            clearTimeout @timeout

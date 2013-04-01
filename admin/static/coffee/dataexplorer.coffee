# Copyright 2010-2012 RethinkDB, all rights reserved.
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
        query_error_template: Handlebars.templates['dataexplorer-query_error-template']

        # Constants
        limit: 40 # How many results we display per page // Final for now
        line_height: 13 # Define the height of a line (used for a line is too long)
        size_history: 100
        
        max_size_stack: 100 # If the stack of the query (including function, string, object etc. is greater than @max_size_stack, we stop parsing the query
        max_size_query: 1000 # If the query is more than 1000 char, we don't show suggestion (codemirror doesn't highlight/parse if the query is more than 1000 characters too

        events:
            'mouseup .CodeMirror': 'handle_click'
            'mousedown .suggestion_name_li': 'select_suggestion' # Keep mousedown to compete with blur on .input_query
            'mouseover .suggestion_name_li' : 'mouseover_suggestion'
            'mouseout .suggestion_name_li' : 'mouseout_suggestion'
            'click .clear_query': 'clear_query'
            'click .execute_query': 'execute_query'
            'click .change_size': 'toggle_size'
            'click #reconnect': 'reconnect'
            'click .more_valid_results': 'show_more_results'
            'click .close': 'close_alert'
            'click .clear_queries_link': 'clear_history_view'
            'click .close_queries_link': 'open_close_history'

        clear_history_view: (event) =>
            @history_view.clear_history event
            @clear_history()

        open_close_history: (event, args) =>
            @history_view.open_close_history(args)
            if @history_view.state is 'visible'
                @saved_data.history_state = 'visible'
                if args?.no_animation is true
                    @$('.clear_queries_link').show()
                else
                    @$('.clear_queries_link').fadeIn 'fast'
                @$('.close_queries_link').addClass 'active'
            else
                @saved_data.history_state = 'hidden'
                @$('.clear_queries_link').fadeOut 'fast'
                @$('.close_queries_link').removeClass 'active'

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

        # Once we are done moving the doc, we could generate a .js in the makefile file with the data so we don't have to do an ajax request+all this stuff
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
                    args: ('( '+command['langs']['js']['body']+' )' if command['langs']['js']['dont_need_parenthesis'] isnt true)
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

        # All the commands we are going to ignore
        # TODO update the set of ignored commands for 1.4
        ignored_commands:
            'connect': true
            'close': true
            'reconnect': true
            'use': true
            'runp': true
            'next': true
            'collect': true
            'run': true

        # Method called on the content of reql_docs.json
        # Load the suggestions in @suggestions, @map_state, @descriptions
        set_docs: (data) =>
            # Create the suggestions table
            suggestions = {}
            for group in data['sections']
                for command in group['commands']
                    tag = command['langs']['js']['name']
                    if tag of @ignored_commands
                        continue
                    if tag is 'row' # In case we have row(, we give a better description than just (
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

            if DataExplorerView.Container.prototype.focus_on_codemirror is true
                # "@" refers to prototype -_-
                # In case we give focus to codemirror then load the docs, we show the suggestion
                window.router.current_view.handle_keypress()

        # Save the query in the history
        # The size of the history is infinite per session. But we will just save @size_history queries in localStorage
        save_query: (args) =>
            query = args.query
            broken_query = args.broken_query
            # Remove empty lines
            query = query.replace(/^\s*$[\n\r]{1,}/gm, '')
            query = query.replace(/\s*$/, '') # Remove the white spaces at the end of the query (like newline/space/tab)
            if window.localStorage?
                if @history.length is 0 or @history[@history.length-1].query isnt query and @regex.white.test(query) is false
                    @history.push
                        query: query
                        broken_query: broken_query
                    if @history.length>@size_history
                        window.localStorage.rethinkdb_history = JSON.stringify @history.slice @history.length-@size_history
                    else
                        window.localStorage.rethinkdb_history = JSON.stringify @history
                    @history_view.add_query
                        query: query
                        broken_query: broken_query

        clear_history: =>
            @history.length = 0
            window.localStorage.rethinkdb_history = JSON.stringify @history

        initialize: (args) =>
            # We do not load data from localStorage.
            if not DataExplorerView.Container.prototype.saved_data?
                DataExplorerView.Container.prototype.saved_data =
                    current_query: null
                    query: null
                    results: null
                    cursor: null
                    metadata: null
                    cursor_timed_out: true
                    view: 'tree'
                    history_state: 'hidden'

            # Load history, keep it in memory for the session
            if not DataExplorerView.Container.prototype.history?
                if window.localStorage?.rethinkdb_history?
                    try
                        DataExplorerView.Container.prototype.history = JSON.parse window.localStorage.rethinkdb_history
                    catch err
                        DataExplorerView.Container.prototype.history = []
                else
                    DataExplorerView.Container.prototype.history = []
            @history = DataExplorerView.Container.prototype.history

            # Let's have a shortcut
            @prototype = DataExplorerView.Container.prototype
            @saved_data = DataExplorerView.Container.prototype.saved_data
            @show_query_warning = @saved_data.query isnt @saved_data.current_query # Show a warning in case the displayed results are not the one of the query in code mirror
            @current_results = @saved_data.results

            # Index used to navigate through history with the keyboard
            @history_displayed_id = 0 # 0 means we are showing the draft, n>0 means we are showing the nth query in the history

            # We escape the last function because we are building a regex on top of it.
            # Structure: [ [ pattern, replacement], [pattern, replacement], ... ]
            @unsafe_to_safe_regexstr = [
                [/\\/g, '\\\\'] # This one has to be first
                [/\(/g, '\\(']
                [/\)/g, '\\)']
                [/\^/g, '\\^']
                [/\$/g, '\\$']
                [/\*/g, '\\*']
                [/\+/g, '\\+']
                [/\?/g, '\\?']
                [/\./g, '\\.']
                [/\|/g, '\\|']
                [/\{/g, '\\{']
                [/\}/g, '\\}']
                [/\[/g, '\\[']
            ]

            @results_view = new DataExplorerView.ResultView
                container: @
                limit: @limit
                view: @saved_data.view

            @history_view = new DataExplorerView.HistoryView
                container: @
                history: @history

            @driver_handler = new DataExplorerView.DriverHandler
                container: @
                on_success: @success_on_connect
                on_fail: @error_on_connect

            # One callback to rule them all
            $(window).mousemove @handle_mousemove
            $(window).mouseup @handle_mouseup

            @id_execution = 0

            @render()

        handle_mousemove: (event) =>
            @results_view.handle_mousemove event
            @history_view.handle_mousemove event

        handle_mouseup: (event) =>
            @results_view.handle_mouseup event
            @history_view.handle_mouseup event

        render: =>
            @$el.html @template()
            @$('.input_query_full_container').html @input_query_template()
            
            # Check if the browser supports the JavaScript driver
            # We do not support internet explorer (even IE 10) and old browsers.
            if navigator?.appName is 'Microsoft Internet Explorer'
                @$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                    is_internet_explorer: true
                @$('.reason_dataexplorer_broken').slideDown 'fast'
                @$('.button_query').prop 'disabled', 'disabled'
            else if (not DataView?) or (not Uint8Array?) # The main two components that the javascript driver requires.
                @$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                @$('.reason_dataexplorer_broken').slideDown 'fast'
                @$('.button_query').prop 'disabled', 'disabled'
            else if not window.r? # In case the javascript driver is not found (if build from source for example)
                @$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                    no_driver: true
                @$('.reason_dataexplorer_broken').slideDown 'fast'
                @$('.button_query').prop 'disabled', 'disabled'

            # Let's bring back the data explorer to its old state (if there was)
            if @saved_data?.query? and @saved_data?.results? and @saved_data?.metadata?
                @$('.results_container').html @results_view.render_result({
                    show_query_warning: @show_query_warning
                    results: @saved_data.results
                    metadata: @saved_data.metadata
                }).$el
                # The query in code mirror is set in init_after_dom_rendered (because we can't set it now)
            else
                @$('.results_container').html @results_view.render_default().$el

            # If driver not conneced
            if @driver_connected is false
                @error_on_connect()
    
            @$('.history_container').html @history_view.render().$el
            return @

        # This method has to be called AFTER the el element has been inserted in the DOM tree, mostly for codemirror
        init_after_dom_rendered: =>
            @codemirror = CodeMirror.fromTextArea document.getElementById('input_query'),
                mode:
                    name: 'javascript'
                onKeyEvent: @handle_keypress
                lineNumbers: true
                lineWrapping: true
                matchBrackets: true
                tabSize: 2

            @codemirror.on 'blur', @on_blur
            @codemirror.on 'gutterClick', @handle_gutter_click

            @codemirror.setSize '100%', 'auto'
            if @saved_data.current_query?
                @codemirror.setValue @saved_data.current_query
            @codemirror.focus() # Give focus
            
            # Track if the focus is on codemirror
            # We use it to refresh the docs once the reql_docs.json is loaded
            @prototype.focus_on_codemirror = true

            @codemirror.setCursor @codemirror.lineCount(), 0
            if @codemirror.getValue() is '' # We show suggestion for an empty query only
                @handle_keypress()
            @results_view.expand_raw_textarea()

            @draft = @codemirror.getValue()

            if @saved_data.history_state is 'visible' # If the history was visible, we show it
                @open_close_history event,
                    no_animation: true

        on_blur: =>
            @prototype.focus_on_codemirror = false
            @hide_suggestion_and_description()

        # We have to keep track of a lot of things because web-kit browsers handle the events keydown, keyup, blur etc... in a strange way.
        current_suggestions: []
        current_highlighted_suggestion: -1
        current_conpleted_query: ''
        query_first_part: ''
        query_last_part: ''
        mouse_type_event:
            click: true
            dblclick: true
            mousedown: true
            mouseup: true
            mouseover: true
            mouseout: true
            mousemove: true
        char_breakers:
            '.': true
            '}': true
            ')': true
            ',': true
            ';': true
            ']': true
            
        handle_click: (event) =>
            @handle_keypress null, event

        # Pair ', ", {, [, (
        # Return true if we want code mirror to ignore the key event
        pair_char: (event, stack) =>
            if event?.which?
                # If there is a selection and the user hit a quote, we wrap the seleciton in quotes
                if @codemirror.getSelection() isnt '' and event.type is 'keypress' # This is madness. If we look for keydown, shift+right arrow match a single quote...
                    char_to_insert = String.fromCharCode event.which
                    if char_to_insert? and char_to_insert is '"' or char_to_insert is "'"
                        @codemirror.replaceSelection(char_to_insert+@codemirror.getSelection()+char_to_insert)
                        event.preventDefault()
                        return true
                    return true

                if event.which is 8 # Backspace
                    if event.type isnt 'keydown'
                        return true
                    previous_char = @get_previous_char()
                    if previous_char is null
                        return true
                    # If the user remove the opening bracket and the next char is the closing bracket, we delete both
                    if previous_char of @matching_opening_bracket
                        next_char = @get_next_char()
                        if next_char is @matching_opening_bracket[previous_char]
                            num_not_closed_bracket = @count_not_closed_brackets previous_char
                            if num_not_closed_bracket <= 0
                                @remove_next()
                                return true
                    # If the user remove the first quote of an empty string, we remove both quotes
                    else if previous_char is '"' or previous_char is "'"
                        next_char = @get_next_char()
                        if next_char is previous_char and @get_previous_char(2) isnt '\\'
                            num_quote = @count_char char_to_insert
                            if num_quote%2 is 0
                                @remove_next()
                                return true
                    return true

                char_to_insert = String.fromCharCode event.which
                if char_to_insert?
                    if event.type isnt 'keypress' # We catch keypress because single and double quotes have not the same keyCode on keydown/keypres #thisIsMadness
                        return true

                    if char_to_insert is '"' or char_to_insert is "'"
                        num_quote = @count_char char_to_insert
                        next_char = @get_next_char()
                        if next_char is char_to_insert # Next char is a single quote
                            if num_quote%2 is 0
                                if @last_element_type_if_incomplete(stack) is 'string' # We are at the end of a string and the user just wrote a quote 
                                    @move_cursor 1
                                    event.preventDefault()
                                    return true
                                else
                                    # We are at the begining of a string, so let's just add one quote
                                    return true
                            else
                                # Let's add the closing/opening quote missing
                                return true
                        else
                            if num_quote%2 is 0 # Next char is not a single quote and the user has an even number of quotes. 
                                # Let's keep a number of quote even, so we add one extra quote
                                if @last_element_type_if_incomplete(stack) isnt 'string'
                                    @insert_next char_to_insert
                                else # We add a quote inside a string, probably something like that 'He doesn|\'t'
                                    return true
                            else # Else we'll just insert one quote
                                return true
                    else if @last_element_type_if_incomplete(stack) isnt 'string'
                        next_char = @get_next_char()

                        if char_to_insert of @matching_opening_bracket
                            num_not_closed_bracket = @count_not_closed_brackets char_to_insert
                            if num_not_closed_bracket >= 0 # We insert a closing bracket only if it help having a balanced number of opened/closed brackets
                                @insert_next @matching_opening_bracket[char_to_insert]
                                return true
                            return true
                        else if char_to_insert of @matching_closing_bracket
                            opening_char = @matching_closing_bracket[char_to_insert]
                            num_not_closed_bracket = @count_not_closed_brackets opening_char
                            if next_char is char_to_insert
                                if num_not_closed_bracket <= 0 # g(f(...|) In this case we add a closing parenthesis. Same behavior as in Ace
                                    @move_cursor 1
                                    event.preventDefault()
                                return true
            return false

        get_next_char: =>
            cursor_end = @codemirror.getCursor()
            cursor_end.ch++
            return @codemirror.getRange @codemirror.getCursor(), cursor_end

        get_previous_char: (less_value) =>
            cursor_start = @codemirror.getCursor()
            cursor_end = @codemirror.getCursor()
            if less_value?
                cursor_start.ch -= less_value
                cursor_end.ch -= (less_value-1)
            else
                cursor_start.ch--
            if cursor_start.ch < 0
                return null
            return @codemirror.getRange cursor_start, cursor_end


        # Insert str after the cursor in codemirror
        insert_next: (str) =>
            @codemirror.replaceRange str, @codemirror.getCursor()
            @move_cursor -1

        remove_next: =>
            end_cursor = @codemirror.getCursor()
            end_cursor.ch++
            @codemirror.replaceRange '', @codemirror.getCursor(), end_cursor

        # Move cursor of move_value
        # A negative value move the cursor to the left
        move_cursor: (move_value) =>
            cursor = @codemirror.getCursor()
            cursor.ch += move_value
            if cursor.ch < 0
                cursor.ch = 0
            @codemirror.setCursor cursor


        # Count how many time char_to_count appeared ignoring strings and comments
        count_char: (char_to_count) =>
            query = @codemirror.getValue()

            is_parsing_string = false
            to_skip = 0
            result = 0

            for char, i in query
                if to_skip > 0 # Because we cannot mess with the iterator in coffee-script
                    to_skip--
                    continue

                if is_parsing_string is true
                    if char is string_delimiter and query[i-1]? and query[i-1] isnt '\\' # We were in a string. If we see string_delimiter and that the previous character isn't a backslash, we just reached the end of the string.
                        is_parsing_string = false # Else we just keep parsing the string
                        if char is char_to_count
                            result++
                else # if element.is_parsing_string is false
                    if char is char_to_count
                        result++

                    if char is '\'' or char is '"' # So we get a string here
                        is_parsing_string = true
                        string_delimiter = char
                        continue
                    
                    result_inline_comment = @regex.inline_comment.exec query.slice i
                    if result_inline_comment?
                        to_skip = result_inline_comment[0].length-1
                        start += result_inline_comment[0].length
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        to_skip = result_multiple_line_comment[0].length-1
                        start += result_multiple_line_comment[0].length
                        continue

            return result

        matching_opening_bracket:
            '(': ')'
            '{': '}'
            '[': ']'
        matching_closing_bracket:
            ')': '('
            '}': '{'
            ']': '['


        # opening_char has to be in @matching_bracket
        # Count how many time opening_char has been opened but not closed
        # A result < 0 means that the closing char has been found more often than the opening one
        count_not_closed_brackets: (opening_char) =>
            query = @codemirror.getValue()

            is_parsing_string = false
            to_skip = 0
            result = 0

            for char, i in query
                if to_skip > 0 # Because we cannot mess with the iterator in coffee-script
                    to_skip--
                    continue

                if is_parsing_string is true
                    if char is string_delimiter and query[i-1]? and query[i-1] isnt '\\' # We were in a string. If we see string_delimiter and that the previous character isn't a backslash, we just reached the end of the string.
                        is_parsing_string = false # Else we just keep parsing the string
                else # if element.is_parsing_string is false
                    if char is opening_char
                        result++
                    else if char is @matching_opening_bracket[opening_char]
                        result--

                    if char is '\'' or char is '"' # So we get a string here
                        is_parsing_string = true
                        string_delimiter = char
                        continue
                    
                    result_inline_comment = @regex.inline_comment.exec query.slice i
                    if result_inline_comment?
                        to_skip = result_inline_comment[0].length-1
                        start += result_inline_comment[0].length
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        to_skip = result_multiple_line_comment[0].length-1
                        start += result_multiple_line_comment[0].length
                        continue

            return result


        # Handle events on codemirror
        # Return true if we want code mirror to ignore the event
        handle_keypress: (editor, event) =>
            if @ignored_next_keyup is true
                if event?.type is 'keyup' and event?.which isnt 9
                    @ignored_next_keyup = false
                return true

            @prototype.focus_on_codemirror = true

            # Let's hide the tooltip if the user just clicked on the textarea. We'll only display later the suggestions if there are (no description)
            if event?.type is 'mouseup'
                @hide_suggestion_and_description()

            # Save the last query (even incomplete)
            @saved_data.current_query = @codemirror.getValue()

            # Look for special commands
            if event?.which?
                if event.which is 27 # ESC
                    event.preventDefault() # Keep focus on code mirror
                    @hide_suggestion_and_description()
                    return true
                else if event.which is 13 and (event.shiftKey is false and event.ctrlKey is false and event.metaKey is false)
                    if event.type is 'keydown'
                        if @current_highlighted_suggestion > -1
                            event.preventDefault()
                            @handle_keypress()
                            return true

                        previous_char = @get_previous_char()
                        if previous_char of @matching_opening_bracket
                            next_char = @get_next_char()
                            if @matching_opening_bracket[previous_char] is next_char
                                cursor = @codemirror.getCursor()
                                @insert_next '\n'
                                @codemirror.indentLine cursor.line+1, 'smart'
                                @codemirror.setCursor cursor
                                return false
                else if event.which is 9 # If the user hit tab, we switch the highlighted suggestion
                    event.preventDefault()
                    if event.type is 'keydown'
                        if @current_suggestions?.length > 0
                            if @$('.suggestion_name_list').css('display') is 'none'
                                @show_suggestion()
                                return true
                            else
                                # We can retrieve the content of codemirror only on keyup events. The users may write "r." then hit "d" then "tab" If the events are triggered this way
                                # keydown d - keydown tab - keyup d - keyup tab
                                # We want to only show the suggestions for r.d
                                if @written_suggestion is null
                                    cached_query = @query_first_part+@current_element+@query_last_part
                                else
                                    cached_query = @query_first_part+@written_suggestion+@query_last_part
                                if cached_query isnt @codemirror.getValue() # We fired a keydown tab before a keyup, so our suggestions are not up to date
                                    @current_element = @codemirror.getValue().slice @query_first_part.length, @codemirror.getValue().length-@query_last_part.length
                                    regex = @create_safe_regex @current_element
                                    new_suggestions = []
                                    new_highlighted_suggestion = -1
                                    for suggestion, index in @current_suggestions
                                        if index < @current_highlighted_suggestion
                                            new_highlighted_suggestion = new_suggestions.length
                                        if regex.test(suggestion) is true
                                            new_suggestions.push suggestion
                                    @current_suggestions = new_suggestions
                                    @current_highlighted_suggestion = new_highlighted_suggestion
                                    if @current_suggestions.length > 0
                                        @.$('.suggestion_name_list').empty()
                                        for suggestion, i in @current_suggestions
                                            @.$('.suggestion_name_list').append @template_suggestion_name
                                                id: i
                                                suggestion: suggestion
                                        @ignored_next_keyup = true
                                    else
                                        @hide_suggestion_and_description()


                                # Switch throught the suggestions
                                if event.shiftKey
                                    @current_highlighted_suggestion--
                                    if @current_highlighted_suggestion < -1
                                        @current_highlighted_suggestion = @current_suggestions.length-1
                                    else if @current_highlighted_suggestion < 0
                                        @show_suggestion_without_moving()
                                        @remove_highlight_suggestion()
                                        @write_suggestion
                                            suggestion_to_write: @current_element
                                        @ignore_tab_keyup = true # If we are switching suggestion, we don't want to do anything else related to tab
                                        return true
                                else
                                    @current_highlighted_suggestion++
                                    if @current_highlighted_suggestion >= @current_suggestions.length
                                        @show_suggestion_without_moving()
                                        @remove_highlight_suggestion()
                                        @write_suggestion
                                            suggestion_to_write: @current_element
                                        @ignore_tab_keyup = true # If we are switching suggestion, we don't want to do anything else related to tab
                                        @current_highlighted_suggestion = -1
                                        return true
                                if @current_suggestions[@current_highlighted_suggestion]?
                                    @show_suggestion_without_moving()
                                    @highlight_suggestion @current_highlighted_suggestion # Highlight the current suggestion
                                    @write_suggestion
                                        suggestion_to_write: @current_suggestions[@current_highlighted_suggestion] # Auto complete with the highlighted suggestion
                                    @ignore_tab_keyup = true # If we are switching suggestion, we don't want to do anything else related to tab
                                    return true
                        else if @extra_suggestions? and @extra_suggestions.length > 0 and @extra_suggestion.start_body is @extra_suggestion.start_body
                            # Trim suggestion
                            if @extra_suggestion?.body?[0]?.type is 'string'
                                if @extra_suggestion.body[0].complete is true
                                    @extra_suggestions = []
                                else
                                    # Remove quotes around the table/db name
                                    current_name = @extra_suggestion.body[0].name.replace(/^\s*('|")/, '').replace(/('|")\s*$/, '')
                                    regex = @create_safe_regex current_name
                                    new_extra_suggestions = []
                                    for suggestion in @extra_suggestions
                                        if regex.test(suggestion) is true
                                            new_extra_suggestions.push suggestion
                                    @extra_suggestions = new_extra_suggestions

                            if @extra_suggestions.length > 0 # If there are still some valid suggestions
                                query = @codemirror.getValue()

                                # We did not parse what is after the cursor, so let's take a look
                                start_search = @extra_suggestion.start_body
                                if @extra_suggestion.body?[0]?.name.length?
                                    start_search += @extra_suggestion.body[0].name.length

                                # Define @query_first_part and @query_last_part
                                # Note that ) is not a valid character for a db/table name
                                end_body = query.indexOf ')', start_search
                                @query_last_part = ''
                                if end_body isnt -1
                                    @query_last_part = query.slice end_body
                                @query_first_part = query.slice 0, @extra_suggestion.start_body
                                lines = @query_first_part.split('\n')
                                # Because we may have slice before @cursor_for_auto_completion, we re-define it
                                @cursor_for_auto_completion =
                                    line: lines.length-1
                                    ch: lines[lines.length-1].length

                                if event.shiftKey is true
                                    @current_highlighted_extra_suggestion--
                                else
                                    @current_highlighted_extra_suggestion++
                                    
                                if @current_highlighted_extra_suggestion >= @extra_suggestions.length
                                    @current_highlighted_extra_suggestion = -1
                                else if @current_highlighted_extra_suggestion < -1
                                    @current_highlighted_extra_suggestion = @extra_suggestions.length-1

                                # Create the next suggestion
                                suggestion = ''
                                if @current_highlighted_extra_suggestion is -1
                                    if @current_extra_suggestion?
                                        if /^\s*'/.test(@current_extra_suggestion) is true
                                            suggestion = @current_extra_suggestion+"'"
                                        else if /^\s*"/.test(@current_extra_suggestion) is true
                                            suggestion = @current_extra_suggestion+'"'
                                else
                                    if /^\s*'/.test(@current_extra_suggestion) is true
                                        string_delimiter = "'"
                                    else if /^\s*"/.test(@current_extra_suggestion) is true
                                        string_delimiter = '"'
                                    else
                                        move_outside = true
                                        string_delimiter = "'"
                                    suggestion = string_delimiter+@extra_suggestions[@current_highlighted_extra_suggestion]+string_delimiter
                                
                                @write_suggestion
                                    move_outside: move_outside
                                    suggestion_to_write: suggestion
                                @ignore_tab_keyup = true # If we are switching suggestion, we don't want to do anything else related to tab


                # If the user hit enter and (Ctrl or Shift)
                else if event.which is 13 and (event.shiftKey or event.ctrlKey or event.metaKey)
                    @hide_suggestion_and_description()
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return true
                    @execute_query()
                    return true
                # Ctrl/Cmd + V
                else if (event.ctrlKey or event.metaKey) and event.which is 86 and event.type is 'keydown'
                    @last_action_is_paste = true
                    @num_released_keys = 0 # We want to know when the user release Ctrl AND V
                    if event.metaKey
                        @num_released_keys++ # Because on OS X, the keyup event is not fired when the metaKey is pressed (true for Firefox, Chrome, Safari at least...)
                    @hide_suggestion_and_description()
                    return true
                # When the user release Ctrl/Cmd after a Ctrl/Cmd + V
                else if event.type is 'keyup' and @last_action_is_paste is true and (event.which is 17 or event.which is 91)
                    @num_released_keys++
                    if @num_released_keys is 2
                        @last_action_is_paste = false
                    @hide_suggestion_and_description()
                    return true
                # When the user release V after a Ctrl/Cmd + V
                else if event.type is 'keyup' and @last_action_is_paste is true and event.which is 86
                    @num_released_keys++
                    if @num_released_keys is 2
                        @last_action_is_paste = false
                    @hide_suggestion_and_description()
                    return true
                # Catching history navigation
                else if event.type is 'keyup' and event.altKey and event.which is 38 # Key up
                    if @history_displayed_id < @history.length
                        @history_displayed_id++
                        @codemirror.setValue @history[@history.length-@history_displayed_id].query
                        event.preventDefault()
                        return true
                else if event.type is 'keyup' and event.altKey and event.which is 40 # Key down
                    if @history_displayed_id > 1
                        @history_displayed_id--
                        @codemirror.setValue @history[@history.length-@history_displayed_id].query
                        event.preventDefault()
                        return true
                    else if @history_displayed_id is 1
                        @history_displayed_id--
                        @codemirror.setValue @draft
                        @codemirror.setCursor @codemirror.lineCount(), 0 # We hit the draft and put the cursor at the end
                else if event.type is 'keyup' and event.altKey and event.which is 33 # Page up
                    @history_displayed_id = @history.length
                    @codemirror.setValue @history[@history.length-@history_displayed_id].query
                    event.preventDefault()
                    return true
                else if event.type is 'keyup' and event.altKey and event.which is 34 # Page down
                    @history_displayed_id = @history.length
                    @codemirror.setValue @history[@history.length-@history_displayed_id].query
                    @codemirror.setCursor @codemirror.lineCount(), 0 # We hit the draft and put the cursor at the end
                    event.preventDefault()
                    return true
            # If there is a hilighted suggestion, we want to catch enter
            if @$('.suggestion_name_li_hl').length > 0
                if event?.which is 13
                    event.preventDefault()
                    @handle_keypress()
                    return true

            # We are scrolling in history
            if @history_displayed_id isnt 0 and event?
                # We catch ctrl, shift, alt, 
                if event.ctrlKey or event.shiftKey or event.altKey or event.which is 16 or event.which is 17 or event.which is 18 or event.which is 20 or event.which is 91 or event.which is 92 or event.type of @mouse_type_event
                    return false

            # Avoid arrows+home+end+page down+pageup
            # if event? and (event.which is 24 or event.which is ..)
            # 0 is for firefox...
            if not event? or (event.which isnt 37 and event.which isnt 38 and event.which isnt 39 and event.which isnt 40 and event.which isnt 33 and event.which isnt 34 and event.which isnt 35 and event.which isnt 36 and event.which isnt 0)
                @history_displayed_id = 0
                @draft = @codemirror.getValue()

            # The expensive operations are coming. If the query is too long, we just don't parse the query
            if @codemirror.getValue().length > @max_size_query
                return false

            query_before_cursor = @codemirror.getRange {line: 0, ch: 0}, @codemirror.getCursor()
            query_after_cursor = @codemirror.getRange @codemirror.getCursor(), {line:@codemirror.lineCount()+1, ch: 0}

            # Compute the structure of the query written by the user.
            # We compute it earlier than before because @pair_char also listen on keydown and needs stack
            stack = @extract_data_from_query
                size_stack: 0
                query: query_before_cursor
                position: 0

            if stack is null # Stack is null if the query was too big for us to parse
                @ignore_tab_keyup = false
                @hide_suggestion_and_description()
                return false
            @pair_char(event, stack) # Pair brackets/quotes

            # We just look at key up so we don't fire the call 3 times
            if event?.type? and event.type isnt 'keyup' and event.which isnt 9 and event.type isnt 'mouseup'
                return false
            if event?.which is 16 # We don't do anything with Shift.
                return false

            # Tab is an exception, we let it pass (tab bring back suggestions) - What we want is to catch keydown
            if @ignore_tab_keyup is true and event?.which is 9
                if event.type is 'keyup'
                    @ignore_tab_keyup = false
                return true

            @current_highlighted_suggestion = -1
            @current_highlighted_extra_suggestion = -1
            @.$('.suggestion_name_list').empty()

            # Valid step, let's save the data
            @query_last_part = query_after_cursor

            # If a selection is active, we just catch shift+enter
            if @codemirror.getSelection() isnt ''
                @hide_suggestion_and_description()
                if event? and event.which is 13 and (event.shiftKey or event.ctrlKey or event.metaKey) # If the user hit enter and (Ctrl or Shift or Cmd)
                    @hide_suggestion_and_description()
                    if event.type isnt 'keydown'
                        return true
                    @execute_query()
                    return true # We do not replace the selection with a new line
                # If the user select something and end somehwere with suggestion
                if event?.type isnt 'mouseup'
                    return false
                else
                    return true

            @current_suggestions = []
            @current_element = ''
            @current_extra_suggestion = ''
            @written_suggestion = null
            @cursor_for_auto_completion = @codemirror.getCursor()

            result =
                status: null
                # create_suggestion is going to fill to_complete and to_describe
                #to_complete: undefined
                #to_describe: undefined
                
            # If we are in the middle of a function (text after the cursor - that is not an element in @char_breakers), we just show a description, not a suggestion
            result_non_white_char_after_cursor = @regex.get_first_non_white_char.exec(query_after_cursor)

            if result_non_white_char_after_cursor isnt null and not(result_non_white_char_after_cursor[1]?[0] of @char_breakers)
                result.status = 'break_and_look_for_description'
                @hide_suggestion()
            else
                result_last_char_is_white = @regex.last_char_is_white.exec(query_before_cursor[query_before_cursor.length-1])
                if result_last_char_is_white isnt null
                    result.status = 'break_and_look_for_description'
                    @hide_suggestion()

            # Create the suggestion/description
            @create_suggestion
                stack: stack
                query: query_before_cursor
                result: result

            if result.suggestions?.length > 0
                for suggestion, i in result.suggestions
                    @current_suggestions.push suggestion
                    @.$('.suggestion_name_list').append @template_suggestion_name
                        id: i
                        suggestion: suggestion
                @show_suggestion()
                @hide_description()
            else if result.description? and event?.type isnt 'mouseup'
                @hide_suggestion()
                @show_description result.description
            else
                @hide_suggestion_and_description()

            if event?.which is 9 # Catch tab
                # If you're in a string, you add a TAB. If you're at the beginning of a newline with preceding whitespace, you add a TAB. If it's any other case do nothing.
                if @last_element_type_if_incomplete(stack) isnt 'string' and @regex.white_or_empty.test(@codemirror.getLine(@codemirror.getCursor().line).slice(0, @codemirror.getCursor().ch)) isnt true
                    return true
                else
                    return false
           
            return true

        # Extract information from the current query
        # Regex used
        regex:
            anonymous:/^(\s)*function\(([a-zA-Z0-9,\s]*)\)(\s)*{/
            loop:/^(\s)*(for|while)(\s)*\(([^\)]*)\)(\s)*{/
            method: /^(\s)*([a-zA-Z]*)\(/ # forEach( merge( filter(
            method_var: /^(\s)*([a-zA-Z]+)\./ # r. r.row. (r.count will be caught later)
            return : /^(\s)*return(\s)*/
            object: /^(\s)*{(\s)*/
            array: /^(\s)*\[(\s)*/
            white: /^(\s)+$/
            white_or_empty: /^(\s)*$/
            white_replace: /\s/g
            white_start: /^(\s)+/
            comma: /^(\s)*,(\s)*/
            semicolon: /^(\s)*;(\s)*/
            number: /^[0-9]+\.?[0-9]*/
            inline_comment: /^(\s)*\/\/.*\n/
            multiple_line_comment: /^(\s)*\/\*[^]*\*\//
            get_first_non_white_char: /\s*(\S+)/
            last_char_is_white: /.*(\s+)$/
        stop_char: # Just for performance (we look for a stop_char in constant time - which is better than having 3 and conditions) and cleaner code
            opening:
                '(': ')'
                '{': '}'
                '[': ']'
            closing:
                ')': '(' # Match the opening character
                '}': '{'
                ']': '['

        # Return the type of the last incomplete object or an empty string
        last_element_type_if_incomplete: (stack) =>
            if stack.length is 0
                return ''

            element = stack[stack.length-1]
            if element.body?
                return @last_element_type_if_incomplete(element.body)
            else
                if element.complete is false
                    return element.type
                else
                    return ''

        # We build a stack of the query.
        # Chained functions are in the same array, arguments/inner queries are in a nested array
        # element.type in ['string', 'function', 'var', 'separator', 'anonymous_function', 'object', 'array_entry', 'object_key' 'array']
        extract_data_from_query: (args) =>
            size_stack = args.size_stack
            query = args.query
            context = if args.context? then DataUtils.deep_copy(args.context) else {}
            position = args.position

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
                    if char is string_delimiter and query[i-1]? and query[i-1] isnt '\\' # We were in a string. If we see string_delimiter and that the previous character isn't a backslash, we just reached the end of the string.
                        is_parsing_string = false # Else we just keep parsing the string
                        if element.type is 'string'
                            element.name = query.slice start, i+1
                            element.complete = true
                            stack.push element
                            size_stack++
                            if size_stack > @max_size_stack
                                return null
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
                    
                    result_inline_comment = @regex.inline_comment.exec query.slice i
                    if result_inline_comment?
                        to_skip = result_inline_comment[0].length-1
                        start += result_inline_comment[0].length
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        to_skip = result_multiple_line_comment[0].length-1
                        start += result_multiple_line_comment[0].length
                        continue

                    if element.type is null
                        if start is i
                            result_white = @regex.white_start.exec query.slice i
                            if result_white?
                                to_skip = result_white[0].length-1
                                start += result_white[0].length
                                continue

                            # Check for anonymous function
                            result_regex = @regex.anonymous.exec query.slice i
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

                            # Check for a for loop 
                            result_regex = @regex.loop.exec query.slice i
                            if result_regex isnt null
                                element.type = 'loop'
                                element.context = context
                                to_skip = result_regex[0].length
                                body_start = i+result_regex[0].length
                                stack_stop_char = ['{']
                                continue
                                                       
                            # Check for return
                            result_regex = @regex.return.exec query.slice i
                            if result_regex isnt null
                                # I'm not sure we need to keep track of return, but let's keep it for now
                                element.type = 'return'
                                element.complete = true
                                to_skip = result_regex[0].length-1
                                stack.push element
                                size_stack++
                                if size_stack > @max_size_stack
                                    return null
                                element =
                                    type: null
                                    context: context
                                    complete: false

                                start = i+result_regex[0].length
                                continue

                            # Check for object
                            result_regex = @regex.object.exec query.slice i
                            if result_regex isnt null
                                element.type = 'object'
                                element.next_key = null
                                element.body = [] # We need to keep tracker of the order of pairs
                                element.current_key_start = i+result_regex[0].length
                                to_skip = result_regex[0].length-1
                                stack_stop_char = ['{']
                                continue

                            # Check for array
                            result_regex = @regex.array.exec query.slice i
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

                            # Check for a standard method
                            result_regex = @regex.method.exec query.slice new_start
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
                                        element.real_type = 'json'
                                        element.type = 'var'
                                        element.name = result_regex[0].slice(0, position_opening_parenthesis)
                                        stack.push element
                                        size_stack++
                                        if size_stack > @max_size_stack
                                            return null
                                        element =
                                            type: 'function'
                                            name: '('
                                            position: position+position_opening_parenthesis+1
                                            context: context
                                            complete: 'false'
                                        stack_stop_char = ['(']
                                        start = position_opening_parenthesis+1
                                        to_skip = result_regex[0].length-1

                            # Check for method without parenthesis r., r.row., doc.
                            result_regex = @regex.method_var.exec query.slice new_start
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
                                size_stack++
                                if size_stack > @max_size_stack
                                    return null
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
                                size_stack++
                                if size_stack > @max_size_stack
                                    return null
                                element =
                                    type: null
                                    context: context
                                    complete: false
                                start = i+result_regex[0].length-1+1
                                to_skip = result_regex[0].length-1
                                continue

                            # Look for a semi colon
                            result_regex = @regex.semicolon.exec query.slice i
                            if result_regex isnt null
                                # element should have been pushed in stack. If not, the query is malformed
                                element.complete = true
                                stack.push
                                    type: 'separator'
                                    complete: true
                                    name: query.slice i, result_regex[0].length
                                size_stack++
                                if size_stack > @max_size_stack
                                    return null
                                element =
                                    type: null
                                    context: context
                                    complete: false
                                start = i+result_regex[0].length-1+1
                                to_skip = result_regex[0].length-1
                                continue
                        #else # if element.start isnt i
                        # We caught the white spaces, so there is nothing to do here
                        else
                            if char is ';'
                                # We just encountered a semi colon. We have an unknown element
                                # So We just got a random javascript statement, let's just ignore it
                                start = i+1
                    else # element.type isnt null
                        # Catch separator like for groupedMapReduce
                        result_regex = @regex.comma.exec(query.slice(i))
                        if result_regex isnt null and stack_stop_char.length < 1
                            # element should have been pushed in stack. If not, the query is malformed
                            stack.push
                                type: 'separator'
                                complete: true
                                name: query.slice i, result_regex[0].length
                                position: position+i
                            size_stack++
                            if size_stack > @max_size_stack
                                return null
                            element =
                                type: null
                                context: context
                                complete: false
                            start = i+result_regex[0].length-1
                            to_skip = result_regex[0].length-1
                            continue

                        # Catch for anonymous function
                        else if element.type is 'anonymous_function'
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        element.body = @extract_data_from_query
                                            size_stack: size_stack
                                            query: query.slice body_start, i
                                            context: element.context
                                            position: position+body_start
                                        if element.body is null
                                            return null
                                        element.complete = true
                                        stack.push element
                                        size_stack++
                                        if size_stack > @max_size_stack
                                            return null
                                        element =
                                            type: null
                                            context: context
                                        start = i+1
                                #else the written query is broken here. The user forgot to close something?
                                #TODO Default behavior? Wait for Brackets/Ace to see how we handle errors
                        else if element.type is 'loop'
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        element.body = @extract_data_from_query
                                            size_stack: size_stack
                                            query: query.slice body_start, i
                                            context: element.context
                                            position: position+body_start
                                        if element.body
                                            return null
                                        element.complete = true
                                        stack.push element
                                        size_stack++
                                        if size_stack > @max_size_stack
                                            return null
                                        element =
                                            type: null
                                            context: context
                                        start = i+1
                        # Catch for function
                        else if element.type is 'function'
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        element.body = @extract_data_from_query
                                            size_stack: size_stack
                                            query: query.slice start+element.name.length, i
                                            context: element.context
                                            position: position+start+element.name.length
                                        if element.body is null
                                            return null
                                        element.complete = true
                                        stack.push element
                                        size_stack++
                                        if size_stack > @max_size_stack
                                            return null
                                        element =
                                            type: null
                                            context: context
                                        start = i+1

                        # Catch for object
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
                                            body = @extract_data_from_query
                                                size_stack: size_stack
                                                query: query.slice element.current_value_start, i
                                                context: element.context
                                                position: position+element.current_value_start
                                            if body is null
                                                return null
                                            new_element =
                                                type: 'object_key'
                                                key: element.next_key
                                                key_complete: true
                                                complete: false
                                                body: body
                                            element.body[element.body.length-1] = new_element
                                        element.next_key = null # No more next_key
                                        element.complete = true
                                        # if not element.next_key?
                                        # The next key is not defined, this is a broken query.
                                        # TODO show error once brackets/ace will be used

                                        stack.push element
                                        size_stack++
                                        if size_stack > @max_size_stack
                                            return null
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
                                        size_stack++
                                        if size_stack > @max_size_stack
                                            return null
                                    else
                                        element.body[element.body.length-1] = new_element
                                    element.next_key = query.slice element.current_key_start, i
                                    element.current_value_start = i+1
                            else
                                result_regex = @regex.comma.exec query.slice i
                                if stack_stop_char.length is 1 and result_regex isnt null #We reached the end of a value
                                    body = @extract_data_from_query
                                        size_stack: size_stack
                                        query: query.slice element.current_value_start, i
                                        context: element.context
                                        position: element.current_value_start
                                    if body is null
                                        return null
                                    new_element =
                                        type: 'object_key'
                                        key:  element.next_key
                                        key_complete: true
                                        body: body
                                    element.body[element.body.length-1] = new_element
                                    to_skip = result_regex[0].length-1
                                    element.next_key = null
                                    element.current_key_start = i+result_regex[0].length
                        # Catch for array
                        else if element.type is 'array'
                            if char of @stop_char.opening
                                stack_stop_char.push char
                            else if char of @stop_char.closing
                                if stack_stop_char[stack_stop_char.length-1] is @stop_char.closing[char]
                                    stack_stop_char.pop()
                                    if stack_stop_char.length is 0
                                        # We just reach a ], it's the end of the object
                                        body = @extract_data_from_query
                                            size_stack: size_stack
                                            query: query.slice entry_start, i
                                            context: element.context
                                            position: position+entry_start
                                        if body is null
                                            return null
                                        new_element =
                                            type: 'array_entry'
                                            complete: true
                                            body: body
                                        if new_element.body.length > 0
                                            element.body.push new_element
                                            size_stack++
                                            if size_stack > @max_size_stack
                                                return null
                                        continue

                            if stack_stop_char.length is 1 and char is ','
                                body = @extract_data_from_query
                                    size_stack: size_stack
                                    query: query.slice entry_start, i
                                    context: element.context
                                    position: position+entry_start
                                if body is null
                                    return null
                                new_element =
                                    type: 'array_entry'
                                    complete: true
                                    body: body
                                if new_element.body.length > 0
                                    element.body.push new_element
                                    size_stack++
                                    if size_stack > @max_size_stack
                                        return null
                                entry_start = i+1

            # We just reached the end, let's try to find the type of the incomplete element
            if element.type isnt null
                element.complete = false
                if element.type is 'function'
                    element.body = @extract_data_from_query
                        size_stack: size_stack
                        query: query.slice start+element.name.length
                        context: element.context
                        position: position+start+element.name.length
                    if element.body is null
                        return null
                else if element.type is 'anonymous_function'
                    element.body = @extract_data_from_query
                        size_stack: size_stack
                        query: query.slice body_start
                        context: element.context
                        position: position+body_start
                    if element.body is null
                        return null
                else if element.type is 'loop'
                    element.body = @extract_data_from_query
                        size_stack: size_stack
                        query: query.slice body_start
                        context: element.context
                        position: position+body_start
                    if element.body is null
                        return null
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
                        size_stack++
                        if size_stack > @max_size_stack
                            return null
                        element.next_key = query.slice element.current_key_start
                    else
                        body = @extract_data_from_query
                            size_stack: size_stack
                            query: query.slice element.current_value_start
                            context: element.context
                            position: position+element.current_value_start
                        if body is null
                            return null
                        new_element =
                            type: 'object_key'
                            key: element.next_key
                            key_complete: true
                            complete: false
                            body: body
                        element.body[element.body.length-1] = new_element
                        element.next_key = null # No more next_key
                else if element.type is 'array'
                    body = @extract_data_from_query
                        size_stack: size_stack
                        query: query.slice entry_start
                        context: element.context
                        position: position+entry_start
                    if body is null
                        return null
                    new_element =
                        type: 'array_entry'
                        complete: false
                        body: body
                    if new_element.body.length > 0
                        element.body.push new_element
                        size_stack++
                        if size_stack > @max_size_stack
                            return null
                stack.push element
                size_stack++
                if size_stack > @max_size_stack
                    return null
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
                    element.type = 'function'
                    element.position = position+start
                    element.name = query.slice start+1
                    element.complete = false
                else
                    element.name = query.slice start
                    element.position = position+start
                    element.complete = false
                stack.push element
                size_stack++
                if size_stack > @max_size_stack
                    return null
            return stack

        # Decide if we have to show a suggestion or a description
        # Mainly use the stack created by extract_data_from_query
        create_suggestion: (args) =>
            stack = args.stack
            query = args.query
            result = args.result

            # No stack, ie an empty query
            if result.status is null and stack.length is 0
                result.suggestions = []
                result.status = 'done'
                @query_first_part = ''
                if @suggestions['']? # The docs may not have loaded
                    for suggestion in @suggestions['']
                        result.suggestions.push suggestion

            for i in [stack.length-1..0] by -1
                element = stack[i]
                if element.body? and element.body.length > 0 and element.complete is false
                    @create_suggestion
                        stack: element.body
                        query: args?.query
                        result: args.result

                if result.status is 'done'
                    continue


                if result.status is null
                    # Top of the stack
                    if element.complete is true
                        if element.type is 'function'
                            if element.complete is true or element.name is ''
                                result.suggestions = null
                                result.status = 'look_for_description'
                                break
                            else
                                result.suggestions = null
                                result.description = element.name
                                #Define the current argument we have. It's the suggestion whose index is -1
                                @extra_suggestion =
                                    start_body: element.position + element.name.length
                                    body: element.body
                                if element.body?[0]?.name?.length?
                                    @cursor_for_auto_completion.ch -= element.body[0].name.length
                                    @current_extra_suggestion = element.body[0].name
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
                                @extra_suggestion =
                                    start_body: element.position + element.name.length
                                    body: element.body
                                if element.body?[0]?.name?.length?
                                    @cursor_for_auto_completion.ch -= element.body[0].name.length
                                    @current_extra_suggestion = element.body[0].name
                                result.status = 'done'
                                break
                            else
                                # function not complete, need suggestion
                                result.suggestions = []
                                result.suggestions_regex = @create_safe_regex element.name # That means we are going to give all the suggestions that match element.name and that are in the good group (not yet defined)
                                result.description = null
                                @query_first_part = query.slice 0, element.position+1
                                @current_element = element.name
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
                            result.status = 'look_for_description'
                            break
                else if result.status is 'look_for_description'
                    if element.type is 'function'
                        result.description = element.name
                        @extra_suggestion =
                            start_body: element.position + element.name.length
                            body: element.body
                        if element.body?[0]?.name?.length?
                            @cursor_for_auto_completion.ch -= element.body[0].name.length
                            @current_extra_suggestion = element.body[0].name
                        result.suggestions = null
                        result.status = 'done'
                    else
                        break
                if result.status is 'break_and_look_for_description'
                    if element.type is 'function' and element.complete is false and element.name.indexOf('(') isnt -1
                        result.description = element.name
                        @extra_suggestion =
                            start_body: element.position + element.name.length
                            body: element.body
                        if element.body?[0]?.name?.length?
                            @cursor_for_auto_completion.ch -= element.body[0].name.length
                            @current_extra_suggestion = element.body[0].name
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
                        #else # This is a non valid ReQL function.
                        # It may be a personalized function defined in the data explorer...
                        result.status = 'done'
                    else if element.type is 'var' and element.complete is true
                        result.state = element.real_type
                        if @suggestions[result.state]?
                            for suggestion in @suggestions[result.state]
                                if result.suggestions_regex.test(suggestion) is true
                                    result.suggestions.push suggestion
                        result.status = 'done'
                    #else # Is that possible? A function can only be preceded by a function (except for r)

        # Create regex based on the user input. We make it safe
        create_safe_regex: (str) =>
            for char in @unsafe_to_safe_regexstr
                str = str.replace char[0], char[1]
            return new RegExp('^('+str+')', 'i')

        # Show suggestion and determine where to put the box
        show_suggestion: =>
            @move_suggestion()
            margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'
            @.$('.suggestion_full_container').css 'margin-top', margin
            @.$('.arrow').css 'margin-top', margin

            @.$('.suggestion_name_list').show()
            @.$('.arrow').show()
        
        # If want to show suggestion without moving the arrow
        show_suggestion_without_moving: =>
            @.$('.arrow').show()
            @.$('.suggestion_name_list').show()

        # Show description and determine where to put it
        show_description: (fn) =>
            if @descriptions[fn]? # Just for safety
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
        hide_suggestion_and_description: =>
            @hide_suggestion()
            @hide_description()

        # Show the arrow if suggestion or/and description is being displayed
        show_or_hide_arrow: =>
            if @.$('.suggestion_name_list').css('display') is 'none' and @.$('.suggestion_description').css('display') is 'none'
                @.$('.arrow').hide()
            else
                @.$('.arrow').show()

        # Move the suggestion. We have steps of 200 pixels and try not to overlaps button if we can. If we cannot, we just hide them all since their total width is less than 200 pixels
        move_suggestion: =>
            margin_left = parseInt(@.$('.CodeMirror-cursor').css('left').replace('px', ''))+23
            @.$('.arrow').css 'margin-left', margin_left
            if margin_left < 200
                @.$('.suggestion_full_container').css 'left', '0px'
            else
                max_margin = @.$('.CodeMirror-scroll').width()-418

                margin_left_bloc = Math.min max_margin, Math.floor(margin_left/200)*200
                if margin_left > max_margin+418-150-23 # We are really at the end
                    @.$('.suggestion_full_container').css 'left', (max_margin-34)+'px'
                else if margin_left_bloc > max_margin-150-23
                    @.$('.suggestion_full_container').css 'left', (max_margin-34-150)+'px'
                else
                    @.$('.suggestion_full_container').css 'left', (margin_left_bloc-100)+'px'

        #Highlight suggestion. Method called when the user hit tab or mouseover
        highlight_suggestion: (id) =>
            @remove_highlight_suggestion()
            @.$('.suggestion_name_li').eq(id).addClass 'suggestion_name_li_hl'
            @.$('.suggestion_description').html @description_template @extend_description @current_suggestions[id]
            
            @.$('.suggestion_description').show()

        remove_highlight_suggestion: =>
            @.$('.suggestion_name_li').removeClass 'suggestion_name_li_hl'

        # Write the suggestion in the code mirror
        write_suggestion: (args) =>
            suggestion_to_write = args.suggestion_to_write
            move_outside = args.move_outside is true # So default value is false

            ch = @cursor_for_auto_completion.ch+suggestion_to_write.length
            if suggestion_to_write[suggestion_to_write.length-1] is '(' and @count_not_closed_brackets('(') >= 0
                @codemirror.setValue @query_first_part+suggestion_to_write+')'+@query_last_part
                @written_suggestion = suggestion_to_write+')'
            else
                @codemirror.setValue @query_first_part+suggestion_to_write+@query_last_part
                @written_suggestion = suggestion_to_write
                if (move_outside is false) and (suggestion_to_write[suggestion_to_write.length-1] is '"' or suggestion_to_write[suggestion_to_write.length-1] is "'")
                    ch--
            @codemirror.focus() # Useful if the user used the mouse to select a suggestion
            @codemirror.setCursor
                line: @cursor_for_auto_completion.line
                ch:ch

        # Select the suggestion. Called by mousdown .suggestion_name_li
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
            @hide_description()

        # Extend description for .db() and .table() with dbs/tables names
        extend_description: (fn) =>
            if @options?.can_extend? and @options?.can_extend is false
                @extra_suggestions= null
                return @descriptions[fn]

            if fn is 'db(' or fn is 'dbDrop('
                description = _.extend {}, @descriptions[fn]
                if databases.length is 0
                    data =
                        no_database: true
                else
                    databases_available = databases.models.map (database) -> return database.get('name')
                    data =
                        no_database: false
                        databases_available: databases_available
                description.description = @databases_suggestions_template(data)+description.description
                @extra_suggestions= databases_available # @extra_suggestions store the suggestions for arguments. So far they are just for db(), dbDrop(), table(), tableDrop()
            else if fn is 'table(' or fn is 'tableDrop('
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

                description.description = @namespaces_suggestions_template(data) + description.description

                @extra_suggestions= namespaces_available
            else
                description = @descriptions[fn]
                @extra_suggestions= null
            return description

        # We could create a new stack with @extract_data_from_query, but that would be a more expensive for not that much
        # We can not use the previous stack too since autocompletion doesn't validate the query until you hit enter (or another key than tab)
        extract_database_used: =>
            query_before_cursor = @codemirror.getRange {line: 0, ch: 0}, @codemirror.getCursor()
            # We cannot have ".db(" in a db name
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

        # Function triggered when the user click on 'more results'
        show_more_results: (event) =>
            event.preventDefault()
            @skip_value += @current_results.length
            try
                @current_results = []
                @start_time = new Date()

                @id_execution++
                get_result_callback = @generate_get_result_callback @id_execution
                @saved_data.cursor.next get_result_callback
                $(window).scrollTop(@.$('.results_container').offset().top)
            catch err
                @.$('.loading_query_img').css 'display', 'none'
                @results_view.render_error(@query, err)

        # Function that execute the queries in a synchronous way.
        execute_query: =>
            # The user just executed a query, so we reset cursor_timed_out to false
            @saved_data.cursor_timed_out = false
            @saved_data.show_query_warning = false

            # postpone reconnection
            @driver_handler.postpone_reconnection()

            @raw_query = @codemirror.getValue()
            @query = @replace_new_lines_in_query @raw_query # Save it because we'll use it in @callback_multilples_queries
            
            # Execute the query
            try
                # Separate queries
                @non_rethinkdb_query = '' # Store the statements that don't return a rethinkdb query (like "var a = 1;")
                @index = 0 # index of the query currently being executed

                @raw_queries = @separate_queries @raw_query # We first split raw_queries
                @queries = @separate_queries @query

                if @queries.length is 0
                    error = @query_error_template
                        no_query: true
                    @results_view.render_error(null, error)
                else
                    @.$('.loading_query_img').show()
                    @execute_portion()

            catch err
                @.$('.loading_query_img').hide()
                @results_view.render_error(@query, err)
                @save_query
                    query: @raw_query
                    broken_query: true

        # A portion is one query of the whole input.
        execute_portion: =>
            @saved_data.cursor = null
            while @queries[@index]?
                @driver_handler.reset_count()

                full_query = @non_rethinkdb_query
                full_query += @queries[@index]

                try
                    rdb_query = @evaluate(full_query)
                catch err
                    @.$('.loading_query_img').hide()
                    @results_view.render_error(@raw_queries[@index], err)
                    @save_query
                        query: @raw_query
                        broken_query: true
                    return false

                @index++

                if rdb_query instanceof TermBase
                    @skip_value = 0
                    @start_time = new Date()
                    @current_results = []

                    @id_execution++ # Update the id_execution and use it to tag the callbacks
                    rdb_global_callback = @generate_rdb_global_callback @id_execution
                    rdb_query.private_run @driver_handler.connection, rdb_global_callback # @rdb_global_callback can be fire more than once
                    return true
                else if rdb_query instanceof DataExplorerView.DriverHandler
                    # Nothing to do
                    return true
                else
                    @non_rethinkdb_query += @queries[@index-1]
                    if @index is @queries.length
                        @.$('.loading_query_img').hide()
                        error = @query_error_template
                            last_non_query: true
                        @results_view.render_error(@raw_queries[@index-1], error)
                        @save_query
                            query: @raw_query
                            broken_query: true

        
        # Create a callback for when a query returns
        # We tag the callback to make sure that we display the results only of the last query executed by the user
        generate_rdb_global_callback: (id_execution) =>
            rdb_global_callback = (error, cursor) =>
                if @id_execution is id_execution # We execute the query only if it is the last one
                    get_result_callback = @generate_get_result_callback id_execution

                    if error?
                        @.$('.loading_query_img').hide()
                        @results_view.render_error(@raw_queries[@index-1], error)
                        @save_query
                            query: @raw_query
                            broken_query: true

                        return false
                    
                    if @index is @queries.length # @index was incremented in execute_portion
                        if cursor?
                            @saved_data.cursor = @cursor

                        if cursor?.hasNext?
                            @cursor = cursor
                            if cursor.hasNext() is true
                                @cursor.next get_result_callback
                            else
                                get_result_callback() # Display results
                        else
                            @.$('.loading_query_img').hide()

                            # Save the last executed query and the last displayed results
                            @current_results = cursor

                            @saved_data.query = @query
                            @saved_data.results = @current_results
                            @saved_data.metadata =
                                limit_value: if @current_results?.length? then @current_results.length else 1 # If @current_results.length is not defined, we have a single value
                                skip_value: @skip_value
                                execution_time: new Date() - @start_time
                                query: @query
                                has_more_data: false

                            @results_view.render_result
                                results: @current_results # The first parameter is null ( = query, so we don't display it)
                                metadata: @saved_data.metadata

                            # Successful query, let's save it in the history
                            @save_query
                                query: @raw_query
                                broken_query: false
                    else
                        @execute_portion()

            return rdb_global_callback

        # Create a callback used in cursor.next()
        # We tag the callback to make sure that we display the results only of the last query executed by the user
        generate_get_result_callback: (id_execution) =>
            get_result_callback = (error, data) =>
                if @id_execution is id_execution
                    if error?
                        @results_view.render_error(@query, error)
                        return false

                    if data isnt undefined
                        @current_results.push data
                        if @current_results.length < @limit and @cursor.hasNext() is true
                            @cursor.next get_result_callback
                            return true

                    @.$('.loading_query_img').hide()

                    # if data is undefined or @current_results.length is @limit
                    @saved_data.cursor = @cursor # Let's save the cursor, there may be mor edata to retrieve
                    @saved_data.query = @query
                    @saved_data.results = @current_results
                    @saved_data.metadata =
                        limit_value: if @current_results?.length? then @current_results.length else 1 # If @current_results.length is not defined, we have a single value
                        skip_value: @skip_value
                        execution_time: new Date() - @start_time
                        query: @query
                        has_more_data: @cursor.hasNext()

                    @results_view.render_result
                        results: @current_results # The first parameter is null ( = query, so we don't display it)
                        metadata: @saved_data.metadata

                    # Successful query, let's save it in the history
                    @save_query
                        query: @raw_query
                        broken_query: false
                else
                    @execute_portion()

        get_result_callback: (error, data) =>
            if error?
                @results_view.render_error(@query, error)
                @save_query
                    query: @raw_query
                    broken_query: true
                return false

            if data isnt undefined
                @current_results.push data
                if @current_results.length < @limit and @cursor.hasNext() is true
                    @cursor.next @get_result_callback
                    return true

            # if data is undefined or @current_results.length is @limit
            @saved_data.cursor = @cursor # Let's save the cursor, there may be mor edata to retrieve
            @saved_data.query = @query
            @saved_data.results = @current_results
            @saved_data.metadata =
                limit_value: @current_results.length
                skip_value: @skip_value
                execution_time: new Date() - @start_time
                query: @query
                has_more_data: @cursor.hasNext()

            @results_view.render_result
                results: @current_results # The first parameter is null ( = query, so we don't display it)
                metadata: @saved_data.metadata

            # Successful query, let's save it in the history
            @save_query
                query: @raw_query
                broken_query: false

            return get_result_callback

        # Evaluate the query
        # We cannot force eval to a local scope, but "use strict" will declare variables in the scope at least
        evaluate: (query) =>
            "use strict"
            return eval(query)

        # In a string \n becomes \\\\n, outside a string we just remove \n, so
        #   r
        #   .expr('hello
        #   world')
        # becomes
        #   r.expr('hello\nworld')
        replace_new_lines_in_query: (query) ->
            is_parsing_string = false
            start = 0

            result_query = ''
            for char, i in query
                if to_skip > 0
                    to_skip--
                    continue

                if is_parsing_string is true
                    if char is string_delimiter and query[i-1]? and query[i-1] isnt '\\'
                        result_query += query.slice(start, i+1).replace(/\n/g, '\\\\n')
                        start = i+1
                        is_parsing_string = false
                        continue
                else # if element.is_parsing_string is false
                    if char is '\'' or char is '"'
                        result_query += query.slice(start, i).replace(/\n/g, '')
                        start = i
                        is_parsing_string = true
                        string_delimiter = char
                        continue

                    result_inline_comment = @regex.inline_comment.exec query.slice i
                    if result_inline_comment?
                        to_skip = result_inline_comment[0].length-1
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        to_skip = result_multiple_line_comment[0].length-1
                        continue
            if is_parsing_string
                result_query += query.slice(start, i).replace(/\n/g, '\\\\n')
            else
                result_query += query.slice(start, i).replace(/\n/g, '')

            return result_query

        # Split input in queries. We use semi colon, pay attention to string, brackets and comments
        separate_queries: (query) =>
            queries = []
            is_parsing_string = false
            stack = []
            start = 0
            
            position =
                char: 0
                line: 1

            for char, i in query
                if char is '\n'
                    position.line++
                    position.char = 0
                else
                    position.char++

                if to_skip > 0 # Because we cannot mess with the iterator in coffee-script
                    to_skip--
                    continue

                if is_parsing_string is true
                    if char is string_delimiter and query[i-1]? and query[i-1] isnt '\\'
                        is_parsing_string = false
                        continue
                else # if element.is_parsing_string is false
                    if char is '\'' or char is '"'
                        is_parsing_string = true
                        string_delimiter = char
                        continue

                    result_inline_comment = @regex.inline_comment.exec query.slice i
                    if result_inline_comment?
                        to_skip = result_inline_comment[0].length-1
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        to_skip = result_multiple_line_comment[0].length-1
                        continue
                    

                    if char of @stop_char.opening
                        stack.push char
                    else if char of @stop_char.closing
                        if stack[stack.length-1] isnt @stop_char.closing[char]
                            throw @query_error_template
                                syntax_error: true
                                bracket: char
                                line: position.line
                                position: position.char
                        else
                            stack.pop()
                    else if char is ';' and stack.length is 0
                        queries.push query.slice start, i+1
                        start = i+1

            if start < query.length-1
                last_query = query.slice start
                if @regex.white.test(last_query) is false
                    queries.push last_query

            return queries

        # Clear the input
        clear_query: =>
            @codemirror.setValue ''
            @codemirror.focus()

        # Called if the driver could connect
        success_on_connect: (connection) =>
            @connection = connection

            @results_view.cursor_timed_out()
            if @reconnecting is true
                @.$('#user-alert-space').hide()
                @.$('#user-alert-space').html @alert_reconnection_success_template()
                @.$('#user-alert-space').slideDown 'fast'
            @reconnecting = false
            @driver_connected = 'connected'

        # Called if the driver could not connect
        error_on_connect: =>
            @results_view.cursor_timed_out()
            # We fail to connect, so we display a message except if we were already disconnected and we are not trying to manually reconnect
            # So if the user fails to reconnect after a failure, the alert will still flash
            @.$('#user-alert-space').hide()
            @.$('#user-alert-space').html @alert_connection_fail_template({})
            @.$('#user-alert-space').slideDown 'fast'
            @reconnecting = false
            @driver_connected = 'error'

        # Reconnect, function triggered if the user click on reconnect
        reconnect: (event) =>
            @reconnecting = true
            event.preventDefault()
            @driver_handler.connect()

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
            @$('.option_icon').removeClass 'fullscreen_exit'
            @$('.option_icon').addClass 'fullscreen'

        display_full: =>
            $('#cluster').removeClass 'container'
            $('#cluster').addClass 'cluster_with_margin'
            @.$('.wrapper_scrollbar').css 'width', ($(window).width()-92)+'px'
            @$('.option_icon').removeClass 'fullscreen'
            @$('.option_icon').addClass 'fullscreen_exit'

        destroy: =>
            @results_view.destroy()
            @history_view.destroy()
            @driver_handler.destroy()

            @display_normal()
            $(window).off 'resize', @display_full
            $(document).unbind 'mousemove', @handle_mousemove
            $(document).unbind 'mouseup', @handle_mouseup


            clearTimeout @timeout_driver_connect
            # We do not destroy the cursor, because the user might come back and use it.
    
    class @ResultView extends Backbone.View
        className: 'result_view'
        template: Handlebars.templates['dataexplorer_result_container-template']
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
        primitive_key: '_-primitive value-_' # We suppose that there is no key with such value in the database.
        events:
            # For Tree view
            'click .jt_arrow': 'toggle_collapse'
            # For Table view
            'mousedown .click_detector': 'handle_mousedown'
            'click .jta_arrow_h': 'expand_tree_in_table'
            'click .link_to_tree_view': 'show_tree'
            'click .link_to_table_view': 'show_table'
            'click .link_to_raw_view': 'show_raw'

        current_result: []

        initialize: (args) =>
            @container = args.container
            @set_limit args.limit
            @set_skip 0
            if args.view?
                @view = args.view
            else
                @view = 'tree'

            @last_keys = [] # Arrays of the last keys displayed
            @last_columns_size = {} # Size of the columns displayed. Undefined if a column has the default size

        set_limit: (limit) =>
            @limit = limit
        set_skip: (skip) =>
            @skip = skip

        show_tree: (event) =>
            event.preventDefault()
            @set_view 'tree'
        show_table: (event) =>
            event.preventDefault()
            @set_view 'table'
        show_raw: (event) =>
            event.preventDefault()
            @set_view 'raw'

        set_view: (view) =>
            @view = view
            @container.saved_data.view = view
            @render_result()

        render_error: (query, err) =>
            @.$el.html @error_template
                query: query
                error: err.toString().replace(/^(\s*)/, '')
            return @

        json_to_tree: (result) =>
            return @template_json_tree.container
                tree: @json_to_node(result)

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
                    map[@primitive_key] = Infinity

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
                    is_primitive: element[0] is @primitive_key
                    key: element[0]
                    col: col
 
            return @template_json_table.tr_attr
                attr: attr

        json_to_table_get_values: (result, keys_stored) =>
            document_list = []
            for element, i in result
                new_document = {}
                new_document.cells = []
                for key_container, col in keys_stored
                    key = key_container[0]
                    if key is @primitive_key
                        if jQuery.isPlainObject(element)
                            value = undefined
                        else
                            value = element
                    else
                        if element?
                            value = element[key]
                        else
                            value = undefined

                    new_document.cells.push @json_to_table_get_td_value value, col
                new_document.record = @container.saved_data.start_record + i
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
            @.$(event.target).remove()
            data = dom_element.data('json_data')
            result = @json_to_tree data
            dom_element.html result
            classname_to_change = dom_element.parent().attr('class').split(' ')[0]
            $('.'+classname_to_change).css 'max-width', 'none'
            classname_to_change = dom_element.parent().parent().attr('class')
            $('.'+classname_to_change).css 'max-width', 'none'
            dom_element.css 'max-width', 'none'
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

        mouse_down: false
        handle_mousedown: (event) =>
            if event?.target?.className is 'click_detector'
                @col_resizing = @$(event.target).parent().data('col')
                @start_width = @$(event.target).parent().width()
                @start_x = event.pageX
                @mouse_down = true
                $('body').toggleClass('resizing', true)

        handle_mousemove: (event) =>
            if @mouse_down
                @last_columns_size[@col_resizing] = Math.max 5, @start_width-@start_x+event.pageX # Save the personalized size
                @resize_column @col_resizing, @last_columns_size[@col_resizing] # Resize

        resize_column: (col, size) =>
            $('.col-'+col).css 'max-width', size
            $('.value-'+col).css 'max-width', size-20
            $('.col-'+col).css 'width', size
            $('.value-'+col).css 'width', size-20
            if size < 20
                $('.value-'+col).css 'padding-left', (size-5)+'px'
                $('.value-'+col).css 'visibility', 'hidden'
            else
                $('.value-'+col).css 'padding-left', '15px'
                $('.value-'+col).css 'visibility', 'visible'


        handle_mouseup: (event) =>
            if @mouse_down is true
                @mouse_down = false
                $('body').toggleClass('resizing', false)
                @set_scrollbar()

        default_size_column: 310 # max-width value of a cell of a table (as defined in the css file)

        render_result: (args) =>
            if args? and args.results isnt undefined
                @results = args.results
                @results_array = null # if @results is not an array (possible starting from 1.4), we will transform @results_array to [@results] for the table view
            if args?.metadata?
                @metadata = args.metadata
            if args?.metadata?.skip_value?
                # @container.saved_data.start_record is the old value of @container.saved_data.skip_value
                # Here we just deal with start_record
                @container.saved_data.start_record = args.metadata.skip_value

                if args.metadata.execution_time?
                    if args.metadata.execution_time < 1000
                        @metadata.execution_time_pretty = args.metadata.execution_time+"ms"
                    else if args.metadata.execution_time < 60*1000
                        @metadata.execution_time_pretty = (args.metadata.execution_time/1000).toFixed(2)+"s"
                    else # We do not expect query to last one hour.
                        minutes = Math.floor(args.metadata.execution_time/(60*1000))
                        @metadata.execution_time_pretty = minutes+"min "+((args.metadata.execution_time-minutes*60*1000)/1000).toFixed(2)+"s"

            num_results = @metadata.skip_value
            if @metadata.has_more_data isnt true
                if @results?.length?
                    num_results += @results.length
                else # @results can be a single value or null
                    num_results += 1

            @.$el.html @template _.extend @metadata,
                show_query_warning: args?.show_query_warning
                show_more_data: @metadata.has_more_data is true and @container.saved_data.cursor_timed_out is false
                cursor_timed_out_template: (@cursor_timed_out_template() if @metadata.has_more_data is true and @container.saved_data.cursor_timed_out is true)
                execution_time_pretty: @metadata.execution_time_pretty
                no_results: @metadata.has_more_data isnt true and @results?.length is 0 and @metadata.skip_value is 0
                num_results: num_results

            switch @view
                when 'tree'
                    @.$('.json_tree_container').html @json_to_tree @results
                    @$('.results').hide()
                    @$('.tree_view_container').show()
                    @.$('.link_to_tree_view').addClass 'active'
                    @.$('.link_to_tree_view').parent().addClass 'active'
                when 'table'
                    previous_keys = @last_keys # Save previous keys. @last_keys will be updated in @json_to_table
                    if Object.prototype.toString.call(@results) is '[object Array]'
                        @.$('.table_view').html @json_to_table @results
                    else
                        if not @results_array?
                            @results_array = []
                            @results_array.push @results
                        @.$('.table_view').html @json_to_table @results_array
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
                        for index in [0..@last_keys.length-1] # We skip the column record
                            real_size = 0
                            @$('.col-'+index).children().children().children().each((i, bloc) ->
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
                    @.$('.raw_view_textarea').html JSON.stringify @results
                    @$('.results').hide()
                    @$('.raw_view_container').show()
                    @expand_raw_textarea()
                    @.$('.link_to_raw_view').addClass 'active'
                    @.$('.link_to_raw_view').parent().addClass 'active'

            @set_scrollbar()
            @delegateEvents()
            return @
 
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
                        # We show the scrollbar and stick it to the bottom of the window because there ismore content below
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

            
        # Check if the cursor timed out. If yes, make sure that the user cannot fetch more results
        cursor_timed_out: =>
            @container.saved_data.cursor_timed_out = true
            if @container.saved_data.metadata?.has_more_data is true
                @$('.more_results_paragraph').html @cursor_timed_out_template()

        render: =>
            @delegateEvents()
            return @

        render_default: =>
            return @

        toggle_collapse: (event) =>
            @.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed')
            @.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed')
            @.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed')
            @.$(event.target).toggleClass('jt_arrow_hidden')
            @set_scrollbar()

        expand_raw_textarea: =>
            if $('.raw_view_textarea').length > 0
                height = $('.raw_view_textarea')[0].scrollHeight
                $('.raw_view_textarea').height(height)

        destroy: =>
            $(window).unbind 'scroll'
            $(window).unbind 'resize'

    class @HistoryView extends Backbone.View
        dataexplorer_history_template: Handlebars.templates['dataexplorer-history-template']
        dataexplorer_query_li_template: Handlebars.templates['dataexplorer-query_li-template']
        className: 'history'
        
        size_history_displayed: 300
        state: 'hidden' # hidden, visible
        index_displayed: 0

        events:
            'click .load_query': 'load_query'
            'mousedown .nano_border_bottom': 'start_resize'

        start_resize: (event) =>
            @start_y = event.pageY
            @start_height = @$('.nano').height()
            @mouse_down = true
            $('body').toggleClass('resizing', true)

        handle_mousemove: (event) =>
            if @mouse_down is true
                @height_history = Math.max 0, @start_height-@start_y+event.pageY
                @$('.nano').height @height_history

        handle_mouseup: (event) =>
            if @mouse_down is true
                @mouse_down = false
                $('.nano').nanoScroller({preventPageScrolling: true})
                $('body').toggleClass('resizing', false)

        initialize: (args) =>
            @container = args.container
            @history = args.history
            @height_history = 204

        render: =>
            @$el.html @dataexplorer_history_template()
            if @history.length is 0
                @$('.history_list').append @dataexplorer_query_li_template
                    no_query: true
                    displayed_class: 'displayed'
            else
                for query, i in @history
                    @$('.history_list').append @dataexplorer_query_li_template
                        query: query.query
                        broken_query: query.broken_query
                        id: i
                        num: i+1
            @delegateEvents()
            return @

        load_query: (event) =>
            id = @$(event.target).data().id
            # Set + save codemirror
            @container.codemirror.setValue @history[parseInt(id)].query
            @container.saved_data.current_query = @history[parseInt(id)].query

        add_query: (args) =>
            query = args.query
            broken_query = args.broken_query
            that = @
            is_at_bottom = @$('.history_list').height() is @$('.nano > .content').scrollTop()+@$('.nano').height()
            @$('.history_list').append @dataexplorer_query_li_template
                query: query
                broken_query: broken_query
                id: @history.length-1
                num: @history.length
            if @state is 'visible'
                if @$('.no_history').length isnt 0
                    @$('.no_history').slideUp 'fast', ->
                        $(@).remove()
                    that.resize
                        extra: -32
                        is_at_bottom: is_at_bottom
                else
                    @resize
                        is_at_bottom: is_at_bottom
            else
                if @$('.no_history').length isnt 0
                    @$('.no_history').remove()
 

        clear_history: (event) =>
            that = @
            event.preventDefault()
            @container.clear_history()
            @history = @container.history

            @$('.query_history').slideUp 'fast', ->
                $(@).remove()
            if @$('.no_history').length is 0
                @$('.history_list').append @dataexplorer_query_li_template
                    no_query: true
                    displayed_class: 'hidden'
                @$('.no_history').slideDown 'fast'
                if @state is 'visible'
                    @resize
                        size: 32

        open_close_history: (args) =>
            that = @
            if @state is 'visible'
                @state = 'hidden'
                @deactivate_overflow()
                @$('.nano').animate
                    height: 0
                    , 200
                    , ->
                        $('body').css 'overflow', 'auto'
                        $(@).css 'visibility', 'hidden'
                        $('.nano_border').hide() # In case the user trigger hide/show really fast
                        $('.arrow_history').hide() # In case the user trigger hide/show really fast
                @$('.nano_border').slideUp 'fast'
                @$('.arrow_history').slideUp 'fast'
            else
                @state = 'visible'
                @$('.arrow_history').show()
                @$('.nano_border').show()
                @resize(args)
                @$('.nano >.content').scrollTop $('.history_list').height()


        # args =
        #   size: absolute size
        #   extra: extra size
        # Note, we can define extra OR extra but not both
        resize: (args) =>
            if args?.size?
                size = args.size
            else
                if args?.extra?
                    size = Math.min @$('.history_list').height()+args.extra, @height_history
                else
                    size = Math.min @$('.history_list').height(), @height_history
            @$('.nano').css 'visibility', 'visible'
            if args?.no_animation is true
                @$('.nano').height size
                @$('.nano').css 'visibility', 'visible' # In case the user trigger hide/show really fast
                @$('.arrow_history').show() # In case the user trigger hide/show really fast
                @$('.nano_border').show() # In case the user trigger hide/show really fast
                @$('.nano').nanoScroller({preventPageScrolling: true})
            else
                @deactivate_overflow()
                duration = Math.max 150, size
                duration = Math.min duration, 250
                @$('.nano').stop(true, true).animate
                    height: size
                    , duration
                    , ->
                        $('body').css 'overflow', 'auto'
                        $(@).css 'visibility', 'visible' # In case the user trigger hide/show really fast
                        $('.arrow_history').show() # In case the user trigger hide/show really fast
                        $('.nano_border').show() # In case the user trigger hide/show really fast
                        $(@).nanoScroller({preventPageScrolling: true})
                        if args?.is_at_bottom is true
                            $('.nano >.content').animate
                                scrollTop: $('.history_list').height()
                                , 300

        # The 3 secrets of French cuisine is butter, butter and butter
        # We deactivate the scrollbar (if there isn't) while animating to have a smoother experience. We´ll put back the scrollbar once the animation is done.
        deactivate_overflow: =>
            if $(window).height() >= $(document).height()
                $('body').css 'overflow', 'hidden'

    class @DriverHandler
        # I don't want that thing in window
        constructor: (args) ->
            that = @

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

            @hack_driver()
            @connect()
        
        reset_count: =>
            @count = 0
            @done = 0

        # Hack the driver, remove .run() and private_run()
        hack_driver: =>
            if not TermBase.prototype.private_run?
                that = @
                TermBase.prototype.private_run = TermBase.prototype.run
                TermBase.prototype.run = ->
                    throw that.container.query_error_template
                        found_run: true

        connect: =>
            that = @
            # Whether we are going to reconnect or not, the cursor might have timed out.
            @container.saved_data.cursor_timed_out = true
            if @timeout?
                clearTimeout @timeout

            if @connection?
                if @driver_status is 'connected'
                    try
                        @connection.close()
                    catch err
                        # Nothing bad here, let's just not pollute the console
            try
                r.connect @server, (err, connection) ->
                    if err?
                        that.on_fail()
                    else
                        that.connection = connection
                        that.on_success(connection)
                @container.results_view.cursor_timed_out()
                @timeout = setTimeout @connect, 5*60*1000
            catch err
                @on_fail()
    
        postpone_reconnection: =>
            clearTimeout @timeout
            @timeout = setTimeout @connect, 5*60*1000

        destroy: =>
            try
                @connection.close()
            catch err
                # Nothing bad here, let's just not pollute the console
            clearTimeout @timeout

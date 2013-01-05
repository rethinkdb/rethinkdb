# Copyright 2010-2012 RethinkDB, all rights reserved.
# TODO: We changed so many times the behavior of the suggestions, the code is a mess. We should refactor soon.
module 'DataExplorerView', ->
    class @Container extends Backbone.View
        id: 'dataexplorer'
        template: Handlebars.templates['dataexplorer_view-template']
        description_template: Handlebars.templates['dataexplorer-description-template']
        template_suggestion_name: Handlebars.templates['dataexplorer_suggestion_name_li-template']
        description_with_example_template: Handlebars.templates['dataexplorer-description_with_example-template']
        alert_connection_fail_template: Handlebars.templates['alert-connection_fail-template']
        alert_reconnection_success_template: Handlebars.templates['alert-reconnection_success-template']
        databases_suggestions_template: Handlebars.templates['dataexplorer-databases_suggestions-template']
        namespaces_suggestions_template: Handlebars.templates['dataexplorer-namespaces_suggestions-template']
        reason_dataexplorer_broken_template: Handlebars.templates['dataexplorer-reason_broken-template']

        # That's all the thing we want to store so we can display the view as it was (when the user left the data explorer)
        saved_query: null # Last value @codemirror.getValue()
        last_query: null # Last executed query
        last_results: null # Last results
        last_cursor: null # Last cursor
        last_metadat: null # Last metadata
        cursor_timed_out: false # Whether the cursor timed out or not (ie. we reconnected)

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

        displaying_full_view: false # Boolean for the full view (true if full view)

        # Method to close an alert/warning/arror
        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        # Map function -> state
        map_state:
            '': ''
        # Descriptions
        descriptions: {}
        # Suggestions[state] = function for this state
        suggestions: {}
    
        # Method called on the content of reql_docs.json
        # Load the suggestions in @suggestions, @map_state, @descriptions
        set_docs: (data) =>
            # Create the suggestions table
            suggestions = {}
            for group in data['sections']
                for command in group['commands']
                    tag = command['langs']['js']['name']
                    if tag is '()'
                        continue
                    if tag is 'r' and command['langs']['js']['dont_need_parenthesis'] is true
                        full_tag = tag # Here full_tag is just the name of the tag
                        @descriptions[tag] =
                            name: tag
                            description: @description_with_example_template
                                description: command['description']
                                examples: command['langs']['js']['examples']
                    else
                        if tag is 'run'
                            full_tag = tag+'(' # full tag is the name plus a parenthesis (we will match the parenthesis too)
                            @descriptions[full_tag] =
                                name: tag
                                args: '( )'
                                description: @description_with_example_template
                                    description: command['description']
                                    examples: command['langs']['js']['examples']
                        else
                            full_tag = tag+'(' # full tag is the name plus a parenthesis (we will match the parenthesis too)
                            @descriptions[full_tag] =
                                name: tag
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
                    suggestions[parent].push full_tag #+something

                    @map_state[full_tag] = command['returns'] # We use full_tag because we need to differentiate between r. and r(

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

        # Define the height of a line (used for a line is too long)
        line_height: 13
        #TODO bind suggestions to keyup so we don't have an extra line when at the end of a line without a next char
        num_char_per_line: 106
        default_num_char_per_line: 106

        # We have to keep track of a lot of things because web-kit browsers handle the events keydown, keyup, blur etc... in a strange way.
        current_suggestions: []
        current_highlighted_suggestion: -1
        current_conpleted_query: ''
        query_first_part: ''
        query_last_part: ''

        show_or_hide_arrow: =>
            if @.$('.suggestion_name_list').css('display') is 'none' and @.$('.suggestion_description').css('display') is 'none'
                @.$('.arrow').hide()
            else
                @.$('.arrow').show()
    
        # Write the suggestion
        write_suggestion: (suggestion_to_write) =>
            @codemirror.setValue @query_first_part + @current_completed_query + suggestion_to_write + @query_last_part

        # Highlight a suggestion in case of a mouseover
        mouseover_suggestion: (event) =>
            @highlight_suggestion event.target.dataset.id

        # Hide suggestion in case of a mouse out
        mouseout_suggestion: (event) =>
            @hide_suggestion_description()

        # Highlight suggestion
        highlight_suggestion: (id) =>
            @.$('.suggestion_name_li').removeClass 'suggestion_name_li_hl'
            @.$('.suggestion_name_li').eq(id).addClass 'suggestion_name_li_hl'

            @.$('.suggestion_description').html @description_template @extend_description @current_suggestions[id]

            @show_suggestion_description()

        # Set cursor to position
        position_cursor: (position) =>
            @codemirror.setCursor position

        # Select the suggestion highlighted
        select_suggestion: (event) =>
            saved_cursor = @codemirror.getCursor()

            suggestion_to_write = @.$(event.target).html()
            @write_suggestion suggestion_to_write

            start_line_index = (@query_first_part + @current_completed_query).lastIndexOf('\n')
            if start_line_index is -1
                start_line_index = 0
            else
                start_line_index += 1
            
            ch = (@query_first_part + @current_completed_query + suggestion_to_write).length - start_line_index

            @cursor =
                line: saved_cursor.line
                ch: ch

            setTimeout @position_cursor_after_click, 1 # Ugliest hack ever.

        # Set the position of the cursor after a suggestion has been clicked
        position_cursor_after_click: =>
            @codemirror.focus()
            @position_cursor @cursor
            @handle_keypress()
        
        # Hide the suggestion
        hide_suggestion: =>
            @.$('.suggestion_name_list').css 'display', 'none'
            @hide_suggestion_description()
            @current_suggestions = []
            @show_or_hide_arrow()

        # Hide the description
        hide_suggestion_description: =>
            @.$('.suggestion_description').html ''
            @.$('.suggestion_description').css 'display', 'none'
            @show_or_hide_arrow()

        # Change the num_char_per_line value when we switch from normal view to full view and vice versa 
        set_char_per_line: =>
            if @displaying_full_view is true
                @num_char_per_line = Math.floor (@.$('.CodeMirror-scroll').width()-37)/8
            else
                @num_char_per_line = @default_num_char_per_line

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

        #TODO refactor show_suggestion, show_suggestion_description, add_description
        show_suggestion: =>
            margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'
            @.$('.suggestion_full_container').css 'margin-top', margin
            @.$('.arrow').css 'margin-top', margin
            @.$('.suggestion_name_list').css 'display', 'block'
            @move_suggestion()
            @show_or_hide_arrow()

        show_suggestion_description: =>
            margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'
            @.$('.suggestion_full_container').css 'margin-top', margin
            @.$('.arrow').css 'margin-top', margin
            @.$('.suggestion_description').css 'display', 'block'
            @show_or_hide_arrow()

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

        # Make suggestions when the user is writing
        handle_keypress: (editor, event) =>
            # Save the last query (even incomplete)
            DataExplorerView.Container.prototype.saved_query = @codemirror.getValue()
            saved_cursor = @codemirror.getCursor()
            if event?.which?
                # If the user hit tab, we switch the highlighted suggestion
                if event.which is 9
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return true
                    if event.shiftKey
                        @current_highlighted_suggestion--
                        if @current_highlighted_suggestion < 0
                            @current_highlighted_suggestion = @current_suggestions.length-1
                    else
                        @current_highlighted_suggestion++
                        if @current_highlighted_suggestion >= @current_suggestions.length
                            @current_highlighted_suggestion = 0

                    if @current_suggestions[@current_highlighted_suggestion]?
                        @highlight_suggestion @current_highlighted_suggestion
                        @write_suggestion @current_suggestions[@current_highlighted_suggestion]
                
                        start_line_index = (@query_first_part + @current_completed_query).lastIndexOf('\n')
                        if start_line_index is -1
                            start_line_index = 0
                        else
                            start_line_index += 1
                        position = (@query_first_part + @current_completed_query + @current_suggestions[@current_highlighted_suggestion]).length - start_line_index 
                        @position_cursor
                            line: saved_cursor.line
                            ch: position

                    if @current_suggestions.length is 0
                        query_lines = @codemirror.getValue().split '\n'

                        # Get query before the cursor
                        query_before_cursor = ''
                        if @codemirror.getCursor().line > 0
                            for i in [0..@codemirror.getCursor().line-1]
                                query_before_cursor += query_lines[i] + '\n'
                        query_before_cursor += query_lines[@codemirror.getCursor().line].slice 0, @codemirror.getCursor().ch
                        if query_before_cursor[query_before_cursor.length-1] isnt '('
                            return false

                    return true

                # If the user hit enter and (Ctrl or Shift)
                if event.which is 13 and (event.shiftKey or event.ctrlKey)
                    @hide_suggestion()
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return true
                    @show_or_hide_arrow()
                    @execute_query()
            
            # We just look at key up so we don't fire the call 3 times
            if event?.type? and event.type isnt 'keyup' or (event?.which? and event.which is 16) # We don't do anything for shift
                return false

            @current_highlighted_suggestion = -1
            @.$('.suggestion_name_list').html ''

            query_lines = @codemirror.getValue().split '\n'

            # Get query before the cursor
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


            # Check if we are in a string or in the middle of a word, if we are in a string, we just show the description
            if @is_in_string(query_before_cursor) is true
                @hide_suggestion()
                last_function_for_description = @extract_last_function_for_description(query_before_cursor)
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

            return false

        # Extract the last function to give a description, regardless of if we are in a string or not
        # Note: We are really not efficient... We can do way better parsing from left to right. Let's do it next time we refactor all this thing
        extract_last_function_for_description: (query) =>
            # query = query_before_cursor
            count_dot = 0
            num_not_open_parenthesis = 0

            for i in [query.length-1..0] by -1
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

        # Check if we are in a string (for the last position)
        is_in_string: (query) ->
            is_string = false
            char_used = ''

            for i in [query.length-1..0] by -1
                if is_string is false
                    if (query[i] is '"' or query[i] is '\'')
                        is_string = true
                        char_used = query[i]
                else if is_string is true
                    if query[i] is char_used
                        if query[i-1]? and query[i-1] is '\\'
                            continue
                        else
                            is_string = false
            return is_string

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
                DataExplorerView.Container.prototype.last_query = @query
                DataExplorerView.Container.prototype.last_results = @current_results
                @results_view.render_result null, @current_results # The first parameter null is the query because we don't want to display it.
                #TODO Refactor render_results since it's not really friendly as it is

                execution_time = new Date() - @start_time
                metadata =
                    limit_value: @current_results.length
                    skip_value: @skip_value
                    execution_time: execution_time
                    query: @query
                    has_more_data: (true if data?) # if data is undefined, it means that there is no more data
                DataExplorerView.Container.prototype.last_metadata = metadata # Let's save the metadata too.

                @results_view.render_metadata metadata

                if data isnt undefined #there is nore data
                    @skip_value += @current_results.length
                    @current_results = []
                    @current_results.push data
                return false

        # Function triggered when the user click on 'more results'
        show_more_results: (event) =>
            if @last_results isnt null
                @results_view.start_record += @last_results.length
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
                    DataExplorerView.Container.prototype.last_query = @query
                    DataExplorerView.Container.prototype.last_results = @current_results
                    @results_view.render_result null, @current_results # The first parameter is null ( = query, so we don't display it)

                    execution_time = new Date() - @start_time
                    metadata =
                        limit_value: @current_results.length
                        skip_value: @skip_value
                        execution_time: execution_time
                        query: @query
                        has_more_data: (true if data?) # if data is undefined, it means that there is no more data
                    DataExplorerView.Container.prototype.last_metadata = metadata # Saving the metadata

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
                    DataExplorerView.Container.prototype.last_cursor = @cursor # Saving the cursor so the user can call more results later
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
            @results_view.start_record = 1
            # The user just executed a query, so we reset cursor_timed_out to false
            DataExplorerView.Container.prototype.cursor_timed_out = false

            # Postpone the reconnection
            if window.timeout_driver_connect?
                clearTimeout timeout_driver_connect
            window.timeout_driver_connect = setTimeout driver_connect, 5*60*1000
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
                DataExplorerView.Container.prototype.last_cursor = @cursor
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

        # Reconnect, function triggered if the user click on reconnect
        reconnect: (event) =>
            @reconnecting = true
            event.preventDefault()
            clearTimeout window.timeout_driver_connect
            window.driver_connect()

        initialize: (options) =>
            if options?
                @options = options

            @limit = 40

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


            @input_query = new DataExplorerView.InputQuery
            @results_view = new DataExplorerView.ResultView @limit

            @render()

            $(window).on 'resize', @set_char_per_line

            # Connect the driver
            if window.driver_connected isnt true and window.r?
                window.driver_connect()

        render: =>
            @.$el.html @template
            @.$('.input_query_full_container').html @input_query.render().$el
            @.$('.results_container').html @results_view.render().$el
            @.$('.results_container').html @results_view.render_default().$el

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


            if @last_query? and @last_results? and @last_metadata?
                @results_view.render_result @last_query, @last_results
                @results_view.render_metadata @last_metadata

            # If driver not conneced
            if window.driver_connected is false
                @error_on_connect()

            return @

        # Create a code mirror instance
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
            if @saved_query?
                @codemirror.setValue @saved_query

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
                @set_char_per_line()
            else
                @display_full()
                $(window).on 'resize', @display_full
                @displaying_full_view = true
                @set_char_per_line()
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
            @input_query.destroy()
            @results_view.destroy()
            # We do not destroy the cursor, because the user might come back and use it.
    
    class @InputQuery extends Backbone.View
        className: 'query_control'
        template: Handlebars.templates['dataexplorer_input_query-template']
 
        render: =>
            @.$el.html @template()
            return @

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

        initialize: (limit) =>
            @set_limit limit
            @set_skip 0
            @view ='tree'
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
            @view = 'tree'
            @render_result()
        show_table: (event) =>
            event.preventDefault()
            @view = 'table'
            @render_result()
        show_raw: (event) =>
            event.preventDefault()
            @view = 'raw'
            @render_result()

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
        default_size_column: 310
        render_result: (query, result) =>
            if query?
                @query = query
            if result?
                @result = result
            @.$el.html @template
                query: @query

            switch @view
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
                    console.log 'extra_size_table: '+extra_size_table
                    if extra_size_table > 0 # The table doesn't take the full width
                        expandable_columns = []
                        for index in [0..@last_keys.length-2] # We skip the column record
                            real_size = 0
                            @$('.col-'+index).children().children().each((i, bloc) ->
                                $bloc = $(bloc)
                                if real_size<$bloc.width()
                                    real_size = $bloc.width()
                            )
                            console.log 'real_size:'+real_size
                            if real_size? and real_size is real_size and real_size > @default_size_column
                                expandable_columns.push
                                    col: index
                                    size: real_size+20 # 20 for padding
                        while expandable_columns.length > 0
                            console.log JSON.stringify expandable_columns
                            console.log 'extra_size_table: '+extra_size_table
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
                content_container = '.json_table_container'
            else if @view is 'tree'
                content_name = '.json_tree'
                content_container = '.tree_view'
            else if @view is 'raw'
                @$('.wrapper_scrollbar').hide()
                # There is no scrolbar with the raw view
                return

            # Set the floating scrollbar
            width_value = @$(content_name).width()
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

                # The scrollbar is floating (we just show one and not two). By default its position is fixed with botton: 0px;
                position_scrollbar = ->
                    if not $(content_container).offset()?
                        return ''
                    # Sometimes we don't have to display the scrollbar (when the results are not shown because the query is too big)
                    if $(window).scrollTop()+$(window).height() < $(content_container).offset().top+20 # bottom of the window < beginning of $('.json_table_container') // 20 pixels is the approximate height of the scrollbar (so we don't show JUST the scrollbar)
                        that.$('.wrapper_scrollbar').hide()
                    # We show the scrollbar and stick it to the bottom of the window because there is more content below
                    else if $(window).scrollTop()+$(window).height() < $(content_container).offset().top+$(content_container).height() # bottom of the window < end of $('.json_table_container')
                        that/$('.wrapper_scrollbar').show()
                        that.$('.wrapper_scrollbar').css 'margin-bottom', '0px'
                    # And sometimes we have to place it higher (when the user is at the bottom of the page)
                    else
                        # We can not use the original scrollbar and hide .wrapper_scrollbar (because we cannot scroll a hidden bloc)
                        that.$('.wrapper_scrollbar').show()
                        margin_bottom = $(window).scrollTop()+$(window).height()-($(content_container).offset().top+$(content_container).height())
                        that.$('.wrapper_scrollbar').css 'margin-bottom', margin_bottom+'px'

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
            if DataExplorerView.Container.prototype.cursor_timed_out is true
                @.$('.more_results_paragraph').html @cursor_timed_out_template()


        render: =>
            @delegateEvents()
            return @

        render_default: =>
            @.$el.html @default_template()

        toggle_collapse: (event) =>
            @.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed')
            @.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed')
            @.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed')
            @.$(event.target).toggleClass('jt_arrow_hidden')
            @set_scrollbar()

        handle_keypress: (event) =>
            if event.which is 13 and !event.shiftKey
                event.preventDefault()
                @.$('suggestion_name_list').css 'display', 'none'
                @.$(event.target).blur()


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

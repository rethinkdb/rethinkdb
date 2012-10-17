module 'DataExplorerView', ->
    class @Container extends Backbone.View
        id: 'dataexplorer'
        template: Handlebars.compile $('#dataexplorer_view-template').html()
        description_template: Handlebars.compile $('#dataexplorer-description-template').html()
        template_suggestion_name: Handlebars.compile $('#dataexplorer_suggestion_name_li-template').html()
        alert_connection_fail_template: Handlebars.compile $('#alert-connection_fail-template').html()
        alert_reconnection_success_template: Handlebars.compile $('#alert-reconnection_success-template').html()
        databases_suggestions_template: Handlebars.compile $('#dataexplorer-databases_suggestions-template').html()
        namespaces_suggestions_template: Handlebars.compile $('#dataexplorer-namespaces_suggestions-template').html()

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

            'click .link_to_tree_view': 'save_tab'
            'click .link_to_table_view': 'save_tab'
            'click .link_to_raw_view': 'save_tab'
            
        save_tab: (event) =>
            @results_view.set_view @.$(event.target).data('view')


        displaying_full_view: false # Boolean for the full view (true if full view)

        # Map function -> state
        map_state:
            '': ''
            'r': 'r'
            'db': 'db'
            'table': 'table'
            'get': 'view'
            'filter': 'stream'
            'length': 'value'
            'map': 'array'
            'slice': 'stream'
            'orderby': 'stream'
            'distinct': 'array'
            'reduce': 'stream'
            'pluck': 'stream'
            'extend': 'array'
            'expr': 'expr'
            'R': 'expr'
            'add': 'expr'
            'sub': 'expr'
            'mul': 'expr'
            'div': 'expr'
            'mod': 'expr'
            'eq': 'expr'
            'ne': 'expr'
            'lt': 'expr'
            'le': 'expr'
            'gt': 'expr'
            'ge': 'expr'
            'not': 'expr'
            'and': 'expr'
            'or': 'expr'
            'range': 'stream'

        # Descriptions of all the functions
        descriptions:
            'get(':
                name: 'get'
                args: '( id )'
                description: 'Get the document which primary key has the value id'
            'filter(':
                name: 'filter'
                args: '( predicate )'
                description: 'Filter a stream. A document passes the filter if the predicate returns true'
            'map(':
                name: 'map'
                args: '( attribute )'
                description: 'Map a stream to an array'
            'slice(':
                name: 'slice'
                args: '( start, end )'
                description: 'Returned a sliced array stating from start to end'
            'orderby(':
                name: 'orderby'
                args: '( expression )'
                description: 'order the stream with the expression'
            'distinct(':
                name: 'distinct'
                args: '( expression )'
                description: 'Formating text is not funny. I need a bunny. I need a hat first to make my bunny appear.'
            'pluck(':
                name: 'pluck'
                args: '( attribute )'
                description: 'Filter some attributes'
            'extend(':
                name: 'extend'
                args: '( object )'
                description: 'Extend something that I do not know'
            'pickAttrs(':
                name: 'pickAttrs'
                args: '( attribute_to pick... )'
                description: 'The VP debate is ridiculous. They should talk about serious issues...'
            'del(':
                name: 'del'
                args: '()'
                description: 'Delete all the rows returned by the query'
            'table(':
                name: 'table'
                args: '( table_name )'
                description: 'Select a table'
            'list(':
                name: 'list'
                args: '()'
                description: 'List all the data'
            'create(':
                name: 'create'
                args: '( table_name )'
                description: 'Create a new table'
            'drop(':
                name: 'drop'
                args: '()'
                description: 'Drop a table'
            'insert(':
                name: 'insert'
                args: '( document)'
                description:' Insert a new document'
            'dbCreate(':
                name: 'dbCreate'
                args: '( database_name )'
                description: 'Create a database'
            'db(': 
                name: 'db'
                args: '( database_name )'
                description: 'Select a database'
            'dbDrop(':
                name: 'dbDrop'
                args: '( database_name )'
                description: 'Drop a database'
            'dbList(': 
                name: 'dbList'
                args: '()'
                description: 'list the databases'
            'expr(':
                name: 'expr'
                args: '( expression )'
                description: 'Create an expression'
            'fn(':
                name: 'fn'
                args: '( argument..., body )'

                descrition: 'Create a function'
            'ifThenElse(':
                name: 'ifThenElse'
                args: '( expression, callback_true, callback_false)'
                description: 'In French we say si ou sinon for if then else'
            'let(':
                name: 'let'
                args: '( arguments..., body)'
                description: 'Create a variable'
            'length(':
                name: 'length'
                args: '()'
                description: 'Return the length of the array'
            'limit(':
                name: 'limit'
                args: '( number )'
                description: 'Limit the number of results'
            'r': 
                name: 'r'
                description: 'The main ReQL namespace.'
            'R(': 
                name: 'R'
                args: '( attribute_string )'
                description: 'Select an attribute'
            'add':
                name: 'add'
                args: '( expression )'
                description: 'Add a value'
            'sub(':
                name: 'sub'
                args: '( expression )'
                description: 'Sub a value'
            'mul(':
                name: 'mul'
                args: '( expression )'
                description: 'Multiply a value'
            'div(':
                name: 'div'
                args: '( expression )'
                description: 'Divide something by something and youhouuuuuUUUuuu'
            'mod(':
                name: 'mod'
                args: '( expression )'
                description: 'Mod by a value'
            'eq(':
                name: 'eq'
                args: '( expression )'
                description: 'Check for equality'
            'ne(':
                name: 'ne'
                args: '( expression )'
                description: 'It is not really cool to format stuff, I am bored -_-'
            'lt(':
                name: 'lt'
                args: '( expression )'
                description: 'Less than'
            'le(':
                name: 'le'
                args: '( expression )'
                description: 'Less than'
            'gt(':
                name: 'gt'
                args: '( expression )'
                description: 'Greater than'
            'ge(':
                name: 'ge'
                args: '( expression )'
                description: 'Greater or equal'
            'not(':
                name: 'not'
                args: '( expression )'
                description: 'Not'
            'and(':
                name: 'and'
                args: '( expression )'
                description: 'And'
            'or(':
                name: 'or'
                args: '( expression )'
                description: 'Or means Gold in French. That is cool no?'
            'run(':
                name: 'run'
                args: '()'
                description: 'Run the query'

        # Suggestions[state] = function for this state
        suggestions:
            stream: ['get(' ,'filter(', 'length(', 'map(', 'slice(', 'orderby(', 'distinct(', 'reduce(', 'pluck(', 'extend(', 'run(']
            view:['pickAttrs(', 'del(', 'run(']
            db:['table(', 'list(', 'create(', 'drop(', 'run(']
            table:['insert(', 'get(' ,'filter(', 'length(', 'map(', 'slice(', 'orderby(', 'distinct(', 'reduce(', 'pluck(', 'extend(', 'run(']
            r:['db(', 'dbCreate(', 'table(', 'dbDrop(', 'dbList(','expr(', 'fn(', 'ifThenElse(', 'let(', 'run(']
            array :['length(', 'limit(', 'run(']
            "" :['r', 'R(']
            expr: ['add(', 'sub(', 'mul(', 'div(', 'mod(', 'eq(', 'ne(', 'lt(', 'le(', 'gt(', 'ge(', 'not(', 'and(', 'or(', 'run(']

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
        hide_suggestion_description: ->
            @.$('.suggestion_description').html ''
            @.$('.suggestion_description').css 'display', 'none'
            @show_or_hide_arrow()

        # Change the num_char_per_line value when we switch from normal view to full view and vice versa 
        set_char_per_line: =>
            if @displaying_full_view is true
                @num_char_per_line = Math.floor (@.$('Codemirror-scroll').width()-37)/8
            else
                @num_char_per_line = @default_num_char_per_line

        # Compute the number of extra lines because of long lines
        compute_extra_lines: =>
            query_lines = @codemirror.getValue().split '\n'
            i = 0
            extra_lines = 0
            while i < query_lines.length-1
                extra_lines += Math.floor(query_lines[i].length/@num_char_per_line)
                i++
            extra_lines += Math.floor(@codemirror.getCursor().ch/@num_char_per_line)
            return extra_lines

        move_suggestion: =>
            margin_left = (@codemirror.getCursor().ch%@num_char_per_line)*8+27
            @.$('.arrow').css 'margin-left', margin_left

            if margin_left < 200
                @.$('.suggestion_full_container').css 'left', '0px'
            else if margin_left > 468
                @.$('.suggestion_full_container').css 'left', '440px'
            else
                margin_left = Math.min 468, Math.floor(margin_left/100)*100
                @.$('.suggestion_full_container').css 'left', (margin_left-100)+'px'

        #TODO refactor show_suggestion, show_suggestion_description, add_description
        show_suggestion: =>
            margin = ((@codemirror.getCursor().line+1+@compute_extra_lines())*@line_height) + 'px'
            @.$('.suggestion_full_container').css 'margin-top', margin
            @.$('.arrow').css 'margin-top', margin
            @.$('.suggestion_name_list').css 'display', 'block'
            @move_suggestion()
            @show_or_hide_arrow()

        show_suggestion_description: =>
            margin = ((@codemirror.getCursor().line+1+@compute_extra_lines())*@line_height) + 'px'
            @.$('.suggestion_full_container').css 'margin-top', margin
            @.$('.arrow').css 'margin-top', margin
            @.$('.suggestion_description').css 'display', 'block'
            @show_or_hide_arrow()

        # Extend description for db() and table() with a list of databases or namespaces
        extend_description: (fn) =>
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
                return {
                    db_found: false
                    error: false
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
                margin = ((@codemirror.getCursor().line+1+@compute_extra_lines())*@line_height) + 'px'
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
                        return false

                    return true

                # If the user hit enter and (Ctrl or Shift)
                if event.which is 13 and (event.shiftKey or event.ctrlKey)
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return true
                    @.$('suggestion_name_list').css 'display', 'none'
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
            if next_non_white_character? and next_non_white_character isnt '.' and next_non_white_character isnt ')' and next_non_white_character isnt ';'
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
            # Hack in case a new parenthesis is opened
            if last_function is ''
                last_function_full = @extract_last_function_for_description(query_before_cursor)
                if last_function_full isnt ''
                    last_function = last_function_full

            # Hack because last_function returns 'r' if the query is 'r'. and r isn't a function
            if last_function is query and last_function is 'r'
                last_function = null

            if @map_state[last_function]? and @suggestions[@map_state[last_function]]?
                if not @suggestions[@map_state[last_function]]? or @suggestions[@map_state[last_function]].length is 0
                    @hide_suggestion()
                    last_function_for_description = @extract_last_function_for_description(query_before_cursor)

                    if last_function_for_description isnt ''
                        @add_description last_function_for_description
                else
                    @append_suggestion(query, @suggestions[@map_state[last_function]])
            else
                @hide_suggestion()
                last_function_for_description = @extract_last_function_for_description(query_before_cursor)

                if last_function_for_description isnt ''
                    @add_description last_function_for_description

            return false

        # Extract the last function to give a description, regardless of if we are in a string or not
        extract_last_function_for_description: (query) =>
            # query = query_before_cursor
            count_dot = 0
            num_not_open_parenthesis = 0

            is_string = false
            char_used = ''
            for i in [query.length-1..0] by -1
                if is_string is false
                    if (query[i] is '"' or query[i] is '\'')
                        #is_string = true
                        char_used = query[i]
                    else if query[i] is '(' and num_not_open_parenthesis is 0
                        num_not_open_parenthesis--
                        end = i+1
                    else if query[i] is '(' and num_not_open_parenthesis isnt 0
                        return query.slice i+1, end
                    else if query[i] is ')'
                        return ''
                    else if query[i] is '.' and num_not_open_parenthesis is 0
                        return ''
                    else if query[i] is '.' and num_not_open_parenthesis isnt 0
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

        # Extract the last function of the current line, taking in account if we are in a string
        extract_last_function: (query) =>
            start = 0
            count_dot = 0
            num_not_open_parenthesis = 0

            is_string = false
            char_used = ''
            for i in [query.length-1..0] by -1
                if is_string is false
                    if (query[i] is '"' or query[i] is '\'')
                        is_string = true
                        char_used = query[i]
                    else if query[i] is '('
                        num_not_open_parenthesis--
                    else if query[i] is ')'
                        num_not_open_parenthesis++
                    else if query[i] is '.' and num_not_open_parenthesis <= 0
                        count_dot++
                        if count_dot is 2
                            start = i+1
                            break
                else if is_string is true
                    if query[i] is char_used
                        if query[i-1]? and query[i-1] is '\\'
                            continue
                        else
                            is_string = false

            dot_position = query.indexOf('.', start) 
            dot_position = query.length if dot_position is -1
            parenthesis_position = query.indexOf('(', start) 
            parenthesis_position = query.length if parenthesis_position is -1

            end = Math.min dot_position, parenthesis_position

            return query.slice(start, end).replace(/\s/g, '')



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
        append_suggestion: (query, suggestions) =>
            @hide_suggestion()
            splitdata = query.split('.')
            @current_completed_query = ''
            if splitdata.length>1
                for i in [0..splitdata.length-2]
                    @current_completed_query += splitdata[i]+'.'
            element_currently_written = splitdata[splitdata.length-1]


            for char in @unsafe_to_safe_regexstr
                element_currently_written = element_currently_written.replace char.pattern, char.replacement
            found_suggestion = false
            pattern = new RegExp('^('+element_currently_written+')', 'i')
            @current_suggestions = []
            for suggestion, i in suggestions
                if pattern.test(suggestion)
                    if splitdata[splitdata.length-1] is suggestion
                        continue # Skip the case when we have an exact match including parenthesis
                    found_suggestion = true
                    @current_suggestions.push suggestion
                    @.$('.suggestion_name_list').append @template_suggestion_name 
                        id: i
                        suggestion: suggestion

            if found_suggestion
                @show_suggestion()
            else
                @hide_suggestion()
            return @

        # Callback used by the cursor when the user hit 'more results'
        callback_query: (data) =>
            # If we get a run time error
            if data instanceof rethinkdb.errors.RuntimeError or data instanceof rethinkdb.errors.BadQuery or data instanceof rethinkdb.errors.ClientError or data instanceof rethinkdb.errors.ClientError
                @.$('.loading_query_img').css 'display', 'none'
                @.results_view.render_error(@query, data)
                return false
            
            # if it's a valid result and we have not reach the maximum of results displayed
            if data? and  @current_results.length < @limit
                @current_results.push data
                return true
            else
                # Else we are going to display what we have
                @.$('.loading_query_img').css 'display', 'none'
                @results_view.render_result @query, @current_results

                execution_time = new Date() - @start_time
                @results_view.render_metadata
                    limit_value: @current_results.length
                    skip_value: @skip_value
                    execution_time: execution_time
                    query: @query
                    has_more_data: true if data? # if data is undefined, it means that there is no more data

                if data? #there is nore data
                    @skip_value += @current_results.length
                    @current_results = []
                    @current_results.push data
                return false

        # Function triggered when the user click on 'more results'
        show_more_results: (event) =>
            try
                event.preventDefault()
                @cursor.next(@callback_query)
                $(window).scrollTop(@.$('.results_container').offset().top)
            catch err
                @.$('.loading_query_img').css 'display', 'none'
                @results_view.render_error(@query, err)

        # Create tagged callback so we just execute the last query executed by the user
        # Results:
        # User execute q1
        # User execute q2
        # q1 returns but no results is displayed
        # q2 returns, we display q2 results
        create_tagged_callback: =>
            id = Math.random() # That should be good enough for our application
            @last_id = id
            @current_results = []
            @skip_value = 0
            @current_query_index = 0
            
            # The callback with the id that we are going to return
            callback_multiple_queries = (data) =>
                if id is @last_id # We display things or fire the next query only if the user hasn't execute another query
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
                        if data? and  @current_results.length < @limit
                            @current_results.push data
                            return true
                        else
                            # Else we are going to display what we have
                            @.$('.loading_query_img').css 'display', 'none'
                            @results_view.render_result @query, @current_results

                            execution_time = new Date() - @start_time
                            @results_view.render_metadata
                                limit_value: @current_results.length
                                skip_value: @skip_value
                                execution_time: execution_time
                                query: @query
                                has_more_data: true if data? # if data is undefined, it means that there is no more data

                            if data? #there is nore data
                                @skip_value += @current_results.length
                                @current_results = []
                                @current_results.push data
                            return false
                    else #  Else if it's not the last query, we just execute the next query
                        try
                            if @cursor?
                                @cursor.close()

                            @cursor = eval(@queries[@current_query_index])
                            @cursor.next(callback_multiple_queries)
                        catch err
                            @.$('.loading_query_img').css 'display', 'none'
                            @results_view.render_error(@query, err)

                    return false
                else
                    @cursor.close()
                    return false
            return callback_multiple_queries
        # Function that execute the query
        execute_query: =>
            # Postpone the reconnection
            clearTimeout @timeout
            @timeout = setTimeout @connect, 5*60*1000
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

                tagged_callback = @create_tagged_callback()
                @cursor = eval(@queries[@current_query_index])
                @cursor.next(tagged_callback)

            catch err
                @.$('.loading_query_img').css 'display', 'none'
                @results_view.render_error(@query, err)

        # Separate the queries so we can execute them in an synchronous order
        separate_queries: (query) =>
            start = 0
            count_dot = 0

            is_string = false
            char_used = ''
            queries = []
            for i in [0..query.length-1]
                if is_string is false
                    if (query[i] is '"' or query[i] is '\'')
                        is_string = true
                        char_used = query[i]
                    else if query[i] is ';'
                        queries.push query.slice start, i
                        start = i+1
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

        # Connect to the server
        connect: (data) =>
            server =
                host: window.location.hostname
                port: parseInt window.location.port

            try
                that = @
                r.connect server
                if data? and data.reconnecting is true
                    @.$('#user-alert-space').html @alert_reconnection_success_template({})
            catch err
                @.$('#user-alert-space').css 'display', 'none'
                @.$('#user-alert-space').html @alert_connection_fail_template({})
                @.$('#user-alert-space').slideDown()
            if @timeout?
                clearTimeout @timeout
            @timeout = setTimeout @connect, 5*60*1000
        # Reconnect, function triggered if the user click on reconnect
        reconnect: (event) =>
            event.preventDefault()
            @connect
                reconnecting: true

        initialize: =>
            @timeout = setTimeout @connect, 5*60*1000
            window.r = rethinkdb
            window.R = r.R

            @connect()

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

        render: =>
            @.$el.html @template
            @.$('.input_query_full_container').html @input_query.render().$el
            @.$('.results_container').html @results_view.render().$el
            @.$('.results_container').html @results_view.render_default().$el
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
            if @displaying_full_view
                @display_normal()
                $(window).unbind 'resize', @display_full
                @displaying_full_view = false
                @set_char_per_line()
            else
                @display_full()
                $(window).bind 'resize', @display_full
                @displaying_full_view = true
                @set_char_per_line()

        display_normal: =>
            $('#cluster').addClass 'container'
            @.$('.json_table_container').css 'width', '888px'

        display_full: =>
            $('#cluster').removeClass 'container'
            @.$('.json_table_container').css 'width', ($(window).width()-52)+'px'

        destroy: =>
            @display_normal()
            @input_query.destroy()
            @results_view.destroy()
            try
                window.conn.close()
            catch err
                #console.log 'Could not destroy connection'
            if @cursor?
                @cursor.close()
            clearTimeout @timeout
    
    class @InputQuery extends Backbone.View
        className: 'query_control'
        template: Handlebars.compile $('#dataexplorer_input_query-template').html()
 
        render: =>
            @.$el.html @template()
            return @

    class @ResultView extends Backbone.View
        className: 'result_view'
        template: Handlebars.compile $('#dataexplorer_result_container-template').html()
        default_template: Handlebars.compile $('#dataexplorer_default_result_container-template').html()
        metadata_template: Handlebars.compile $('#dataexplorer-metadata-template').html()
        option_template: Handlebars.compile $('#dataexplorer-option_page-template').html()
        error_template: Handlebars.compile $('#dataexplorer-error-template').html()
        template_no_result: Handlebars.compile $('#dataexplorer_result_empty-template').html()
        template_json_tree:
            'container' : Handlebars.compile $('#dataexplorer_result_json_tree_container-template').html()
            'span': Handlebars.compile $('#dataexplorer_result_json_tree_span-template').html()
            'span_with_quotes': Handlebars.compile $('#dataexplorer_result_json_tree_span_with_quotes-template').html()
            'url': Handlebars.compile $('#dataexplorer_result_json_tree_url-template').html()
            'email': Handlebars.compile $('#dataexplorer_result_json_tree_email-template').html()
            'object': Handlebars.compile $('#dataexplorer_result_json_tree_object-template').html()
            'array': Handlebars.compile $('#dataexplorer_result_json_tree_array-template').html()

        template_json_table:
            'container' : Handlebars.compile $('#dataexplorer_result_json_table_container-template').html()
            'tr_attr': Handlebars.compile $('#dataexplorer_result_json_table_tr_attr-template').html()
            'td_attr': Handlebars.compile $('#dataexplorer_result_json_table_td_attr-template').html()
            'tr_value': Handlebars.compile $('#dataexplorer_result_json_table_tr_value-template').html()
            'td_value': Handlebars.compile $('#dataexplorer_result_json_table_td_value-template').html()
            'td_value_content': Handlebars.compile $('#dataexplorer_result_json_table_td_value_content-template').html()
            'data_inline': Handlebars.compile $('#dataexplorer_result_json_table_data_inline-template').html()

        events:
            # Global events
            'click .link_to_raw_view': 'expand_raw_textarea'
            # For Tree view
            'click .jt_arrow': 'toggle_collapse'
            # For Table view
            'mousedown td': 'handle_mousedown'
            'click .jta_arrow_v': 'expand_tree_in_table'
            'click .jta_arrow_h': 'expand_table_in_table'

        current_result: []

        initialize: (limit) =>
            @set_limit limit
            @set_skip 0
            @set_view 'tree'
            $(window).mousemove @handle_mousemove
            $(window).mouseup @handle_mouseup

        set_limit: (limit) =>
            @limit = limit
        set_skip: (skip) =>
            @skip = skip
        set_view: (view) =>
            @view = view

        render_error: (query, err) =>
            @.$el.html @error_template
                query: query
                error: err.toString()
                forgot_run: err.type? and err.type is 'undefined_method' and err['arguments']?[0]? and err['arguments'][0] is 'next' # Check if next is undefined, in which case the user probably forgot to append .run()
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

            return @template_json_table.container
                table_attr: @json_to_table_get_attr keys_sorted
                table_data: @json_to_table_get_values result, keys_sorted

        json_to_table_get_attr: (keys_sorted) ->
            attr = []
            for element, col in keys_sorted
                attr.push
                    key: element[0]
                    col: col
 
            return @template_json_table.tr_attr 
                attr: attr

        json_to_table_get_values: (result, keys_stored) ->
            document_list = []
            for element in result
                new_document = {}
                new_document.cells = []
                for key_container, col in keys_stored
                    key = key_container[0]
                    if key is '_anonymous object'
                        value = element
                    else
                        value = element[key]

                    new_document.cells.push @json_to_table_get_td_value value, col

                document_list.push new_document
            return @template_json_table.tr_value
                document: document_list

        json_to_table_get_td_value: (value, col) =>
            data = @compute_data_for_type(value, col)

            return @template_json_table.td_value
                class_td: 'col-'+col
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


        # Expand the table with new columns (with the attributes of the expanded object)
        expand_table_in_table: (event) ->
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

                $('.'+classcolumn) .remove();

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
        handle_mousedown: (event) ->
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
                # +30 for padding
                # Fix bug with Firefox (-30) or fix chrome bug...
                $('.col-'+@col_resizing).css 'max-width', @start_width-@start_x+event.pageX
                $('.value-'+@col_resizing).css 'max-width', @start_width-@start_x+event.pageX-20

                $('.col-'+@col_resizing).css 'width' ,@start_width-@start_x+event.pageX
                $('.value-'+@col_resizing).css 'width', @start_width-@start_x+event.pageX-20

        handle_mouseup: (event) =>
            @mouse_down = false
            @.$('.json_table').toggleClass('resizing', false)

        render_result: (query, result) =>
            @.$el.html @template
                query: query
            @.$('.tree_view').html @json_to_tree result
            @.$('.table_view').html @json_to_table result
            @.$('.raw_view_textarea').html JSON.stringify result
            @expand_raw_textarea()

            switch @view
                when 'tree'
                    @.$('.link_to_tree_view').tab 'show'
                when 'table'
                    @.$('.link_to_table_view').tab 'show'
                when 'raw'
                    @.$('.link_to_raw_view').tab 'show'
 
        # Render the metadata of an the results
        render_metadata: (data) =>
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
                execution_time: execution_time_pretty if execution_time_pretty?

            @.$('.metadata').html @metadata_template data

            #render pagination
            if has_more_data? and has_more_data is true
                @.$('.more_results').show()
            else
                @.$('.more_results').hide()
                
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

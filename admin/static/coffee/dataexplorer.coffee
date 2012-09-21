module 'DataExplorerView', ->
    class @Container extends Backbone.View
        className: 'dataexplorer_container'
        template: Handlebars.compile $('#dataexplorer_view-template').html()
        template_suggestion_name: Handlebars.compile $('#dataexplorer_suggestion_name_li-template').html()
        alert_connection_fail_template: Handlebars.compile $('#alert-connection_fail-template').html()
        alert_reconnection_success_template: Handlebars.compile $('#alert-reconnection_success-template').html()

        events:
            'click .CodeMirror': 'handle_keypress'
            'mousedown .suggestion_name_li': 'select_suggestion' # Keep mousedown to compete with blur on .input_query
            'mouseover .suggestion_name_li' : 'mouseover_suggestion'
            'mouseout .suggestion_name_li' : 'mouseout_suggestion'
            'click .clear_query': 'clear_query'
            'click .execute_query': 'execute_query'
            'click .namespace_link': 'write_query_namespace'
            'click .old_query': 'write_query_old'
            'click .change_size': 'toggle_size'
            'click #reconnect': 'reconnect'

        displaying_full_view: false
        has_been_initialized:
            value: false #We use that boolean to track if suggestions['stream'] has been append to suggestions['table']
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

        suggestions:
            stream: [
                {
                    suggestion: 'get()'
                    description: 'get( id )'
                    has_argument: true
                }
                {
                    suggestion: 'filter()'
                    description: 'filter( predicate )'
                    has_argument: true
                }
                {
                    suggestion: 'length()'
                    description: 'length()'
                    has_argument: false
                }
                {
                    suggestion: 'map()'
                    description: 'map( attribute )'
                    has_argument: true
                }
                {
                    suggestion: 'slice()'
                    description: 'slice( start, end )'
                    has_argument: true
                }
                {
                    suggestion: 'orderby()'
                    description: 'orderby( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'distinct()'
                    description: 'distinct( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'reduce()'
                    description: 'reduce( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'pluck()'
                    description: 'pluck( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'extend()'
                    description: 'extend( expression )'
                    has_argument: true
                }
            ]
            view:[
                {
                    suggestion: 'pickAttrs()'
                    description: 'pickAttrs( key )'
                    has_argument: true
                }
                {
                    suggestion: 'del()'
                    description: 'del()'
                    has_argument: false
                }
            ]
            db:[
                {
                    suggestion: 'table()'
                    description: 'table( table_name )'
                    has_argument: true
                }
                {
                    suggestion: 'list()'
                    description: 'list()'
                    has_argument: false
                }
                {
                    suggestion: 'create()'
                    description: 'create( database_name, primary_datacenter_id )'
                    has_argument: true
                }
                {
                    suggestion: 'drop()'
                    description: 'drop()'
                    has_argument: false
                }
            ]
            table:[
                {
                    suggestion: 'insert()'
                    description: 'insert( document )'
                    has_argument: true
                }
                
            ]
            r:[
                {
                    suggestion: 'dbCreate()'
                    description: 'dbCreate( database_name )'
                    has_argument: true
                }
                {
                    suggestion: 'dbDrop()'
                    description: 'dbDrop( database_name )'
                    has_argument: true
                }
                {
                    suggestion: 'dbList()'
                    description: 'dbList()'
                    has_argument: false
                }
                {
                    suggestion: 'expr()'
                    description:'expr( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'fn()'
                    description: 'fn( argument..., body )'
                    has_argument: true
                }
                {
                    suggestion: 'ifThenElse()'
                    description: 'ifThenElse( expression, callback_true, callback_false)'
                    has_argument: true
                }
                {
                    suggestion: 'let()'
                    description: 'let( arguments..., body)'
                    has_argument: true
                }

            ]
            array :[
                {
                    suggestion: 'length()'
                    description : 'Return the length of the array'
                    has_argument: false
                }
                {
                    suggestion: 'limit()'
                    description : 'limit( number )'
                    has_argument: true
                }
            ]
            "" :[
                {
                    suggestion: 'r'
                    description : 'The main ReQL namespace'
                    has_argument: false
                }
                {
                    suggestion: 'r()'
                    description : 'Variable'
                    has_argument: true
                }

                {
                    suggestion: 'R()'
                    description : 'Attribute Selector'
                    has_argument: true
                }
            ]
            expr: [
                {
                    suggestion: 'add()'
                    description: 'add( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'sub()'
                    description: 'sub( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'mul()'
                    description: 'mul( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'div()'
                    description: 'div( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'mod()'
                    description: 'mod( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'eq()'
                    description: 'eq( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'ne()'
                    description: 'ne( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'lt()'
                    description: 'lt( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'le()'
                    description: 'le( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'gt()'
                    description: 'gt( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'ge()'
                    description: 'ge( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'not()'
                    description: 'not( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'and()'
                    description: 'and( expression )'
                    has_argument: true
                }
                {
                    suggestion: 'or()'
                    description: 'or( expression )'
                    has_argument: true
                }

            ]

        # We have to keep track of a lot of things because web-kit browsers handle the events keydown, keyup, blur etc... in a strange way.
        current_suggestions: []
        current_highlighted_suggestion: -1
        current_conpleted_query: ''
        query_first_part: ''
        query_last_part: ''

        write_suggestion: (suggestion_to_write) =>
            @codemirror.setValue @query_first_part + @current_completed_query + suggestion_to_write + @query_last_part

        mouseover_suggestion: (event) =>
            @highlight_suggestion event.target.dataset.id

        mouseout_suggestion: (event) =>
            @hide_suggestion_description()

        highlight_suggestion: (id) =>
            @.$('.suggestion_name_li').removeClass 'suggestion_name_li_hl'
            @.$('.suggestion_name_li').eq(id).addClass 'suggestion_name_li_hl'

            @.$('.suggestion_description').html @current_suggestions[id].description
            @show_suggestion_description()


        position_cursor: (position) =>
            @codemirror.setCursor position

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
            if @.$(event.target).data('has_argument') is true
                ch--

            @cursor =
                line: saved_cursor.line
                ch: ch

            setTimeout @position_cursor_after_click, 1 # Ugliest hack ever.

        position_cursor_after_click: =>
            @codemirror.focus()
            @position_cursor @cursor
            @handle_keypress()


        hide_suggestion: =>
            @.$('.suggestion_name_list').css 'display', 'none'
            @hide_suggestion_description()

        hide_suggestion_description: ->
            @.$('.suggestion_description').html ''
            @.$('.suggestion_description').css 'display', 'none'

        show_suggestion: ->
            @.$('.suggestion_name_list').css 'display', 'block'

        show_suggestion_description: ->
            @.$('.suggestion_description').css 'display', 'block'


        expand_textarea: (event) =>
            if @.$('.input_query').length is 1
                @.$('.input_query').height 0
                height = @.$('.input_query').prop('scrollHeight') # We should have -8 but Firefox doesn't add padding in scrollHeight... Maybe we should start adding hacks...
                @.$('.input_query').css 'height', height if @.$('.input_query').height() isnt height

        # Make suggestions when the user is writing
        handle_keypress: (editor, event) =>
            saved_cursor = @codemirror.getCursor()
            if event?.which?
                if event.which is 9 # is tab
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
                        @write_suggestion @current_suggestions[@current_highlighted_suggestion].suggestion
                
                        start_line_index = (@query_first_part + @current_completed_query).lastIndexOf('\n')
                        if start_line_index is -1
                            start_line_index = 0
                        else
                            start_line_index += 1
                        position = (@query_first_part + @current_completed_query + @current_suggestions[@current_highlighted_suggestion].suggestion).length - start_line_index 
                        if @current_suggestions[@current_highlighted_suggestion].has_argument is true
                            position--
                        @position_cursor
                            line: saved_cursor.line
                            ch: position


                    if @current_suggestions.length is 0
                        return false

                    return true
                if event.which is 13 and (event.shiftKey or event.ctrlKey) #Ctrl or shift + enter
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return true
                    @.$('suggestion_name_list').css 'display', 'none'
                    @execute_query()
            
            if event?.type? and event.type isnt 'keyup'
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

            # Check if we are in a string
            if @is_in_string(query_before_cursor) is true
                @hide_suggestion()
                return ''

            slice_index = @extract_query_first_part query_before_cursor
            query = query_before_cursor.slice slice_index
            
            @query_first_part = query_before_cursor.slice 0, slice_index
            # We might want to use tab to move faster (like w in vim)
            @query_last_part = query_after_cursor #.slice slice_position

            last_function = @extract_last_function(query)
            if @map_state[last_function]? and @suggestions[@map_state[last_function]]?
                suggestions = []
                for suggestion in @suggestions[@map_state[last_function]]
                    suggestions.push suggestion
                if last_function is 'r'
                    for database in databases.models
                        suggestions.unshift
                            suggestion: 'db(\''+database.get('name')+'\')'
                            description: 'Select database '+database.get('name')
                else if last_function is 'db'
                    for namespace in namespaces.models
                        suggestions.unshift
                            suggestion: 'table(\''+namespace.get('name')+'\')'
                            description: 'Select table '+namespace.get('name')

                if suggestions.length is 0
                    @hide_suggestion()
                else
                    @append_suggestion(query, suggestions)
            else
                @hide_suggestion()

            return false
        
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

        # Extract the last function of the current line
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
                    else if query[i] is '('
                        count_opening_parenthesis++
                        if count_opening_parenthesis > 0
                            k = 0
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
                if pattern.test(suggestion.suggestion)
                    if splitdata[splitdata.length-1] is suggestion.suggestion
                        continue # Skip the case when we have an exact match including parenthesis
                    found_suggestion = true
                    @current_suggestions.push suggestion
                    @.$('.suggestion_name_list').append @template_suggestion_name 
                        id: i
                        has_argument: suggestion.has_argument
                        suggestion: suggestion.suggestion

            if found_suggestion
                @show_suggestion()
            else
                @hide_suggestion()
            return @

        create_tagged_callbacks: =>
            LIMIT = 20
            id = Math.random()
            @last_id = id
            iter_callback = (data) =>
                if id is @last_id
                    @count_results++
                    @current_results = []
                    if @count_results < LIMIT
                        @current_results.push data
                    else
                        if @count_results is LIMIT
                            @results_view.render_result @query, data
            last_callback = =>
                console.log 'last'
                if id is @last_id
                    execution_time = new Date() - @start_time
                    if @count_results < LIMIT
                        @results_view.render_result @query, data

                    @.$('.loading_query_img').css 'display', 'none'
                    @results_view.render_metadata @count_results, execution_time


            iter: iter_callback
            last: last_callback

        execute_query: =>
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

            @.$('.loading_query_img').css 'display', 'block'
            
            

            full_query = @query+'\n'+'.iter(iter_callback, last_callback)' # The new line is added in case the last one has an inline comment (//)
            try
                callbacks = @create_tagged_callbacks()
                iter_callback = callbacks.iter
                last_callback = callbacks.last
                @start_time = new Date()
                eval(full_query)
            catch err
                @.$('.loading_query_img').css 'display', 'none'
                @results_view.render_error(@query, err)
            
            # Display query in sidebar
            window.app.sidebar.add_query @codemirror.getValue()

        clear_query: =>
            @codemirror.setValue ''
            @codemirror.focus()


        # Write a query for the namespace clicked
        write_query_namespace: (event) =>
            event.preventDefault()
            query = 'r.db("'+event.target.dataset.database+'").table("'+event.target.dataset.name+'").filter()'
            @codemirror.setValue query
            @codemirror.focus()
            @codemirror.setCursor
                line: 0
                ch: Infinity
            @handle_keypress()

        # Write an old query in the input
        write_query_old: (event) =>
            event.preventDefault()
            @codemirror.setValue event.target.dataset.query
            @codemirror.focus()
            @codemirror.setCursor
                line: Infinity
                ch: Infinity
        connect: (data) =>
            host = window.location.hostname
            port = parseInt window.location.port
            try
                r.connect
                    host: host
                    port: port
                if data? and data.reconnecting is true
                    @.$('#user-alert-space').html @alert_reconnection_success_template({})
            catch err
                @.$('#user-alert-space').css 'display', 'none'
                @.$('#user-alert-space').html @alert_connection_fail_template({})
                @.$('#user-alert-space').slideDown()

        reconnect: (event) =>
            event.preventDefault()
            @connect
                reconnecting: true

        initialize: =>
            if @has_been_initialized.value is false
                for suggestion in @suggestions.stream
                    @suggestions.table.push suggestion
                @has_been_initialized.value = true
            
            @timeout = setTimeout @connect, 5*60*1000
            window.r = rethinkdb
            window.R = r.R

            @connect()

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

            @input_query = new DataExplorerView.InputQuery
            @results_view = new DataExplorerView.ResultView

            @render()

        render: =>
            @.$el.html @template
            @.$el.append @input_query.render().el
            @.$el.append @results_view.render().el
            return @

        call_codemirror: =>
            @codemirror = CodeMirror.fromTextArea document.getElementById('input_query'),
                mode:
                    name: 'javascript'
                    json: true
                onKeyEvent: @handle_keypress
                onBlur: @hide_suggestion
                lineNumbers: true
                lineWrapping: true
                matchBrackets: true

            @codemirror.setSize 698, 100

        toggle_size: =>
            if @displaying_full_view
                @display_normal()
                $(window).unbind 'resize', @display_full
                @displaying_full_view = false
            else
                @display_full()
                $(window).bind 'resize', @display_full
                @displaying_full_view = true

        display_normal: =>
            $('.main-container').width '940'
            $('#cluster').width 700
            @codemirror.setSize 698, 100
            #$('.input_query').width 678
            $('.dataexplorer_container').removeClass 'full_container'
            $('.dataexplorer_container').css 'margin', '0px'
            $('.change_size').val 'Full view'

        display_full: =>
            width = $(window).width() - 220 -40

            $('.main-container').width '100%'
            $('#cluster').width width
            @codemirror.setSize width-22, 100
            #$('.input_query').width width-45
            $('.dataexplorer_container').addClass 'full_container'
            $('.dataexplorer_container').css 'margin', '0px 0px 0px 20px'
            $('.change_size').val 'Smaller view'

        destroy: =>
            @display_normal()
            @input_query.destroy()
            @results_view.destroy()
            ###
            try
                window.conn.close()
            catch err
                #console.log 'Could not destroy connection'
            ###
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
        metadata_template: Handlebars.compile $('#dataexplorer-metadata-template').html()
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

        initialize: =>
            $(document).mousemove @handle_mousemove
            $(document).mouseup @handle_mouseup

        render_error: (query, err) =>
            @.$el.html @error_template 
                query: query
                error: err.toString()
            return @

        json_to_tree: (result) =>
            return @template_json_tree.container 
                tree: @json_to_node(result)

        #TODO catch RangeError: Maximum call stack size exceeded?
        #TODO what to do with new line?
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

        render_result: (query, result, id) =>
            @.$el.html @template
                query: query
            @.$('#tree_view').html @json_to_tree result
            @.$('#table_view').html @json_to_table result
            @.$('.raw_view_textarea').html JSON.stringify result
 
            @.$('.link_to_tree_view').tab 'show'
            @.$('#tree_view').addClass 'active'
            @.$('#table_view').removeClass 'active'
            @.$('#raw_view').removeClass 'active'

        render_metadata: (count_results, execution_time) =>
            LIMIT = 20
            if execution_time?
                if execution_time < 1000
                    execution_time_pretty = execution_time+"ms"
                else if execution_time < 60*1000
                    execution_time_pretty = (execution_time/1000).toFixed(2)+"s"
                else # We do not expect query to last one hour.
                    minutes = Math.floor(execution_time/(60*1000))
                    execution_time_pretty = minutes+"min "+((execution_time-minutes*60*1000)/1000).toFixed(2)+"s"       

            @.$('.metadata').html @metadata_template
                num_rows_displayed: LIMIT
                num_rows: count_results
                execution_time: execution_time if execution_time?


        render: =>
            return @

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

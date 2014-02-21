# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'DataExplorerView', ->
    @state =
        current_query: null
        query: null
        results: null
        profile: null
        cursor: null
        metadata: null
        cursor_timed_out: true
        view: 'tree'
        history_state: 'hidden'
        last_keys: []
        last_columns_size: {}
        options_state: 'hidden'
        options:
            suggestions: true
            electric_punctuation: false # False by default
            profiler: false
        history: []
        focus_on_codemirror: true
        last_query_has_profile: true
        #saved_data: {}



    class @Container extends Backbone.View
        id: 'dataexplorer'
        template: Handlebars.templates['dataexplorer_view-template']
        input_query_template: Handlebars.templates['dataexplorer_input_query-template']
        description_template: Handlebars.templates['dataexplorer-description-template']
        template_suggestion_name: Handlebars.templates['dataexplorer_suggestion_name_li-template']
        description_with_example_template: Handlebars.templates['dataexplorer-description_with_example-template']
        alert_connection_fail_template: Handlebars.templates['alert-connection_fail-template']
        databases_suggestions_template: Handlebars.templates['dataexplorer-databases_suggestions-template']
        namespaces_suggestions_template: Handlebars.templates['dataexplorer-namespaces_suggestions-template']
        reason_dataexplorer_broken_template: Handlebars.templates['dataexplorer-reason_broken-template']
        query_error_template: Handlebars.templates['dataexplorer-query_error-template']

        # Constants
        limit: 40 # How many results we display per page // Final for now
        line_height: 13 # Define the height of a line (used for a line is too long)
        size_history: 50
        
        max_size_stack: 100 # If the stack of the query (including function, string, object etc. is greater than @max_size_stack, we stop parsing the query
        max_size_query: 1000 # If the query is more than 1000 char, we don't show suggestion (codemirror doesn't highlight/parse if the query is more than 1000 characdd_ters too

        delay_show_abort: 70 # If a query didn't return during this period (ms) we let people abort the query

        events:
            'mouseup .CodeMirror': 'handle_click'
            'mousedown .suggestion_name_li': 'select_suggestion' # Keep mousedown to compete with blur on .input_query
            'mouseover .suggestion_name_li' : 'mouseover_suggestion'
            'mouseout .suggestion_name_li' : 'mouseout_suggestion'
            'click .clear_query': 'clear_query'
            'click .execute_query': 'execute_query'
            'click .abort_query': 'abort_query'
            'click .change_size': 'toggle_size'
            'click #reconnect': 'execute_query'
            'click .more_results': 'show_more_results'
            'click .close': 'close_alert'
            'click .clear_queries_link': 'clear_history_view'
            'click .close_queries_link': 'toggle_history'
            'click .toggle_options_link': 'toggle_options'
            'mousedown .nano_border_bottom': 'start_resize_history'
            # Let people click on the description and select text
            'mousedown .suggestion_description': 'mouse_down_description'
            'click .suggestion_description': 'stop_propagation'
            'mouseup .suggestion_description': 'mouse_up_description'
            'mousedown .suggestion_full_container': 'mouse_down_description'
            'click .suggestion_full_container': 'stop_propagation'
            'mousedown .CodeMirror': 'mouse_down_description'
            'click .CodeMirror': 'stop_propagation'

        mouse_down_description: (event) =>
            @keep_suggestions_on_blur = true
            @stop_propagation event

        stop_propagation: (event) =>
            event.stopPropagation()

        mouse_up_description: (event) =>
            @keep_suggestions_on_blur = false
            @stop_propagation event

        start_resize_history: (event) =>
            @history_view.start_resize event

        clear_history_view: (event) =>
            that = @
            @clear_history() # Delete from localstorage
            @history_view.history = @history

            @history_view.clear_history event

        # Method that make sure that just one button (history or option) is active
        # We give this button an "active" class that make it looks like it's pressed.
        toggle_pressed_buttons: =>
            if @history_view.state is 'visible'
                @state.history_state = 'visible'
                @$('.clear_queries_link').fadeIn 'fast'
                @$('.close_queries_link').addClass 'active'
            else
                @state.history_state = 'hidden'
                @$('.clear_queries_link').fadeOut 'fast'
                @$('.close_queries_link').removeClass 'active'

            if @options_view.state is 'visible'
                @state.options_state = 'visible'
                @$('.toggle_options_link').addClass 'active'
            else
                @state.options_state = 'hidden'
                @$('.toggle_options_link').removeClass 'active'

        # Show/hide the history view
        toggle_history: (args) =>
            that = @

            @deactivate_overflow()
            if args.no_animation is true
                # We just show the history
                @history_view.state = 'visible'
                @$('.content').html @history_view.render(true).$el
                @move_arrow
                    type: 'history'
                    move_arrow: 'show'
                @adjust_collapsible_panel_height
                    no_animation: true
                    is_at_bottom: true

            else if @options_view.state is 'visible'
                @options_view.state = 'hidden'
                @move_arrow
                    type: 'history'
                    move_arrow: 'animate'
                @options_view.$el.fadeOut 'fast', ->
                    that.$('.content').html that.history_view.render(false).$el
                    that.history_view.state = 'visible'
                    that.history_view.$el.fadeIn 'fast'
                    that.adjust_collapsible_panel_height
                        is_at_bottom: true
                    that.toggle_pressed_buttons() # Re-execute toggle_pressed_buttons because we delay the fadeIn
            else if @history_view.state is 'hidden'
                @history_view.state = 'visible'
                @$('.content').html @history_view.render(true).$el
                @history_view.delegateEvents()
                @move_arrow
                    type: 'history'
                    move_arrow: 'show'
                @adjust_collapsible_panel_height
                    is_at_bottom: true
            else if @history_view.state is 'visible'
                @history_view.state = 'hidden'
                @hide_collapsible_panel 'history'

            @toggle_pressed_buttons()

        # Show/hide the options view
        toggle_options: (args) =>
            that = @

            @deactivate_overflow()
            if args?.no_animation is true
                @options_view.state = 'visible'
                @$('.content').html @options_view.render(true).$el
                @options_view.delegateEvents()
                @move_arrow
                    type: 'options'
                    move_arrow: 'show'
                @adjust_collapsible_panel_height
                    no_animation: true
                    is_at_bottom: true
            else if @history_view.state is 'visible'
                @history_view.state = 'hidden'
                @move_arrow
                    type: 'options'
                    move_arrow: 'animate'
                @history_view.$el.fadeOut 'fast', ->
                    that.$('.content').html that.options_view.render(false).$el
                    that.options_view.state = 'visible'
                    that.options_view.$el.fadeIn 'fast'
                    that.adjust_collapsible_panel_height()
                    that.toggle_pressed_buttons()
                    that.$('.profiler_enabled').css 'visibility', 'hidden'
                    that.$('.profiler_enabled').hide()
            else if @options_view.state is 'hidden'
                @options_view.state = 'visible'
                @$('.content').html @options_view.render(true).$el
                @options_view.delegateEvents()
                @move_arrow
                    type: 'options'
                    move_arrow: 'show'
                @adjust_collapsible_panel_height
                    cb: args?.cb
            else if @options_view.state is 'visible'
                @options_view.state = 'hidden'
                @hide_collapsible_panel 'options'

            @toggle_pressed_buttons()

        # Hide the collapsible_panel whether it contains the option or history view
        hide_collapsible_panel: (type) =>
            that = @
            @deactivate_overflow()
            @$('.nano').animate
                height: 0
                , 200
                , ->
                    that.activate_overflow()
                    # We don't want to hide the view if the user changed the state of the view while it was being animated
                    if (type is 'history' and that.history_view.state is 'hidden') or (type is 'options' and that.options_view.state is 'hidden')
                        that.$('.nano_border').hide() # In case the user trigger hide/show really fast
                        that.$('.arrow_dataexplorer').hide() # In case the user trigger hide/show really fast
                        that.$(@).css 'visibility', 'hidden'
            @$('.nano_border').slideUp 'fast'
            @$('.arrow_dataexplorer').slideUp 'fast'

        # Move the arrow that points to the active button (on top of the collapsible panel). In case the user switch from options to history (or the opposite), we need to animate it
        move_arrow: (args) =>
            # args =
            #   type: 'options'/'history'
            #   move_arrow: 'show'/'animate'
            if args.type is 'options'
                margin_right = 74
            else if args.type is 'history'
                margin_right = 154

            if args.move_arrow is 'show'
                @$('.arrow_dataexplorer').css 'margin-right', margin_right
                @$('.arrow_dataexplorer').show()
            else if args.move_arrow is 'animate'
                @$('.arrow_dataexplorer').animate
                    'margin-right': margin_right
                    , 200
            @$('.nano_border').show()

        # Adjust the height of the container of the history/option view
        # Arguments:
        #   size: size of the collapsible panel we want // If not specified, we are going to try to figure it out ourselves
        #   no_animation: boolean (do we need to animate things or just to show it)
        #   is_at_bottom: boolean (if we were at the bottom, we want to scroll down once we have added elements in)
        #   delay_scroll: boolean, true if we just added a query - It speficied if we adjust the height then scroll or do both at the same time 
        adjust_collapsible_panel_height: (args) =>
            that = @
            if args?.size?
                size = args.size
            else
                if args?.extra?
                    size = Math.min @$('.content > div').height()+args.extra, @history_view.height_history
                else
                    size = Math.min @$('.content > div').height(), @history_view.height_history

            @deactivate_overflow()

            duration = Math.max 150, size
            duration = Math.min duration, 250
            #@$('.nano').stop(true, true).animate
            @$('.nano').css 'visibility', 'visible' # In case the user trigger hide/show really fast
            if args?.no_animation is true
                @$('.nano').height size
                @$('.nano > .content').scrollTop @$('.nano > .content > div').height()
                @$('.nano').css 'visibility', 'visible' # In case the user trigger hide/show really fast
                @$('.arrow_dataexplorer').show() # In case the user trigger hide/show really fast
                @$('.nano_border').show() # In case the user trigger hide/show really fast
                if args?.no_animation is true
                    @$('.nano').nanoScroller({preventPageScrolling: true})
                @activate_overflow()

            else
                @$('.nano').animate
                    height: size
                    , duration
                    , ->
                        that.$(@).css 'visibility', 'visible' # In case the user trigger hide/show really fast
                        that.$('.arrow_dataexplorer').show() # In case the user trigger hide/show really fast
                        that.$('.nano_border').show() # In case the user trigger hide/show really fast
                        that.$(@).nanoScroller({preventPageScrolling: true})
                        that.activate_overflow()
                        if args? and args.delay_scroll is true and args.is_at_bottom is true
                            that.$('.nano > .content').animate
                                scrollTop: that.$('.nano > .content > div').height()
                                , 200
                        if args?.cb?
                            args.cb()

                if args? and args.delay_scroll isnt true and args.is_at_bottom is true
                    that.$('.nano > .content').animate
                        scrollTop: that.$('.nano > .content > div').height()
                        , 200



        # We deactivate the scrollbar (if there isn't) while animating to have a smoother experience. We´ll put back the scrollbar once the animation is done.
        deactivate_overflow: =>
            if $(window).height() >= $(document).height()
                $('body').css 'overflow', 'hidden'

        activate_overflow: =>
            $('body').css 'overflow', 'auto'


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

        types:
            value: ['number', 'bool', 'string', 'array', 'object', 'time']
            any: ['number', 'bool', 'string', 'array', 'object', 'stream', 'selection', 'table', 'db', 'r', 'error' ]
            sequence: ['table', 'selection', 'stream', 'array']

        # Convert meta types (value, any or sequence) to an array of types or return an array composed of just the type
        convert_type: (type) =>
            if @types[type]?
                return @types[type]
            else
                return [type]

        # Flatten an array
        expand_types: (ar) =>
            result = []
            if _.isArray(ar)
                for element in ar
                    result.concat @convert_type element
            else
                result.concat @convert_type element
            return result

        # Once we are done moving the doc, we could generate a .js in the makefile file with the data so we don't have to do an ajax request+all this stuff
        set_doc_description: (command, tag, suggestions) =>
            if command['body']?
                dont_need_parenthesis = not (new RegExp(tag+'\\(')).test(command['body'])
                if dont_need_parenthesis
                    full_tag = tag # Here full_tag is just the name of the tag
                else
                    full_tag = tag+'(' # full tag is the name plus a parenthesis (we will match the parenthesis too)

                @descriptions[full_tag] =
                    name: tag
                    args: /.*(\(.*\))/.exec(command['body'])?[1]
                    description: @description_with_example_template
                        description: command['description']
                        example: command['example']

            parents = {}
            returns = []
            for pair in command.io
                parent_values = if (pair[0] == null) then '' else pair[0]
                return_values = pair[1]

                parent_values = @convert_type parent_values
                return_values = @convert_type return_values
                returns = returns.concat return_values
                for parent_value in parent_values
                    parents[parent_value] = true
                
            if full_tag isnt '('
                for parent_value of parents
                    if not suggestions[parent_value]?
                        suggestions[parent_value] = []
                    suggestions[parent_value].push full_tag

            @map_state[full_tag] = returns # We use full_tag because we need to differentiate between r. and r(


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
            for key of data
                command = data[key]
                tag = command['name']
                if tag of @ignored_commands
                    continue
                if tag is '()' # The parentheses will be added later
                    tag = ''
                @set_doc_description command, tag, @suggestions

            relations = data['types']

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
                if @state.history.length is 0 or @state.history[@state.history.length-1].query isnt query and @regex.white.test(query) is false
                    @state.history.push
                        query: query
                        broken_query: broken_query
                    if @state.history.length>@size_history
                        window.localStorage.rethinkdb_history = JSON.stringify @state.history.slice @state.history.length-@size_history
                    else
                        window.localStorage.rethinkdb_history = JSON.stringify @state.history
                    @history_view.add_query
                        query: query
                        broken_query: broken_query

        clear_history: =>
            @state.history.length = 0
            window.localStorage.rethinkdb_history = JSON.stringify @state.history

        initialize: (args) =>
            @TermBaseConstructor = r.expr(1).constructor.__super__.constructor.__super__.constructor

            @state = args.state
            @executing = false

            # Load options from local storage
            if window.localStorage?.options?
                @state.options = JSON.parse window.localStorage.options

            if window.localStorage?.rethinkdb_history?
                @state.history = JSON.parse window.localStorage.rethinkdb_history

            @show_query_warning = @state.query isnt @state.current_query # Show a warning in case the displayed results are not the one of the query in code mirror
            @current_results = @state.results
            @profile = @state.profile

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
                view: @state.view

            @options_view = new DataExplorerView.OptionsView
                container: @
                options: @state.options

            @history_view = new DataExplorerView.HistoryView
                container: @
                history: @state.history

            @driver_handler = new DataExplorerView.DriverHandler
                container: @

            # One callback to rule them all
            $(window).mousemove @handle_mousemove
            $(window).mouseup @handle_mouseup
            $(window).mousedown @handle_mousedown
            @id_execution = 0
            @keep_suggestions_on_blur = false

            @render()

        handle_mousemove: (event) =>
            @results_view.handle_mousemove event
            @history_view.handle_mousemove event

        handle_mouseup: (event) =>
            @results_view.handle_mouseup event
            @history_view.handle_mouseup event

        handle_mousedown: (event) =>
            # $(window) caught a mousedown event, so it wasn't caught by $('.suggestion_description')
            # Let's hide the suggestion/description
            @keep_suggestions_on_blur = false
            @hide_suggestion_and_description()

        render: =>
            @$el.html @template()
            @$('.input_query_full_container').html @input_query_template()
            
            # Check if the browser supports the JavaScript driver
            # We do not support internet explorer (even IE 10) and old browsers.
            if navigator?.appName is 'Microsoft Internet Explorer'
                @$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                    is_internet_explorer: true
                @$('.reason_dataexplorer_broken').slideDown 'fast'
                @$('.button_query').prop 'disabled', true
            else if (not DataView?) or (not Uint8Array?) # The main two components that the javascript driver requires.
                @$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                @$('.reason_dataexplorer_broken').slideDown 'fast'
                @$('.button_query').prop 'disabled', true
            else if not window.r? # In case the javascript driver is not found (if build from source for example)
                @$('.reason_dataexplorer_broken').html @reason_dataexplorer_broken_template
                    no_driver: true
                @$('.reason_dataexplorer_broken').slideDown 'fast'
                @$('.button_query').prop 'disabled', true

            # Let's bring back the data explorer to its old state (if there was)
            if @state?.query? and @state?.results? and @state?.metadata?
                @$('.results_container').html @results_view.render_result({
                    show_query_warning: @show_query_warning
                    results: @state.results
                    metadata: @state.metadata
                    profile: @state.profile
                }).$el
                # The query in code mirror is set in init_after_dom_rendered (because we can't set it now)
            else
                @$('.results_container').html @results_view.render_default().$el

            # If driver not conneced
            if @driver_connected is false
                @error_on_connect()
    
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
            if @state.current_query?
                @codemirror.setValue @state.current_query
            @codemirror.focus() # Give focus
            
            # Track if the focus is on codemirror
            # We use it to refresh the docs once the reql_docs.json is loaded
            @state.focus_on_codemirror = true

            @codemirror.setCursor @codemirror.lineCount(), 0
            if @codemirror.getValue() is '' # We show suggestion for an empty query only
                @handle_keypress()
            @results_view.expand_raw_textarea()

            @draft = @codemirror.getValue()

            if @state.history_state is 'visible' # If the history was visible, we show it
                @toggle_history
                    no_animation: true

            if @state.options_state is 'visible' # If the history was visible, we show it
                @toggle_options
                    no_animation: true

        on_blur: =>
            @state.focus_on_codemirror = false
            # We hide the description only if the user isn't selecting text from a description.
            if @keep_suggestions_on_blur is false
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

                if event.type isnt 'keypress' # We catch keypress because single and double quotes have not the same keyCode on keydown/keypres #thisIsMadness
                    return true

                char_to_insert = String.fromCharCode event.which
                if char_to_insert? # and event.which isnt 91 # 91 map to [ on OS X
                    if @codemirror.getSelection() isnt ''
                        if (char_to_insert of @matching_opening_bracket or char_to_insert of @matching_closing_bracket)
                            @codemirror.replaceSelection ''
                        else
                            return true
                    last_element_incomplete_type = @last_element_type_if_incomplete(stack)
                    if char_to_insert is '"' or char_to_insert is "'"
                        num_quote = @count_char char_to_insert
                        next_char = @get_next_char()
                        if next_char is char_to_insert # Next char is a single quote
                            if num_quote%2 is 0
                                if last_element_incomplete_type is 'string' or last_element_incomplete_type is 'object_key' # We are at the end of a string and the user just wrote a quote 
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
                                last_key = @get_last_key(stack)
                                if last_element_incomplete_type is 'string'
                                    return true
                                else if last_element_incomplete_type is 'object_key' and (last_key isnt '' and @create_safe_regex(char_to_insert).test(last_key) is true) # A key in an object can be seen as a string
                                    return true
                                else
                                    @insert_next char_to_insert
                            else # Else we'll just insert one quote
                                return true
                    else if last_element_incomplete_type isnt 'string'
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
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        to_skip = result_multiple_line_comment[0].length-1
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
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        to_skip = result_multiple_line_comment[0].length-1
                        continue

            return result


        # Handle events on codemirror
        # Return true if we want code mirror to ignore the event
        handle_keypress: (editor, event) =>
            if @ignored_next_keyup is true
                if event?.type is 'keyup' and event?.which isnt 9
                    @ignored_next_keyup = false
                return true

            @state.focus_on_codemirror = true

            # Let's hide the tooltip if the user just clicked on the textarea. We'll only display later the suggestions if there are (no description)
            if event?.type is 'mouseup'
                @hide_suggestion_and_description()

            # Save the last query (even incomplete)
            @state.current_query = @codemirror.getValue()

            # Look for special commands
            if event?.which?
                if event.type isnt 'keydown' and ((event.ctrlKey is true or event.metaKey is true) and event.which is 32)
                    # Because on event.type == 'keydown' we are going to change the state (hidden or displayed) of @$('.suggestion_description') and @.$('.suggestion_name_list'), we don't want to fire this event a second time
                    return true

                if event.which is 27 or (event.type is 'keydown' and ((event.ctrlKey is true or event.metaKey is true) and event.which is 32) and (@$('.suggestion_description').css('display') isnt 'none' or @.$('.suggestion_name_list').css('display') isnt 'none'))
                    # We caugh ESC or (Ctrl/Cmd+space with suggestion/description being displayed)
                    event.preventDefault() # Keep focus on code mirror
                    @hide_suggestion_and_description()
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

                    @current_highlighted_suggestion = -1
                    @current_highlighted_extra_suggestion = -1
                    @.$('.suggestion_name_list').empty()

                    # Valid step, let's save the data
                    @query_last_part = query_after_cursor

                    @current_suggestions = []
                    @current_element = ''
                    @current_extra_suggestion = ''
                    @written_suggestion = null
                    @cursor_for_auto_completion = @codemirror.getCursor()
                    @description = null

                    result =
                        status: null
                        # create_suggestion is going to fill to_complete and to_describe
                        #to_complete: undefined
                        #to_describe: undefined
                        
                    # Create the suggestion/description
                    @create_suggestion
                        stack: stack
                        query: query_before_cursor
                        result: result
                    result.suggestions = @uniq result.suggestions

                    if result.suggestions?.length > 0
                        for suggestion, i in result.suggestions
                            result.suggestions.sort() # We could eventually sort things earlier with a merge sort but for now that should be enough
                            @current_suggestions.push suggestion
                            @.$('.suggestion_name_list').append @template_suggestion_name
                                id: i
                                suggestion: suggestion
                    else if result.description?
                        @description = result.description

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
                else if event.which is 9  or (event.type is 'keydown' and ((event.ctrlKey is true or event.metaKey is true) and event.which is 32) and (@$('.suggestion_description').css('display') is 'none' and @.$('.suggestion_name_list').css('display') is 'none'))
                    # If the user just hit tab, we are going to show the suggestions if they are hidden
                    # or if they suggestions are already shown, we are going to cycle through them.
                    #
                    # If the user just hit Ctrl/Cmd+space with suggestion/description being hidden we show the suggestions
                    # Note that the user cannot cycle through suggestions because we make sure in the IF condition that suggestion/description are hidden
                    # If the suggestions/description are visible, the event will be caught earlier with ESC
                    event.preventDefault()
                    if event.type isnt 'keydown'
                        return false
                    else
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
                        else if @description?
                            if @$('.suggestion_description').css('display') is 'none'
                                # We show it once only because we don't want to move the cursor around
                                @show_description()
                                return true

                            if @extra_suggestions? and @extra_suggestions.length > 0 and @extra_suggestion.start_body is @extra_suggestion.start_body
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
                                        if @state.options.electric_punctuation is false
                                            move_outside = true
                                        if /^\s*'/.test(@current_extra_suggestion) is true
                                            string_delimiter = "'"
                                        else if /^\s*"/.test(@current_extra_suggestion) is true
                                            string_delimiter = '"'
                                        else
                                            string_delimiter = "'"
                                            move_outside = true
                                        suggestion = string_delimiter+@extra_suggestions[@current_highlighted_extra_suggestion]+string_delimiter
                                    
                                    @write_suggestion
                                        move_outside: move_outside
                                        suggestion_to_write: suggestion
                                    @ignore_tab_keyup = true # If we are switching suggestion, we don't want to do anything else related to tab

                # If the user hit enter and (Ctrl or Shift)
                if event.which is 13 and (event.shiftKey or event.ctrlKey or event.metaKey)
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
                    if @history_displayed_id < @state.history.length
                        @history_displayed_id++
                        @codemirror.setValue @state.history[@state.history.length-@history_displayed_id].query
                        event.preventDefault()
                        return true
                else if event.type is 'keyup' and event.altKey and event.which is 40 # Key down
                    if @history_displayed_id > 1
                        @history_displayed_id--
                        @codemirror.setValue @state.history[@state.history.length-@history_displayed_id].query
                        event.preventDefault()
                        return true
                    else if @history_displayed_id is 1
                        @history_displayed_id--
                        @codemirror.setValue @draft
                        @codemirror.setCursor @codemirror.lineCount(), 0 # We hit the draft and put the cursor at the end
                else if event.type is 'keyup' and event.altKey and event.which is 33 # Page up
                    @history_displayed_id = @state.history.length
                    @codemirror.setValue @state.history[@state.history.length-@history_displayed_id].query
                    event.preventDefault()
                    return true
                else if event.type is 'keyup' and event.altKey and event.which is 34 # Page down
                    @history_displayed_id = @history.length
                    @codemirror.setValue @state.history[@state.history.length-@history_displayed_id].query
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
                # We catch ctrl, shift, alt and command 
                if event.ctrlKey or event.shiftKey or event.altKey or event.which is 16 or event.which is 17 or event.which is 18 or event.which is 20 or (event.which is 91 and event.type isnt 'keypress') or event.which is 92 or event.type of @mouse_type_event
                    return false

            # We catch ctrl, shift, alt and command but don't look for active key (active key here refer to ctrl, shift, alt being pressed and hold)
            if event? and (event.which is 16 or event.which is 17 or event.which is 18 or event.which is 20 or (event.which is 91 and event.type isnt 'keypress') or event.which is 92)
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

            if @state.options.electric_punctuation is true
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
            @description = null

            result =
                status: null
                # create_suggestion is going to fill to_complete and to_describe
                #to_complete: undefined
                #to_describe: undefined
                
            # If we are in the middle of a function (text after the cursor - that is not an element in @char_breakers or a comment), we just show a description, not a suggestion
            result_non_white_char_after_cursor = @regex.get_first_non_white_char.exec(query_after_cursor)

            if result_non_white_char_after_cursor isnt null and not(result_non_white_char_after_cursor[1]?[0] of @char_breakers or result_non_white_char_after_cursor[1]?.match(/^((\/\/)|(\/\*))/) isnt null)
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
            result.suggestions = @uniq result.suggestions

            if result.suggestions?.length > 0
                for suggestion, i in result.suggestions
                    @current_suggestions.push suggestion
                    @.$('.suggestion_name_list').append @template_suggestion_name
                        id: i
                        suggestion: suggestion
                if @state.options.suggestions is true
                    @show_suggestion()
                else
                    @hide_suggestion()
                @hide_description()
            else if result.description?
                @hide_suggestion()
                @description = result.description
                if @state.options.suggestions is true and event?.type isnt 'mouseup'
                    @show_description()
                else
                    @hide_description()
            else
                @hide_suggestion_and_description()
            if event?.which is 9 # Catch tab
                # If you're in a string, you add a TAB. If you're at the beginning of a newline with preceding whitespace, you add a TAB. If it's any other case do nothing.
                if @last_element_type_if_incomplete(stack) isnt 'string' and @regex.white_or_empty.test(@codemirror.getLine(@codemirror.getCursor().line).slice(0, @codemirror.getCursor().ch)) isnt true
                    return true
                else
                    return false
            return true

        # Similar to underscore's uniq but faster with a hashmap
        uniq: (ar) ->
            if not ar? or ar.length is 0
                return ar
            result = []
            hash = {}
            for element in ar
                hash[element] = true
            for key of hash
                result.push key
            result.sort()
            return result

        # Extract information from the current query
        # Regex used
        regex:
            anonymous:/^(\s)*function\(([a-zA-Z0-9,\s]*)\)(\s)*{/
            loop:/^(\s)*(for|while)(\s)*\(([^\)]*)\)(\s)*{/
            method: /^(\s)*([a-zA-Z0-9]*)\(/ # forEach( merge( filter(
            row: /^(\s)*row\(/
            method_var: /^(\s)*(\d*[a-zA-Z][a-zA-Z0-9]*)\./ # r. r.row. (r.count will be caught later)
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
            multiple_line_comment: /^(\s)*\/\*[^(\*\/)]*\*\//
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
            if (not stack?) or stack.length is 0
                return ''

            element = stack[stack.length-1]
            if element.body?
                return @last_element_type_if_incomplete(element.body)
            else
                if element.complete is false
                    return element.type
                else
                    return ''

         # Get the last key if the last element is a key of an object
         get_last_key: (stack) =>
            if (not stack?) or stack.length is 0
                return ''

            element = stack[stack.length-1]
            if element.body?
                return @get_last_key(element.body)
            else
                if element.complete is false and element.key?
                    return element.key
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


                    if element.type is null
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
                                element.args = []
                                new_context = DataUtils.deep_copy context
                                for arg in list_args
                                    arg = arg.replace(/(^\s*)|(\s*$)/gi,"") # Removing leading/trailing spaces
                                    new_context[arg] = true
                                    element.args.push arg
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
                                result_regex_row = @regex.row.exec query.slice new_start
                                if result_regex_row isnt null
                                    position_opening_parenthesis = result_regex_row[0].indexOf('(')
                                    element.type = 'function' # TODO replace with function
                                    element.name = 'row'
                                    stack.push element
                                    size_stack++
                                    if size_stack > @max_size_stack
                                        return null
                                    element =
                                        type: 'function'
                                        name: '('
                                        position: position+3+1
                                        context: context
                                        complete: 'false'
                                    stack_stop_char = ['(']
                                    start += position_opening_parenthesis
                                    to_skip = result_regex[0].length-1+new_start-i
                                    continue

                                else
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
                                            element.real_type = @types.value
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
                                            start = position_opening_parenthesis
                                            to_skip = result_regex[0].length-1
                                            continue
                                        ###
                                        # This last condition is a special case for r(expr)
                                        else if position_opening_parenthesis isnt -1 and result_regex[0].slice(0, position_opening_parenthesis) is 'r'
                                            element.type = 'var'
                                            element.name = 'r'
                                            element.real_type = @types.value
                                            element.position = position+new_start
                                            start += new_start-i
                                            to_skip = result_regex[0].length-1+new_start-i
                                            stack_stop_char = ['(']
                                            continue
                                        ###


                            # Check for method without parenthesis r., r.row., doc.
                            result_regex = @regex.method_var.exec query.slice new_start
                            if result_regex isnt null
                                if result_regex[0].slice(0, result_regex[0].length-1) of context
                                    element.type = 'var'
                                    element.real_type = @types.value
                                else
                                    element.type = 'function'
                                element.position = position+new_start
                                element.name = result_regex[0].slice(0, result_regex[0].length-1).replace(/\s/, '')
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
                                start -= new_start-i
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
                    element.real_type = @types.value
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
                        if @map_state[element.name]?
                            for state in @map_state[element.name]
                                if @suggestions[state]?
                                    for suggestion in @suggestions[state]
                                        if result.suggestions_regex.test(suggestion) is true
                                            result.suggestions.push suggestion
                        #else # This is a non valid ReQL function.
                        # It may be a personalized function defined in the data explorer...
                        result.status = 'done'
                    else if element.type is 'var' and element.complete is true
                        result.state = element.real_type
                        for type in result.state
                            if @suggestions[type]?
                                for suggestion in @suggestions[type]
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
        show_description: =>
            if @descriptions[@description]? # Just for safety
                margin = (parseInt(@.$('.CodeMirror-cursor').css('top').replace('px', ''))+@line_height)+'px'

                @.$('.suggestion_full_container').css 'margin-top', margin
                @.$('.arrow').css 'margin-top', margin

                @.$('.suggestion_description').html @description_template @extend_description @description

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

            if @state.options.electric_punctuation is true
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

            # Put back in the stack
            setTimeout =>
                @handle_keypress() # That's going to describe the function the user just selected
                @codemirror.focus() # Useful if the user used the mouse to select a suggestion
            , 0 # Useful if the user used the mouse to select a suggestion

        # Highlight a suggestion in case of a mouseover
        mouseover_suggestion: (event) =>
            @highlight_suggestion event.target.dataset.id

        # Hide suggestion in case of a mouse out
        mouseout_suggestion: (event) =>
            @hide_description()

        # Extend description for .db() and .table() with dbs/tables names
        extend_description: (fn) =>
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
                @state.cursor.next get_result_callback
                $(window).scrollTop(@.$('.results_container').offset().top)
            catch err
                @toggle_executing false
                # We print the query here (the user just hit `more data`)
                @results_view.render_error(@query, err)

        abort_query: =>
            @driver_handler.close_connection()
            @id_execution++
            @toggle_executing false

        # Function that execute the queries in a synchronous way.
        execute_query: =>
            # We don't let people execute more than one query at a time on the same connection
            # While we remove the button run, `execute_query` could still be called with Shift+Enter
            if @executing is true
                return

            # Hide the option, if already hidden, nothing happens.
            @$('.profiler_enabled').slideUp 'fast'

            # The user just executed a query, so we reset cursor_timed_out to false
            @state.cursor_timed_out = false
            @state.show_query_warning = false

            @raw_query = @codemirror.getValue()

            @query = @clean_query @raw_query # Save it because we'll use it in @callback_multilples_queries
            
            # Execute the query
            try
                if @state.cursor?
                    @state.cursor.close?
                # Separate queries
                @non_rethinkdb_query = '' # Store the statements that don't return a rethinkdb query (like "var a = 1;")
                @index = 0 # index of the query currently being executed

                @raw_queries = @separate_queries @raw_query # We first split raw_queries
                @queries = @separate_queries @query

                if @queries.length is 0
                    error = @query_error_template
                        no_query: true
                    @results_view.render_error(null, error, true)
                else
                    @toggle_executing true
                    @execute_portion()

            catch err
                @toggle_executing false
                # Missing brackets, so we display everything (we don't know if we properly splitted the query)
                @results_view.render_error(@query, err, true)
                @save_query
                    query: @raw_query
                    broken_query: true

        toggle_executing: (executing) =>
            if executing is true
                @executing = true
                @timeout_show_abort = setTimeout =>
                    #TODO Delay a few ms
                    @.$('.loading_query_img').show()
                    @$('.execute_query').hide()
                    @$('.abort_query').show()
                , @delay_show_abort
            else if executing is false
                clearTimeout @timeout_show_abort
                @executing = false
                @.$('.loading_query_img').hide()
                @$('.execute_query').show()
                @$('.abort_query').hide()

        # A portion is one query of the whole input.
        execute_portion: =>
            @state.cursor = null
            while @queries[@index]?
                full_query = @non_rethinkdb_query
                full_query += @queries[@index]

                try
                    rdb_query = @evaluate(full_query)
                catch err
                    @toggle_executing false
                    if @queries.length > 1
                        @results_view.render_error(@raw_queries[@index], err, true)
                    else
                        @results_view.render_error(null, err, true)

                    @save_query
                        query: @raw_query
                        broken_query: true
                    return false

                @index++
                if rdb_query instanceof @TermBaseConstructor
                    @skip_value = 0
                    @start_time = new Date()
                    @current_results = []

                    @id_execution++ # Update the id_execution and use it to tag the callbacks
                    rdb_global_callback = @generate_rdb_global_callback @id_execution
                    # Date are displayed in their raw format for now.
                    @state.last_query_has_profile = @state.options.profiler
                    @driver_handler.create_connection (error, connection) =>
                        if (error)
                            @error_on_connect error
                        else
                            rdb_query.private_run {connection: connection, timeFormat: "raw", profile: @state.options.profiler}, rdb_global_callback # @rdb_global_callback can be fired more than once
                    , @id_execution, @error_on_connect

                    return true
                else if rdb_query instanceof DataExplorerView.DriverHandler
                    # Nothing to do
                    return true
                else
                    @non_rethinkdb_query += @queries[@index-1]
                    if @index is @queries.length
                        @toggle_executing false
                        error = @query_error_template
                            last_non_query: true
                        @results_view.render_error(@raw_queries[@index-1], error, true)

                        @save_query
                            query: @raw_query
                            broken_query: true

        
        # Create a callback for when a query returns
        # We tag the callback to make sure that we display the results only of the last query executed by the user
        generate_rdb_global_callback: (id_execution) =>
            rdb_global_callback = (error, results) =>
                if @id_execution is id_execution # We execute the query only if it is the last one
                    get_result_callback = @generate_get_result_callback id_execution

                    if error?
                        @toggle_executing false
                        if @queries.length > 1
                            @results_view.render_error(@raw_queries[@index-1], error)
                        else
                            @results_view.render_error(null, error)
                        @save_query
                            query: @raw_query
                            broken_query: true

                        return false

                    if results?.profile? and @state.last_query_has_profile is true
                        cursor = results.value
                        @profile = results.profile
                        @state.profile = @profile
                    else
                        cursor = results
                        @profile = null # @profile is null if the user deactivated the profiler
                        @state.profile = @profile

                    
                    if @index is @queries.length # @index was incremented in execute_portion
                        if cursor?.hasNext?
                            @state.cursor = cursor
                            if cursor.hasNext() is true
                                @state.cursor.next get_result_callback
                            else
                                get_result_callback() # Display results
                        else
                            @toggle_executing false

                            # Save the last executed query and the last displayed results
                            @current_results = cursor

                            @state.query = @query
                            @state.results = @current_results
                            @state.metadata =
                                limit_value: if Object::toString.call(@results) is '[object Array]' then @current_results.length else 1 # If @current_results.length is not defined, we have a single value
                                skip_value: @skip_value
                                execution_time: new Date() - @start_time
                                query: @query
                                has_more_data: false

                            @results_view.render_result
                                results: @current_results # The first parameter is null ( = query, so we don't display it)
                                metadata: @state.metadata
                                profile: @profile

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
                        if @queries.length > 1
                            @results_view.render_error(@query, error)
                        else
                            @results_view.render_error(null, error)
                        return false

                    if data isnt undefined
                        @current_results.push data
                        if @current_results.length < @limit and @state.cursor.hasNext() is true
                            @state.cursor.next get_result_callback
                            return true

                    @toggle_executing false

                    # if data is undefined or @current_results.length is @limit
                    @state.query = @query
                    @state.results = @current_results
                    @state.metadata =
                        limit_value: if @current_results?.length? then @current_results.length else 1 # If @current_results.length is not defined, we have a single value
                        skip_value: @skip_value
                        execution_time: new Date() - @start_time
                        query: @query
                        has_more_data: @state.cursor.hasNext()

                    @results_view.render_result
                        results: @current_results # The first parameter is null ( = query, so we don't display it)
                        metadata: @state.metadata
                        profile: @profile

                    # Successful query, let's save it in the history
                    @save_query
                        query: @raw_query
                        broken_query: false

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
        #  We also remove comments from the query
        clean_query: (query) ->
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
                        result_query += query.slice(start, i).replace(/\n/g, '')
                        start = i
                        to_skip = result_inline_comment[0].length-1
                        start += result_inline_comment[0].length
                        continue
                    result_multiple_line_comment = @regex.multiple_line_comment.exec query.slice i
                    if result_multiple_line_comment?
                        result_query += query.slice(start, i).replace(/\n/g, '')
                        start = i
                        to_skip = result_multiple_line_comment[0].length-1
                        start += result_multiple_line_comment[0].length
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

        # Called if the driver could not connect
        error_on_connect: (error) =>
            if /^(Unexpected token)/.test(error.message)
                # Unexpected token, the server couldn't parse the protobuf message
                # The truth is we don't know which query failed (unexpected token), but it seems safe to suppose in 99% that the last one failed.
                @toggle_executing false
                @results_view.render_error(null, error)

                # We save the query since the callback will never be called.
                @save_query
                    query: @raw_query
                    broken_query: true

            else
                @results_view.cursor_timed_out()
                # We fail to connect, so we display a message except if we were already disconnected and we are not trying to manually reconnect
                # So if the user fails to reconnect after a failure, the alert will still flash
                @$('#user-alert-space').hide()
                @$('#user-alert-space').html @alert_connection_fail_template({})
                @$('#user-alert-space').slideDown 'fast'
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
    
    class @SharedResultView extends Backbone.View
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

        default_size_column: 310 # max-width value of a cell of a table (as defined in the css file)
        mouse_down: false

        events: ->
            'click .link_to_profile_view': 'show_profile'
            'click .link_to_tree_view': 'show_tree'
            'click .link_to_table_view': 'show_table'
            'click .link_to_raw_view': 'show_raw'
            # For Tree view
            'click .jt_arrow': 'toggle_collapse'
            # For Table view
            'mousedown .click_detector': 'handle_mousedown'
            'click .jta_arrow_h': 'expand_tree_in_table'

        expand_raw_textarea: =>
            if $('.raw_view_textarea').length > 0
                height = $('.raw_view_textarea')[0].scrollHeight
                $('.raw_view_textarea').height(height)


        toggle_collapse: (event) =>
            @.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed')
            @.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed')
            @.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed')
            @.$(event.target).toggleClass('jt_arrow_hidden')
            @set_scrollbar()

        handle_mousedown: (event) =>
            if event?.target?.className is 'click_detector'
                @col_resizing = @$(event.target).parent().data('col')
                @start_width = @$(event.target).parent().width()
                @start_x = event.pageX
                @mouse_down = true
                $('body').toggleClass('resizing', true)

        handle_mousemove: (event) =>
            if @mouse_down
                @container.state.last_columns_size[@col_resizing] = Math.max 5, @start_width-@start_x+event.pageX # Save the personalized size
                @resize_column @col_resizing, @container.state.last_columns_size[@col_resizing] # Resize

        resize_column: (col, size) =>
            @$('.col-'+col).css 'max-width', size
            @$('.value-'+col).css 'max-width', size-20
            @$('.col-'+col).css 'width', size
            @$('.value-'+col).css 'width', size-20
            if size < 20
                @$('.value-'+col).css 'padding-left', (size-5)+'px'
                @$('.value-'+col).css 'visibility', 'hidden'
            else
                @$('.value-'+col).css 'padding-left', '15px'
                @$('.value-'+col).css 'visibility', 'visible'

        handle_mouseup: (event) =>
            if @mouse_down is true
                @mouse_down = false
                $('body').toggleClass('resizing', false)
                @set_scrollbar()



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



        show_tree: (event) =>
            event.preventDefault()
            @set_view 'tree'
        show_profile: (event) =>
            event.preventDefault()
            @set_view 'profile'
        show_table: (event) =>
            event.preventDefault()
            @set_view 'table'
        show_raw: (event) =>
            event.preventDefault()
            @set_view 'raw'

        set_view: (view) =>
            @view = view
            @container.state.view = view
            @render_result()



        # We build the tree in a recursive way
        json_to_node: (value) =>
            value_type = typeof value
            
            output = ''
            if value is null
                return @template_json_tree.span
                    classname: 'jt_null'
                    value: 'null'
            else if Object::toString.call(value) is '[object Array]'
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
            else if value_type is 'object' and value.$reql_type$ is 'TIME' and value.epoch_time?
                return @template_json_tree.span
                    classname: 'jt_date'
                    value: @date_to_string(value)
            else if value_type is 'object'
                sub_keys = []
                for key of value
                    sub_keys.push key
                sub_keys.sort()

                sub_values = []
                for key in sub_keys
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
 
        ###
        keys =
            primitive_value_count: <int>
            object:
                key_1: <keys>
                key_2: <keys>
        ###
        build_map_keys: (args) =>
            keys_count = args.keys_count
            result = args.result

            if jQuery.isPlainObject(result)
                if result.$reql_type$ is 'TIME' and result.epoch_time?
                    keys_count.primitive_value_count++
                else
                    for key, row of result
                        if not keys_count['object']?
                            keys_count['object'] = {} # That's define only if there are keys!
                        if not keys_count['object'][key]?
                            keys_count['object'][key] =
                                primitive_value_count: 0
                        @build_map_keys
                            keys_count: keys_count['object'][key]
                            result: row
            else
                keys_count.primitive_value_count++

        # Compute occurrence of each key. The occurence can be a float since we compute the average occurence of all keys for an object
        compute_occurrence: (keys_count) =>
            if not keys_count['object']? # That means we are accessing only a primitive value
                keys_count.occurrence = keys_count.primitive_value_count
            else
                count_key = if keys_count.primitive_value_count > 0 then 1 else 0
                count_occurrence = keys_count.primitive_value_count
                for key, row of keys_count['object']
                    count_key++
                    @compute_occurrence row
                    count_occurrence += row.occurrence
                keys_count.occurrence = count_occurrence/count_key # count_key cannot be 0

        # Sort the keys per level
        order_keys: (keys) =>
            copy_keys = []
            if keys.object?
                for key, value of keys.object
                    if jQuery.isPlainObject(value)
                        @order_keys value

                    copy_keys.push
                        key: key
                        value: value.occurrence
                # If we could know if a key is a primary key, that would be awesome
                copy_keys.sort (a, b) ->
                    if b.value-a.value
                        return b.value-a.value
                    else
                        if a.key > b.key
                            return 1
                        else # We cannot have two times the same key
                            return -1
            keys.sorted_keys = _.map copy_keys, (d) -> return d.key
            if keys.primitive_value_count > 0
                keys.sorted_keys.unshift @primitive_key

        # Build the table
        # We order by the most frequent keys then by alphabetic order
        # if indexes is null, it means that we can order all fields
        json_to_table: (result, primary_key, can_sort, indexes) =>
            # While an Array type is never returned by the driver, we still build an Array in the data explorer
            # when a cursor is returned (since we just print @limit results)
            if not result.constructor? or result.constructor isnt Array
                result = [result]

            keys_count =
                primitive_value_count: 0

            for result_entry in result
                @build_map_keys
                    keys_count: keys_count
                    result: result_entry
            @compute_occurrence keys_count

            @order_keys keys_count

            flatten_attr = []

            @get_all_attr # fill attr[]
                keys_count: keys_count
                attr: flatten_attr
                prefix: []
                prefix_str: ''
            for value, index in flatten_attr
                value.col = index

            flatten_attr: flatten_attr
            result: result


        # Flatten the object returns by build_map_keys().
        # We get back an array of keys
        get_all_attr: (args) =>
            keys_count = args.keys_count
            attr = args.attr
            prefix = args.prefix
            prefix_str = args.prefix_str
            for key in keys_count.sorted_keys
                if key is @primitive_key
                    new_prefix_str = prefix_str # prefix_str without the last dot
                    if new_prefix_str.length > 0
                        new_prefix_str = new_prefix_str.slice(0, -1)
                    attr.push
                        prefix: prefix
                        prefix_str: new_prefix_str
                        is_primitive: true
                else
                    if keys_count['object'][key]['object']?
                        new_prefix = DataUtils.deep_copy(prefix)
                        new_prefix.push key
                        @get_all_attr
                            keys_count: keys_count.object[key]
                            attr: attr
                            prefix: new_prefix
                            prefix_str: (if prefix_str? then prefix_str else '')+key+'.'
                    else
                        attr.push
                            prefix: prefix
                            prefix_str: prefix_str
                            key: key


        json_to_tree: (result) =>
            return @template_json_tree.container
                tree: @json_to_node(result)

        json_to_table_get_attr: (flatten_attr) =>
            return @template_json_table.tr_attr
                attr: flatten_attr

        json_to_table_get_values: (args) =>
            result = args.result
            flatten_attr = args.flatten_attr

            document_list = []
            for single_result, i in result
                new_document =
                    cells: []
                for attr_obj, col in flatten_attr
                    key = attr_obj.key
                    value = single_result
                    for prefix in attr_obj.prefix
                        value = value?[prefix]
                    if attr_obj.is_primitive isnt true
                        if value?
                            value = value[key]
                        else
                            value = undefined
                    new_document.cells.push @json_to_table_get_td_value value, col
                @tag_record new_document, i
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
            else if value_type is 'object' and value.$reql_type$ is 'TIME' and value.epoch_time?
                data['value'] = @date_to_string(value)
                data['classname'] = 'jta_date'
            else if value_type is 'object'
                data['value'] = '{ ... }'
                data['is_object'] = true
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
                data.value = if value is true then 'true' else 'false'

            return data

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

        set_scrollbar: =>
            if @view is 'table'
                content_name = '.json_table'
                content_container = '.table_view_container'
            else if @view is 'tree'
                content_name = '.json_tree'
                content_container = '.tree_view_container'
            else if @view is 'profile'
                content_name = '.json_tree'
                content_container = '.profile_view_container'
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

 
        date_to_string: (date) =>
            if date.timezone?
                timezone = date.timezone
                
                # Extract data from the timezone
                timezone_array = date.timezone.split(':')
                sign = timezone_array[0][0] # Keep the sign
                timezone_array[0] = timezone_array[0].slice(1) # Remove the sign

                # Save the timezone in minutes
                timezone_int = (parseInt(timezone_array[0])*60+parseInt(timezone_array[1]))*60
                if sign is '-'
                    timezone_int = -1*timezone_int
                # Add the user local timezone
                timezone_int += (new Date()).getTimezoneOffset()*60
            else
                timezone = '+00:00'
                timezone_int = (new Date()).getTimezoneOffset()*60

            # Tweak epoch and create a date
            raw_date_str = (new Date((date.epoch_time+timezone_int)*1000)).toString()

            # Remove the timezone and replace it with the good one
            return raw_date_str.slice(0, raw_date_str.indexOf('GMT')+3)+timezone





    class @ResultView extends DataExplorerView.SharedResultView
        className: 'result_view'
        template: Handlebars.templates['dataexplorer_result_container-template']
        metadata_template: Handlebars.templates['dataexplorer-metadata-template']
        option_template: Handlebars.templates['dataexplorer-option_page-template']
        error_template: Handlebars.templates['dataexplorer-error-template']
        template_no_result: Handlebars.templates['dataexplorer_result_empty-template']
        cursor_timed_out_template: Handlebars.templates['dataexplorer-cursor_timed_out-template']
        no_profile_template: Handlebars.templates['dataexplorer-no_profile-template']
        profile_header_template: Handlebars.templates['dataexplorer-profiler_header-template']
        primitive_key: '_-primitive value-_--' # We suppose that there is no key with such value in the database.

        events: ->
            _.extend super,
                'click .activate_profiler': 'activate_profiler'

        current_result: []

        initialize: (args) =>
            @container = args.container
            @limit = args.limit
            @skip = 0
            if args.view?
                @view = args.view
            else
                @view = 'tree'

            @last_keys = @container.state.last_keys # Arrays of the last keys displayed
            @last_columns_size = @container.state.last_columns_size # Size of the columns displayed. Undefined if a column has the default size

            ZeroClipboard.setDefaults
                moviePath: 'js/ZeroClipboard.swf'
                forceHandCursor: true #TODO Find a fix for chromium(/linux?)
            @clip = new ZeroClipboard()

        activate_profiler: (event) =>
            event.preventDefault()
            if @container.options_view.state is 'hidden'
                @container.toggle_options
                    cb: =>
                        setTimeout( =>
                            if @container.state.options.profiler is false
                                @container.options_view.$('.option_description[data-option="profiler"]').click()
                            @container.options_view.$('.profiler_enabled').show()
                            @container.options_view.$('.profiler_enabled').css 'visibility', 'visible'
                        , 100)
            else
                if @container.state.options.profiler is false
                    @container.options_view.$('.option_description[data-option="profiler"]').click()
                @container.options_view.$('.profiler_enabled').hide()
                @container.options_view.$('.profiler_enabled').css 'visibility', 'visible'
                @container.options_view.$('.profiler_enabled').slideDown 'fast'



        render_error: (query, err, js_error) =>
            @.$el.html @error_template
                query: query
                error: err.toString().replace(/^(\s*)/, '')
                js_error: js_error is true
            return @

        json_to_table: (result) =>
            {flatten_attr, result} = super result

            @last_keys = flatten_attr.map (attr, i) ->
                if attr.prefix_str isnt ''
                    return attr.prefix_str+attr.key
                return attr.key
            @container.state.last_keys = @last_keys


            return @template_json_table.container
                table_attr: @json_to_table_get_attr flatten_attr
                table_data: @json_to_table_get_values
                    result: result
                    flatten_attr: flatten_attr

        tag_record: (doc, i) =>
            doc.record = @metadata.skip_value + i

        render_result: (args) =>
            if args? and args.results isnt undefined
                @results = args.results
                @results_array = null # if @results is not an array (possible starting from 1.4), we will transform @results_array to [@results] for the table view
            if args? and args.profile isnt undefined
                @profile = args.profile
            if args?.metadata?
                @metadata = args.metadata
            if args?.metadata?.skip_value?
                # @container.state.start_record is the old value of @container.state.skip_value
                # Here we just deal with start_record
                # TODO May have to remove this line as we have metadata.start now
                @container.state.start_record = args.metadata.skip_value

                if args.metadata.execution_time?
                    @metadata.execution_time_pretty = @prettify_duration args.metadata.execution_time

            num_results = @metadata.skip_value
            if @metadata.has_more_data isnt true
                if Object::toString.call(@results) is '[object Array]'
                    num_results += @results.length
                else # @results can be a single value or null
                    num_results += 1

            @.$el.html @template _.extend @metadata,
                show_query_warning: args?.show_query_warning
                show_more_data: @metadata.has_more_data is true and @container.state.cursor_timed_out is false
                cursor_timed_out_template: (@cursor_timed_out_template() if @metadata.has_more_data is true and @container.state.cursor_timed_out is true)
                execution_time_pretty: @metadata.execution_time_pretty
                no_results: @metadata.has_more_data isnt true and @results?.length is 0 and @metadata.skip_value is 0
                num_results: num_results


            # Set the text to copy
            @$('.copy_profile').attr('data-clipboard-text', JSON.stringify(@profile, null, 2))

            if @view is 'profile'
                @$('.more_results').hide()
                @$('.profile_summary').show()
            else
                @$('.more_results').show()
                @$('.profile_summary').hide()
                @$('.copy_profile').hide()

            switch @view
                when 'profile'
                    if @profile is null
                        @$('.profile_container').html @no_profile_template()
                        @$('.copy_profile').hide()
                    else
                        @.$('.profile_container').html @json_to_tree @profile
                        @$('.copy_profile').show()
                        @$('.profile_summary_container').html @profile_header_template
                            total_duration: @metadata.execution_time_pretty
                            server_duration: @prettify_duration @compute_total_duration @profile
                            num_shard_accesses: @compute_num_shard_accesses @profile

                        @clip.glue($('button.copy_profile'))

                    @$('.results').hide()
                    @$('.profile_view_container').show()
                    @.$('.link_to_profile_view').addClass 'active'
                    @.$('.link_to_profile_view').parent().addClass 'active'
                when 'tree'
                    @.$('.json_tree_container').html @json_to_tree @results
                    @$('.results').hide()
                    @$('.tree_view_container').show()
                    @.$('.link_to_tree_view').addClass 'active'
                    @.$('.link_to_tree_view').parent().addClass 'active'
                when 'table'
                    previous_keys = @container.state.last_keys # Save previous keys. @last_keys will be updated in @json_to_table
                    if Object::toString.call(@results) is '[object Array]'
                        if @results.length is 0
                            @.$('.table_view').html @template_no_result()
                        else
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
                    if @container.state.last_keys.length isnt previous_keys.length
                        same_keys = false
                    else
                        same_keys = true
                        for keys, index in @container.state.last_keys
                            if @container.state.last_keys[index] isnt previous_keys[index]
                                same_keys = false

                    # TODO we should just check if previous_keys is included in last_keys
                    # If the keys are the same, we are going to resize the columns as they were before
                    if same_keys is true
                        for col, value of @container.state.last_columns_size
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
 
           
        # Check if the cursor timed out. If yes, make sure that the user cannot fetch more results
        cursor_timed_out: =>
            @container.state.cursor_timed_out = true
            if @container.state.metadata?.has_more_data is true
                @$('.more_results_paragraph').html @cursor_timed_out_template()

        render: =>
            @delegateEvents()
            return @

        render_default: =>
            return @

        prettify_duration: (duration) ->
            if duration < 1
                return '<1ms'
            else if duration < 1000
                return duration.toFixed(0)+"ms"
            else if duration < 60*1000
                return (duration/1000).toFixed(2)+"s"
            else # We do not expect query to last one hour.
                minutes = Math.floor(duration/(60*1000))
                return minutes+"min "+((duration-minutes*60*1000)/1000).toFixed(2)+"s"

        compute_total_duration: (profile) ->
            total_duration = 0
            for task in profile
                if task['duration(ms)']?
                    total_duration += task['duration(ms)']
                else if task['mean_duration(ms)']?
                    total_duration += task['mean_duration(ms)']

            total_duration


        compute_num_shard_accesses: (profile) ->
            num_shard_accesses = 0
            for task in profile
                if task['description'] is 'Perform read on shard.'
                    num_shard_accesses += 1
                if Object::toString.call(task['sub_tasks']) is '[object Array]'
                    num_shard_accesses += @compute_num_shard_accesses task['sub_tasks']
                if Object::toString.call(task['parallel_tasks']) is '[object Array]'
                    num_shard_accesses += @compute_num_shard_accesses task['parallel_tasks']

                # In parallel tasks, we get arrays of tasks instead of a super task
                if Object::toString.call(task) is '[object Array]'
                    num_shard_accesses += @compute_num_shard_accesses task

            return num_shard_accesses



        destroy: =>
            $(window).unbind 'scroll'
            $(window).unbind 'resize'

    class @OptionsView extends Backbone.View
        dataexplorer_options_template: Handlebars.templates['dataexplorer-options-template']
        className: 'options_view'

        events:
            'click li': 'toggle_option'

        initialize: (args) =>
            @container = args.container
            @options = args.options
            @state = 'hidden'

        toggle_option: (event) =>
            event.preventDefault()
            new_target = @$(event.target).data('option')
            @$('#'+new_target).prop 'checked', !@options[new_target]
            if event.target.nodeName isnt 'INPUT' # Label we catch if for us
                new_value = @$('#'+new_target).is(':checked')
                @options[new_target] = new_value
                if window.localStorage?
                    window.localStorage.options = JSON.stringify @options
                if new_target is 'profiler' and new_value is false
                    @$('.profiler_enabled').slideUp 'fast'

        render: (displayed) =>
            @$el.html @dataexplorer_options_template @options
            if displayed is true
                @$el.show()
            @delegateEvents()
            return @

    class @HistoryView extends Backbone.View
        dataexplorer_history_template: Handlebars.templates['dataexplorer-history-template']
        dataexplorer_query_li_template: Handlebars.templates['dataexplorer-query_li-template']
        className: 'history_container'
        
        size_history_displayed: 300
        state: 'hidden' # hidden, visible
        index_displayed: 0

        events:
            'click .load_query': 'load_query'
            'click .delete_query': 'delete_query'

        start_resize: (event) =>
            @start_y = event.pageY
            @start_height = @container.$('.nano').height()
            @mouse_down = true
            $('body').toggleClass('resizing', true)

        handle_mousemove: (event) =>
            if @mouse_down is true
                @height_history = Math.max 0, @start_height-@start_y+event.pageY
                @container.$('.nano').height @height_history

        handle_mouseup: (event) =>
            if @mouse_down is true
                @mouse_down = false
                $('.nano').nanoScroller({preventPageScrolling: true})
                $('body').toggleClass('resizing', false)

        initialize: (args) =>
            @container = args.container
            @history = args.history
            @height_history = 204

        render: (displayed) =>
            @$el.html @dataexplorer_history_template()
            if displayed is true
                @$el.show()
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
            @container.state.current_query = @history[parseInt(id)].query

        delete_query: (event) =>
            that = @

            # Remove the query and overwrite localStorage.rethinkdb_history
            id = parseInt(@$(event.target).data().id)
            @history.splice(id, 1)
            window.localStorage.rethinkdb_history = JSON.stringify @history

            # Animate the deletion
            is_at_bottom = @$('.history_list').height() is $('.nano > .content').scrollTop()+$('.nano').height()
            @$('#query_history_'+id).slideUp 'fast', =>
                that.$(this).remove()
                that.render()
                that.container.adjust_collapsible_panel_height
                    is_at_bottom: is_at_bottom


        add_query: (args) =>
            query = args.query
            broken_query = args.broken_query

            that = @
            is_at_bottom = @$('.history_list').height() is $('.nano > .content').scrollTop()+$('.nano').height()

            @$('.history_list').append @dataexplorer_query_li_template
                query: query
                broken_query: broken_query
                id: @history.length-1
                num: @history.length

            if @$('.no_history').length > 0
                @$('.no_history').slideUp 'fast', ->
                    $(@).remove()
                    that.container.adjust_collapsible_panel_height
                        is_at_bottom: is_at_bottom
            else if @state is 'visible'
                @container.adjust_collapsible_panel_height
                    delay_scroll: true
                    is_at_bottom: is_at_bottom

        clear_history: (event) =>
            that = @
            event.preventDefault()
            @container.clear_history()
            @history = @container.history

            @$('.query_history').slideUp 'fast', ->
                $(@).remove()
                if that.$('.no_history').length is 0
                    that.$('.history_list').append that.dataexplorer_query_li_template
                        no_query: true
                        displayed_class: 'hidden'
                    that.$('.no_history').slideDown 'fast'
            that.container.adjust_collapsible_panel_height
                size: 40
                move_arrow: 'show'
                is_at_bottom: 'true'

    class @DriverHandler
        ping_time: 5*60*1000

        query_error_template: Handlebars.templates['dataexplorer-query_error-template']

        # I don't want that thing in window
        constructor: (args) ->
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
                pathname: window.location.pathname

            @hack_driver()
        
        # Hack the driver, remove .run() and private_run()
        hack_driver: =>
            TermBase = r.expr(1).constructor.__super__.constructor.__super__
            if not TermBase.private_run?
                that = @
                TermBase.private_run = TermBase.run
                TermBase.run = ->
                    throw that.query_error_template
                        found_run: true

        close_connection: =>
            if @connection?.open is true
                @connection.close {noreplyWait: false}
                @connection = null

        create_connection: (cb, id_execution, connection_cb) =>
            that = @
            @id_execution = id_execution

            try
                ((_id_execution) =>
                    r.connect @server, (error, connection) =>
                        if _id_execution is @id_execution
                            connection.removeAllListeners 'error' # See issue 1347
                            connection.on 'error', connection_cb
                            @connection = connection
                            cb(error, connection)
                        else
                            connection.cancel()
                )(id_execution)
   
            catch err
                @on_fail(err)

        destroy: =>
            @close_connection()

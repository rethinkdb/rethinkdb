#TODO remove the test functions
generate_id = (n) ->
    chars = '0123456789qwertasdfgzxcvbyuioplkjhnm'
    result = ''
    for i in [0..n]
        num = Math.floor(Math.random()*(chars.length+1))
        result += chars.substring(num, num+1)

    return result


generate_number = (n) ->
    return Math.floor(Math.random()*n)

generate_string = (n) ->
    chars = 'qwertasdfgzxcvbyuioplkjhnm'
    result = ''
    limit = Math.floor(Math.random()*n)+3
    for i in [Math.floor(limit/2)..limit]
        num = Math.floor(Math.random()*(chars.length+1))
        result += chars.substring(num, num+1)

    return result



#TODO destroy views
#TODO maintain data
module 'DataExplorerView', ->
    class @Container extends Backbone.View
        className: 'dataexplorer_container'
        template: Handlebars.compile $('#dataexplorer_view-template').html()

        events: ->
            'keypress .input_query': 'handle_keypress'
            'click .clear_query': 'clear_query'
            'click .execute_query': 'execute_query'
            'click .namespace_link': 'write_query'
            'click .home_view': 'display_home'



        handle_keypress: (event) =>
            if event.which is 13 and !event.shiftKey
                event.preventDefault()
                @execute_query()
            else
                @make_suggestion()

        execute_query: =>
            query = @.$('.input_query').val() 
            @data_container.add_query(query)
            #TODO ajax call + loading
            if query is '0'
                result = []
            else if query is '1'
                result =  [
                    id: '_97a54c09-112e-4e32-86f8-24522e6e1df4'
                    login: 'michel'
                    first_name: 'Michel'
                    last_name: 'Tu'
                    email: 'michel@rethinkdb.com'
                    messages: 52.54
                    inscription_date: '04/26/1998'
                    citizen_id: null
                    member: true
                    website: "http://www.neumino.com"
                    groups: [
                            id: 'a53e4190-dfcc-4810-aded-a3d4f422e9b9'
                            name: 'front end developers'
                        ,
                            id: 'ebcdb08d-302c-4403-a392-eadad3f5bea5'
                            name: 'rethinkDB'
                    ]
                    skills:
                        development:
                            'javascript': 9
                            'css': 9
                            'c++': 2
                        music:
                            'violin': 0
                            'piano': 1
                            'guitare': 2
                    last_score: [ 54, 43, 11, 95, 78]
                ]
            else
                result = []
                for i in [0..20]
                    element = {}
                    element['_id'] = generate_id(25)
                    element['name'] = generate_string(9)+' '+generate_string(9)
                    element['mail'] = generate_string(8)+'@'+generate_string(6)+'.com'
                    element['age'] = generate_number(100)
                    element['possess_car'] = false
                    element['driver_license'] = null
                    element['last_scores'] = []
                    limit = generate_number(20)
                    for p in [0..limit]
                        element['last_scores'].push generate_number(100)
                    element['phone'] =
                        home: generate_number(10)+''+generate_number(10)+''+generate_number(10)+'-'+generate_number(10)+''+generate_number(10)+''+generate_number(10)+''+generate_number(10)+'-'+generate_number(10)+''+generate_number(10)+''+generate_number(10)+''+generate_number(10)
                        mobile: generate_number(10)+''+generate_number(10)+''+generate_number(10)+'-'+generate_number(10)+''+generate_number(10)+''+generate_number(10)+''+generate_number(10)+'-'+generate_number(10)+''+generate_number(10)+''+generate_number(10)+''+generate_number(10)
                    element['website'] = 'http://www.'+generate_string(12)+'.com'
                    #element[generate_string(10)] = generate_string(10)


                    result.push element
            @data_container.render(query, result)

        make_suggestion: ->
            console.log 'suggestion'
            return @

        clear_query: =>
            @.$('.input_query').val ''
            @.$('.input_query').focus()

 
        write_query: (event) =>
            event.preventDefault()
            query = 'r.'+event.target.dataset.name+'.find()'
            @.$('.input_query').val query

        initialize: =>
            log_initial '(initializing) sidebar view:'

            @input_query = new DataExplorerView.InputQuery
            @data_container = new DataExplorerView.DataContainer

            @render()

        render: =>
            @.$el.html @template
            @.$el.append @input_query.render().el
            @.$el.append @data_container.render().el

            return @

        display_home: =>
            console.log 'fdes'
            @.$el.append @data_container.render().el
            return @

        destroy: =>
            @input_query.destroy()
            @data_container.destroy()

    
    class @InputQuery extends Backbone.View
        className: 'query_control'
        template: Handlebars.compile $('#dataexplorer_input_query-template').html()

        initialize: ->
            namespaces.on 'all', @render

        render: =>
            @.$el.html @template
            return @

        destroy: ->
            @.$('.input_query').off()


    class @DataContainer extends Backbone.View
        className: 'data_container'

        initialize: ->
            @default_view = new DataExplorerView.DefaultView
            @result_view = new DataExplorerView.ResultView

        add_query: (query) =>
            @default_view.add_query(query)

        render: (query, result) =>
            if query? and result?
                @.$el.html @result_view.render(query, result).el
            else
                @.$el.html @default_view.render().el

            return @

        destroy: =>
            @default_view.destroy()


    class @ResultView extends Backbone.View
        className: 'result_view'
        template: Handlebars.compile $('#dataexplorer_result_container-template').html()
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
 
        #TODO Fix unbinding when view changes
        events: ->
            # For Tree view
            'click .jt_arrow': 'toggle_collapse'
            'keypress .jt_editable': 'handle_keypress'
            'blur .jt_editable': 'send_update'
            
            # For Table view
            'mousedown td': 'handle_mousedown'
            'click .jta_arrow_v': 'expand_tree_in_table'
            'click .jta_arrow_h': 'expand_table_in_table'

        current_result: []

        initialize: =>
            $(document).mousemove @handle_mousemove
            $(document).mouseup @handle_mouseup

        json_to_tree: (result) =>
            return @template_json_tree.container 
                tree: @json_to_node(result)

        #TODO catch RangeError: Maximum call stack size exceeded
        #TODO check special characters
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
                    value: value
 


        json_to_table: (result) =>
            map = {}
            for element in result
                for key of element
                    if map[key]?
                        map[key]++
                    else
                        map[key] = 1
            
            keys_sorted = []
            for key of map
                keys_sorted.push [key, map[key]]

            # TODO Use a stable sort, Mozilla doesn't use a stable sort for .sort
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
            document = []
            for element in result
                new_document = {}
                new_document.value = []
                for key_container, col in keys_stored
                    key = key_container[0]
                    value = element[key]

                    new_document.value.push 
                        value: value
                        col: col

                    value_type = typeof value
                    if value is null
                        new_document.value[new_document.value.length-1]['classname'] = 'jta_null'
                        new_document.value[new_document.value.length-1]['value'] = 'null'
                    else if value.constructor? and value.constructor is Array
                        if value.length is 0
                            return '[ ]'
                        else
                            #TODO Build preview
                            #TODO Add arrows for attributes
                            new_document.value[new_document.value.length-1]['value'] = '[ ... ]'
                            new_document.value[new_document.value.length-1]['data_to_expand'] = JSON.stringify(value)
                    else if value_type is 'object'
                        new_document.value[new_document.value.length-1]['value'] = '{ ... }'
                        new_document.value[new_document.value.length-1]['data_to_expand'] = JSON.stringify(value)
                    else if value_type is 'number'
                        new_document.value[new_document.value.length-1]['classname'] = 'jta_num'
                    else if value_type is 'string'
                        if /^(http|https):\/\/[^\s]+$/i.test(value)
                            new_document.value[new_document.value.length-1]['classname'] = 'jta_url'
                        else if /^[a-z0-9]+@[a-z0-9]+.[a-z0-9]{2,4}/i.test(value) # We don't handle .museum extension and special characters
                            new_document.value[new_document.value.length-1]['classname'] = 'jta_email'
                        else
                            new_document.value[new_document.value.length-1]['classname'] = 'jta_string'
                    else if value_type is 'boolean'
                        new_document.value[new_document.value.length-1]['classname'] = 'jta_bool'


                document.push new_document
            return @template_json_table.tr_value
                document: document

        expand_tree_in_table: (event) ->
            dom_element = @.$(event.target).nextAll('.jta_object')
            data = dom_element.data('json_data')
            result = @json_to_tree data
            dom_element.html result
            @.$(event.target).remove()

        expand_table_in_table: (event) ->
            dom_element = @.$(event.target).nextAll('.jta_object')
            parent = dom_element.parent()
            classname = dom_element.parent().attr('class').split(' ')[0] #TODO Use a regex
            data = dom_element.data('json_data')
            if data.constructor? and data.constructor is Array
                classcolumn = dom_element.parent().parent().attr('class')
                $('.'+classcolumn).css 'max-width', 'none'
                $('.'+classname).each ->
                    new_data = $(this).children('.jta_object').data('json_data')
                    if new_data? and new_data.constructor? and new_data.constructor is Array
                        $(this).children('.jta_object').html new_data.join ', '
                        $(this).children('.jta_arrow_h').css 'display', 'none'
                        
                    $(this).css 'max-width', 'none'



                data = data.join(', ') #TODO Make the join considering the type of element
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
                    template_json_table_td_value = @template_json_table.td_value

                    collection.each ->
                        if is_description
                            is_description = false
                            prefix = $(this).children('.jta_attr').html()
                            $(this).after template_json_table_td_attr
                                classtd: classcolumn+'-'+i
                                key: prefix+'.'+key[0]
                        else
                            new_data = $(this).children().children('.jta_object').data('json_data')
                            if new_data? and new_data[key[0]]?
                                to_print = new_data[key[0]]
                            else
                                to_print = 'undefined'
                            $(this).after template_json_table_td_value
                                classtd: classcolumn+'-'+i
                                value: to_print
                                data_to_expand: false
                                col: 0
                                classname: 'noclass'
                        return true

                $('.'+classcolumn) .remove();


        insert_columns: (column) ->
            console.log


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
            console.log 'mouseup'
            @mouse_down = false
            @.$('.json_table').toggleClass('resizing', false)

        render: (query, result, type) =>
            @current_result = result

            @.$el.html @template
                query: query

            if @current_result.length is 0
                @.$('.results').html @template_no_result
                return @

            #type = "json", "table", "raw"
            if !type?
                if @current_result.length is 1
                    type = "json"
                else
                    type = "table"

            switch type
                when  'json'
                     @.$('.results').html @json_to_tree @current_result
                when  'table'
                     @.$('.results').html @json_to_table @current_result
                when  'raw'
                     @.$('.results').html @json_to_tree @current_result

            return @

        toggle_collapse: (event) =>
            @.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed')
            @.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed')
            @.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed')
            @.$(event.target).toggleClass('jt_arrow_hidden')

       
        handle_keypress: (event) =>
            if event.which is 13 and !event.shiftKey
                event.preventDefault()
                @.$(event.target).blur()

        on_editable_blur: (data) ->
            @send_update(data)

        #TODO complete method
        #TODO change color
        #TOOD handle change type
        send_update: (target) ->
            console.log 'update'

    class @DefaultView extends Backbone.View
        className: 'helper_view'
        template: Handlebars.compile $('#dataexplorer_default_view-template').html()
        template_query: Handlebars.compile $('#dataexplorer_query_element-template').html()
        template_query_list: Handlebars.compile $('#dataexplorer_query_list-template').html()


        history_queries: []

        initialize :->
            namespaces.on 'all', @render

        add_query: (query) =>
            @history_queries.unshift query
            # Let's check if the list already exists
            if @history_queries.length is 1
                @.$('.history_query_container').html @template_query_list

            #TODO Trunk query
            @.$('.history_query_list').prepend @template_query 
                query: query

        render: => 
            json = {}
            json.namespaces = []
            for namespace in namespaces.models
                json.namespaces.push
                    name: namespace.get('name')

            json.has_namespaces = if json.namespaces.length is 0 then false else true

            json.has_old_queries = if @history_queries.length is 0 then false else true
            json.old_queries = @history_queries

            @.$el.html @template json
        
            return @

        destroy: ->
            namespaces.off()

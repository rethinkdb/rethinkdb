#TODO destroy views
module 'DataExplorerView', ->
    class @Container extends Backbone.View
        className: 'dataexplorer_container'
        template: Handlebars.compile $('#dataexplorer_view-template').html()

        events: ->
            'keypress .input_query': 'handle_keypress'
            'click .clear_query': 'clear_query'
            'click .execute_query': 'execute_query'

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
                    messages: 52
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
                result.push
                    id: '_97a54c09-112e-4e32-86f8-24522e6e1df4'
                    login: 'michel'
                    first_name: 'Michel'
                    last_name: 'Tu'
                    messages: 52
                    inscription_date: '04/26/1998'
                    citizen_id: null
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
            
                result.push
                    id: '_97a54c09-112e-4e32-86f8-24522e6e1df4'
                    login: 'michel'
                    first_name: 'Michel'
                    last_name: 'Tu'
                    messages: 52
                    inscription_date: '04/26/1998'
                    citizen_id: null
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

            @data_container.render(query, result)

        make_suggestion: ->
            console.log 'suggestion'
            return @

        clear_query: =>
            @.$('.input_query').val ''
            @.$('.input_query').focus()

 
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
            'object': Handlebars.compile $('#dataexplorer_result_json_tree_object-template').html()
            'array': Handlebars.compile $('#dataexplorer_result_json_tree_array-template').html()

        events:
            'click .jt_arrow': 'toggle_collapse'



        current_result: []

        initialize: ->


        json_to_tree: (result) =>
            return @template_json_tree.container 
                tree: @json_to_node(result)

        #TODO catch RangeError: Maximum call stack size exceeded
        #TODO check special characters
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
                    sub_values[sub_values.length-1]['is_last'] = true
                    return @template_json_tree.array
                        values: sub_values
            else if value_type is 'object'
                sub_values = []
                for key of value
                    last_key = key
                    sub_values.push
                        key: key
                        value: @json_to_node value[key]

                sub_values[sub_values.length-1]['is_last'] = true

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
                else
                    return @template_json_tree.span_with_quotes
                        classname: 'jt_string'
                        value: value
            else if value_type is 'boolean'
                return @template_json_tree.span
                    classname: 'jt_bool'
                    value: value
 


            #TODO check special characters
            #TODO what to do with new line?




        render: (query, result, type) =>
            @current_result = result

            @.$el.html @template
                query: query

            if @current_result.length is 0
                @.$('.results').html @template_no_result
                return @

            #type = "json", "table", "raw"
            if !type?
                if @current_result.length < 10 #is 1
                    type = "json"
                else
                    type = "table"

            switch type
                when  'json'
                     @.$('.results').html @json_to_tree @current_result
                when  'table'
                     @.$('.results').html @template_json
                when  'raw'
                     @.$('.results').html @template_json
   
            return @

        toggle_collapse: (event) =>
            @.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed')
            @.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed')
            @.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed')
            @.$(event.target).toggleClass('jt_arrow_hidden')

       

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
                    id: namespace.get('id')
                    name: namespace.get('name')

            json.has_namespaces = if json.namespaces.length is 0 then false else true

            json.has_old_queries = if @history_queries.length is 0 then false else true
            json.old_queries = @history_queries

            @.$el.html @template json
        
            return @


        destroy: ->
            namespaces.off 

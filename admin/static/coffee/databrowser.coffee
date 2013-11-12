# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'DataBrowserView', ->
    @state =
        view: 'table'
        last_keys: []
        last_columns_size: {}

    class @Container extends Backbone.View
        id: 'databrowser'
        template: Handlebars.templates['databrowser_view-template']
        actions_template: Handlebars.templates['databrowser_actions_container-template']
        events:
            'change .list_namespaces': 'handle_select_namespace'
        token: 0
        num_docs_per_page: 100

        initialize: (state) =>
            @state = state

            @driver_handler = new DataExplorerView.DriverHandler
                on_success: @success_on_connect
                on_fail: @error_on_connect

            @results_view = new DataBrowserView.ResultView
                container: @
                num_docs_per_page: @num_docs_per_page

 

        render: =>
            @$el.html @template()

            @render_actions()
            @$('.results_container').html @results_view.render().$el
            return @

        post_render: =>
            # TODO Tweak a little?
            @$(".list_namespaces").chosen()
            
        render_actions: =>
            dbs_hash = {}
            for db in databases.models
                dbs_hash[db.get('id')] =
                    name: db.get 'name'
                    namespaces: []

            for namespace in namespaces.models
                dbs_hash[namespace.get('database')].namespaces.push
                    name: namespace.get 'name'
                    primary_key: namespace.get 'primary_key'

            # Sort namespaces
            for id, db of dbs_hash
                db.namespaces.sort (a, b) -> if a.name>b.name then 1 else -1

            # Create an array for handlebars
            dbs = []
            for id, db of dbs_hash
                dbs.push db

            dbs.sort (a, b) -> if a.name>b.name then 1 else -1

            @$('.action_container').html @actions_template
                databases: dbs


        handle_select_namespace: (event) =>
            @get_results
                db: @$(event.currentTarget).find(':selected').parent().attr 'value'
                namespace: @$(event.currentTarget).val()
                primary_key: @$(event.currentTarget).find(':selected').data 'primary_key'
                page: 0
                order_by: [@$(event.currentTarget).find(':selected').data('primary_key')]
                sort_by_index: true
                asc: true

        show_loading: =>
            @$('.loading_query_img').show()
        hide_loading: =>
            @$('.loading_query_img').hide()

        get_results: (args) =>
            @db = args.db
            @namespace = args.namespace
            @primary_key = args.primary_key
            @page = args.page
            @order_by = args.order_by # That's an array!
            @sort_by_index = args.sort_by_index
            @asc = args.asc

            skip_value = @page*@num_docs_per_page

            if @connection?
                @token++
                @show_loading()

                if @sort_by_index is true
                    #TODO Add docs about this behavior -- Having a dot in the field name produce an unexpected behavior
                    #Or having an index that has the same name as a field but doesn't map to the field
                    if @asc is true
                        query = r.expr({
                            count: r.db(@db).table(@namespace).count()
                            indexes: r.db(@db).table(@namespace).indexList()
                            documents: r.db(@db).table(@namespace).orderBy({index: @order_by.join('.')}).skip(skip_value).limit(@num_docs_per_page).coerceTo('ARRAY')
                        })
                    else
                        query = r.expr({
                            count: r.db(@db).table(@namespace).count()
                            indexes: r.db(@db).table(@namespace).indexList()
                            documents: r.db(@db).table(@namespace).orderBy({index: r.desc(@order_by.join('.'))}).skip(skip_value).limit(@num_docs_per_page).coerceTo('ARRAY')
                        })
                else
                    order_by_reql = r.row
                    for field in @order_by
                        order_by_reql = order_by_reql(field)
                    if @asc is true
                        query = r.expr({
                            count: r.db(@db).table(@namespace).count()
                            indexes: r.db(@db).table(@namespace).indexList()
                            documents: r.db(@db).table(@namespace).orderBy(order_by_reql).skip(skip_value).limit(@num_docs_per_page).coerceTo('ARRAY')
                        })
                    else
                        query = r.expr({
                            count: r.db(@db).table(@namespace).count()
                            indexes: r.db(@db).table(@namespace).indexList()
                            documents: r.db(@db).table(@namespace).orderBy(r.desc(order_by_reql)).skip(skip_value).limit(@num_docs_per_page).coerceTo('ARRAY')
                        })
                query.private_run
                    connection: @driver_handler.connection,
                    timeFormat: "raw"
                , @generate_callback_run(@token)

            else
                #TODO
                console.log 'Connection not ready'
            
        success_on_connect: (connection) =>
            @connection = connection

        error_on_connect: =>
            # TODO
            console.log 'Fail to open a new connection'

        render_error: (query, err, js_error) =>
            @.$el.html @error_template
                query: query
                error: err.toString().replace(/^(\s*)/, '')
                js_error: js_error is true
            return @


        generate_callback_run: (token) =>
            callback = (error, result) =>
                if @token is token
                    @hide_loading()

                    if error?
                        @results_view.render_error
                            error: error
                            db: @db
                            namespace: @namespace
                            page: @page
                    else
                        @results_to_display = []

                        # The primary key is always a valid index
                        result.indexes.push @primary_key

                        @results_view.render_result
                            results: result.documents
                            count: result.count
                            indexes: result.indexes
                            db: @db
                            namespace: @namespace
                            primary_key: @primary_key
                            page: @page
                            order_by: @order_by
                            sort_by_index: @sort_by_index
                            asc: @asc
                # else, we just got back outdated results so we just ignore the event

            return callback
            
        callback_run: (err, cursor) =>
            if (err)
                # TODO
                console.log err
            else
                console.log 'TOOD'

        #destroy: =>
        #TODO!

    class @ResultView extends DataExplorerView.SharedResultView
        last_keys: []
        last_columns_size: {}
        limit_sort_without_index: 1000 #TODO Update
        view: 'table'
        template: Handlebars.templates['databrowser_result_container-template']
        error_template: Handlebars.templates['databrowser-error-template']
        events: ->
            _.extend super,
                'change .jumper': 'jump_page'
                'click .next_page': 'next_page'
                'click .previous_page': 'previous_page'
                'click .index_sort_desc': 'index_sort_desc'
                'click .index_sort_asc': 'index_sort_asc'
                'click .sort_desc': 'sort_desc'
                'click .sort_asc': 'sort_asc'

        json_to_table: (result, primary_key, can_sort, indexes) =>
            {flatten_attr, result} = super result

            if indexes?
                indexes_hash = {}
                for index in indexes
                    indexes_hash[index] = true


                # Check if there is an index
                for attr in flatten_attr
                    name = attr.prefix_str+attr.key
                    if name of indexes_hash
                        attr.has_index = true
                    else
                        if can_sort is true
                            attr.can_sort = true
                        else
                            attr.can_sort = false

            for attr in flatten_attr
                path = []
                path.push.apply path, attr.prefix
                path.push attr.key
                attr.path = JSON.stringify path

            # The primary key comes first
            for attr, i in flatten_attr
                if attr.prefix_str is '' and attr.key is primary_key
                    pk = flatten_attr.splice i, 1
                    break
            flatten_attr.unshift.apply flatten_attr, pk

            # Reset col
            for attr, i in flatten_attr
                attr.col = i


            return @template_json_table.container
                table_attr: @json_to_table_get_attr flatten_attr
                table_data: @json_to_table_get_values
                    result: result
                    flatten_attr: flatten_attr




        index_sort_desc: (event) =>
            @sort_results
                event: event
                sort_by_index: true
                asc: false
        index_sort_asc: (event) =>
            @sort_results
                event: event
                sort_by_index: true
                asc: true
        sort_asc: (event) =>
            @sort_results
                event: event
                sort_by_index: false
                asc: true
        sort_desc: (event) =>
            @sort_results
                event: event
                sort_by_index: false
                asc: false

        sort_results: (args) =>
            args.event.preventDefault()
            @container.get_results
                db: @db
                namespace: @namespace
                primary_key: @primary_key
                page: 0
                order_by: @$(args.event.currentTarget).data('path')
                sort_by_index: args.sort_by_index
                asc: args.asc

        jump_page: (event) =>
            #TODO check if selected, if it's the case, do nothing
            page = parseInt @$(event.currentTarget).val()
            @container.get_results
                db: @db
                namespace: @namespace
                primary_key: @primary_key
                page: page
                order_by: @order_by
                sort_by_index: @sort_by_index
                asc: @asc

        next_page: (event) =>
            event.preventDefault()
            @container.get_results
                db: @db
                namespace: @namespace
                primary_key: @primary_key
                page: @page+1
                order_by: @order_by
                sort_by_index: @sort_by_index
                asc: @asc


        # TODO Add check for the value of page
        previous_page: (event) =>
            event.preventDefault()
            @container.get_results
                db: @db
                namespace: @namespace
                primary_key: @primary_key
                page: @page-1
                order_by: @order_by
                sort_by_index: @sort_by_index
                asc: @asc


        initialize: (args) =>
            @template_json_table = {}
            #TODO Please... clean this
            for key, value of @__proto__.__proto__.template_json_table
                @template_json_table[key] = value

            @template_json_table['tr_attr'] = Handlebars.templates['databrowser_result_json_table_tr_attr-template']

            @container = args.container
            @num_docs_per_page = args.num_docs_per_page
            @skip = 0

            $(window).mousemove @handle_mousemove
            $(window).mouseup @handle_mouseup
            super


        render: =>
            return @

        render_result: (args) =>
            # Store the state
            @results = args.results
            @count = args.count
            @indexes = args.indexes
            @db = args.db
            @namespace = args.namespace
            @primary_key = args.primary_key
            @page = args.page
            @order_by = args.order_by
            @sort_by_index = args.sort_by_index
            @asc = args.asc

            # Extra variables for convenience purpose
            @results_array = null # if @results is not an array (possible starting from 1.4), we will transform @results_array to [@results] for the table view
            @skip_value = args.page*@num_docs_per_page

            # TODO Refactor DataExplorerView.Shared... to remove that
            @metadata =
                skip_value: @skip_value


            pages = []
            i = 0
            while i*@num_docs_per_page < @count-1
                pages.push
                    page: i+1
                    value: i
                    selected: i is @page
                i++

            @.$el.html @template
                pages: pages
                previous: @skip_value > 0
                next: @page < pages.length-1

            @$('.jumper').chosen()

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
                        @.$('.table_view').html @json_to_table @results, @primary_key, @count<@limit_sort_without_index, @indexes
                    else
                        if not @results_array?
                            @results_array = []
                            @results_array.push @results
                        @.$('.table_view').html @json_to_table @results_array
                    @$('.results').hide()
                    @$('.table_view_container').show()
                    @.$('.link_to_table_view').addClass 'active'
                    @.$('.link_to_table_view').parent().addClass 'active'

                    # TODO we should just check if previous_keys is included in last_keys
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

        render_error: (args) =>
            @$el.html @error_template
                error: args.error.toString().replace(/^(\s*)/, '')
                db: args.db
                namespace: args.namespace
                page: args.page


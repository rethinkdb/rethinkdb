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
            'change .list_namespaces': 'show_results'
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


        # TODO Rename this method
        show_results: (event) =>
            @db = @$(event.currentTarget).find(':selected').parent().attr 'value'
            @namespace = @$(event.currentTarget).val()
            @primary_key = @$(event.currentTarget).find(':selected').data 'primary_key'
            @page = 0
            
            @get_results @page

        show_loading: =>
            @$('.loading_query_img').show()
        hide_loading: =>
            @$('.loading_query_img').hide()

        get_page: =>
            @page

        get_results: (page) =>
            skip_value = page*@num_docs_per_page
            @page = page

            if @connection?
                @token++
                @show_loading()
                r.expr({
                    count: r.db(@db).table(@namespace).count()
                    documents: r.db(@db).table(@namespace).orderBy({index: @primary_key}).skip(skip_value).limit(@num_docs_per_page).coerceTo('ARRAY')
                }).private_run
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
                        #@retrieve_results cursor, token, @num_docs_to_display

                        @results_view.render_result
                            results: result.documents
                            page: @page
                            count: result.count
                            primary_key: @primary_key
                # else, we just got back outdated results so we just ignore the event

            return callback
            
        callback_run: (err, cursor) =>
            if (err)
                # TODO
                console.log err
            else
                console.log 'TOOD'

        #events:
        
        #destroy: =>

    class @ResultView extends DataExplorerView.SharedResultView
        last_keys: []
        last_columns_size: {}
        view: 'table'
        template: Handlebars.templates['databrowser_result_container-template']
        error_template: Handlebars.templates['databrowser-error-template']
        events: ->
            _.extend super,
                'change .jumper': 'jump_page'
                'click .next_page': 'next_page'
                'click .previous_page': 'previous_page'


        jump_page: (event) =>
            #TODO check if selected, if it's the case, do nothing
            page = parseInt @$(event.currentTarget).val()
            @container.get_results page



        next_page: (event) =>
            event.preventDefault()
            @container.get_results @container.get_page()+1

        # TODO Add check for the value of page
        previous_page: (event) =>
            event.preventDefault()
            @container.get_results @container.get_page()-1

        initialize: (args) =>
            @container = args.container
            @num_docs_per_page = args.num_docs_per_page
            @skip = 0

            $(window).mousemove @handle_mousemove
            $(window).mouseup @handle_mouseup
            super


        render: =>
            return @

        render_result: (args) =>
            if args?
                if args.results isnt undefined
                    @results = args.results
                    @results_array = null # if @results is not an array (possible starting from 1.4), we will transform @results_array to [@results] for the table view
                if args.count?
                    @count = args.count
                if args.primary_key?
                    @primary_key = args.primary_key
                if args.page?
                    @skip_value = args.page*@num_docs_per_page
                    @page = args.page
                    # TODO Refactor the view in the DE so we don't have this @metadata object
                    @metadata =
                        skip_value: @skip_value

            pages = []
            i = 0
            while i*@num_docs_per_page < @count-1
                pages.push
                    page: i+1
                    value: i
                    selected: i is @container.get_page()
                i++

            @.$el.html @template
                pages: pages
                previous: @skip_value > 0
                next: @container.get_page() < pages.length-1

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
                        @.$('.table_view').html @json_to_table @results, @primary_key
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


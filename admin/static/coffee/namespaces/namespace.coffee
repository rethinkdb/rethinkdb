# Copyright 2010-2012 RethinkDB, all rights reserved.
# Namespace view
module 'NamespaceView', ->
    class @NotFound extends Backbone.View
        template: Handlebars.templates['element_view-not_found-template']
        initialize: (id) ->
            @id = id
        render: =>
            @.$el.html @template
                id: @id
                type: 'table'
                type_url: 'tables'
                type_all_url: 'tables'
            return @

    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'
        template: Handlebars.templates['namespace_view-container-template']
        alert_tmpl: Handlebars.templates['modify_shards-alert-template']

        events: ->
            'click .tab-link': 'change_route'
            'click .close': 'close_alert'
            'click .change_shards-link': 'change_shards'
            # operations in the dropdown menu
            'click .operations .rename': 'rename_namespace'
            'click .operations .delete': 'delete_namespace'

        initialize: ->
            log_initial '(initializing) namespace view: container'

            @model.load_key_distr()

            # Panels for namespace view
            @title = new NamespaceView.Title(model: @model)
            @profile = new NamespaceView.Profile(model: @model)
            @replicas = new NamespaceView.Replicas(model: @model)
            @secondary_indexes_view = new NamespaceView.SecondaryIndexesView(model: @model)
            @shards = new NamespaceView.Sharding(model: @model)
            @server_assignments = new NamespaceView.ServerAssignments(model: @model)
            @performance_graph = new Vis.OpsPlot(@model.get_stats_for_performance,
                width:  564             # width in pixels
                height: 210             # height in pixels
                seconds: 73             # num seconds to track
                type: 'table'
            )

            namespaces.on 'remove', @check_if_still_exists

        check_if_still_exists: =>
            exist = false
            for namespace in namespaces.models
                if namespace.get('id') is @model.get('id')
                    exist = true
                    break
            if exist is false
                window.router.navigate '#tables'
                window.app.index_namespaces
                    alert_message: "The table <a href=\"#tables/#{@model.get('id')}\">#{@model.get('name')}</a> could not be found and was probably deleted."

        change_route: (event) =>
            # Because we are using bootstrap tab. We should remove them later. TODO
            window.router.navigate @.$(event.target).attr('href')

        render: =>
            log_render '(rendering) namespace view: container'

            @.$el.html @template
                namespace_id: @model.get 'id'

            # fill the title of this page
            @.$('.main_title').html @title.render().$el

            # Add the replica and shards views
            @.$('.profile').html @profile.render().$el
            @.$('.performance-graph').html @performance_graph.render().$el

            # Display the replicas
            @.$('.replication').html @replicas.render().el

            # Display the shards
            @.$('.sharding').html @shards.render().el

            # Display the server assignments
            @.$('.server-assignments').html @server_assignments.render().el

            # Display the secondary indexes
            @.$('.secondary_indexes').html @secondary_indexes_view.render().el


            return @

        close_alert: (event) ->
            event.preventDefault()
            $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())

        change_shards: (event) =>
            event.preventDefault()
            @.$('#namespace-sharding-link').tab('show')

        change_pinning: (event) =>
            event.preventDefault()
            @.$('#namespace-pinning-link').tab('show')
            $(event.currentTarget).parent().parent().slideUp('fast', -> $(this).remove())

        # Rename operation
        rename_namespace: (event) =>
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'table'
            rename_modal.render()

        # Delete operation
        delete_namespace: (event) ->
            event.preventDefault()
            remove_namespace_dialog = new NamespaceView.RemoveNamespaceModal
            namespace_to_delete = @model

            remove_namespace_dialog.on_success = (response) =>
                window.router.navigate '#tables'
                window.app.index_namespaces
                    alert_message: "The table #{@model.get('name')} was successfully deleted."
                namespaces.remove @model.get 'id'

            remove_namespace_dialog.render [@model]

        destroy: =>
            namespaces.off 'remove', @check_if_still_exists
            @model.clear_timeout()
            @title.destroy()
            @profile.destroy()
            @replicas.destroy()
            @shards.destroy()
            @server_assignments.destroy()
            @performance_graph.destroy()
            @secondary_indexes_view.destroy()

    # NamespaceView.Title
    class @Title extends Backbone.View
        className: 'namespace-info-view'
        template: Handlebars.templates['namespace_view_title-template']
        initialize: ->
            @name = @model.get('name')
            @model.on 'change:name', @update

        update: =>
            if @name isnt @model.get('name')
                @name = @model.get('name')
                @render()

        render: =>
            @.$el.html @template
                name: @name
            return @

        destroy: =>
            @model.off 'change:name', @update

    # Profile view
    class @Profile extends Backbone.View
        className: 'namespace-profile'
        template: Handlebars.templates['namespace_view-profile-template']

        initialize: ->
            # @model is a namespace.  somebody is supposed to pass model: namespace to the constructor.
            @model.on 'all', @render
            directory.on 'all', @render
            progress_list.on 'all', @render
            log_initial '(initializing) namespace view: replica'
            @data = {}

        render: =>
            data = DataUtils.get_namespace_status(@model.get('id'))

            #Compute the total number of keys
            data.total_keys_available = false
            if @model.get('key_distr')?
                # we can't use 'total_keys' because we need to
                # distinguish between zero and unavailable.
                data.total_keys_available = true
                data.total_keys = 0
                for key of @model.get('key_distr')
                    data.total_keys += parseInt @model.get('key_distr')[key]
            if data.total_keys_available is true and data.total_keys? and data.total_keys is data.total_keys # Check for NaN, just in case
                data.total_keys = DataUtils.approximate_count data.total_keys

            data.stats_up_to_date = true
            for machine in machines.models
                if machine.get('stats')? and @model.get('id') of machine.get('stats') and machine.is_reachable
                    if machine.get('stats_up_to_date') is false
                        data.stats_up_to_date = false
                        break

            if not _.isEqual @data, data
                @data = data
                @.$el.html @template data

            return @

        destroy: =>
            @model.off 'all', @render
            directory.off 'all', @render
            progress_list.off 'all', @render

    class @SecondaryIndexesView extends Backbone.View
        template: Handlebars.templates['namespace_view-secondary_indexes-template']
        alert_message_template: Handlebars.templates['secondary_indexes-alert_msg-template']
        error_template: Handlebars.templates['secondary_indexes-error-template']
        events:
            'click .create_link': 'show_add_index'
            'click .create_btn': 'create_index'
            'keydown .new_index_name': 'handle_keypress'
            'click .cancel_btn': 'hide_add_index'
            'click .reconnect_link': 'init_connection'
            'click .close_hide': 'hide_alert'
        error_interval: 5*1000 # In case of an error, we try to retrieve the secondary index in 5 seconds
        normal_interval: 10*1000 # Retrieve secondary indexes every minute
        short_interval: 1000 # Interval when an index is being created

        initialize: (args) =>
            @model = args.model

            @db = databases.get @model.get('database')
            @update_info()

            @db.on 'change:name', @update_info
            @model.on 'change:name', @update_info

            @indexes = {}
            @indexes_count = 0

            @adding_index = false
            @loading = true

            @init_connection()

        # Update table and database names
        update_info: =>
            @db_name = @db.get 'name'
            @table = @model.get 'name'

        # Create a connection and fetch secondary indexes
        init_connection: (event) =>
            if event?
                event.preventDefault()

            @loading = true
            @driver_handler = new DataExplorerView.DriverHandler
                container: @
            @get_indexes
                timer: true

        error_on_connect: (error) =>
            console.log '*************** error ***************'
            console.log error
            @render_error
                index_list_fail: true
            @timeout = setTimeout @get_indexes, @error_interval

        get_indexes: (args) =>
            #@driver_handler.close_connection()
            @driver_handler.create_connection (error, connection) =>
                if (error)
                    @error_on_connect error
                else
                    if args?.timer is true
                        r.db(@db_name).table(@table).indexStatus().private_run connection, (err, result) =>
                            @on_index_list_repeat err, result
                            setTimeout ->
                                connection.close()
                            , 0
                    else
                        r.db(@db_name).table(@table).indexStatus().private_run connection, (err, result) =>
                            @on_index_list err, result
                            setTimeout ->
                                connection.close()
                            , 0
            , 0, @error_on_connect

        on_index_list_repeat: (err, result) =>
            @on_index_list err, result, true

        on_index_list: (err, result, timer) =>
            if @loading is true
                @loading = false
                @render()
            if err?
                @render_error
                    index_list_fail: true
                @timeout = setTimeout @get_indexes, @error_interval
            else
                index_hash = {}
                indexes_not_ready = 0
                for index, i in result
                    index_hash[index.index] = true

                    if index.ready is false
                        indexes_not_ready++

                    if @indexes[index.index]?
                        @indexes[index.index].update index
                    else # new index
                        position_new_index = -1
                        for displayed_index of @indexes
                            if displayed_index < index.index
                                position_new_index++

                        @indexes[index.index] = new NamespaceView.SecondaryIndexView index, @
                
                        if position_new_index is -1
                            @$('.list_secondary_indexes').prepend @indexes[index.index].render().$el
                        else
                            @$('.index_container').eq(position_new_index).after @indexes[index.index].render().$el

                if timer
                    if @timeout?
                        clearTimeout @timeout
                    if indexes_not_ready > 0
                        @timeout = setTimeout =>
                            @get_indexes
                                timer: true
                        , @short_interval
                    else
                        @timeout = setTimeout =>
                            @get_indexes
                                timer: true
                        , @normal_interval


                count = 0
                for name, index of @indexes
                    if not index_hash[name]?
                        @$('.index-'+name).remove()
                        delete @indexes[name]
                    else
                        count++
                if count is 0
                    @$('.no_index').show()
                else
                    @$('.no_index').hide()

        render_error: (args) =>
            @$('.alert_error_content').html @error_template args
            @$('.main_alert_error').slideDown 'fast'
            @$('.main_alert').slideUp 'fast'

        render_feedback: (args) =>
            @$('.alert_content').html @alert_message_template args
            @$('.main_alert').slideDown 'fast'
            @$('.main_alert_error').slideUp 'fast'

        # Delete a secondary index
        delete_index: (index) =>
            #@driver_handler.close_connection()
            @driver_handler.create_connection (error, connection) =>
                if (error)
                    @error_on_connect error
                else
                    r.db(@db_name).table(@table).indexDrop(index).private_run connection, (err, result) =>
                        @on_drop err, result, index
                        setTimeout ->
                            connection.close()
                        , 0


            , @id_execution, @error_on_connect


        remove_index: (index) =>
            @$('.index_container[data-name="'+index+'"]').slideUp 200, ->
                @remove()
            delete @indexes[index]

            count = 0
            for key of @indexes
                count++
                break
            if count is 0
                @$('.no_index').hide()

        # Callback for indexDrop()
        on_drop: (err, result, index) =>
            if err?
                @render_error
                    delete_fail: true
                    message: err.msg.replace('\n', '<br/>')
            else if result?.dropped isnt 1
                @render_error
                    delete_fail: true
                    message: "Unknown error"
            else
                @remove_index index
                @render_feedback
                    delete_ok: true
                    name: index

        render: =>
            @$el.html @template
                loading: @loading
                adding_index: @adding_index
            return @

        # Show the form to add a secondary index
        show_add_index: (event) =>
            event.preventDefault()
            @$('.add_index_li').slideDown 'fast'
            @$('.create_container').slideUp 'fast'
            @$('.new_index_name').focus()
        
        # Hide the form to add a secondary index
        hide_add_index: =>
            @$('.add_index_li').slideUp 'fast'
            @$('.create_container').slideDown 'fast'
            @$('.new_index_name').val ''

        # We catch enter and esc when the user is writing a secondary index name
        handle_keypress: (event) =>
            if event.which is 13 # Enter
                event.preventDefault()
                @create_index()
            else if event.which is 27 # ESC
                event.preventDefault()
                @hide_add_index()
       
        on_fail_to_connect: =>
            @loading = false
            @render_error
                connect_fail: true
            return @

        create_index: =>
            @$('.create_btn').prop 'disabled', 'disabled'
            @$('.cancel_btn').prop 'disabled', 'disabled'

            index_name = $('.new_index_name').val()
            #@driver_handler.close_connection()
            @driver_handler.create_connection (error, connection) =>
                if (error)
                    @error_on_connect error
                else
                    r.db(@db_name).table(@table).indexCreate(index_name).private_run connection, (err, result) =>
                        @on_create err, result, index_name, index_name
                        # We need a setimeout to avoid running two queries on the same connection
                        setTimeout ->
                            connection.close()
                        , 0

            , 0, @error_on_connect


        # Callback on indexCreate()
        on_create: (err, result, index_name) =>
            @$('.create_btn').prop 'disabled', false
            @$('.cancel_btn').prop 'disabled', false
            that = @
            if err?
                @render_error
                    create_fail: true
                    message: err.msg.replace('\n', '<br/>')
            else
                @$('.no_index').hide()
                
                position_new_index = -1
                for displayed_index of @indexes
                    if displayed_index < index_name
                        position_new_index++

                if not @indexes[index_name]?
                    @indexes[index_name] = new NamespaceView.SecondaryIndexView
                        index: index_name
                        ready: false
                        blocks_processed: 0
                        blocks_total: Infinity
                    , @

                    if position_new_index is -1
                        @$('.list_secondary_indexes').prepend @indexes[index_name].render().$el
                    else
                        @$('.index_container').eq(position_new_index).after @indexes[index_name].render().$el


                if @timeout?
                    clearTimeout @timeout
                @timeout = setTimeout =>
                    @get_indexes
                        timer: true
                , 1000 # We delay the call of 1 second to retrieve a better estimate of the total number of blocks

                @render_feedback
                    create_ok: true
                    name: index_name

                @hide_add_index()

        # Hide alert BUT do not remove it
        hide_alert: (event) ->
            if event? and @$(event.target)?.data('name')?
                @deleting_secondary_index = null
            event.preventDefault()
            $(event.target).parent().slideUp 'fast'
        
        destroy: =>
            if @timeout?
                clearTimeout @timeout
            @db.off 'change:name', @update_info
            @model.off 'change:name', @update_info
            for key, view of @indexes
                view.destroy()

    class @SecondaryIndexView extends Backbone.View
        template: Handlebars.templates['namespace_view-secondary_index-template']
        progress_template: Handlebars.templates['simple_progressbar-template']
        events:
            'click .delete_link': 'confirm_delete'
            'click .delete_index_btn': 'delete_index'
            'click .cancel_delete_btn': 'cancel_delete'

        tagName: 'li'
        className: 'index_container'

        initialize: (index, container) =>
            @name = index.index
            @ready = index.ready
            @progress_bar = null
            @container = container
            @$el.attr('data-name', @name)
            @blocks_processed = index.blocks_processed
            @blocks_total = index.blocks_total


        update: (args) =>
            if args.ready is false
                @blocks_processed = args.blocks_processed
                @blocks_total = args.blocks_total
                @render_progress_bar()
            else
                if @ready isnt args.ready
                    @blocks_processed = @blocks_total
                    @render_progress_bar()

                @ready = args.ready

            

        render: =>
            @$el.html @template
                is_empty: false
                name: @name
                ready: @ready
            if @ready is false
                @render_progress_bar()
            @

        render_progress_bar: =>
            if @progress_bar?
                @progress_bar.render @blocks_processed, @blocks_total,
                    got_response: true
                    check: true
                    , => @render()
            else
                @progress_bar = new UIComponents.OperationProgressBar @progress_template
                @$('.progress_li').html @progress_bar.render(0, Infinity, {new_value: true, check: true}).$el

        # Show a confirmation before deleting a secondary index
        confirm_delete: (event) =>
            event.preventDefault()
            @$('.alert_confirm_delete').slideDown 'fast'

        delete_index: =>
            @$('.btn').prop 'disabled', 'disabled'
            @container.delete_index @name

        # Close to hide_alert, but the way to reach the alert is slightly different than with the x link
        cancel_delete: ->
            @$('.alert_confirm_delete').slideUp 'fast'

        destroy: =>
            if @progress_bar?
                @progress_bar.destroy()

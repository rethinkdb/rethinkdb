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
        content_template: Handlebars.templates['namespace_view-secondary_indexes-content-template']
        add_index_template: Handlebars.templates['namespace_view-secondary_indexes-add-template']
        alert_message_template: Handlebars.templates['secondary_indexes-alert_msg-template']
        error_template: Handlebars.templates['secondary_indexes-error-template']
        events:
            'click .delete_link': 'confirm_delete'
            'click .delete_index_btn': 'delete_secondary_index'
            'click .cancel_delete_btn': 'cancel_delete'
            'click .create_link': 'show_add_index'
            'click .create_btn': 'create_index'
            'keydown .secondary_index_name': 'handle_keypress'
            'click .cancel_btn': 'hide_add_index'
            'click .reconnect_link': 'init_connection'
            'click .close_hide': 'hide_alert'
        error_interval: 5*1000 # In case of an error, we try to retrieve the secondary index in 5 seconds
        normal_interval: 60*1000 # Retrieve secondary indexes every minute
        short_interval: 1*1000 # Retrieve secondary indexes every 2 seconds (when an index is being created)

        initialize: =>
            @init_connection()
            @secondary_indexes = null
            @deleting_secondary_index = null
            @indexes_progress = {}

        # Create a connection and fetch secondary indexes
        init_connection: (event) =>
            if event?
                event.preventDefault()

            @loading = true
            @driver_handler = new DataExplorerView.DriverHandler
                on_success: @set_interval_get_indexes
                on_fail: @on_fail_to_connect
                dont_timeout_connection: true
            @secondary_indexes = null # list of secondary indexes available
            @deleting_secondary_index = null # The secondary index being deleted (with a confirm box displayed)

            @db = databases.get(@model.get('database'))
            @db.on 'change:name', @save_name
            @model.on 'change:name', @save_name
            @save_name()

        save_name: =>
            @table = @model.get('name')
            @db_name = @db.get('name')

        # Show a confirmation before deleting a secondary index
        confirm_delete: (event) =>
            event.preventDefault()
            deleting_secondary_index = @$(event.target).data('name')
            @adding_index = false # Less states = better, so we hide the form to create an index if the user is about to delete one
            if deleting_secondary_index isnt @deleting_secondary_index
                @deleting_secondary_index = deleting_secondary_index
                @$('.alert_confirm_delete').slideUp 'fast'
                @$('.alert_confirm_delete_'+@deleting_secondary_index).slideDown 'fast'
            else
                @deleting_secondary_index = null
                @$('.alert_confirm_delete').slideUp 'fast'

        # Delete a secondary index
        delete_secondary_index: (event) =>
            @$(event.target).prop 'disabled', 'disabled'
            @current_secondary_name = @$(event.target).data('name')
            @deleting_secondary_index = @$(event.target).data('name')
            r.db(@db_name).table(@table).indexDrop(@current_secondary_name.toString()).private_run @driver_handler.connection, @on_drop

        # Callback for indexDrop()
        on_drop: (err, result) =>
            if @current_secondary_name is @deleting_secondary_index
                @deleting_secondary_index = null

            if err? or result?.dropped isnt 1
                @loading = false
                @$('.alert_error_content').html @error_template
                    delete_fail: true
                    message: err.msg.replace('\n', '<br/>')
                @$('.main_alert').slideUp 'fast'
                @$('.main_alert_error').slideDown 'fast'
                @get_indexes()

            else
                @get_indexes()
                @$('.alert_content').html @alert_message_template
                    delete_ok: true
                    name: @current_secondary_name
                @$('.main_alert').slideDown 'fast'
                @$('.main_alert_error').slideUp 'fast'


        # Show the form to add a secondary index
        show_add_index: (event) =>
            event.preventDefault()
            @adding_index = true
            @deleting_secondary_index = null
            @render_add_index()
            @$('.alert_confirm_delete').slideUp 'fast'
        
        # Hide the form to add a secondary index
        hide_add_index: =>
            @adding_index = false
            @render_add_index()

        render_add_index: =>
            @$('.add_index_container').html @add_index_template
                show_add_index: @adding_index
            if @adding_index
                @$('.secondary_index_name').focus()

        # Render list of secondary index
        render_content: (args) =>
            that = @
            if not args?
                args = {}

            template_args =
                loading: @loading
                secondary_indexes: @secondary_indexes
                no_secondary_indexes: @secondary_indexes? and @secondary_indexes.length is 0
                secondary_index_name: @secondary_index_name
            @$('.content').html @content_template _.extend template_args, args
            @delegateEvents()

        render: =>
            @$el.html @template()
            @render_content()
            @render_add_index()
            return @
       
        # Retrieve secondary indexes with an interval
        set_interval_get_indexes: =>
            @get_indexes
                set_timeout: true

        get_indexes: (args) =>
            if args?.set_timeout is true
                r.db(@db_name).table(@table).indexStatus().private_run @driver_handler.connection, @on_index_list_repeat
            else
                r.db(@db_name).table(@table).indexStatus().private_run @driver_handler.connection, @on_index_list

        # Callback on indexList
        on_index_list: (err, result) =>
            if err?
                @loading = false
                @render_content
                    error: true
                    index_list: true
            else
                @loading = false
                secondary_indexes = result.sort (a, b) ->
                    if a.name > b.name
                        return 1
                    else if a.name < b.name
                        return -1
                    return 0

                if not _.isEqual @secondary_indexes, secondary_indexes
                    @secondary_indexes = secondary_indexes
                    @render_content()

        # Same as on_index_list except we set a timeout.
        on_index_list_repeat: (err, result) =>
            if err?
                @loading = false
                @render_content
                    error: true
                    index_list: true
                @timeout = setTimeout @set_interval_get_indexes, @error_interval
            else
                short_inteval = false
                @loading = false
                secondary_indexes = result.sort (a, b) ->
                    if a.index > b.index
                        return 1
                    else if a.index < b.index
                        return -1
                    return 0

                for index in secondary_indexes
                    index.is_empty = index.index is ''
                    index.display = @deleting_secondary_index is index.index
                    if index.ready is false
                        # Make sure that the progress bar never goes back
                        new_percentage = Math.floor(index.blocks_remaining/index.blocks_total*100)
                        if (not @indexes_progress[index.index]?) or @indexes_progress[index.index] < new_percentage
                            @indexes_progress[index.index] = new_percentage

                        index.percentage = @indexes_progress[index.index]
                        short_interval = true
                    else
                        if @indexes_progress[index.index]?
                            @$('.alert_content').html @alert_message_template
                                built: true
                                name: index.index
                            @$('.main_alert_error').slideUp 'fast'
                            @$('.main_alert').slideDown 'fast'


                            delete @indexes_progress[index.index]

                if not _.isEqual @secondary_indexes, secondary_indexes
                    @secondary_indexes = secondary_indexes
                    @render_content()

                if short_interval is true
                    @timeout = setTimeout @set_interval_get_indexes, @short_interval
                else
                    @timeout = setTimeout @set_interval_get_indexes, @normal_interval


        on_fail_to_connect: =>
            @loading = false
            @$el.html @template
                error: true
                connect: true
            return @

        # We catch enter and esc when the user is writing a secondary index name
        handle_keypress: (event) =>
            if event.which is 13 # Enter
                event.preventDefault()
                @create_index()
            else if event.which is 27 # ESC
                event.preventDefault()
                @hide_add_index()
            else
                @current_secondary_name = $('.secondary_index_name').val()

        create_index: =>
            @current_secondary_name = $('.secondary_index_name').val()
            r.db(@db_name).table(@table).indexCreate(@current_secondary_name).private_run @driver_handler.connection, @on_create
            @$('.create_btn').prop 'disabled', 'disabled'

        # Callback on indexCreate()
        on_create: (err, result) =>
            that = @

            @$('.create_btn').removeProp 'disabled'
            if err?
                @$('.alert_error_content').html @error_template
                    create_fail: true
                    message: err.msg.replace('\n', '<br/>')
                @$('.main_alert_error').slideDown 'fast'
                @$('.main_alert').slideUp 'fast'
            else
                @adding_index = false
                @render_add_index()
                @get_indexes()
                @$('.alert_content').html @alert_message_template
                    create_ok: true
                    name: @current_secondary_name
                @$('.main_alert_error').slideUp 'fast'
                @$('.main_alert').slideDown 'fast'

            if @timeout?
                clearTimeout @timeout
                # We sligthly delay indexStatus to retrieve more accurate data (for the progress)
                setTimeout ->
                    that.get_indexes
                        set_timeout: true
                    , @short_interval


        # Hide alert BUT do not remove it
        hide_alert: (event) ->
            if event? and @$(event.target)?.data('name')?
                @deleting_secondary_index = null
            event.preventDefault()
            $(event.target).parent().slideUp 'fast'
        
        # Close to hide_alert, but the way to reach the alert is slightly different than with the x link
        cancel_delete: (event) ->
            if event? and @$(event.target)?.data('name')?
                @deleting_secondary_index = null
            @$(event.target).parent().parent().slideUp 'fast'

        destroy: =>
            if @timeout?
                clearTimeout @timeout
            @db.off 'change:name', @save_name
            @model.off 'change:name', @save_name


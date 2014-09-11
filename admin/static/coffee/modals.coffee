# Copyright 2010-2012 RethinkDB, all rights reserved.

module 'Modals', ->
    class @AddDatabaseModal extends UIComponents.AbstractModal
        template: Handlebars.templates['add_database-modal-template']
        templates:
            error: Handlebars.templates['error_input-template']

        class: 'add-database'

        initialize: (databases) =>
            super
            @databases = databases


        render: =>
            super
                modal_title: "Add database"
                btn_primary_text: "Add"
            @$('.focus_new_name').focus()

        on_submit: =>
            super
            @formdata = form_data_as_object($('form', @$modal))

            error = null

            # Validate input
            if @formdata.name is ''
                # Empty name is not valid
                error = true
                @$('.alert_modal').html @templates.error
                    database_is_empty: true
            else if /^[a-zA-Z0-9_]+$/.test(@formdata.name) is false
                # Only alphanumeric char + underscore are allowed
                error = true
                @$('.alert_modal').html @templates.error
                    special_char_detected: true
                    type: 'database'
            else
                # Check if it's a duplicate
                for database in @databases.models
                    if database.get('db') is @formdata.name
                        error = true
                        @$('.alert_modal').html @templates.error
                            database_exists: true
                        break

            if error is null
                driver.run r.dbCreate(@formdata.name), (err, result) =>
                    if (err)
                        @on_error(err)
                    else
                        if result?.created is 1
                            @on_success()
                        else
                            @on_error(new Error("The returned result was not `{created: 1}`"))
            else
                $('.alert_modal_content').slideDown 'fast'
                @reset_buttons()

        on_success: (response) =>
            super()
            window.app.current_view.render_message "The database #{@formdata.name} was successfully created."

    class @RemoveDatabaseModal extends UIComponents.AbstractModal
        template: Handlebars.templates['remove_database-modal-template']
        class: 'remove_database-dialog'

        render: (database_to_delete) ->
            @database_to_delete = database_to_delete

            # Call render of AbstractModal with the data for the template
            super
                modal_title: 'Delete database'
                btn_primary_text: 'Delete'

            @$('.verification_name').focus()


        on_submit: =>
            if @$('.verification_name').val() isnt @database_to_delete.get('name')
                if @.$('.mismatch_container').css('display') is 'none'
                    @.$('.mismatch_container').slideDown('fast')
                else
                    @.$('.mismatch_container').hide()
                    @.$('.mismatch_container').fadeIn()
                @reset_buttons()
                return true

            super

            driver.run r.dbDrop(@database_to_delete.get('name')), (err, result) =>
                if (err)
                    @on_error(err)
                else
                    if result?.dropped is 1
                        @on_success()
                    else
                        @on_error(new Error("The return result was not `{dropped: 1}`"))

        on_success: (response) =>
            if Backbone.history.fragment is 'tables'
                @database_to_delete.destroy()
                super()
            else
                # If the user was on a database view, we have to redirect him
                # If he was on #tables, we are just refreshing
                window.router.navigate '#tables', {trigger: true}

            window.app.current_view.render_message "The database #{@database_to_delete.get('name')} was successfully deleted."

    class @AddTableModal extends UIComponents.AbstractModal
        template: Handlebars.templates['add_table-modal-template']
        templates:
            error: Handlebars.templates['error_input-template']
        class: 'add-table'

        initialize: (databases) =>
            super
            @databases = databases

            @listenTo @databases, 'all', @check_if_can_create_table

            @can_create_table_status = true
            @delegateEvents()


        show_advanced_settings: (event) =>
            event.preventDefault()
            @$('.show_advanced_settings-link_container').fadeOut 'fast', =>
                @$('.hide_advanced_settings-link_container').fadeIn 'fast'
            @$('.advanced_settings').slideDown 'fast'

        hide_advanced_settings: (event) =>
            event.preventDefault()
            @.$('.hide_advanced_settings-link_container').fadeOut 'fast', =>
                $('.show_advanced_settings-link_container').fadeIn 'fast'
            @.$('.advanced_settings').slideUp 'fast'

        # Check if we have a database (if not, we cannot create a table)
        check_if_can_create_table: =>
            if @databases.length is 0
                if @can_create_table_status
                    @.$('.btn-primary').prop 'disabled', true
                    @$('.alert_modal').html @templates.error
                        create_db_first: true
                    $('.alert_modal_content').slideDown 'fast'
            else
                if @can_create_table_status is false
                    @.$('.alert_modal').empty()
                    @.$('.btn-primary').prop 'disabled', false


        render: =>
            ordered_databases = @databases.map (d) ->
                name: d.get('name')
            ordered_databases = _.sortBy ordered_databases, (d) -> d.db

            super
                modal_title: 'Add table'
                btn_primary_text: 'Add'
                databases: ordered_databases

            @check_if_can_create_table()
            @$('.show_advanced_settings-link').click @show_advanced_settings
            @$('.hide_advanced_settings-link').click @hide_advanced_settings

            @$('#focus_table_name').focus()

        on_submit: =>
            super

            @formdata = form_data_as_object($('form', @$modal))
            # Check if data is safe
            template_error = {}
            input_error = false

            if @formdata.name is '' # Need a name
                input_error = true
                template_error.namespace_is_empty = true
            else if /^[a-zA-Z0-9_]+$/.test(@formdata.name) is false
                input_error = true
                template_error.special_char_detected = true
                template_error.type = 'table'
            else if not @formdata.database? or @formdata.database is ''
                input_error = true
                template_error.no_database = true
            else # And a name that doesn't exist
                database_used = null
                for database in @databases.models
                    if database.get('name') is @formdata.database
                        database_used = database
                        break

                for table in database_used.get('tables')
                    if table.name is @formdata.name
                        input_error = true
                        template_error.namespace_exists = true
                        break


            if input_error is true
                $('.alert_modal').html @templates.error template_error
                $('.alert_modal_content').slideDown 'fast'
                @reset_buttons()
            else
                optarg = {}
                if @formdata.write_disk is 'yes'
                    optarg.durability = 'soft'

                if @formdata.primary_key isnt ''
                    optarg.primaryKey = @formdata.primary_key


                driver.run r.db(@formdata.database).tableCreate(@formdata.name, optarg), (err, result) =>
                    if (err)
                        @on_error(err)
                    else
                        if result?.created is 1
                            @on_success()
                        else
                            @on_error(new Error("The returned result was not `{created: 1}`"))

        on_success: (response) =>
            super
            window.app.current_view.render_message "The table #{@formdata.database}.#{@formdata.name} was successfully created."

        destroy: =>
            @stopListening()

    class @RemoveTableModal extends UIComponents.AbstractModal
        template: Handlebars.templates['remove_table-modal-template']
        class: 'remove-namespace-dialog'

        render: (tables_to_delete) =>
            @tables_to_delete = tables_to_delete

            super
                modal_title: 'Delete tables'
                btn_primary_text: 'Delete'
                tables: tables_to_delete
                single_delete: tables_to_delete.length is 1

            @$('.btn-primary').focus()

        on_submit: =>
            super

            query = r.expr(@tables_to_delete).forEach (table) ->
                r.db(table("database")).tableDrop(table("table"))

            driver.run query, (err, result) =>
                if (err)
                    @on_error(err)
                else
                    if result?.dropped is @tables_to_delete.length
                        @on_success()
                    else
                        @on_error(new Error("The value returned for `dropped` did not match the number of tables."))


        on_success: (response) =>
            super

            # Build feedback message
            message = "The tables "
            for table, index in @tables_to_delete
                message += "#{table.database}.#{table.table}"
                if index < @tables_to_delete.length-1
                    message += ", "
            if @tables_to_delete.length is 1
                message += " was"
            else
                message += " were"
            message += " successfully deleted."

            if Backbone.history.fragment isnt 'tables'
                window.router.navigate '#tables', {trigger: true}
            window.app.current_view.render_message message
            @remove()

    class @RemoveServerModal extends UIComponents.AbstractModal
        template: Handlebars.templates['declare_machine_dead-modal-template']
        alert_tmpl: Handlebars.templates['resolve_issues-resolved-template']
        template_issue_error: Handlebars.templates['fail_solve_issue-template']

        class: 'declare-machine-dead'

        initialize: ->
            super @template

        render: (_machine_to_kill) ->
            @machine_to_kill = _machine_to_kill
            super
                machine_name: @machine_to_kill.get("name")
                modal_title: "Permanently remove the server"
                btn_primary_text: 'Remove'

        on_submit: ->
            super

            if @$('.verification').val().toLowerCase() is 'remove'
                $.ajax
                    url: "ajax/semilattice/machines/#{@machine_to_kill.id}"
                    type: 'DELETE'
                    contentType: 'application/json'
                    success: @on_success
                    error: @on_error
            else
                @.$('.error_verification').slideDown 'fast'
                @reset_buttons()

        on_success_with_error: =>
            @.$('.error_answer').html @template_issue_error

            if @.$('.error_answer').css('display') is 'none'
                @.$('.error_answer').slideDown('fast')
            else
                @.$('.error_answer').css('display', 'none')
                @.$('.error_answer').fadeIn()
            @reset_buttons()


        on_success: (response) =>
            if (response)
                @on_success_with_error()
                return

            # notify the user that we succeeded
            $('#issue-alerts').append @alert_tmpl
                machine_dead:
                    machine_name: @machine_to_kill.get("name")

            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: 'ajax/issues'
                contentType: 'application/json'
                success: =>
                    set_issues()
                    super


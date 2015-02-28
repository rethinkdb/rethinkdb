# Copyright 2010-2015 RethinkDB

ui_modals = require('./ui_components/modals.coffee')
util = require('./util.coffee')
models = require('./models.coffee')
table_view = require('./tables/table.coffee')
app = require('./app.coffee')
driver = app.driver
system_db = app.system_db

r = require('rethinkdb')

class AddDatabaseModal extends ui_modals.AbstractModal
    template: require('../handlebars/add_database-modal.hbs')
    templates:
        error: require('../handlebars/error_input.hbs')

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
        @formdata = util.form_data_as_object($('form', @$modal))

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
            driver.run_once r.db(system_db).table("db_config").insert({name: @formdata.name}), (err, result) =>
                if (err)
                    @on_error(err)
                else
                    if result.inserted is 1
                        @databases.add new models.Database
                            id: result.generated_keys[0]
                            name: @formdata.name
                            tables: []
                        @on_success()
                    else
                        @on_error(new Error(result.first_error or "Unknown error"))
        else
            $('.alert_modal_content').slideDown 'fast'
            @reset_buttons()

    on_success: (response) =>
        super()
        app.main.router.current_view.render_message "The database #{@formdata.name} was successfully created."

class DeleteDatabaseModal extends ui_modals.AbstractModal
    template: require('../handlebars/delete-database-modal.hbs')
    class: 'delete_database-dialog'

    render: (database_to_delete) ->
        @database_to_delete = database_to_delete

        # Call render of AbstractModal with the data for the template
        super
            modal_title: 'Delete database'
            btn_primary_text: 'Delete'

        @$('.verification_name').focus()


    on_submit: =>
        if @$('.verification_name').val() isnt @database_to_delete.get('name')
            if @$('.mismatch_container').css('display') is 'none'
                @$('.mismatch_container').slideDown('fast')
            else
                @$('.mismatch_container').hide()
                @$('.mismatch_container').fadeIn()
            @reset_buttons()
            return true

        super

        driver.run_once r.dbDrop(@database_to_delete.get('name')), (err, result) =>
            if (err)
                @on_error(err)
            else
                if result?.dbs_dropped is 1
                    @on_success()
                else
                    @on_error(new Error("The return result was not `{dbs_dropped: 1}`"))

    on_success: (response) =>
        super()
        if Backbone.history.fragment is 'tables'
            @database_to_delete.destroy()
        else
            # If the user was on a database view, we have to redirect him
            # If he was on #tables, we are just refreshing
            app.main.router.navigate '#tables', {trigger: true}

        app.main.router.current_view.render_message "The database #{@database_to_delete.get('name')} was successfully deleted."

class AddTableModal extends ui_modals.AbstractModal
    template: require('../handlebars/add_table-modal.hbs')
    templates:
        error: require('../handlebars/error_input.hbs')
    class: 'add-table'

    initialize: (data) =>
        super
        @db_id = data.db_id
        @db_name = data.db_name
        @tables = data.tables
        @container = data.container

        @delegateEvents()

    show_advanced_settings: (event) =>
        event.preventDefault()
        @$('.show_advanced_settings-link_container').fadeOut 'fast', =>
            @$('.hide_advanced_settings-link_container').fadeIn 'fast'
        @$('.advanced_settings').slideDown 'fast'

    hide_advanced_settings: (event) =>
        event.preventDefault()
        @$('.hide_advanced_settings-link_container').fadeOut 'fast', =>
            $('.show_advanced_settings-link_container').fadeIn 'fast'
        @$('.advanced_settings').slideUp 'fast'

    render: =>
        super
            modal_title: "Add table to #{@db_name}"
            btn_primary_text: 'Create table'

        @$('.show_advanced_settings-link').click @show_advanced_settings
        @$('.hide_advanced_settings-link').click @hide_advanced_settings

        @$('.focus-table-name').focus()

    on_submit: =>
        super

        @formdata = util.form_data_as_object($('form', @$modal))
        # Check if data is safe
        template_error = {}
        input_error = false

        if @formdata.name is '' # Need a name
            input_error = true
            template_error.table_name_empty = true
        else if /^[a-zA-Z0-9_]+$/.test(@formdata.name) is false
            input_error = true
            template_error.special_char_detected = true
            template_error.type = 'table'
        else # And a name that doesn't exist
            for table in @tables
                if table.name is @formdata.name
                    input_error = true
                    template_error.table_exists = true
                    break


        if input_error is true
            $('.alert_modal').html @templates.error template_error
            $('.alert_modal_content').slideDown 'fast'
            @reset_buttons()
        else
            # TODO Add durability in the query when the API will be available
            if @formdata.write_disk is 'yes'
                durability = 'soft'
            else
                durability = 'hard'

            if @formdata.primary_key isnt ''
                primary_key = @formdata.primary_key
            else
                primary_key = 'id'


            query = r.db(system_db).table("server_status").filter({status: "connected"}).coerceTo("ARRAY").do (servers) =>
                r.branch(
                    servers.isEmpty(),
                    r.error("No server is connected"),
                    servers.sample(1).nth(0)("name").do( (server) =>
                        r.db(system_db)
                         .table("table_config")
                         .insert({
                            db: @db_name
                            name: @formdata.name
                            primary_key: primary_key
                            shards: [
                                primary_replica: server
                                replicas: [server]
                            ]
                         }, returnChanges: true)
                    )
                )

            driver.run_once query, (err, result) =>
                if (err)
                    @on_error(err)
                else
                    if result?.errors is 0
                        @on_success()
                    else
                        @on_error(new Error("The returned result was not `{created: 1}`"))

    on_success: (response) =>
        super
        app.main.router.current_view.render_message "The table #{@db_name}.#{@formdata.name} was successfully created."

    remove: =>
        @stopListening()
        super()

class RemoveTableModal extends ui_modals.AbstractModal
    template: require('../handlebars/remove_table-modal.hbs')
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

        query = r.db(system_db).table('table_config').filter( (table) =>
            r.expr(@tables_to_delete).contains(
                database: table("db")
                table: table("name")
            )
        ).delete({returnChanges: true})

        driver.run_once query, (err, result) =>
            if (err)
                @on_error(err)
            else
                if @collection? and result?.changes?
                    for change in result.changes
                        keep_going = true
                        for database in @collection.models
                            if keep_going is false
                                break

                            if database.get('name') is change.old_val.db
                                for table, position in database.get('tables')
                                    if table.name is change.old_val.name
                                        database.set
                                            tables: database.get('tables').slice(0, position).concat(database.get('tables').slice(position+1))
                                        keep_going = false
                                        break

                if result?.deleted is @tables_to_delete.length
                    @on_success()
                else
                    @on_error(new Error("The value returned for `deleted` did not match the number of tables."))


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
            app.main.router.navigate '#tables', {trigger: true}
        app.main.router.current_view.render_message message
        @remove()

class RemoveServerModal extends ui_modals.AbstractModal
    template: require('../handlebars/declare_server_dead-modal.hbs')
    alert_tmpl: require('../handlebars/resolve_issues-resolved.hbs')
    template_issue_error: require('../handlebars/fail_solve_issue.hbs')

    class: 'declare-server-dead'

    initialize: (data) ->
        @model = data.model
        super @template

    render:  ->
        super
            server_name: @model.get("name")
            modal_title: "Permanently remove the server"
            btn_primary_text: 'Remove ' + @model.get('name')
        @

    on_submit: ->
        super

        if @$('.verification').val().toLowerCase() is 'remove'
            query = r.db(system_db).table('server_config')
                .get(@model.get('id')).delete()
            driver.run_once query, (err, result) =>
                if err?
                    @on_success_with_error()
                else
                    @on_success()
        else
            @$('.error_verification').slideDown 'fast'
            @reset_buttons()

    on_success_with_error: =>
        @$('.error_answer').html @template_issue_error

        if @$('.error_answer').css('display') is 'none'
            @$('.error_answer').slideDown('fast')
        else
            @$('.error_answer').css('display', 'none')
            @$('.error_answer').fadeIn()
        @reset_buttons()


    on_success: (response) ->
        if (response)
            @on_success_with_error()
            return

        # notify the user that we succeeded
        $('#issue-alerts').append @alert_tmpl
            server_dead:
                server_name: @model.get("name")
        @remove()

        @model.get('parent').set('fixed', true)
        @

# Modals.ReconfigureModal
class ReconfigureModal extends ui_modals.AbstractModal
    template: require('../handlebars/reconfigure-modal.hbs')

    class: 'reconfigure-modal'

    custom_events: $.extend(ui_modals.AbstractModal.events,
        'keyup .replicas.inline-edit': 'change_replicas'
        'keyup .shards.inline-edit': 'change_shards'
        'click .expand-tree': 'expand_link'
    )

    initialize: (obj) =>
        @events = $.extend(@events, @custom_events)
        @model.set('expanded', false)
        @fetch_dryrun()
        @listenTo @model, 'change:num_replicas_per_shard', @fetch_dryrun
        @listenTo @model, 'change:num_shards', @fetch_dryrun
        @listenTo @model, 'change:errors', @change_errors
        @listenTo @model, 'change:server_error', @get_errors
        @listenTo @model, 'change:expanded', @expanded_changed
        # flag for whether to show an error on empty text boxes.
        # When submitting, we want to show an error. When checking
        # on keyup, we shouldn't do anything since the user is
        # probably about to type a number in and we don't want an
        # error to flash up really quick.
        @error_on_empty = false
        super(obj)

    expand_link: (event) =>
        event.preventDefault()
        event.target.blur();
        @model.set(expanded: not @model.get('expanded'))

    expanded_changed: =>
        @$('.reconfigure-diff').toggleClass('expanded')
        @$('.expand-tree').toggleClass('expanded')
        $('.reconfigure-modal').toggleClass('expanded')
        $('.expandbox').toggleClass('expanded')

    render: =>
        super $.extend(@model.toJSON(),
            modal_title: "Sharding and replication for " +
                "#{@model.get('db')}.#{@model.get('name')}"
            btn_primary_text: 'Apply configuration'
        )

        @diff_view = new table_view.ReconfigureDiffView
            model: @model
            el: @$('.reconfigure-diff')[0]
        @

    change_errors: =>
        # This shows and hides all errors. The error messages
        # themselves are stored in the corresponding template and
        # are just hidden by default.
        errors = @model.get('errors')
        @$('.alert.error p.error').removeClass('shown')
        @$('.alert.error .server-msg').html('')
        if errors.length > 0
            @$('.btn.btn-primary').prop disabled: true
            @$('.alert.error').addClass('shown')
            for error in errors
                message = @$(".alert.error p.error.#{error}").addClass('shown')
                if error == 'server-error'
                    @$('.alert.error .server-msg').append(Handlebars.Utils.escapeExpression(@model.get('server_error')))
        else
            @error_on_empty = false
            @$('.btn.btn-primary').removeAttr 'disabled'
            @$('.alert.error').removeClass('shown')

    remove: =>
        if diff_view?
            diff_view.remove()
        super()

    change_replicas: =>
        replicas = parseInt @$('.replicas.inline-edit').val()
        # If the text box isn't a number (empty, or some letters
        # or something), we don't actually update the model. The
        # exception is if we've hit the apply button, which will
        # set @error_on_empty = true. In this case we update the
        # model so that an error will be shown to the user that
        # they must enter a valid value before applying changes.
        if not isNaN(replicas) or @error_on_empty
            @model.set num_replicas_per_shard: replicas

    change_shards: =>
        shards = parseInt @$('.shards.inline-edit').val()
        # See comment in @change_replicas
        if not isNaN(shards) or @error_on_empty
            @model.set num_shards: shards

    on_submit: ->
        # We set @error_on_empty to true to cause validation to
        # fail if the text boxes are empty. Normally, we don't
        # want to show an error, since the box is probably only
        # empty for a moment before the user types in a
        # number. But on submit, we need to ensure an invalid
        # configuration isn't requested.
        @error_on_empty = true
        # Now we call change_replicas, change_shards, and
        # get_errors to ensure validation happens before submit.
        # get_errors will set @error_on_empty to false once all
        # errors have been dealt with.
        @change_replicas()
        @change_shards()
        if @get_errors()
            return
        else
            super
        # Here we pull out only servers that are new or modified,
        # because we're going to set the configuration all at once
        # to the proper value, so deleted servers shouldn't be
        # included.
        new_or_unchanged = (doc) ->
            doc('change').eq('added').or(doc('change').not())
        # We're using ReQL to transform the configuration data
        # structure from the format useful for the view, to the
        # format that table_config requires.
        new_shards = r.expr(@model.get('shards')).filter(new_or_unchanged)
            .map((shard) ->
                primary_replica: shard('replicas').filter(role: 'primary')(0)('id')
                replicas: shard('replicas').filter(new_or_unchanged)('id')
            )

        query = r.db(system_db).table('table_config', identifierFormat: 'uuid')
            .get(@model.get('id'))
            .update {shards: new_shards}, {returnChanges: true}
        driver.run_once query, (error, result) =>
            if error?
                @model.set server_error: error.msg
            else
                @reset_buttons()
                @remove()
                parent = @model.get('parent')
                parent.model.set
                    num_replicas_per_shard: @model.get 'num_replicas_per_shard'
                    num_shards: @model.get 'num_shards'
                # This is so that the progress bar for
                # reconfiguration shows up immediately when the
                # reconfigure modal closes, rather than waiting
                # for the state check every 5 seconds.
                parent.progress_bar.skip_to_processing()
                @model.get('parent').fetch_progress()

    get_errors: =>
        # This method checks for anything the server will reject
        # that we can predict beforehand.
        errors = []
        num_shards = @model.get('num_shards')
        num_replicas = @model.get('num_replicas_per_shard')
        num_servers = @model.get('num_servers')
        num_default_servers = @model.get('num_default_servers')

        # check shard errors
        if num_shards == 0
            errors.push 'zero-shards'
        else if isNaN num_shards
            errors.push 'no-shards'
        else if num_shards > num_default_servers
            if num_shards > num_servers
                errors.push 'too-many-shards'
            else
                errors.push 'not-enough-defaults-shard'

        # check replicas per shard errors
        if num_replicas == 0
            errors.push 'zero-replicas'
        else if isNaN num_replicas
            errors.push 'no-replicas'
        else if num_replicas > num_default_servers
            if num_replicas > num_servers
                errors.push 'too-many-replicas'
            else
                errors.push 'not-enough-defaults-replica'

        # check for server error
        if @model.get('server_error')?
            errors.push 'server-error'

        @model.set errors: errors
        if errors.length > 0
            @model.set shards: []
        errors.length > 0



    fetch_dryrun: =>
        # This method takes the users shards and replicas per
        # server values and runs table.reconfigure(..., {dryRun:
        # true}) to see how the cluster would satisfy those
        # constraints. We don't want to run it however, if either
        # shards or clusters is a bad value.
        if @get_errors()
            return
        id = (x) -> x
        query = r.db(@model.get('db'))
            .table(@model.get('name'))
            .reconfigure(
                shards: @model.get('num_shards')
                replicas: @model.get('num_replicas_per_shard')
                dryRun: true
            )('config_changes')(0)
            .do((diff) ->
                r.object(r.args(diff.keys().map((key) ->
                    [key, diff(key)('shards')]
                ).concatMap(id)))
            ).do((doc) ->
                doc.merge
                    # this creates a servername -> id map we can
                    # use later to create the links
                    lookups: r.object(r.args(
                        doc('new_val')('replicas').concatMap(id)
                            .setUnion(doc('old_val')('replicas').concatMap(id))
                            .concatMap (server) -> [
                                server,
                                r.db(system_db).table('server_status')
                                    .filter(name: server)(0)('id')
                            ]
                    ))
            )

        driver.run_once query, (error, result) =>
            if error?
                @model.set
                    server_error: error.msg
                    shards: []
            else
                @model.set server_error: null
                @model.set
                    shards: @fixup_shards result.old_val,
                        result.new_val
                        result.lookups

    # Sorts shards in order of current primary first, old primary (if
    # any), then secondaries in lexicographic order
    shard_sort: (a, b) ->
        if a.role == 'primary'
            -1
        else if 'primary' in [b.role, b.old_role]
            +1
        else if a.role == b.role
            if a.name == b.name
                0
            else if a.name < b.name
                -1
            else
                +1

    # determines role of a replica
    role: (isPrimary) ->
        if isPrimary then 'primary' else 'secondary'

    fixup_shards: (old_shards, new_shards, lookups) =>
        # This does the mapping from a {new_val: ..., old_val:
        # ...}  result from the dryRun reconfigure, and maps it to
        # a data structure that's convenient for the view to
        # use. (@on_submit maps it back when applying changes to
        # the cluster)
        #
        # All of the "diffing" occurs in this function, it
        # detects added/removed servers and detects role changes.
        # This function has a lot of repetition, but removing the
        # repetition made the function much more difficult to
        # understand(and debug), so I left it in.
        shard_diffs = []
        # first handle shards that are in old (and possibly in new)
        for old_shard, i in old_shards
            if i >= new_shards.length
                new_shard = {primary_replica: null, replicas: []}
                shard_deleted = true
            else
                new_shard = new_shards[i]
                shard_deleted = false

            shard_diff =
                replicas: []
                change: if shard_deleted then 'deleted' else null

            # handle any deleted and remaining replicas for this shard
            for replica in old_shard.replicas
                replica_deleted = replica not in new_shard.replicas
                if replica_deleted
                    new_role = null
                else
                    new_role = @role(replica == new_shard.primary_replica)
                old_role = @role(replica == old_shard.primary_replica)

                shard_diff.replicas.push
                    name: replica
                    id: lookups[replica]
                    change: if replica_deleted then 'deleted' else null
                    role: new_role
                    old_role: old_role
                    role_change: not replica_deleted and new_role != old_role
            # handle any new replicas for this shard
            for replica in new_shard.replicas
                if replica in old_shard.replicas
                    continue
                shard_diff.replicas.push
                    name: replica
                    id: lookups[replica]
                    change: 'added'
                    role: @role(replica == new_shard.primary_replica)
                    old_role: null
                    role_change: false

            shard_diff.replicas.sort @shard_sort
            shard_diffs.push(shard_diff)
        # now handle any new shards beyond what old_shards has
        for new_shard in new_shards.slice(old_shards.length)
            shard_diff =
                replicas: []
                change: 'added'
            for replica in new_shard.replicas
                shard_diff.replicas.push
                    name: replica
                    id: lookups[replica]
                    change: 'added'
                    role: @role(replica == new_shard.primary_replica)
                    old_role: null
                    role_change: false
            shard_diff.replicas.sort @shard_sort
            shard_diffs.push(shard_diff)
        shard_diffs

module.exports =
    AddDatabaseModal: AddDatabaseModal
    DeleteDatabaseModal: DeleteDatabaseModal
    AddTableModal: AddTableModal
    RemoveTableModal: RemoveTableModal
    RemoveServerModal: RemoveServerModal
    ReconfigureModal: ReconfigureModal

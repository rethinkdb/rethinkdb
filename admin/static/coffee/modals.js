// Copyright 2010-2015 RethinkDB

const ui_modals = require('./ui_components/modals.coffee');
const util = require('./util.coffee');
const models = require('./models.coffee');
const table_view = require('./tables/table.coffee');
const app = require('./app.coffee');
const { driver } = app;
const { system_db } = app;

const r = require('rethinkdb');
const Handlebars = require('hbsfy/runtime');

class AddDatabaseModal extends ui_modals.AbstractModal {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.render = this.render.bind(this);
        this.on_submit = this.on_submit.bind(this);
        this.on_success = this.on_success.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.template = require('../handlebars/add_database-modal.hbs');
        this.prototype.templates =
            {error: require('../handlebars/error_input.hbs')};
    
        this.prototype.class = 'add-database';
    }

    initialize(databases) {
        super.initialize(...arguments);
        return this.databases = databases;
    }


    render() {
        super.render({
            modal_title: "Add database",
            btn_primary_text: "Add"
        });
        return this.$('.focus_new_name').focus();
    }

    on_submit(...args) {
        super.on_submit(...args);
        this.formdata = util.form_data_as_object($('form', this.$modal));

        let error = null;

        // Validate input
        if (this.formdata.name === '') {
            // Empty name is not valid
            error = true;
            this.$('.alert_modal').html(this.templates.error({
                database_is_empty: true})
            );
        } else if (/^[a-zA-Z0-9_]+$/.test(this.formdata.name) === false) {
            // Only alphanumeric char + underscore are allowed
            error = true;
            this.$('.alert_modal').html(this.templates.error({
                special_char_detected: true,
                type: 'database'
            })
            );
        } else {
            // Check if it's a duplicate
            for (let database of Array.from(this.databases.models)) {
                if (database.get('db') === this.formdata.name) {
                    error = true;
                    this.$('.alert_modal').html(this.templates.error({
                        database_exists: true})
                    );
                    break;
                }
            }
        }

        if (error === null) {
            return driver.run_once(r.db(system_db).table("db_config").insert({name: this.formdata.name}), (err, {inserted, generated_keys, first_error}) => {
                if (err) {
                    return this.on_error(err);
                } else {
                    if (inserted === 1) {
                        this.databases.add(new models.Database({
                            id: generated_keys[0],
                            name: this.formdata.name,
                            tables: []}));
                        return this.on_success();
                    } else {
                        return this.on_error(new Error(first_error || "Unknown error"));
                    }
                }
            }
            );
        } else {
            $('.alert_modal_content').slideDown('fast');
            return this.reset_buttons();
        }
    }

    on_success(response) {
        super.on_success();
        return app.main.router.current_view.render_message(`The database ${this.formdata.name} was successfully created.`);
    }
}
AddDatabaseModal.initClass();

class DeleteDatabaseModal extends ui_modals.AbstractModal {
    constructor(...args) {
        this.on_submit = this.on_submit.bind(this);
        this.on_success = this.on_success.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.template = require('../handlebars/delete-database-modal.hbs');
        this.prototype.class = 'delete_database-dialog';
    }

    render(database_to_delete) {
        this.database_to_delete = database_to_delete;

        // Call render of AbstractModal with the data for the template
        super.render({
            modal_title: 'Delete database',
            btn_primary_text: 'Delete'
        });

        return this.$('.verification_name').focus();
    }


    on_submit(...args) {
        if ($.trim(this.$('.verification_name').val()) !== this.database_to_delete.get('name')) {
            if (this.$('.mismatch_container').css('display') === 'none') {
                this.$('.mismatch_container').slideDown('fast');
            } else {
                this.$('.mismatch_container').hide();
                this.$('.mismatch_container').fadeIn();
            }
            this.reset_buttons();
            return true;
        }

        super.on_submit(...args);

        return driver.run_once(r.dbDrop(this.database_to_delete.get('name')), (err, result) => {
            if (err) {
                return this.on_error(err);
            } else {
                if (__guard__(result, ({dbs_dropped}) => dbs_dropped) === 1) {
                    return this.on_success();
                } else {
                    return this.on_error(new Error("The return result was not `{dbs_dropped: 1}`"));
                }
            }
        }
        );
    }

    on_success(response) {
        super.on_success();
        if (Backbone.history.fragment === 'tables') {
            this.database_to_delete.destroy();
        } else {
            // If the user was on a database view, we have to redirect him
            // If he was on #tables, we are just refreshing
            app.main.router.navigate('#tables', {trigger: true});
        }

        return app.main.router.current_view.render_message(`The database ${this.database_to_delete.get('name')} was successfully deleted.`);
    }
}
DeleteDatabaseModal.initClass();

class AddTableModal extends ui_modals.AbstractModal {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.show_advanced_settings = this.show_advanced_settings.bind(this);
        this.hide_advanced_settings = this.hide_advanced_settings.bind(this);
        this.render = this.render.bind(this);
        this.on_submit = this.on_submit.bind(this);
        this.on_success = this.on_success.bind(this);
        this.remove = this.remove.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.template = require('../handlebars/add_table-modal.hbs');
        this.prototype.templates =
            {error: require('../handlebars/error_input.hbs')};
        this.prototype.class = 'add-table';
    }

    initialize({db_id, db_name, tables, container}) {
        super.initialize(...arguments);
        this.db_id = db_id;
        this.db_name = db_name;
        this.tables = tables;
        this.container = container;

        return this.delegateEvents();
    }

    show_advanced_settings(event) {
        event.preventDefault();
        this.$('.show_advanced_settings-link_container').fadeOut('fast', () => {
            return this.$('.hide_advanced_settings-link_container').fadeIn('fast');
        }
        );
        return this.$('.advanced_settings').slideDown('fast');
    }

    hide_advanced_settings(event) {
        event.preventDefault();
        this.$('.hide_advanced_settings-link_container').fadeOut('fast', () => {
            return $('.show_advanced_settings-link_container').fadeIn('fast');
        }
        );
        return this.$('.advanced_settings').slideUp('fast');
    }

    render() {
        super.render({
            modal_title: `Add table to ${this.db_name}`,
            btn_primary_text: 'Create table'
        });

        this.$('.show_advanced_settings-link').click(this.show_advanced_settings);
        this.$('.hide_advanced_settings-link').click(this.hide_advanced_settings);

        return this.$('.focus-table-name').focus();
    }

    on_submit(...args) {
        let table;
        super.on_submit(...args);

        this.formdata = util.form_data_as_object($('form', this.$modal));
        // Check if data is safe
        const template_error = {};
        let input_error = false;

        if (this.formdata.name === '') { // Need a name
            input_error = true;
            template_error.table_name_empty = true;
        } else if (/^[a-zA-Z0-9_]+$/.test(this.formdata.name) === false) {
            input_error = true;
            template_error.special_char_detected = true;
            template_error.type = 'table';
        } else { // And a name that doesn't exist
            for (table of Array.from(this.tables)) {
                if (table.name === this.formdata.name) {
                    input_error = true;
                    template_error.table_exists = true;
                    break;
                }
            }
        }


        if (input_error === true) {
            $('.alert_modal').html(this.templates.error(template_error));
            $('.alert_modal_content').slideDown('fast');
            return this.reset_buttons();
        } else {
            // TODO Add durability in the query when the API will be available
            let durability;

            let primary_key;
            if (this.formdata.write_disk === 'yes') {
                durability = 'hard';
            } else {
                durability = 'soft';
            }

            if (this.formdata.primary_key !== '') {
                ({ primary_key } = this.formdata);
            } else {
                primary_key = 'id';
            }


            const query = r.db(system_db).table("server_status").coerceTo("ARRAY")
              .do(servers => {
                return r.branch(
                    servers.isEmpty(),
                    r.error("No server is connected"),
                    servers.sample(1).nth(0)("name").do( server => {
                        return r.db(system_db)
                         .table("table_config")
                         .insert({
                            db: this.db_name,
                            name: this.formdata.name,
                            primary_key,
                            durability,
                            shards: [{
                                primary_replica: server,
                                replicas: [server]
                            }
                            ]
                         }, {returnChanges: true});
                    }
                    )
                );
            }
            );

            return driver.run_once(query, (err, result) => {
                if (err) {
                    return this.on_error(err);
                } else {
                    if (__guard__(result, ({errors}) => errors) === 0) {
                        return this.on_success();
                    } else {
                        return this.on_error(new Error("The returned result was not `{created: 1}`"));
                    }
                }
            }
            );
        }
    }

    on_success(response) {
        super.on_success(...arguments);
        return app.main.router.current_view.render_message(`The table ${this.db_name}.${this.formdata.name} was successfully created.`);
    }

    remove() {
        this.stopListening();
        return super.remove();
    }
}
AddTableModal.initClass();

class RemoveTableModal extends ui_modals.AbstractModal {
    constructor(...args) {
        this.render = this.render.bind(this);
        this.on_submit = this.on_submit.bind(this);
        this.on_success = this.on_success.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.template = require('../handlebars/remove_table-modal.hbs');
        this.prototype.class = 'remove-namespace-dialog';
    }

    render(tables_to_delete) {
        this.tables_to_delete = tables_to_delete;

        super.render({
            modal_title: 'Delete tables',
            btn_primary_text: 'Delete',
            tables: tables_to_delete,
            single_delete: tables_to_delete.length === 1
        });

        return this.$('.btn-primary').focus();
    }

    on_submit(...args) {
        super.on_submit(...args);

        const query = r.db(system_db).table('table_config').filter( table => {
            return r.expr(this.tables_to_delete).contains({
                database: table("db"),
                table: table("name")
            });
        }
        ).delete({returnChanges: true});

        return driver.run_once(query, (err, result) => {
            if (err) {
                return this.on_error(err);
            } else {
                if ((this.collection != null) && (__guard__(result, ({changes}) => changes) != null)) {
                    for (let change of Array.from(result.changes)) {
                        let keep_going = true;
                        for (let database of Array.from(this.collection.models)) {
                            if (keep_going === false) {
                                break;
                            }

                            if (database.get('name') === change.old_val.db) {
                                const iterable = database.get('tables');
                                for (let position = 0; position < iterable.length; position++) {
                                    const table = iterable[position];
                                    if (table.name === change.old_val.name) {
                                        database.set({
                                            tables: database.get('tables').slice(0, position).concat(database.get('tables').slice(position+1))});
                                        keep_going = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if (__guard__(result, ({deleted}) => deleted) === this.tables_to_delete.length) {
                    return this.on_success();
                } else {
                    return this.on_error(new Error("The value returned for `deleted` did not match the number of tables."));
                }
            }
        }
        );
    }


    on_success(response) {
        let message;
        super.on_success(...arguments);

        // Build feedback message
        if (this.tables_to_delete.length === 1) {
            message = "The table ";
        } else {
            message = "The tables ";
        }

        this.tables_to_delete.forEach((table, index) => {
            message += `${table.database}.${table.table}`;
            if (index < (this.tables_to_delete.length-1)) {
                message += ", ";
            }
        });

        if (this.tables_to_delete.length === 1) {
            message += " was";
        } else {
            message += " were";
        }
        message += " successfully deleted.";

        if (Backbone.history.fragment !== 'tables') {
            app.main.router.navigate('#tables', {trigger: true});
        }
        app.main.router.current_view.render_message(message);
        return this.remove();
    }
}
RemoveTableModal.initClass();

class RemoveServerModal extends ui_modals.AbstractModal {
    constructor(...args) {
        this.on_success_with_error = this.on_success_with_error.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.template = require('../handlebars/declare_server_dead-modal.hbs');
        this.prototype.alert_tmpl = require('../handlebars/resolve_issues-resolved.hbs');
        this.prototype.template_issue_error = require('../handlebars/fail_solve_issue.hbs');
    
        this.prototype.class = 'declare-server-dead';
    }

    initialize({model}) {
        this.model = model;
        return super.initialize(this.template);
    }

    render() {
        super.render({
            server_name: this.model.get("name"),
            modal_title: "Permanently remove the server",
            btn_primary_text: `Remove ${this.model.get('name')}`
        });
        return this;
    }

    on_submit(...args) {
        super.on_submit(...args);

        if (this.$('.verification').val().toLowerCase() === 'remove') {
            const query = r.db(system_db).table('server_config')
                .get(this.model.get('id')).delete();
            return driver.run_once(query, (err, result) => {
                if (err != null) {
                    return this.on_success_with_error();
                } else {
                    return this.on_success();
                }
            }
            );
        } else {
            this.$('.error_verification').slideDown('fast');
            return this.reset_buttons();
        }
    }

    on_success_with_error() {
        this.$('.error_answer').html(this.template_issue_error);

        if (this.$('.error_answer').css('display') === 'none') {
            this.$('.error_answer').slideDown('fast');
        } else {
            this.$('.error_answer').css('display', 'none');
            this.$('.error_answer').fadeIn();
        }
        return this.reset_buttons();
    }


    on_success(response) {
        if (response) {
            this.on_success_with_error();
            return;
        }

        // notify the user that we succeeded
        $('#issue-alerts').append(this.alert_tmpl({
            server_dead: {
                server_name: this.model.get("name")
            }
        })
        );
        this.remove();

        this.model.get('parent').set('fixed', true);
        return this;
    }
}
RemoveServerModal.initClass();

// Modals.ReconfigureModal
class ReconfigureModal extends ui_modals.AbstractModal {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.expand_link = this.expand_link.bind(this);
        this.expanded_changed = this.expanded_changed.bind(this);
        this.render = this.render.bind(this);
        this.change_errors = this.change_errors.bind(this);
        this.remove = this.remove.bind(this);
        this.change_replicas = this.change_replicas.bind(this);
        this.change_shards = this.change_shards.bind(this);
        this.get_errors = this.get_errors.bind(this);
        this.fetch_dryrun = this.fetch_dryrun.bind(this);
        this.fixup_shards = this.fixup_shards.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.template = require('../handlebars/reconfigure-modal.hbs');
    
        this.prototype.class = 'reconfigure-modal';
    
        this.prototype.custom_events = $.extend(ui_modals.AbstractModal.events, {
            'keyup .replicas.inline-edit': 'change_replicas',
            'keyup .shards.inline-edit': 'change_shards',
            'click .expand-tree': 'expand_link'
        }
        );
    }

    initialize(obj) {
        this.events = $.extend(this.events, this.custom_events);
        this.model.set('expanded', false);
        this.fetch_dryrun();
        this.listenTo(this.model, 'change:num_replicas_per_shard', this.fetch_dryrun);
        this.listenTo(this.model, 'change:num_shards', this.fetch_dryrun);
        this.listenTo(this.model, 'change:errors', this.change_errors);
        this.listenTo(this.model, 'change:server_error', this.get_errors);
        this.listenTo(this.model, 'change:expanded', this.expanded_changed);
        // flag for whether to show an error on empty text boxes.
        // When submitting, we want to show an error. When checking
        // on keyup, we shouldn't do anything since the user is
        // probably about to type a number in and we don't want an
        // error to flash up really quick.
        this.error_on_empty = false;
        return super.initialize(obj);
    }

    expand_link(event) {
        event.preventDefault();
        event.target.blur();
        return this.model.set({expanded: !this.model.get('expanded')});
    }

    expanded_changed() {
        this.$('.reconfigure-diff').toggleClass('expanded');
        this.$('.expand-tree').toggleClass('expanded');
        $('.reconfigure-modal').toggleClass('expanded');
        return $('.expandbox').toggleClass('expanded');
    }

    render() {
        super.render($.extend(this.model.toJSON(), {
            modal_title: `Sharding and replication for ${`${this.model.get('db')}.${this.model.get('name')}`}`,
            btn_primary_text: 'Apply configuration'
        }
        )
        );

        this.diff_view = new table_view.ReconfigureDiffView({
            model: this.model,
            el: this.$('.reconfigure-diff')[0]});
        if (this.get_errors()) {
            this.change_errors();
        }
        return this;
    }

    change_errors() {
        // This shows and hides all errors. The error messages
        // themselves are stored in the corresponding template and
        // are just hidden by default.
        const errors = this.model.get('errors');
        this.$('.alert.error p.error').removeClass('shown');
        this.$('.alert.error .server-msg').html('');
        if (errors.length > 0) {
            this.$('.btn.btn-primary').prop({disabled: true});
            this.$('.alert.error').addClass('shown');
            return (() => {
                const result = [];
                for (let error of Array.from(errors)) {
                    let item;
                    const message = this.$(`.alert.error p.error.${error}`).addClass('shown');
                    if (error === 'server-error') {
                        item = this.$('.alert.error .server-msg').append(
                            Handlebars.Utils.escapeExpression(this.model.get('server_error')));
                    }
                    result.push(item);
                }
                return result;
            })();
        } else {
            this.error_on_empty = false;
            this.$('.btn.btn-primary').removeAttr('disabled');
            return this.$('.alert.error').removeClass('shown');
        }
    }

    remove() {
        if (typeof diff_view !== 'undefined' && diff_view !== null) {
            diff_view.remove();
        }
        return super.remove();
    }

    change_replicas() {
        const replicas = parseInt(this.$('.replicas.inline-edit').val());
        // If the text box isn't a number (empty, or some letters
        // or something), we don't actually update the model. The
        // exception is if we've hit the apply button, which will
        // set @error_on_empty = true. In this case we update the
        // model so that an error will be shown to the user that
        // they must enter a valid value before applying changes.
        if (!isNaN(replicas) || this.error_on_empty) {
            return this.model.set({num_replicas_per_shard: replicas});
        }
    }

    change_shards() {
        const shards = parseInt(this.$('.shards.inline-edit').val());
        // See comment in @change_replicas
        if (!isNaN(shards) || this.error_on_empty) {
            return this.model.set({num_shards: shards});
        }
    }

    on_submit(...args) {
        // We set @error_on_empty to true to cause validation to
        // fail if the text boxes are empty. Normally, we don't
        // want to show an error, since the box is probably only
        // empty for a moment before the user types in a
        // number. But on submit, we need to ensure an invalid
        // configuration isn't requested.
        this.error_on_empty = true;
        // Now we call change_replicas, change_shards, and
        // get_errors to ensure validation happens before submit.
        // get_errors will set @error_on_empty to false once all
        // errors have been dealt with.
        this.change_replicas();
        this.change_shards();
        if (this.get_errors()) {
            return;
        } else {
            super.on_submit(...args);
        }
        // Here we pull out only servers that are new or modified,
        // because we're going to set the configuration all at once
        // to the proper value, so deleted servers shouldn't be
        // included.
        const new_or_unchanged = doc => doc('change').eq('added').or(doc('change').not());
        // We're using ReQL to transform the configuration data
        // structure from the format useful for the view, to the
        // format that table_config requires.
        const new_shards = r.expr(this.model.get('shards')).filter(new_or_unchanged)
            .map(shard =>
                ({
                    primary_replica: shard('replicas').filter({role: 'primary'})(0)('id'),
                    replicas: shard('replicas').filter(new_or_unchanged)('id')
                })
            );

        const query = r.db(system_db).table('table_config', {identifierFormat: 'uuid'})
            .get(this.model.get('id'))
            .update({shards: new_shards}, {returnChanges: true});
        return driver.run_once(query, (error, {first_error}) => {
            if (error != null) {
                return this.model.set({server_error: error.msg});
            } else if (first_error != null) {
                return this.model.set({server_error: first_error});
            } else {
                this.reset_buttons();
                this.remove();
                const parent = this.model.get('parent');
                parent.model.set({
                    num_replicas_per_shard: this.model.get('num_replicas_per_shard'),
                    num_shards: this.model.get('num_shards')
                });
                // This is so that the progress bar for
                // reconfiguration shows up immediately when the
                // reconfigure modal closes, rather than waiting
                // for the state check every 5 seconds.
                parent.progress_bar.skip_to_processing();
                return this.model.get('parent').fetch_progress();
            }
        }
        );
    }

    get_errors() {
        // This method checks for anything the server will reject
        // that we can predict beforehand.
        const errors = [];
        const num_shards = this.model.get('num_shards');
        const num_replicas = this.model.get('num_replicas_per_shard');
        const num_servers = this.model.get('num_servers');
        const num_default_servers = this.model.get('num_default_servers');

        const MAX_NUM_SHARDS = 64;

        // check shard errors
        if (num_shards === 0) {
            errors.push('zero-shards');
        } else if (isNaN(num_shards)) {
            errors.push('no-shards');
        } else if (num_shards > MAX_NUM_SHARDS) {
            errors.push('too-many-shards-hard-limit');
        } else if (num_shards > num_default_servers) {
            if (num_shards > num_servers) {
                errors.push('too-many-shards');
            } else {
                errors.push('not-enough-defaults-shard');
            }
        }

        // check replicas per shard errors
        if (num_replicas === 0) {
            errors.push('zero-replicas');
        } else if (isNaN(num_replicas)) {
            errors.push('no-replicas');
        } else if (num_replicas > num_default_servers) {
            if (num_replicas > num_servers) {
                errors.push('too-many-replicas');
            } else {
                errors.push('not-enough-defaults-replica');
            }
        }

        // check for server error
        if (this.model.get('server_error') != null) {
            errors.push('server-error');
        }

        this.model.set({errors});
        if (errors.length > 0) {
            this.model.set({shards: []});
        }
        return errors.length > 0;
    }



    fetch_dryrun() {
        // This method takes the users shards and replicas per
        // server values and runs table.reconfigure(..., {dryRun:
        // true}) to see how the cluster would satisfy those
        // constraints. We don't want to run it however, if either
        // shards or clusters is a bad value.
        if (this.get_errors()) {
            return;
        }
        const id = x => x;
        const query = r.db(this.model.get('db'))
            .table(this.model.get('name'))
            .reconfigure({
                shards: this.model.get('num_shards'),
                replicas: this.model.get('num_replicas_per_shard'),
                dryRun: true
            })('config_changes')(0)
            .do(diff =>
                r.object(r.args(diff.keys().map(key => [key, diff(key)('shards')]).concatMap(id)))
            ).do(doc =>
                doc.merge({
                    // this creates a servername -> id map we can
                    // use later to create the links
                    lookups: r.object(r.args(
                        doc('new_val')('replicas').concatMap(id)
                            .setUnion(doc('old_val')('replicas').concatMap(id))
                            .concatMap(server => [
                                server,
                                r.db(system_db).table('server_status')
                                    .filter({name: server})(0)('id').default(null)
                            ] )
                    ))
            }));

        return driver.run_once(query, (error, {old_val, new_val, lookups}) => {
            if (error != null) {
                return this.model.set({
                    server_error: error.msg,
                    shards: []});
            } else {
                this.model.set({server_error: null});
                return this.model.set({
                    shards: this.fixup_shards(old_val,
                        new_val,
                        lookups)
                });
            }
        }
        );
    }

    // Sorts shards in order of current primary first, old primary (if
    // any), then secondaries in lexicographic order
    shard_sort({role, name}, {role, old_role, name}) {
        if (role === 'primary') {
            return -1;
        } else if ([role, old_role].includes('primary')) {
            return +1;
        } else if (role === role) {
            if (name === name) {
                return 0;
            } else if (name < name) {
                return -1;
            } else {
                return +1;
            }
        }
    }

    // determines role of a replica
    role(isPrimary) {
        if (isPrimary) { return 'primary'; } else { return 'secondary'; }
    }

    fixup_shards(old_shards, new_shards, lookups) {
        // This does the mapping from a {new_val: ..., old_val:
        // ...}  result from the dryRun reconfigure, and maps it to
        // a data structure that's convenient for the view to
        // use. (@on_submit maps it back when applying changes to
        // the cluster)
        //
        // All of the "diffing" occurs in this function, it
        // detects added/removed servers and detects role changes.
        // This function has a lot of repetition, but removing the
        // repetition made the function much more difficult to
        // understand(and debug), so I left it in.
        let new_shard;

        let old_role;
        let replica;
        let shard_diff;
        const shard_diffs = [];

        // first handle shards that are in old (and possibly in new)
        old_shards.forEach((old_shard, i) => {
            const old_shard = old_shards[i];
            if (i >= new_shards.length) {
                new_shard = {primary_replica: null, replicas: []};
                shard_deleted = true;
            } else {
                new_shard = new_shards[i];
                shard_deleted = false;
            }

            shard_diff = {
                replicas: [],
                change: shard_deleted ? 'deleted' : null
            };

            // handle any deleted and remaining replicas for this shard
            for (replica of Array.from(old_shard.replicas)) {
                let new_role;
                const replica_deleted = !Array.from(new_shard.replicas).includes(replica);
                if (replica_deleted) {
                    new_role = null;
                } else {
                    new_role = this.role(replica === new_shard.primary_replica);
                }
                old_role = this.role(replica === old_shard.primary_replica);

                shard_diff.replicas.push({
                    name: replica,
                    id: lookups[replica],
                    change: replica_deleted ? 'deleted' : null,
                    role: new_role,
                    old_role,
                    role_change: !replica_deleted && (new_role !== old_role)
                });
            }
            // handle any new replicas for this shard
            for (replica of Array.from(new_shard.replicas)) {
                if (Array.from(old_shard.replicas).includes(replica)) {
                    continue;
                }
                shard_diff.replicas.push({
                    name: replica,
                    id: lookups[replica],
                    change: 'added',
                    role: this.role(replica === new_shard.primary_replica),
                    old_role: null,
                    role_change: false
                });
            }

            shard_diff.replicas.sort(this.shard_sort);
            shard_diffs.push(shard_diff);
        });

        // now handle any new shards beyond what old_shards has
        for (new_shard of Array.from(new_shards.slice(old_shards.length))) {
            shard_diff = {
                replicas: [],
                change: 'added'
            };
            for (replica of Array.from(new_shard.replicas)) {
                shard_diff.replicas.push({
                    name: replica,
                    id: lookups[replica],
                    change: 'added',
                    role: this.role(replica === new_shard.primary_replica),
                    old_role: null,
                    role_change: false
                });
            }
            shard_diff.replicas.sort(this.shard_sort);
            shard_diffs.push(shard_diff);
        }
        return shard_diffs;
    }
}
ReconfigureModal.initClass();

exports.AddDatabaseModal = AddDatabaseModal;
exports.DeleteDatabaseModal = DeleteDatabaseModal;
exports.AddTableModal = AddTableModal;
exports.RemoveTableModal = RemoveTableModal;
exports.RemoveServerModal = RemoveServerModal;
exports.ReconfigureModal = ReconfigureModal;

function __guard__(value, transform) {
  return (typeof value !== 'undefined' && value !== null) ? transform(value) : undefined;
}
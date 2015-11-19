# Copyright 2010-2012 RethinkDB, all rights reserved.

app = require('./app.coffee')
driver = app.driver
system_db = app.system_db
topbar = require('./topbar.coffee')
navbar = require('./navbar.coffee')
models = require('./models.coffee')
modals = require('./modals.coffee')
router = require('./router.coffee')
VERSION = require('rethinkdb-version')

r = require('rethinkdb')

class MainContainer extends Backbone.View
    template: require('../handlebars/body-structure.hbs')
    id: 'main_view'

    initialize: =>
        @fetch_ajax_data()

        @databases = new models.Databases
        @tables = new models.Tables
        @servers = new models.Servers
        @issues = new models.Issues
        @dashboard = new models.Dashboard

        @alert_update_view = new AlertUpdates

        @options_view = new OptionsView
            alert_update_view: @alert_update_view
        @options_state = 'hidden'

        @navbar = new navbar.NavBarView
            databases: @databases
            tables: @tables
            servers: @servers
            options_view: @options_view
            container: @

        @topbar = new topbar.Container
            model: @dashboard
            issues: @issues

    # Should be started after the view is injected in the DOM tree
    start_router: =>
        @router = new router.BackboneCluster
            navbar: @navbar
        Backbone.history.start()

        @navbar.init_typeahead()

    fetch_ajax_data: =>
        driver.connect (error, conn) =>
            if error?
                console.log(error)
                @fetch_data(null)
            else
                conn.server (error, me) =>
                    driver.close conn
                    if error?
                        console.log(error)
                        @fetch_data(null)
                    else
                        @fetch_data(me.name)

    fetch_data: (me) =>
        query = r.expr
            tables: r.db(system_db).table('table_config').pluck('db', 'name', 'id').coerceTo("ARRAY")
            servers: r.db(system_db).table('server_config').pluck('name', 'id').coerceTo("ARRAY")
            issues: driver.queries.issues_with_ids()
            num_issues: r.db(system_db).table('current_issues').count()
            num_servers: r.db(system_db).table('server_config').count()
            num_tables: r.db(system_db).table('table_config').count()
            num_available_tables: r.db(system_db).table('table_status')('status').filter( (status) ->
                status("all_replicas_ready")
            ).count()
            me: me


        @timer = driver.run query, 5000, (error, result) =>
            if error?
                console.log(error)
            else
                for table in result.tables
                    @tables.add new models.Table(table), {merge: true}
                    delete result.tables
                for server in result.servers
                    @servers.add new models.Server(server), {merge: true}
                    delete result.servers
                @issues.set(result.issues)
                delete result.issues

                @dashboard.set result

    render: =>
        @$el.html @template()
        @$('#updates_container').html @alert_update_view.render().$el
        @$('#options_container').html @options_view.render().$el
        @$('#topbar').html @topbar.render().$el
        @$('#navbar-container').html @navbar.render().$el

        @

    hide_options: =>
        @$('#options_container').slideUp 'fast'

    toggle_options: (event) =>
        event.preventDefault()

        if @options_state is 'visible'
            @options_state = 'hidden'
            @hide_options event
        else
            @options_state = 'visible'
            @$('#options_container').slideDown 'fast'

    remove: =>
        driver.stop_timer @timer
        @alert_update_view.remove()
        @options_view.remove()
        @navbar.remove()

class OptionsView extends Backbone.View
    className: 'options_background'
    template: require('../handlebars/options_view.hbs')

    events:
        'click label[for=updates_yes]': 'turn_updates_on'
        'click label[for=updates_no]': 'turn_updates_off'

    initialize: (data) =>
        @alert_update_view = data.alert_update_view

    render: =>
        @$el.html @template
            check_update: if window.localStorage?.check_updates? then JSON.parse window.localStorage.check_updates else true
            version: VERSION
        @

    turn_updates_on: (event) =>
        window.localStorage.check_updates = JSON.stringify true
        window.localStorage.removeItem('ignore_version')
        @alert_update_view.check()

    turn_updates_off: (event) =>
        window.localStorage.check_updates = JSON.stringify false
        @alert_update_view.hide()

class AlertUpdates extends Backbone.View
    has_update_template: require('../handlebars/has_update.hbs')
    className: 'settings alert'

    events:
        'click .no_update_btn': 'deactivate_update'
        'click .close': 'close'

    initialize: =>
        if window.localStorage?
            try
                check_updates = JSON.parse window.localStorage.check_updates
                if check_updates isnt false
                    @check()
            catch err
                # Non valid json doc in check_updates. Let's reset the setting
                window.localStorage.check_updates = JSON.stringify true
                @check()
        else
            # No localstorage, let's just check for updates
            @check()

    # If the user close the alert, we hide the alert + save the version so we can ignore it
    close: (event) =>
        event.preventDefault()
        if @next_version?
            window.localStorage.ignore_version = JSON.stringify @next_version
        @hide()

    hide: =>
        @$el.slideUp 'fast'

    check: =>
        # If it's fail, it's fine - like if the user is just on a local network without access to the Internet.
        $.getJSON "https://update.rethinkdb.com/update_for/#{VERSION}?callback=?", @render_updates

    # Callback on the ajax request
    render_updates: (data) =>
        if data.status is 'need_update'
            try
                ignored_version = JSON.parse(window.localStorage.ignore_version)
            catch err
                ignored_version = null
            if (not ignored_version) or @compare_version(ignored_version, data.last_version) < 0
                @next_version = data.last_version # Save it so users can ignore the update
                @$el.html @has_update_template
                    last_version: data.last_version
                    link_changelog: data.link_changelog
                @$el.slideDown 'fast'

    # Compare version with the format %d.%d.%d
    compare_version: (v1, v2) =>
        v1_array_str = v1.split('.')
        v2_array_str = v2.split('.')
        v1_array = []
        for value in v1_array_str
            v1_array.push parseInt value
        v2_array = []
        for value in v2_array_str
            v2_array.push parseInt value

        for value, index in v1_array
            if value < v2_array[index]
                return -1
            else if value > v2_array[index]
                return 1
        return 0


    render: =>
        return @

    deactivate_update: =>
        @$el.slideUp 'fast'
        if window.localStorage?
            window.localStorage.check_updates = JSON.stringify false


class IsDisconnected extends Backbone.View
    el: 'body'
    className: 'is_disconnected_view'
    template: require('../handlebars/is_disconnected.hbs')
    message: require('../handlebars/is_disconnected_message.hbs')
    initialize: =>
        @render()
        setInterval ->
            driver.run_once r.expr(1)
        , 2000

    render: =>
        @$('#modal-dialog > .modal').css('z-index', '1')
        @$('.modal-backdrop').remove()
        @$el.append @template()
        @$('.is_disconnected').modal
            'show': true
            'backdrop': 'static'
        @animate_loading()

    animate_loading: =>
        if @$('.three_dots_connecting')
            if @$('.three_dots_connecting').html() is '...'
                @$('.three_dots_connecting').html ''
            else
                @$('.three_dots_connecting').append '.'
            setTimeout(@animate_loading, 300)

    display_fail: =>
        @$('.animation_state').fadeOut 'slow', =>
            $('.reconnecting_state').html(@message)
            $('.animation_state').fadeIn('slow')


exports.MainContainer = MainContainer
exports.OptionsView = OptionsView
exports.AlertUpdates = AlertUpdates
exports.IsDisconnected = IsDisconnected

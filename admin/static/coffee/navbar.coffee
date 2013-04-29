# Copyright 2010-2012 RethinkDB, all rights reserved.

# navigation bar view. Not sure where it should go, putting it here
# because it's global.
class NavBarView extends Backbone.View
    id: 'navbar'
    className: 'container'
    template: Handlebars.templates['navbar_view-template']
    events:
        'click .options_link': 'show_options'
        'click .close': 'show_options'
        'click label[for=updates_yes]': 'turn_updates_on'
        'click label[for=updates_no]': 'turn_updates_off'
        'click .options_container': 'stop_propagation'


    initialize: =>
        @first_render = true
        @options_state = 'hidden' # can be 'hidden' or 'visible'

    init_typeahead: => # Has to be called after we have injected the template
        @.$('input.search-box').typeahead
            source: (typeahead, query) ->
                _machines = _.map machines.models, (machine) ->
                    uuid: machine.get('id')
                    name: machine.get('name') + ' (machine)'
                    type: 'servers'
                _datacenters = _.map datacenters.models, (datacenter) ->
                    uuid: datacenter.get('id')
                    name: datacenter.get('name') + ' (datacenter)'
                    type: 'datacenters'
                _namespaces = _.map namespaces.models, (namespace) ->
                    uuid: namespace.get('id')
                    name: namespace.get('name') + ' (namespace)'
                    type: 'tables'
                _databases = _.map databases.models, (database) ->
                    uuid: database.get('id')
                    name: database.get('name') + ' (database)'
                    type: 'databases'
                return _machines.concat(_datacenters).concat(_namespaces).concat(_databases)
            property: 'name'
            onselect: (obj) ->
                window.app.navigate('#' + obj.type + '/' + obj.uuid , { trigger: true })

    render: (route) =>
        log_render '(rendering) NavBarView'
        @.$el.html @template
            check_update: if window.localStorage?.check_updates? then JSON.parse window.localStorage.check_updates else false
            version: window.VERSION
        # set active tab
        @set_active_tab route

        if @first_render is true
            # Initialize typeahead
            @init_typeahead()
            @first_render = false

        return @

    set_active_tab: (route) =>
        if route?
            @.$('ul.nav-left li').removeClass('active')
            switch route 
                when 'route:dashboard'          then @.$('li#nav-dashboard').addClass('active')
                when 'route:index_namespaces'   then @.$('li#nav-namespaces').addClass('active')
                when 'route:namespace'          then @.$('li#nav-namespaces').addClass('active')
                when 'route:database'           then @.$('li#nav-namespaces').addClass('active')
                when 'route:index_servers'      then @.$('li#nav-servers').addClass('active')
                when 'route:server'             then @.$('li#nav-servers').addClass('active')
                when 'route:datacenter'         then @.$('li#nav-servers').addClass('active')
                when 'route:dataexplorer'       then @.$('li#nav-dataexplorer').addClass('active')
                when 'route:logs'               then @.$('li#nav-logs').addClass('active')

    # Stop propagation of the event when the user click on $('.options_container')
    stop_propagation: (event) =>
        event.stopPropagation()

    # Hide the options view
    hide: (event) =>
        event.preventDefault()
        @options_state = 'hidden'
        @$('.options_container_arrow').hide()
        @$('.options_container_arrow_overlay').hide()
        @$('.options_container').hide()
        @$('.cog_icon').removeClass 'active'
        $(window).unbind 'click', @hide

    # Show the options view
    show_options: (event) =>
        event.preventDefault()
        event.stopPropagation()
        if @options_state is 'visible'
            @hide event
        else
            @options_state = 'visible'
            @$('.options_container_arrow').show()
            @$('.options_container_arrow_overlay').show()
            @$('.options_container').show()
            @$('.cog_icon').addClass 'active'
            $(window).click @hide
            @delegateEvents()

    turn_updates_on: (event) =>
        window.localStorage.check_updates = JSON.stringify true
        window.localStorage.removeItem('ignore_version')
        window.alert_update_view.check()

    turn_updates_off: (event) =>
        window.localStorage.check_updates = JSON.stringify false
        window.alert_update_view.hide()

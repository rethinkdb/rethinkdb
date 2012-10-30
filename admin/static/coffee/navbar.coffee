# Copyright 2010-2012 RethinkDB, all rights reserved.

# navigation bar view. Not sure where it should go, putting it here
# because it's global.
class NavBarView extends Backbone.View
    id: 'navbar'
    className: 'container'
    template: Handlebars.compile $('#navbar_view-template').html()

    initialize: =>
        @first_render = true

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
        @.$el.html @template()
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

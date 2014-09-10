# Copyright 2010-2012 RethinkDB, all rights reserved.

module 'TopBar', ->
    class @NavBarView extends Backbone.View
        id: 'navbar'
        className: 'container'
        template: Handlebars.templates['navbar_view-template']
        events:
            'click .options_link': 'update_cog_icon'

        initialize: (data) =>
            @databases = data.databases
            @tables = data.tables
            @servers = data.servers

            @container = data.container

            @options_state = 'hidden' # can be 'hidden' or 'visible'

        init_typeahead: => # Has to be called after we have injected the template
            @$('input.search-box').typeahead
                source: (typeahead, query) =>
                    servers = _.map @servers.models, (server) ->
                        id: server.get('id')
                        name: server.get('name') + ' (machine)'
                        type: 'servers'
                    tables = _.map @tables.models, (table) ->
                        id: table.get('id')
                        name: table.get('name') + ' (table)'
                        type: 'tables'
                    databases = _.map @databases.models, (database) ->
                        id: database.get('id')
                        name: database.get('name') + ' (database)'
                        type: 'databases'
                    return servers.concat(tables).concat(databases)
                property: 'name'
                onselect: (obj) ->
                    window.app.navigate('#' + obj.type + '/' + obj.id , { trigger: true })

        render: (route) =>
            @$el.html @template()
            @set_active_tab route
            @

        set_active_tab: (route) =>
            if route?
                switch route
                    when 'dashboard'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-dashboard').addClass('active')
                    when 'index_tables'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-namespaces').addClass('active')
                    when 'table'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-namespaces').addClass('active')
                    when 'database'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-namespaces').addClass('active')
                    when 'index_servers'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-servers').addClass('active')
                    when 'server'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-servers').addClass('active')
                    when 'datacenter'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-servers').addClass('active')
                    when 'dataexplorer'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-dataexplorer').addClass('active')
                    when 'logs'
                        @$('ul.nav-left li').removeClass('active')
                        @$('li#nav-logs').addClass('active')

        update_cog_icon: (event) =>
            @$('.cog_icon').toggleClass 'active'
            @container.toggle_options event

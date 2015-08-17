# TopBar view

issues = require('./issues.coffee')
app = require('./app.coffee')
driver = app.driver

class Container extends Backbone.View
    className: 'sidebar-container'
    template: require('../handlebars/sidebar-container.hbs')

    initialize: (data) =>
        @model = data.model
        @issues = data.issues

        @client_panel = new ClientConnectionStatus
            model: @model
        @servers_panel = new ServersConnected
            model: @model
        @tables_panel = new TablesAvailable
            model: @model
        @issues_panel = new Issues
            model: @model
        @issues_banner = new issues.IssuesBanner
            model: @model
            collection: @issues

    render: =>
        @$el.html @template()

        # Render connectivity status
        @$('.client-connection-status').html @client_panel.render().el
        @$('.servers-connected').html @servers_panel.render().el
        @$('.tables-available').html @tables_panel.render().el

        # Render issue summary and issue banner
        @$('.issues').html @issues_panel.render().el
        @$el.append(@issues_banner.render().el)
        @

    remove_parent_alert: (event) =>
        element = $(event.target).parent()
        element.slideUp 'fast', =>
            element.remove()
            @issues_being_resolved()
            @issues_banner.render()

    remove: =>
        @client_panel.remove()
        @servers_panel.remove()
        @tables_panel.remove()
        @issues_panel.remove()
        @issues_banner.remove()
        super()


class ClientConnectionStatus extends Backbone.View
    className: 'client-connection-status'
    template: require('../handlebars/sidebar-client_connection_status.hbs')

    initialize: =>
        @listenTo @model, 'change:me', @render

    render: =>
        @$el.html @template
            disconnected: false
            me: @model.get 'me'
        @

    remove: =>
        @stopListening()
        super()


class ServersConnected extends Backbone.View
    template: require('../handlebars/sidebar-servers_connected.hbs')

    initialize: =>
        @listenTo @model, 'change:num_servers', @render

    render: =>
        @$el.html @template
            num_servers: @model.get 'num_servers'
        @

    remove: =>
        @stopListening()
        super()


class TablesAvailable extends Backbone.View
    template: require('../handlebars/sidebar-tables_available.hbs')

    initialize: =>
        @listenTo @model, 'change:num_tables', @render
        @listenTo @model, 'change:num_available_tables', @render


    render: =>
        @$el.html @template
            num_tables: @model.get 'num_tables'
            num_available_tables: @model.get 'num_available_tables'
            error: @model.get('num_available_tables') < @model.get 'num_tables'
        @

    remove: =>
        @stopListening()
        super()


# Issue count panel at the top
class Issues extends Backbone.View
    className: 'issues'
    template: require('../handlebars/sidebar-issues.hbs')

    initialize: =>
        @listenTo @model, 'change:num_issues', @render

    render: =>
        @$el.html @template
            num_issues: @model.get 'num_issues'
        @

    remove: =>
        @stopListening()
        super()


exports.Container = Container
exports.ClientConnectionStatus = ClientConnectionStatus
exports.ServersConnected = ServersConnected
exports.TablesAvailable = TablesAvailable
exports.Issues = Issues

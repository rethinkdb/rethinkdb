# Copyright 2010-2015 RethinkDB
# Resolve issues view

models = require('./models.coffee')
modals = require('./modals.coffee')
ui_modals = require('./ui_components/modals.coffee')


templates =
    # local issues: local_issue_aggregator.hpp
    log_write_error: # log_write.cc
        Handlebars.templates['issue-log_write_error']
    outdated_index:
        Handlebars.templates['issue-outdated_index']
    server_disconnected:
        Handlebars.templates['issue-server_disconnected']
    server_ghost:
        Handlebars.templates['issue-server_ghost']
    # name collision issues: name_collision.hpp
    server_name_collision:
        Handlebars.templates['issue-name-collision']
    db_name_collision:
        Handlebars.templates['issue-name-collision']
    table_name_collision:
        Handlebars.templates['issue-name-collision']

    # invalid config issues: invalid_config.hpp
    table_needs_primary:
        Handlebars.templates['issue-table_needs_primary']
    data_lost:
        Handlebars.templates['issue-data_lost']
    write_acks:
        Handlebars.templates['issue-write-acks']
    # catchall
    unknown:
        Handlebars.templates['resolve_issues-unknown-template']

class Issue extends Backbone.View
    className: 'issue-container'
    initialize: (data) =>
        @type = data.model.get 'type'
        @model.set('collisionType', @parseCollisionType @model.get('type'))
        @listenTo @model, 'change', @render

    events:
        'click .remove-server': 'remove_server'
        'click .rename': 'rename'

    render: =>
        if templates[@type]?
            @$el.html templates[@type] @model.toJSON()
        else
            @$el.html templates.unknown @model.toJSON()
        @

    parseCollisionType: (fulltype) =>
        match = fulltype.match(/(db|server|table)_name_collision/)
        if match?
            if match[1] == 'db'
                'database'
            else
                match[1]
        else
            match

    rename: (event) =>
        Type = switch @model.get('collisionType')
            when 'database' then models.Database
            when 'server'   then models.Server
            when 'table'    then models.Table

        @modal = new ui_modals.RenameItemModal
            model: new Type
                name: @model.get('info').name
                id: $(event.target).attr('id').match(/rename_(.*)/)[1]
        @modal.render()

    remove_server: (event) =>
        modalModel = new Backbone.Model
            name: @model.get('info').disconnected_server
            id: @model.get('info').disconnected_server_id
            parent: @model
        @modal = new modals.RemoveServerModal
            model: modalModel

        @modal.render()

module.exports =
    templates: templates
    Issue: Issue

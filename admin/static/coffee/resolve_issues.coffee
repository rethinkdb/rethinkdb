# Copyright 2010-2015 RethinkDB
# Resolve issues view

models = require('./models.coffee')
modals = require('./modals.coffee')
ui_modals = require('./ui_components/modals.coffee')


templates =
    # local issues: local_issue_aggregator.hpp
    log_write_error: # log_write.cc
        require('../handlebars/issue-log_write_error.hbs')
    outdated_index:
        require('../handlebars/issue-outdated_index.hbs')
    server_disconnected:
        require('../handlebars/issue-server_disconnected.hbs')
    server_ghost:
        require('../handlebars/issue-server_ghost.hbs')
    # name collision issues: name_collision.hpp
    server_name_collision:
        require('../handlebars/issue-name-collision.hbs')
    db_name_collision:
        require('../handlebars/issue-name-collision.hbs')
    table_name_collision:
        require('../handlebars/issue-name-collision.hbs')

    # invalid config issues: invalid_config.hpp
    table_needs_primary:
        require('../handlebars/issue-table_needs_primary.hbs')
    data_lost:
        require('../handlebars/issue-data_lost.hbs')
    write_acks:
        require('../handlebars/issue-write-acks.hbs')
    # catchall
    unknown:
        require('../handlebars/issue-unknown.hbs')

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


exports.templates = templates
exports.Issue = Issue

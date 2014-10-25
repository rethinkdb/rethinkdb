# Copyright 2010-2012 RethinkDB, all rights reserved.
# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Issue
    @templates =
        server_down:
            Handlebars.templates['resolve_issues-server_down-template']
        server_ghost:
            Handlebars.templates['resolve_issues-server_ghost-template']
        server_name_collision:
            Handlebars.templates['resolve_issues-name_collision-template']
        db_name_collision:
            Handlebars.templates['resolve_issues-name_collision-template']
        table_name_collision:
            Handlebars.templates['resolve_issues-name_collision-template']
        outdated_index:
            Handlebars.templates['resolve_issues-outdated_index-template']
        no_such_server:
            Handlebars.templates['resolve_issues-port_conflict-template']
        log_write_error:
            Handlebars.templates['resolve_issues-logfile_write-template']
        outdated_index:
            Handlebars.templates['resolve_issues-outdated_index-template']
        unknown:
            Handlebars.templates['resolve_issues-unknown-template']

    class @Issue extends Backbone.View
        className: 'issue-container'
        initialize: (data) =>
            @type = data.model.get 'type'
            @model = data.model
            @model.set('collisionType', @parseCollisionType @model.get('type'))
            @listenTo @model, 'change', @render
            @listenTo

        events:
            'click .solve-issue': 'solve'
            'click .rename': 'rename'

        render: =>
            templates = ResolveIssuesView.templates
            if templates[@model.get('type')]?
                @$el.html templates[@model.get('type')] @model.toJSON()
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
                when 'database' then Database
                when 'server'   then Server
                when 'table'    then Table

            @modal = new UIComponents.RenameItemModal
                model: new Type
                    name: @model.get('info').name
                    id: $(event.target).attr('id').match(/rename_(.*)/)[1]
            @modal.render()

        solve: (event) =>
            console.log event
            switch @model.get('type')
                when 'server_down'
                    console.log @model
                    @modal = new Modals.RemoveServerModal
                        model: new Backbone.Model
                            name: @model.get('info').server
                            id: @model.get('info').server_id
                    @modal.render(@model)
                when 'outdated_index'
                    #TODO We do not provide a way to fix this issue
                    console.log 'Cannot fix indexes'
                when 'no_such_server'
                    #TODO Create new model
                    #model =
                    @modal = new Modals.RemoveServerModal
                        model: model
                    @modal.render()
                else
                    #TODO Remove once testing is done
                    debugger

        #TODO
        #On solve, we should update the model to "fixed"

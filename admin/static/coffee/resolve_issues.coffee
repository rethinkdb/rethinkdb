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
        templates: @templates
        initialize: (data) =>
            @type = data.model.get 'type'
            @model = data.model
            @listenTo @model, 'change', @render

        events:
            'click .solve-issue': @solve

        render: =>
            templates = ResolveIssuesView.templates
            viewmodel = @model.toJSON()
            viewmodel.collisionType = @parseCollisionType(viewmodel.type)
            if templates[@model.get('type')]?
                @$el.html templates[@model.get('type')] viewmodel
            else
                @$el.html templates.unknown viewmodel
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

        solve: (event) =>
            switch @model.get('type')
                when 'server_down'
                    #TODO Create new model
                    #model =
                    @modal = new Modals.RemoveServerModal
                        model: @model
                    @modal.render()
                when 'server_ghost'
                    #TODO Are we shipping something to kill ghosts?
                    console.log 'Cannot kill ghost'
                when 'server_name_collision'
                    #TODO Create new model
                    #model =
                    @modal = new UIComponents.RenameItemModal
                        model: model
                    @modal.render()
                when 'db_name_collision'
                    #TODO Create new model
                    #model =
                    @modal = new UIComponents.RenameItemModal
                        model: model
                    @modal.render()
                when 'table_name_collision'
                    #TODO Create new model
                    #model =
                    @modal = new UIComponents.RenameItemModal
                        model: model
                    @modal.render()
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

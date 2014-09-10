# Copyright 2010-2012 RethinkDB, all rights reserved.
# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Issue
    class @Issue extends Backbone.View
        className: 'issue-container'
        templates: #TODO Lower case the keys
            server_down: Handlebars.templates['resolve_issues-machine_down-template']
            server_ghost: Handlebars.templates['resolve_issues-name_conflict-template']
            server_name_collision: Handlebars.templates['resolve_issues-logfile_write-template']
            db_name_collision: Handlebars.templates['resolve_issues-vclock_conflict-template']
            table_name_collision: Handlebars.templates['resolve_issues-unsatisfiable_goals-template']
            outdated_index: Handlebars.templates['resolve_issues-machine_ghost-template']
            no_such_server: Handlebars.templates['resolve_issues-port_conflict-template']
            unknown: Handlebars.templates['resolve_issues-unknown-template']
            #TODO Add other issues not yet available

        initialize: =>
            @listenTo @model, 'change', @render

        events:
            'click .solve-issue': @solve

        render: =>
            if @templates[@model.get('type')]?
                @$el.html @templates[@model.get('type')] @model.toJSON()
            else
                @$el.html @templates.unknown @model.toJSON()

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

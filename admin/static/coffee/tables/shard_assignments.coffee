# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'TableView', ->
    class @ShardAssignmentsView extends Backbone.View
        template: Handlebars.templates['shard_assignments-template']

        initialize: (data) =>
            @model = data.model
            if data.collection?
                @loading = false
                @collection = data.collection
            else
                @loading = true
            @assignments_view = []

        set_assignments: (assignments) =>
            @loading = false
            @collection = assignments
            @render()

        render: =>
            @$el.html @template
                loading: @loading
            if @collection?
                @collection.each (assignment) =>
                    view = new TableView.ShardAssignmentView
                        model: assignment
                        container: @
                    # The first time, the collection is sorted
                    @assignments_view.push view
                    @$('.assignments_list').append view.render().$el

                @listenTo @collection, 'add', (assignment) =>
                    view = new TableView.ShardAssignmentView
                        model: assignment
                        container: @
                    @assignments_view.push view

                    position = @collection.indexOf assignment
                    if @collection.length is 1
                        @$('.assignments_list').html view.render().$el
                    else if position is 0
                        @$('.assignments_list').prepend view.render().$el
                    else
                        @$('.assignment_container').eq(position-1).after view.render().$el

                @listenTo @collection, 'remove', (assignment) =>
                    for view, position in @assignments_view
                        if view.model is assignment
                            assignment.destroy()
                            view.remove()
                            @assignments_view.splice position, 1
                            break
            @

        remove: =>
            @stopListening()

            for view in @assignments_view
                view.model.destroy()
                view.remove()
            super()


    class @ShardAssignmentView extends Backbone.View
        template: Handlebars.templates['shard_assignment-template']
        className: 'assignment_container'

        initialize: =>
            @listenTo @model, 'change', @render

        render: =>
            @$el.html @template @model.toJSON()
            @

        remove: =>
            @stopListening()
            super()

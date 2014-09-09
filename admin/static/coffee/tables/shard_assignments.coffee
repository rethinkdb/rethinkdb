# Copyright 2010-2012 RethinkDB, all rights reserved.
module 'TableView', ->
    class @ShardAssignmentsView extends Backbone.View
        template: Handlebars.templates['shard_assignments-template']

        initialize: (data) =>
            @model = data.model
            @collection = data.collection
            @assignments_view = []

        render: =>
            @$el.html @template()
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
                for view in @assignments_view
                    if view.model is assignment
                        assignment.destroy()
                        view.remove()
                        break
            @

        remove: =>
            @stopListeningTo()

            for view in @assignments_view
                assignment.destroy()
                view.remove()


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


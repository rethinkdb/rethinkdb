# Copyright 2010-2015 RethinkDB

models = require('../models.coffee')

class ShardAssignmentsView extends Backbone.View
    template: require('../../handlebars/shard_assignments.hbs')

    initialize: (data) =>
        @listenTo @model, 'change:info_unavailable', @set_warnings
        if data.collection?
            @collection = data.collection
        @assignments_view = []

    set_warnings: ->
        if @model.get('info_unavailable')
            @$('.unavailable-error').show()
        else
            @$('.unavailable-error').hide()

    set_assignments: (assignments) =>
        @collection = assignments
        @render()

    render: =>
        @$el.html @template @model.toJSON()
        if @collection?
            @collection.each (assignment) =>
                view = new ShardAssignmentView
                    model: assignment
                    container: @
                # The first time, the collection is sorted
                @assignments_view.push view
                @$('.assignments_list').append view.render().$el

            @listenTo @collection, 'add', (assignment) =>
                new_view = new ShardAssignmentView
                    model: assignment
                    container: @

                if @assignments_view.length is 0
                    @assignments_view.push new_view
                    @$('.assignments_list').html new_view.render().$el
                else
                    added = false
                    for view, position in @assignments_view
                        if models.ShardAssignments.prototype.comparator(view.model, assignment) > 0
                            added = true
                            @assignments_view.splice position, 0, new_view
                            if position is 0
                                @$('.assignments_list').prepend new_view.render().$el
                            else
                                @$('.assignment_container').eq(position-1).after new_view.render().$el
                            break
                    if added is false
                        @assignments_view.push new_view
                        @$('.assignments_list').append new_view.render().$el


            @listenTo @collection, 'remove', (assignment) =>
                for view, position in @assignments_view
                    if view.model is assignment
                        assignment.destroy()
                        view.remove()
                        @assignments_view.splice position, 1
                        break
        @set_warnings()
        @

    remove: =>
        @stopListening()

        for view in @assignments_view
            view.model.destroy()
            view.remove()
        super()


class ShardAssignmentView extends Backbone.View
    template: require('../../handlebars/shard_assignment.hbs')
    className: 'assignment_container'

    initialize: =>
        @listenTo @model, 'change', @render

    render: =>
        @$el.html @template @model.toJSON()
        @

    remove: =>
        @stopListening()
        super()


exports.ShardAssignmentsView = ShardAssignmentsView
exports.ShardAssignmentView = ShardAssignmentView

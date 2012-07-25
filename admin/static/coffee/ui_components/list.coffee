# This file extends the UIComponents module with commonly used list
# components.
module 'UIComponents', ->
    # Abstract list container
    class @AbstractList extends Backbone.View
        # Use a generic template by default for a list
        template: Handlebars.compile $('#abstract_list-template').html()

        # Abstract lists take several arguments:
        #   collection: Backbone collection that backs the list
        #   element_view_class: Backbone view that each element in the list will be rendered with
        #   container: JQuery selector string specifying the div that each element view will be appended to
        # They can also take several options:
        #   filter: optional filter that defines what elements to use from the collection using a truth test
        #           (function whose argument is a Backbone model and whose output is true/false)
        #   sort: optional function that defines a sort order for the lista (defaults to Array.prototype.sort)
        initialize: (collection, element_view_class, container, options) ->
            #log_initial '(initializing) list view: ' + class_name @collection
            @collection = collection
            @element_views = []
            @container = container
            @element_view_class = element_view_class
            @options = options
            @size = 0

            # This filter defines which models from the collection should be represented by this list.
            # If one was not provided, define a filter that allows all models
            if @options? and @options.filter?
                @filter = @options.filter
            else
                @filter = (model) -> true

            if @options? and @options.sort?
                @sort = @options.sort
            else @sort = (a,b) -> 0

            # Initially we need to populate the element views list
            @reset_element_views()

            # Collection is reset, create all new views
            @collection.on 'reset', (collection) =>
                @reset_element_views()
                @render()

            # When an element is added to the collection, create a view for it
            @collection.on 'add', (model, collection) =>
                # Make sure the model is relevant for this list before we add it
                if @filter model
                    @add_element model
                    @render()

            @collection.on 'remove', (model, collection) =>
                # Make sure the model is relevant for this list before we remove it
                if @filter model
                    @remove_elements model

        render: =>
            @element_views.sort @sort
            #log_render '(rendering) list view: ' + class_name @collection
            # Render and append the list template to the DOM
            @.$el.html(@template({}))

            #log_action '#render_elements of collection ' + class_name @collection
            # Render the view for each element in the list
            @.$(@container).append(view.render().$el) for view in @element_views

            @.delegateEvents()
            return @

        reset_element_views: =>
            # Wipe out any existing list element views
            for view in @element_views
                view.destroy()
                view.remove() 
            @element_views = []

            # Add an element view for each model in the collection that's relevant for this list
            for model in @collection.models
                @add_element model if @filter model

        add_element: (model) =>
            # adds a list element view to element_views
            element_view = new @element_view_class
                model: model
                collection: @collection
                args: @options.element_args if @options?

            @element_views.push element_view
            @size = @element_views.length
            @.trigger 'size_changed'

            return element_view

        remove_elements: (model) =>
            # Find any views that are based on the model being removed
            matching_views = (view for view in @element_views when view.model is model)

            # For all matching views, remove the view from the DOM and from the known element views list
            for view in matching_views
                view.destroy()
                view.remove()
                @element_views = _.without @element_views, view

            @size = @element_views.length
            @.trigger 'size_changed'

            # Return the number of views removed
            return matching_views.length


        # Return an array of elements that are selected in the current list
        get_selected_elements: =>
            selected_elements = []
            selected_elements.push view.model for view in @element_views when view.selected
            return selected_elements

        # Gets a list of UI callbacks this list will want to register (e.g. with a CheckboxListElement)
        get_callbacks: -> []

        destroy: =>
            @collection.off()
            for view in @element_views
                view.destroy()

    # Abstract list element that show info about the element and has a checkbox
    class @CheckboxListElement extends Backbone.View
        tagName: 'tr'
        className: 'element'

        events: ->
            'click': 'clicked'
            'click a': 'link_clicked'

        initialize: (template) ->
            @template = template
            @selected = false

        render: =>
            #log_render '(rendering) list element view: ' + class_name @model
            @.$el.html @template(@json_for_template())
            @mark_selection()
            @delegateEvents()

            return @

        json_for_template: =>
            return @model.toJSON()

        clicked: (event) =>
            @selected = not @selected
            @mark_selection()
            @.trigger 'selected'

        link_clicked: (event) =>
            # Prevents checkbox toggling when we click a link.
            event.stopPropagation()

        mark_selection: =>
            # Toggle the selected class  and check / uncheck  its checkbox
            @.$el.toggleClass 'selected', @selected
            @.$(':checkbox').prop 'checked', @selected

    # Abstract list element that allows collapsing the item
    class @CollapsibleListElement extends Backbone.View
        events: ->
            'click .header': 'toggle_showing'
            'click a': 'link_clicked'

        initialize: ->
            @showing = true

        render: =>
            @show()
            @delegateEvents()

        link_clicked: (event) =>
            # Prevents collapsing when we click a link.
            event.stopPropagation()

        toggle_showing: =>
            @showing = not @showing
            @show()

        show: =>
            @.$('.machine-list').toggle @showing
            @swap_divs()

        show_with_transition: =>
            @.$('.machine-list').slideToggle @showing
            @swap_divs()

        swap_divs: =>
            $arrow = @.$('.arrow.collapsed, .arrow.expanded')

            for div in [$arrow]
                if @showing
                    div.removeClass('collapsed').addClass('expanded')
                else
                    div.removeClass('expanded').addClass('collapsed')


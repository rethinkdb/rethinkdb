# Helpers to eliminate some boilerplate for using virtual-dom and
# backbone together.

app = require('./app.coffee')
driver = app.driver

r = require('rethinkdb')
h = require('virtual-dom/h')
diff = require('virtual-dom/diff')
patch = require('virtual-dom/patch')
createElement = require('virtual-dom/create-element')

class VirtualDomView extends Backbone.View
    initialize: (options) ->
        @submodels = options?.submodels or {}
        @timers = options?.timers or {}
        @query_error = null
        if @fetch
            @fetch_results()

        @init(options)

        @current_vdom_tree = @render_vdom()
        @setElement(createElement(@current_vdom_tree))

        if @model?
            @listenTo @model, 'change', @render

    init: =>
        # Override for special initialization if necessary

    render: =>
        new_tree = @render_vdom()
        patch(@el, diff(@current_vdom_tree, new_tree))
        @current_vdom_tree = new_tree
        @timers = @

    fetch_results: =>
        # Note that submodel names should be unique, even among
        # different queries, since they'll be flattened out in the
        # submodels object
        for frequency, query of @fetch
            query_obj = {}
            for name, model of query
                query_obj[name] = model.query()
                @submodels[name] = new model()
            callback = (error, result) =>
                if error?
                    @query_error = error
                    @on_query_error(error)
                else
                    @query_error = null
                    for name in Object.keys(@submodels)
                        @on_query_subresult(name, result[name])

            @timers[frequency] = driver.run(r.expr(query_obj), +frequency, callback)

    on_query_error: (error) =>
        # Override to handle query errors specially
        console.error(error)
        @render()

    on_query_subresult: (name, result) =>
        # Gets the query sub-result name and the sub-result
        # Override to customize handling query sub-results
        @submodels[name].set(result)

    remove: =>
        for frequency, timer in @timers
            driver.stop_timer(timer)
        # clean up any widgets by clearing the tree
        patch(@el, diff(@current_vdom_tree, h('div')))
        @cleanup()
        super

    cleanup: =>
        # For special cleanup actions if necessary


class BackboneViewWidget
    # An adapter class to allow a Backbone View to act as a
    # virtual-dom Widget. This allows the view to update itself and
    # not have to worry about being destroyed and reinserted, causing
    # loss of listeners etc.
    type: "Widget"  # required by virtual-dom to detect this as a widget.
    constructor: (thunk) ->
        # The constructor doesn't create the view or cause a render
        # because many many widgets will be created, and they need to
        # be very lightweight.
        @thunk = thunk
        @view = null
    init: () ->
        # init is called when virtual-dom first notices a widget. This
        # will initialize the view with the given options, render the
        # view, and return its DOM element for virtual-dom to place in
        # the document
        @view = @thunk()
        @view.render()
        @view.el  # init needs to return a raw dom element
    update: (previous, domNode) ->
        # Every time the vdom is updated, a new widget will be
        # created, which will be passed the old widget (previous). We
        # need to copy the state from the old widget over and return
        # null to indicate the DOM element should be re-used and not
        # replaced.
        @view = previous.view
        return null
    destroy: (domNode) ->
        # This is called when virtual-dom detects the widget should be
        # removed from the tree. It just calls `.remove()` on the
        # underlying view so listeners etc can be cleaned up.
        @view?.remove()


exports.VirtualDomView = VirtualDomView
exports.BackboneViewWidget = BackboneViewWidget

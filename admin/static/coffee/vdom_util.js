// Helpers to eliminate some boilerplate for using virtual-dom and
// backbone together.

const app = require('./app.coffee')
const { driver } = app

const r = require('rethinkdb')
const h = require('virtual-dom/h')
const diff = require('virtual-dom/diff')
const patch = require('virtual-dom/patch')
const createElement = require('virtual-dom/create-element')

class VirtualDomView extends Backbone.View {
  constructor (...args) {
    this.init = this.init.bind(this)
    this.render = this.render.bind(this)
    this.fetch_results = this.fetch_results.bind(this)
    this.on_query_error = this.on_query_error.bind(this)
    this.on_query_subresult = this.on_query_subresult.bind(this)
    this.remove = this.remove.bind(this)
    this.cleanup = this.cleanup.bind(this)
    super(...args)
  }

  initialize (options) {
    this.submodels = __guard__(options, ({ submodels }) => submodels) || {}
    this.timers = __guard__(options, ({ timers }) => timers) || {}
    this.query_error = null
    if (this.fetch) {
      this.fetch_results()
    }

    this.init(options)

    this.current_vdom_tree = this.render_vdom()
    this.setElement(createElement(this.current_vdom_tree))

    if (this.model != null) {
      return this.listenTo(this.model, 'change', this.render)
    }
  }

  init () {
  }
  // Override for special initialization if necessary

  render () {
    const new_tree = this.render_vdom()
    patch(this.el, diff(this.current_vdom_tree, new_tree))
    this.current_vdom_tree = new_tree
    return this.timers = this
  }

  fetch_results () {
    // Note that submodel names should be unique, even among
    // different queries, since they'll be flattened out in the
    // submodels object
    return (() => {
      const result = []
      for (let frequency in this.fetch) {
        const query = this.fetch[frequency]
        const query_obj = {}
        for (var name in query) {
          const model = query[name]
          query_obj[name] = model.query()
          this.submodels[name] = new model()
        }
        const callback = (error, result) => {
          if (error != null) {
            this.query_error = error
            return this.on_query_error(error)
          } else {
            this.query_error = null
            return (() => {
              const result1 = []
              for (name of Array.from(Object.keys(this.submodels))) {
                result1.push(this.on_query_subresult(name, result[name]))
              }
              return result1
            })()
          }
        }

        result.push(this.timers[frequency] = driver.run(r.expr(query_obj), +frequency, callback))
      }
      return result
    })()
  }

  on_query_error (error) {
    // Override to handle query errors specially
    console.error(error)
    return this.render()
  }

  on_query_subresult (name, result) {
    // Gets the query sub-result name and the sub-result
    // Override to customize handling query sub-results
    return this.submodels[name].set(result)
  }

  remove (...args) {
    this.timers.forEach((frequency, timer) => {
      driver.stop_timer(timer)
    })

    // clean up any widgets by clearing the tree
    patch(this.el, diff(this.current_vdom_tree, h('div')))
    this.cleanup()
    return super.remove(...args)
  }

  cleanup () {
  }
}
// For special cleanup actions if necessary

class BackboneViewWidget {
  static initClass () {
    // An adapter class to allow a Backbone View to act as a
    // virtual-dom Widget. This allows the view to update itself and
    // not have to worry about being destroyed and reinserted, causing
    // loss of listeners etc.
    this.prototype.type = 'Widget'
    // required by virtual-dom to detect this as a widget.
  }
  constructor (thunk) {
    // The constructor doesn't create the view or cause a render
    // because many many widgets will be created, and they need to
    // be very lightweight.
    this.thunk = thunk
    this.view = null
  }
  init () {
    // init is called when virtual-dom first notices a widget. This
    // will initialize the view with the given options, render the
    // view, and return its DOM element for virtual-dom to place in
    // the document
    this.view = this.thunk()
    this.view.render()
    return this.view.el // init needs to return a raw dom element
  }
  update ({ view }, domNode) {
    // Every time the vdom is updated, a new widget will be
    // created, which will be passed the old widget (previous). We
    // need to copy the state from the old widget over and return
    // null to indicate the DOM element should be re-used and not
    // replaced.
    this.view = view
    return null
  }
  destroy (domNode) {
    // This is called when virtual-dom detects the widget should be
    // removed from the tree. It just calls `.remove()` on the
    // underlying view so listeners etc can be cleaned up.
    return __guard__(this.view, x => x.remove())
  }
}
BackboneViewWidget.initClass()

exports.VirtualDomView = VirtualDomView
exports.BackboneViewWidget = BackboneViewWidget

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

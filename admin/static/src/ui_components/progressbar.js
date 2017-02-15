// Copyright 2010-2015 RethinkDB
// This file extends the UIComponents module with commonly used progressbars.

// Progress bar that has different states
// TODO clean this class, it's messy
class OperationProgressBar extends Backbone.View {
  constructor (...args) {
    this.reset_percent = this.reset_percent.bind(this)
    this.render = this.render.bind(this)
    this.get_stage = this.get_stage.bind(this)
    this.skip_to_processing = this.skip_to_processing.bind(this)
    this.set_none_state = this.set_none_state.bind(this)
    super(...args)
  }

  static initClass () {
    // These are the possible states for the ProgressBar
    this.prototype.states = ['none', 'starting', 'processing', 'finished']
  }

  // The initialize function takes one optional argument:
  //   - template: optional custom template
  initialize (template) {
    this.stage = 'none'

    if (template != null) {
      this.template = template
    } else {
      this.template = require('../../handlebars/progressbar.hbs')
    }
    this.timeout = null
    return this.percent = 0
  }

  reset_percent () {
    return this.percent = 0
  }

  // The render function takes a number of arguments:
  //   - current_value: current value of the operation or status being monitored
  //   - max_value: maximum value of the the operation or status being monitored
  //   - additional_info: optional argument, hash that can include a
  //     number of details to describe the state of the operation, etc.
  //       * new_value: the new desired maximum value as a result of the operation
  //       * got_response: flag indicating that the goals have been set
  //         and we're ready to start processing
  //   - cb: callback to execute once we remove the progress bar
  render (current_value, max_value, additional_info, cb) {
    let percent_complete
    if (current_value !== max_value && this.timeout != null) {
      // We are in a finished state, but current_value != max_value,
      // so the server sent us an update and we start processing again
      clearTimeout(this.timeout)
      this.timeout = null
      this.stage = 'processing'
    }

    // State machine
    if (this.stage === 'none' && additional_info.new_value != null) {
      this.stage = 'starting'
      if (this.timeout != null) {
        // If there is a timeout, we have to clear it, we are not done yet
        clearTimeout(this.timeout)
        this.timeout = null
      }
    }
    if (this.stage === 'starting' && additional_info.got_response != null) {
      this.stage = 'processing'
      if (this.timeout != null) {
        // If there is a timeout, we have to clear it, we are not done yet
        clearTimeout(this.timeout)
        this.timeout = null
      }
    }
    if (this.stage === 'processing' && current_value === max_value) {
      this.stage = 'finished'
      if (this.timeout != null) {
        // If there is a timeout, we have to clear it, we are not done yet
        clearTimeout(this.timeout)
        this.timeout = null
      }
    }

    let data = _.extend(additional_info, {
      current_value,
      max_value,
    })

    // Depending on the current state, choose the correct template data
    if (this.stage === 'starting') {
      data = _.extend(data, {
        operation_active: true,
        starting        : true,
      })
    }

    if (this.stage === 'processing') {
      if (current_value === max_value) {
        percent_complete = 0
      } else {
        percent_complete = Math.floor(current_value / max_value * 100)
      }

      // Make sure we never go back
      if (additional_info.check === true) {
        if (percent_complete < this.percent) {
          percent_complete = this.percent
        } else {
          this.percent = percent_complete
        }
      }

      data = _.extend(data, {
        operation_active: true,
        processing      : true,
        percent_complete,
      })
    }

    if (this.stage === 'finished') {
      data = _.extend(data, {
        operation_active: true,
        finished        : true,
        percent_complete: 100,
      })

      if (this.timeout == null) {
        // If there is already a timeout, no need to add another one
        this.timeout = setTimeout(
          () => {
            this.stage = 'none'
            this.render(current_value, max_value, {})
            this.timeout = null
            if (cb != null) {
              return cb()
            }
          },
          2000
        )
      }
    }

    // Check for NaN values
    if (data.current_value !== data.current_value) {
      data.current_value = 'Unknown'
    }
    if (data.max_value !== data.max_value) {
      data.max_value = 'Unknown'
    }
    this.$el.html(this.template(data))

    return this
  }

  get_stage () {
    return this.stage
  }
  skip_to_processing () {
    this.stage = 'processing'
    if (this.timeout != null) {
      // If there is a timeout, we have to clear it, we are not done yet
      clearTimeout(this.timeout)
      return this.timeout = null
    }
  }
  set_none_state () {
    return this.stage = 'none'
  }
}
OperationProgressBar.initClass()

exports.OperationProgressBar = OperationProgressBar

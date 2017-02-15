// Copyright 2010-2015 RethinkDB
const num_formatter = i => {
  let res
  if (isNaN(i)) {
    return 'N/A'
  }
  if (i / 1000000000 >= 1) {
    res = `${(i / 1000000000).toFixed(1)}`
    if (res.slice(-2) === '.0') {
      res = res.slice(0, -2)
    }
    res += 'B'
  } else if (i / 1000000 >= 1) {
    res = `${(i / 1000000).toFixed(1)}`
    if (res.slice(-2) === '.0') {
      res = res.slice(0, -2)
    }
    res += 'M'
  } else if (i / 1000 >= 1) {
    res = `${(i / 1000).toFixed(1)}`
    if (res.slice(-2) === '.0') {
      res = res.slice(0, -2)
    }
    res += 'K'
  } else {
    res = `${i.toFixed(0)}`
  }
  return res
}

class OpsPlotLegend extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'ops-plot-legend'
    this.prototype.template = require('../handlebars/ops_plot_legend.hbs')
  }

  initialize (_read_metric, _write_metric, _context) {
    this.context = _context
    this.reads = null
    this.writes = null
    this.read_metric = _read_metric
    return this.read_metric.on('change', () => {
      this.reads = _read_metric.valueAt(this.context.size() - 1)
      this.writes = _write_metric.valueAt(this.context.size() - 1)
      return this.render()
    })
  }

  render () {
    this.$el.html(
      this.template({
        read_count : this.reads != null ? num_formatter(this.reads) : 'N/A',
        write_count: this.writes != null ? num_formatter(this.writes) : 'N/A',
      })
    )
    return this
  }

  remove () {
    this.context.stop()
    // TODO?
    // @read_metric.of()
    return super.remove()
  }
}
OpsPlotLegend.initClass()

class OpsPlot extends Backbone.View {
  constructor (...args) {
    this.make_metric = this.make_metric.bind(this)
    this.render = this.render.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'ops-plot'
    this.prototype.template = require('../handlebars/ops_plot.hbs')
    this.prototype.barebones_template = require('../handlebars/ops_plot.hbs')

    // default options for the plot template
    this.prototype.type = 'cluster'

    // default options for the plot itself
    // Please make sure the first is divisible by the second,
    // 'cause who knows wtf is gonna happen if it ain't.
    this.prototype.HEIGHT_IN_PIXELS = 200
    this.prototype.HEIGHT_IN_UNITS = 20500
    this.prototype.WIDTH_IN_PIXELS = 300
    this.prototype.WIDTH_IN_SECONDS = 60
    this.prototype.HAXIS_TICK_INTERVAL_IN_SECS = 15
    this.prototype.HAXIS_MINOR_SUBDIVISION_COUNT = 3
    this.prototype.VAXIS_TICK_SUBDIVISION_COUNT = 5
    this.prototype.VAXIS_MINOR_SUBDIVISION_COUNT = 2
  }

  make_metric (name) {
    // Cache stats using the interpolating cache
    const cache = new InterpolatingCache(
      this.WIDTH_IN_PIXELS,
      this.WIDTH_IN_PIXELS / this.WIDTH_IN_SECONDS,
      () => this.stats_fn()[name]
    )
    const _metric = (start, stop, step, callback) => {
      // Give them however many they want
      start = +start
      stop = +stop
      const num_desired = (stop - start) / step
      return callback(null, cache.step(num_desired))
    }
    this.context.on('focus', i => {
      return d3
        .selectAll('.value')
        .style('right', i === null ? null : `${this.context.size() - i}px`)
    })
    return this.context.metric(_metric, name)
  }

  // The OpsPlot has one required argument:
  //   _stats_fn: function that provides data to be plotted
  // It also has one optional argument:
  //   options: an object specifying any of the following
  //            to override default values:
  //       * height: height of the plot in pixels
  //       * width: width of the plot in pixels
  //       * height_in_units: number of units to render
  //           on the y-axis when the plot is empty
  //       * seconds: number of seconds the plot
  //           tracks; width in pixels must be cleanly
  //           divisible by the number of seconds
  //       * haxis.seconds_per_tick: how many seconds
  //           before placing a tick
  //       * haxis.ticks_per_label: how many ticks before
  //           placing a label on the x-axis
  //       * vaxis.num_per_tick: how many units on the y-axis
  //           before placing a tick
  //       * vaxis.ticks_per_label: how many ticks before
  //           placing a label on the y-axis
  //       * type: the type of plot (used to determine the title
  //           of the plot). valid values include 'cluster',
  //           'datacenter', 'server', 'database', and 'table'
  initialize (_stats_fn, options) {
    if (options != null) {
      // Kludgey way to override custom options given Slava's class variable approach.
      // A sane options object for the entire class would have been preferable.
      if (options.height != null) {
        this.HEIGHT_IN_PIXELS = options.height
      }
      if (options.height_in_units != null) {
        this.HEIGHT_IN_UNITS = options.height_in_units
      }
      if (options.width != null) {
        this.WIDTH_IN_PIXELS = options.width
      }
      if (options.seconds != null) {
        this.WIDTH_IN_SECONDS = options.seconds
      }
      if (options.haxis != null) {
        if (options.haxis.seconds_per_tick != null) {
          this.HAXIS_TICK_INTERVAL_IN_SECS = options.haxis.seconds_per_tick
        }
        if (options.haxis.ticks_per_label != null) {
          this.HAXIS_MINOR_SUBDIVISION_COUNT = options.haxis.ticks_per_label
        }
      }
      if (options.vaxis != null) {
        if (options.vaxis.num_ticks != null) {
          this.VAXIS_TICK_SUBDIVISION_COUNT = options.vaxis.num_ticks
        }
        if (options.vaxis.ticks_per_label != null) {
          this.VAXIS_MINOR_SUBDIVISION_COUNT = options.vaxis.ticks_per_label
        }
      }
      if (options.type != null) {
        this.type = options.type
      }
    }

    super.initialize(...arguments)
    // Set up cubism context
    this.stats_fn = _stats_fn
    this.context = cubism
      .context()
      .serverDelay(0)
      .clientDelay(0)
      .step(1000 / (this.WIDTH_IN_PIXELS / this.WIDTH_IN_SECONDS))
      .size(this.WIDTH_IN_PIXELS)
    this.read_stats = this.make_metric('keys_read')
    this.write_stats = this.make_metric('keys_set')
    return this.legend = new OpsPlotLegend(this.read_stats, this.write_stats, this.context)
  }

  render () {
    // Render the plot container
    this.$el.html(
      this.template({
        cluster   : this.type === 'cluster',
        datacenter: this.type === 'datacenter',
        server    : this.type === 'server',
        database  : this.type === 'database',
        table     : this.type === 'table',
      })
    )

    // Set up the plot
    this.sensible_plot = this.context
      .sensible()
      .height(this.HEIGHT_IN_PIXELS)
      .colors(['#983434', '#729E51'])
      .extent([0, this.HEIGHT_IN_UNITS])
    d3.select(this.$('.plot')[0]).call(div => {
      div.data([[this.read_stats, this.write_stats]])
      // Chart itself
      this.selection = div.append('div').attr('class', 'chart')
      this.selection.call(this.sensible_plot)
      // Horizontal axis
      div
        .append('div')
        .attr('class', 'haxis')
        .call(
          this.context
            .axis()
            .orient('bottom')
            .ticks(d3.time.seconds, this.HAXIS_TICK_INTERVAL_IN_SECS)
            .tickSubdivide(this.HAXIS_MINOR_SUBDIVISION_COUNT - 1)
            .tickSize(6, 3, 0)
            .tickFormat(d3.time.format('%I:%M:%S'))
        )
      // Horizontal axis grid
      div
        .append('div')
        .attr('class', 'hgrid')
        .call(
          this.context
            .axis()
            .orient('bottom')
            .ticks(d3.time.seconds, this.HAXIS_TICK_INTERVAL_IN_SECS)
            .tickSize(this.HEIGHT_IN_PIXELS + 4, 0, 0)
            .tickFormat('')
        )
      // Vertical axis
      div
        .append('div')
        .attr('class', 'vaxis')
        .call(
          this.context
            .axis(
              this.HEIGHT_IN_PIXELS,
              [this.read_stats, this.write_stats],
              this.sensible_plot.scale()
            )
            .orient('left')
            .ticks(this.VAXIS_TICK_SUBDIVISION_COUNT)
            .tickSubdivide(this.VAXIS_MINOR_SUBDIVISION_COUNT - 1)
            .tickSize(6, 3, 0)
            .tickFormat(num_formatter)
        )
      // Vertical axis grid
      div
        .append('div')
        .attr('class', 'vgrid')
        .call(
          this.context
            .axis(
              this.HEIGHT_IN_PIXELS,
              [this.read_stats, this.write_stats],
              this.sensible_plot.scale()
            )
            .orient('left')
            .ticks(this.VAXIS_TICK_SUBDIVISION_COUNT)
            .tickSize(-(this.WIDTH_IN_PIXELS + 35), 0, 0)
            .tickFormat('')
        )
      // Legend
      return this.$('.legend-container').html(this.legend.render().el)
    })

    return this
  }

  remove () {
    this.sensible_plot.remove(this.selection)
    this.context.stop()
    this.legend.remove()
    return super.remove()
  }
}
OpsPlot.initClass()

class SizeBoundedCache {
  constructor (num_data_points, _stat) {
    this.values = []
    this.ndp = num_data_points
    this.stat = _stat
  }

  push (stats) {
    let value
    if (typeof this.stat === 'function') {
      value = this.stat(stats)
    } else {
      if (stats != null) {
        value = stats[this.stat]
      }
    }
    if (isNaN(value)) {
      return
    }
    this.values.push(value)
    // Fill up the cache if there aren't enough values
    if (this.values.length < this.ndp) {
      for (
        let i = this.values.length, end = this.ndp - 1, asc = this.values.length <= end;
        asc ? i < end : i > end;
        asc ? i++ : i--
      ) {
        this.values.push(value)
      }
    }
    // Trim the cache if there are too many values
    if (this.values.length > this.ndp) {
      return this.values = this.values.slice(-this.ndp)
    }
  }

  get () {
    return this.values
  }

  get_latest () {
    return this.values[this.values.length - 1]
  }
}

class InterpolatingCache {
  constructor (num_data_points, num_interpolation_points, get_data_fn) {
    this.ndp = num_data_points
    this.nip = num_interpolation_points // =Pixels/Sec
    this.get_data_fn = get_data_fn
    this.values = []
    for (
      let i = 0, end = this.ndp - 1, asc = end >= 0;
      asc ? i <= end : i >= end;
      asc ? i++ : i--
    ) {
      this.values.push(0)
    }
    this.next_value = null
    this.last_date = null
  }

  step (num_points) {
    // First, grab newest data
    this.push_data()

    // Now give them what they want
    return this.values.slice(-num_points)
  }

  push_data () {
    if (this.last_date === null) {
      // First time we call @push_data
      this.last_date = Date.now()
      this.values.push(this.get_data_fn())
      return true
    }

    // Check if we need to restart interpolation
    const current_value = this.get_data_fn()
    if (this.next_value !== current_value) {
      this.start_value = this.values[this.values.length - 1]
      this.next_value = current_value
      this.interpolation_step = 1
    }

    const elapsed_time = Date.now() - this.last_date
    const missing_steps = Math.max(1, Math.round(elapsed_time / 1000 * this.nip)) // If the tab has focus, we have missing_steps = 1 else we have missing_steps > 1
    for (
      let i = 1, end = missing_steps, asc = end >= 1;
      asc ? i <= end : i >= end;
      asc ? i++ : i--
    ) {
      let value_to_push
      if (this.values[this.values.length - 1] === this.next_value) {
        value_to_push = this.next_value
      } else {
        value_to_push = this.start_value +
          (this.next_value - this.start_value) / this.nip * this.interpolation_step
        this.interpolation_step += 1
        if (this.interpolation_step > this.nip) {
          value_to_push = this.next_value
        }
      }
      this.values.push(value_to_push)
    }

    this.last_date = Date.now()

    // Trim the cache
    if (this.values.length > this.ndp) {
      return this.values = this.values.slice(-this.ndp)
    }
  }
}

exports.OpsPlotLegend = OpsPlotLegend
exports.OpsPlot = OpsPlot
exports.SizeBoundedCache = SizeBoundedCache
exports.InterpolatingCache = InterpolatingCache

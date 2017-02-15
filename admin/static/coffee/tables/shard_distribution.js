// Copyright 2010-2015 RethinkDB

class ShardDistribution extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.set_distribution = this.set_distribution.bind(this)
    this.render = this.render.bind(this)
    this.display_outdated = this.display_outdated.bind(this)
    this.render_data_distribution = this.render_data_distribution.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.className = 'shards_container'
    this.prototype.template = {
      main: require('../../handlebars/shard_distribution_container.hbs'),
    }
  }

  initialize (data) {
    if (this.collection != null) {
      this.listenTo(this.collection, 'update', this.render_data_distribution)
    }
    this.listenTo(this.model, 'change:info_unavailable', this.set_warnings)
    return this.render_data_distribution
  }

  set_warnings () {
    if (this.model.get('info_unavailable')) {
      return this.$('.unavailable-error').show()
    } else {
      return this.$('.unavailable-error').hide()
    }
  }

  set_distribution (shards) {
    this.collection = shards
    this.listenTo(this.collection, 'update', this.render_data_distribution)
    return this.render_data_distribution()
  }

  render () {
    this.$el.html(this.template.main(this.model.toJSON()))
    this.init_chart = false
    setTimeout(
      // Let the element be inserted in the main DOM tree
      () => this.render_data_distribution(), // setTimeout uses the wrong "this"
      0
    )
    this.set_warnings()
    return this
  }

  prettify_num (num) {
    if (num > 1000000000) {
      num = `${`${num}`.slice(0, -9)}M`
    }
    if (num > 1000000 && num < 1000000000) {
      return num = `${`${num}`.slice(0, -6)}m`
    } else if (num > 1000 && num < 1000000) {
      return num = `${`${num}`.slice(0, -3)}k`
    } else {
      return num
    }
  }

  display_outdated () {
    if (this.$('.outdated_distribution').css('display') === 'none') {
      return this.$('.outdated_distribution').slideDown('fast')
    }
  }

  render_data_distribution () {
    let bar_width
    let left
    let left1
    let margin_bar
    $('.tooltip').remove()

    this.$('.loading').slideUp('fast')
    this.$('.outdated_distribution').slideUp('fast')
    this.$('.shard-diagram').show()

    const max_keys = (left = d3.max(this.collection.models, shard => shard.get('num_keys'))) != null
      ? left
      : 100
    const min_keys = (left1 = d3.min(this.collection.models, shard => shard.get('num_keys'))) !=
      null
      ? left1
      : 0

    const svg_width = 328 // Width of the whole svg
    const svg_height = 270 // Height of the whole svg

    const margin_width = 20 // Margin on left of the y-axis
    const margin_height = 20 // Margin under the x-axis
    const min_margin_width_inner = 20 // Min margin on the right of the y-axis

    // We have two special cases
    if (this.collection.length === 1) {
      bar_width = 100
      margin_bar = 20
    } else if (this.collection.length === 2) {
      bar_width = 80
      margin_bar = 20
    } else {
      // 80% for the bar, 20% for the margin
      bar_width = Math.floor(
        0.8 * (328 - margin_width * 2 - min_margin_width_inner * 2) / this.collection.length
      )
      margin_bar = Math.floor(
        0.2 * (328 - margin_width * 2 - min_margin_width_inner * 2) / this.collection.length
      )
    }

    // size of all bars and white space between bars
    const width_of_all_bars = bar_width * this.collection.length +
      margin_bar * (this.collection.length - 1)

    // Update the margin on the right of the y-axis
    const margin_width_inner = Math.floor((svg_width - margin_width * 2 - width_of_all_bars) / 2)

    // Y scale
    const y = d3.scale
      .linear()
      .domain([0, max_keys * 1.05])
      .range([0, svg_height - margin_height * 2.5])

    // Add svg
    // svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height).append('svg:g')
    const svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height)
    const bars_container = svg.select('.bars_container')

    // Add rectangle
    const bars = bars_container.selectAll('rect').data(
      this.collection.map((shard, index) => ({
        index,
        num_keys: shard.get('num_keys'),
      }))
    )

    bars
      .enter()
      .append('rect')
      .attr('class', 'rect')
      .attr('x', (d, i) => margin_width + margin_width_inner + bar_width * i + margin_bar * i)
      .attr('y', ({ num_keys }) => svg_height - y(num_keys) - margin_height - 1) // -1 not to overlap with axe
      .attr('width', bar_width)
      .attr('height', ({ num_keys }) => y(num_keys))
      .attr('title', ({ index, num_keys }) => `Shard ${index + 1}<br />~${num_keys} keys`)

    bars
      .transition()
      .duration(600)
      .attr('x', (d, i) => margin_width + margin_width_inner + bar_width * i + margin_bar * i)
      .attr('y', ({ num_keys }) => svg_height - y(num_keys) - margin_height - 1) // -1 not to overlap with axe
      .attr('width', bar_width)
      .attr('height', ({ num_keys }) => y(num_keys))

    bars.exit().remove()

    // Create axes
    const extra_data = []
    extra_data.push({
      x1: margin_width,
      x2: margin_width,
      y1: margin_height,
      y2: svg_height - margin_height,
    })

    extra_data.push({
      x1: margin_width,
      x2: svg_width - margin_width,
      y1: svg_height - margin_height,
      y2: svg_height - margin_height,
    })

    const extra_container = svg.select('.extra_container')
    const lines = extra_container.selectAll('.line').data(extra_data)
    lines
      .enter()
      .append('line')
      .attr('class', 'line')
      .attr('x1', ({ x1 }) => x1)
      .attr('x2', ({ x2 }) => x2)
      .attr('y1', ({ y1 }) => y1)
      .attr('y2', ({ y2 }) => y2)
    lines.exit().remove()

    // Create legend
    const axe_legend = []
    axe_legend.push({
      x     : margin_width,
      y     : Math.floor(margin_height / 2 + 2),
      string: 'Docs',
      anchor: 'middle',
    })
    axe_legend.push({
      x     : Math.floor(svg_width / 2),
      y     : svg_height,
      string: 'Shards',
      anchor: 'middle',
    })

    const legends = extra_container.selectAll('.legend').data(axe_legend)
    legends
      .enter()
      .append('text')
      .attr('class', 'legend')
      .attr('x', ({ x }) => x)
      .attr('y', d => d.y)
      .attr('text-anchor', ({ anchor }) => anchor)
      .text(({ string }) => string)
    legends.exit().remove()

    // Create ticks
    // First the tick's background
    const ticks = extra_container.selectAll('.cache').data(y.ticks(5))
    ticks
      .enter()
      .append('rect')
      .attr('class', 'cache')
      .attr('width', (d, i) => {
        if (i === 0) {
          return 0 // We don't display 0
        }
        return 4
      })
      .attr('height', 18)
      .attr('x', margin_width - 2)
      .attr('y', d => svg_height - margin_height - y(d) - 14)
      .attr('fill', '#fff')

    ticks.transition().duration(600).attr('y', d => svg_height - margin_height - y(d) - 14)
    ticks.exit().remove()

    // Then the actual tick's value
    const rules = extra_container.selectAll('.rule').data(y.ticks(5))

    // Used to set the attributes of new tick marks, and to update
    // tick marks that are transitioning (this happens when the
    // scale of the graph changes)
    const recalc_height_and_text = selection => {
      return selection
        .attr('y', d => svg_height - margin_height - y(d))
        .attr('x', margin_width)
        .attr('text-anchor', 'middle')
        .attr('class', 'rule')
        .text((d, i) => i !== 0 ? this.prettify_num(d) : '')
    }

    // How to render the axis for new ticks being added:
    rules.enter().append('text').call(recalc_height_and_text)
    // How to render the axis when a tick changes:
    rules.transition().duration(600).call(recalc_height_and_text)

    // Remove ticks that are no longer needed:
    rules.exit().remove()

    return this.$('rect').tooltip({
      trigger: 'hover',
    })
  }
}
ShardDistribution.initClass()

exports.ShardDistribution = ShardDistribution

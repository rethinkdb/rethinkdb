// Issues view

const h = require('virtual-dom/h')
const diff = require('virtual-dom/diff')
const patch = require('virtual-dom/patch')
const createElement = require('virtual-dom/create-element')

const util = require('./util.coffee')
const ui_modals = require('./ui_components/modals.coffee')

class IssuesBanner extends Backbone.View {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.toggle_display = this.toggle_display.bind(this)
    this.render = this.render.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.events = {
      'click .btn-resolve-issues': 'toggle_display',
      'click .change-route'      : 'toggle_display',
    }
  }

  initialize ({ collection }) {
    this.collection = collection
    this.show_resolve = true
    this.current_vdom_tree = render_issues(this.collection.toJSON(), this.show_resolve)

    this.listenTo(this.model, 'change', this.render)
    this.listenTo(this.collection, 'change', this.render)
    this.rename_modal = null

    return this.setElement(createElement(this.current_vdom_tree))
  }

  toggle_display () {
    if (this.show_resolve) {
      this.show_resolve = false
    } else if (this.collection.length > 0) {
      this.show_resolve = true
    }
    if (this.show_resolve) {
      return this.$('.all-issues').slideUp(300, 'swing', this.render)
    } else {
      return this.$('.all-issues').slideDown(300, 'swing', this.render)
    }
  }

  render () {
    const new_tree = render_issues(this.collection.toJSON(), this.show_resolve)
    const patches = diff(this.current_vdom_tree, new_tree)
    patch(this.$el.get(0), patches)
    this.current_vdom_tree = new_tree
    if (this.collection.length === 0) {
      this.show_resolve = true
    }
    return this
  }
}
IssuesBanner.initClass()

const rename_modal = ({ itemType, id, name }) => {
  const modal = new ui_modals.RenameItemModal({
    item_type: itemType,
    model    : new Backbone.Model({
      id,
      name,
    }),
  })
  return modal.render()
}

const render_unknown_issue = ({ type, description }) => ({
  title   : 'Unknown issue',
  subtitle: type,

  details: [
    h('p', 'An unknown issue was sent by the server. Please contact support.'),
    h('p', 'Raw data:'),
    h('code', description),
  ],
})

const availability_string = (
  { all_replicas_ready, ready_for_writes, ready_for_reads, ready_for_outdated_reads }
) => {
  if (all_replicas_ready) {
    return 'is ready'
  } else if (ready_for_writes) {
    return 'has some replicas that are not ready'
  } else if (ready_for_reads) {
    return 'is read only'
  } else if (ready_for_outdated_reads) {
    return 'is ready for outdated reads only'
  } else {
    return 'is not available'
  }
}

const availability_description = (
  { all_replicas_ready, ready_for_writes, ready_for_reads, ready_for_outdated_reads }
) => {
  if (all_replicas_ready) {
    return ' is ready.'
  } else if (ready_for_writes) {
    return ' is available for all operations, but some replicas are not ready.'
  } else if (ready_for_reads) {
    return ' is available for outdated and up-to-date reads, but not writes.'
  } else if (ready_for_outdated_reads) {
    return ' is available for outdated reads, but not up-to-date reads or writes.'
  } else {
    return ' is not available.'
  }
}

const render_table_availability = issue => {
  const { info } = issue
  return {
    title   : 'Table availability',
    subtitle: `${info.db}.${info.table} ${availability_string(info.status)}`,
    details : [
      h('p', [
        'Table ',
        h('a.change-route', { href: `#/tables/${info.table_id}` }, `${info.db}.${info.table}`),
        availability_description(info.status),
      ]),
      info.missing_servers.length > 0
        ? h('p', [
          'The following servers are disconnected:',
          h('ul', info.missing_servers.map(servername => h('li', h('code', servername)))),
        ])
        : undefined,
      info.missing_servers.length === 0
        ? h('p', [
          'None of the replicas for this table are reachable ',
          '(the servers for these replicas may be down or disconnected). ',
          h('br'),
          'No operations can be performed on this table until at least ',
          'one replica is reachable.',
        ])
        : undefined,
    ],
  }
}

const render_name_collision = (collision_type, { info }) => ({
  title   : `${util.capitalize(collision_type)} name conflict`,
  subtitle: `${info.name} is the name of more than one ${collision_type}`,

  details: [
    h('p', [
      util.pluralize_noun(collision_type, info.ids.length, true),
      ` with the name ${info.name}:`,
    ]),
    h(
      'ul',
      info.ids.map(id => {
        let link
        const plural_type = util.pluralize_noun(collision_type, 2)
        if (collision_type !== 'database') {
          link = h(
            'a.change-route',
            { href: `#${plural_type}/${id}` },
            h('span.uuid', util.humanize_uuid(id))
          )
        } else {
          link = h('span.uuid', util.humanize_uuid(id))
        }
        return h('li', [
          link,
          ' (',
          h(
            'a.rename',
            {
              href   : '#',
              onclick: event => {
                event.preventDefault()
                return rename_modal(event.target.dataset)
              },
              dataset: {
                itemType: collision_type,
                id,
                name    : info.name,
              },
            },
            'Rename'
          ),
          ')',
        ])
      })
    ),
  ],
})

const render_outdated_index = ({ info }) => {
  const help_link = 'http://www.rethinkdb.com/docs/troubleshooting/#my-secondary-index-is-outdated'
  const help_command = 'rethinkdb index-rebuild'

  return {
    title   : 'Outdated indexes',
    subtitle: 'Some secondary indexes need to be updated',
    details : [
      h('p', [
        'This cluster contains outdated indexes that were created with ',
        'a previous version of RethinKDB that contained some bugs.',
        h('br'),
        'Use ',
        h('code', help_command),
        ' to apply the latest bug fixes and improvements.',
        ' See ',
        h('a', { href: help_link }, 'the troubleshooting page'),
        ' for more details.',
      ]),
      h(
        'ul',
        info.tables.map(table =>
          h('li', [
            'The table ',
            h('a', { href: `#tables/${table.table_id}` }, `${table.db}.${table.table}`),
            ` has ${table.indexes.length} outdated `,
            util.pluralize_noun('index', table.indexes.length),
            ': ',
            table.indexes.join(', '),
          ]))
      ),
    ],
  }
}

const render_memory_error = ({ info }) => ({
  title: 'Memory issue',

  subtitle: ['A server is using swap memory.'],

  details: [
    h('p', [
      'The following ',
      util.pluralize_noun('server', info.servers.length),
      ' encountered a memory problem:',
    ]),
    h(
      'ul',
      info.servers.map(server =>
        h('li', h('a', { href: `/#servers/${server.server_id}` }, server.server)))
    ),
    h('p', ['The issue reported is: ', h('code', info.message)]),
    h('p', [
      'Please fix the problem that is causing the ',
      util.pluralize_noun('server', info.servers.length),
      ' to use swap memory. This issue will go away ',
      'after ten minutes have passed since a significant amount ',
      'of swap memory was used,',
      ' or after you restart RethinkDB.',
    ]),
  ],
})

const render_non_transitive_error = ({ info }) => ({
  title: 'Connectivity issue',

  subtitle: ['Some servers are only partially connected to the cluster.'],

  details: [
    h('p', ['The following servers are not fully connected:']),
    h(
      'ul',
      info.servers.map(server =>
        h('li', h('a', { href: `/#servers/${server.server_id}` }, server.server)))
    ),
    h('p', [
      'Partial connectivity can cause tables to remain unavailable',
      ' and queries to fail. Please check your network configuration',
      ' if this issue persists for more than a few seconds.',
    ]),
  ],
})

const render_log_write_error = ({ info }) => ({
  title: 'Cannot write logs',

  subtitle: ['Log ', util.pluralize_noun('file', info.servers.length), ' cannot be written to'],

  details: [
    h('p', [
      'The following ',
      util.pluralize_noun('server', info.servers.length),
      ' encountered an error while writing log statements:',
    ]),
    h(
      'ul',
      info.servers.map(server =>
        h('li', h('a', { href: `/#servers/${server.server_id}` }, server.server)))
    ),
    h('p', ['The error message reported is: ', h('code', info.message)]),
    h('p', [
      'Please fix the problem that is preventing the ',
      util.pluralize_noun('server', info.servers.length),
      ' from writing to their log file. This issue will go away ',
      'the next time the server successfully writes to the log file.',
    ]),
  ],
})

const render_issue = issue => {
  const details = (() => {
    switch (issue.type) {
      case 'log_write_error':
        return render_log_write_error(issue)
      case 'memory_error':
        return render_memory_error(issue)
      case 'non_transitive_error':
        return render_non_transitive_error(issue)
      case 'outdated_index':
        return render_outdated_index(issue)
      case 'table_availability':
        return render_table_availability(issue)
      case 'db_name_collision':
        return render_name_collision('database', issue)
      case 'server_name_collision':
        return render_name_collision('server', issue)
      case 'table_name_collision':
        return render_name_collision('table', issue)
      default:
        return render_unknown_issue(issue)
    }
  })()
  const critical_class = issue.critical ? '.critical' : ''
  return h(
    'div.issue-container',
    h(`div.issue${critical_class}`, [
      h('div.issue-header', [h('p.issue-type', details.title), h('p.message', details.subtitle)]),
      h('hr'),
      h('div.issue-details', details.details),
    ])
  )
}

var render_issues = (issues, show_resolve) => {
  // Renders the outer container of the issues banner
  const no_issues_class = issues.length > 0 ? '' : '.no-issues'
  return h(
    'div.issues-banner',
    issues.length > 0
      ? [
        h(
            `div.show-issues-banner${no_issues_class}`,
            h(
              `div.gradient-overlay${no_issues_class}`,
              h('div.container', [
                render_resolve_button(show_resolve),
                h('p.message', render_issues_message(issues.length)),
              ])
            )
          ),
        h(
            'div#resolve-issues.all-issues',
            h('div.container', [
              h('div#issue-alerts'),
              h('div.issues_list', issues.map(render_issue)),
            ])
          ),
      ]
      : undefined
  )
}

var render_issues_message = num_issues => {
  if (num_issues === 0) {
    return 'All issues have been successfully resolved.'
  } else {
    return [
      h('strong', [`${num_issues} ${util.pluralize_noun('issue', num_issues)}`]),
      ` ${util.pluralize_verb('need', num_issues)} to be resolved`,
    ]
  }
}

var render_resolve_button = show_resolve => {
  if (show_resolve) {
    return h('button.btn.btn-resolve-issues.show-issues', 'Show issues')
  } else {
    return h('button.btn.btn-resolve-issues.hide-issues', 'Hide issues')
  }
}

exports.IssuesBanner = IssuesBanner

# Issues view

h = require('virtual-dom/h')
diff = require('virtual-dom/diff')
patch = require('virtual-dom/patch')
createElement = require('virtual-dom/create-element')

util = require('./util.coffee')
ui_modals = require('./ui_components/modals.coffee')


class IssuesBanner extends Backbone.View
    events:
        'click .btn-resolve-issues': 'toggle_display'
        'click .change-route': 'toggle_display'

    initialize: (options) =>
        @collection = options.collection
        @show_resolve = true
        @current_vdom_tree = render_issues(
            @collection.toJSON(), @show_resolve)

        @listenTo @model, 'change', @render
        @listenTo @collection, 'change', @render
        @rename_modal = null

        @setElement(createElement(@current_vdom_tree))

    toggle_display: =>
        if @show_resolve
            @show_resolve = false
        else if @collection.length > 0
            @show_resolve = true
        if @show_resolve
            @$('.all-issues').slideUp 300, "swing", @render
        else
            @$('.all-issues').slideDown 300, "swing", @render

    render: =>
        new_tree = render_issues(@collection.toJSON(), @show_resolve)
        patches = diff(@current_vdom_tree, new_tree)
        patch(@$el.get(0), patches)
        @current_vdom_tree = new_tree
        if @collection.length == 0
            @show_resolve = true
        @

rename_modal = (dataset) =>
    modal = new ui_modals.RenameItemModal
        item_type: dataset.itemType
        model: new Backbone.Model
            id: dataset.id
            name: dataset.name
    modal.render()

render_unknown_issue = (issue) ->
    title: "Unknown issue"
    subtitle: issue.type
    details: [
        h "p", "An unknown issue was sent by the server. Please contact support."
        h "p", "Raw data:"
        h "code", issue.description
    ]

availability_string = (status) ->
    if status.all_replicas_ready
        "is ready"
    else if status.ready_for_writes
        "has some replicas that are not ready"
    else if status.ready_for_reads
        "is read only"
    else if status.ready_for_outdated_reads
        "is ready for outdated reads only"
    else
        "is not available"

availability_description = (status) ->
    if status.all_replicas_ready
        " is ready."
    else if status.ready_for_writes
        " is available for all operations, but some replicas are not ready."
    else if status.ready_for_reads
        " is available for outdated and up-to-date reads, but not writes."
    else if status.ready_for_outdated_reads
        " is available for outdated reads, but not up-to-date reads or writes."
    else
        " is not available."

render_table_availability = (issue) ->
    info = issue.info
    title: "Table availability"
    subtitle: "#{info.db}.#{info.table} #{availability_string(info.status)}"
    details: [
        h "p", [
            "Table "
            h "a.change-route", href: "#/tables/#{info.table_id}",
                "#{info.db}.#{info.table}"
            availability_description(info.status)
        ]
        h "p", [
            "The following servers are disconnected:"
            h "ul", info.missing_servers.map((servername) ->
                h "li",
                    h "code", servername
            )
        ] if info.missing_servers.length > 0
        h "p", [
            "None of the replicas for this table are reachable "
            "(the servers for these replicas may be down or disconnected). "
            h "br"
            "No operations can be performed on this table until at least "
            "one replica is reachable."
        ] if info.missing_servers.length == 0
    ]

render_name_collision = (collision_type, issue) ->
    title: "#{util.capitalize(collision_type)} name conflict"
    subtitle: "#{issue.info.name} is the name of more than one #{collision_type}"
    details: [
        h "p", [
            util.pluralize_noun(collision_type, issue.info.ids.length, true)
            " with the name #{issue.info.name}:"
        ]
        h "ul", issue.info.ids.map((id) ->
            plural_type = util.pluralize_noun(collision_type, 2)
            if collision_type != 'database'
                link = h("a.change-route", href: "##{plural_type}/#{id}",
                    h("span.uuid", util.humanize_uuid(id)))
            else
                link = h("span.uuid", util.humanize_uuid(id))
            h "li", [
                link
                " ("
                h("a.rename",
                    href: "#",
                    onclick: (event) =>
                        event.preventDefault()
                        rename_modal(event.target.dataset)
                    dataset: {
                        itemType: collision_type,
                        id: id,
                        name: issue.info.name,
                    },
                    "Rename")
                ")"
            ]
        )
    ]

render_outdated_index = (issue) ->
    help_link = "http://www.rethinkdb.com/docs/troubleshooting/#my-secondary-index-is-outdated"
    help_command = "rethinkdb index-rebuild"

    title: "Outdated indexes"
    subtitle: "Some secondary indexes need to be updated"
    details: [
        h "p", [
            "This cluster contains outdated indexes that were created with "
            "a previous version of RethinKDB that contained some bugs."
            h "br"
            "Use ", h("code", help_command)
            " to apply the latest bug fixes and improvements."
            " See ", h("a", href: help_link, "the troubleshooting page")
            " for more details."
        ]
        h "ul", issue.info.tables.map((table) ->
            h "li", [
                "The table ",
                h "a", href: "#tables/#{table.table_id}",
                    "#{table.db}.#{table.table}"
                " has #{table.indexes.length} outdated "
                util.pluralize_noun("index", table.indexes.length)
                ": ", table.indexes.join(", ")
            ]
        )

    ]

render_memory_error = (issue) ->
    # Issue raised when the server has problems with swapping memory.
    title: "Memory issue"
    subtitle: [
        "A server is using swap memory."
    ]
    details: [
        h "p", [
            "The following "
            util.pluralize_noun('server', issue.info.servers.length)
            " encountered a memory problem:"
        ]
        h "ul", issue.info.servers.map((server) ->
            h "li",
                h "a", href: "/#servers/#{server.server_id}", server.server)
        h "p", [
            "The issue reported is: ",
            h "code", issue.info.message
        ]
        h "p", [
            "Please fix the problem that is causing the "
            util.pluralize_noun("server", issue.info.servers.length)
            " to use swap memory. This issue will go away "
            "after ten minutes have passed since a significant amount "
            "of swap memory was used,"
            " or after you restart RethinkDB."
        ]
    ]

render_non_transitive_error = (issue) ->
    # Issue raised when network connectivity is non-transitive
    title: "Connectivity issue"
    subtitle: [
        "Some servers are only partially connected to the cluster."
    ]
    details: [
        h "p", [
           "The following servers are not fully connected:"
        ]
        h "ul", issue.info.servers.map((server) ->
            h "li",
                h "a", href: "/#servers/#{server.server_id}", server.server)
        h "p", [
            "Partial connectivity can cause tables to remain unavailable"
            " and queries to fail. Please check your network configuration"
            " if this issue persists for more than a few seconds."
        ]
    ]

render_log_write_error = (issue) ->
    # Issue raised when the server can't write to its log file.
    title: "Cannot write logs"
    subtitle: [
        "Log "
        util.pluralize_noun('file', issue.info.servers.length)
        " cannot be written to"
    ]
    details: [
        h "p", [
            "The following "
            util.pluralize_noun('server', issue.info.servers.length)
            " encountered an error while writing log statements:"
        ]
        h "ul", issue.info.servers.map((server) ->
            h "li",
                h "a", href: "/#servers/#{server.server_id}", server.server)
        h "p", [
            "The error message reported is: ",
            h "code", issue.info.message
        ]
        h "p", [
            "Please fix the problem that is preventing the "
            util.pluralize_noun("server", issue.info.servers.length)
            " from writing to their log file. This issue will go away "
            "the next time the server successfully writes to the log file."
        ]
    ]


render_issue = (issue) ->
    details = switch issue.type
        when 'log_write_error' then render_log_write_error(issue)
        when 'memory_error' then render_memory_error(issue)
        when 'non_transitive_error' then render_non_transitive_error(issue)
        when 'outdated_index' then render_outdated_index(issue)
        when 'table_availability' then render_table_availability(issue)
        when 'db_name_collision' then render_name_collision('database', issue)
        when 'server_name_collision' then render_name_collision('server', issue)
        when 'table_name_collision' then render_name_collision('table', issue)
        else render_unknown_issue(issue)
    critical_class = if issue.critical then ".critical" else ""
    h "div.issue-container",
        h "div.issue#{critical_class}", [
            h "div.issue-header", [
                h "p.issue-type", details.title
                h "p.message", details.subtitle
            ]
            h "hr"
            h "div.issue-details", details.details
        ]


render_issues = (issues, show_resolve) ->
    # Renders the outer container of the issues banner
    no_issues_class = if issues.length > 0 then "" else ".no-issues"
    h "div.issues-banner", ([
        h "div.show-issues-banner#{no_issues_class}",
            h "div.gradient-overlay#{no_issues_class}",
                h "div.container", [
                    render_resolve_button(show_resolve)
                    h("p.message", render_issues_message(issues.length))
                ]
        h "div#resolve-issues.all-issues",
            h "div.container", [
                h "div#issue-alerts"
                h("div.issues_list", issues.map(render_issue))
            ]
    ] if issues.length > 0)

render_issues_message = (num_issues) ->
    if num_issues == 0
        "All issues have been successfully resolved."
    else
        [
            h("strong", [
                "#{num_issues} #{util.pluralize_noun('issue', num_issues)}"
            ])
            " #{util.pluralize_verb("need", num_issues)} to be resolved"
        ]

render_resolve_button = (show_resolve) ->
    if show_resolve
        h "button.btn.btn-resolve-issues.show-issues", "Show issues"
    else
        h "button.btn.btn-resolve-issues.hide-issues", "Hide issues"

exports.IssuesBanner = IssuesBanner

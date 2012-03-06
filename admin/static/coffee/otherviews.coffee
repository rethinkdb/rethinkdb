
class StatusPanelView extends Backbone.View
    # TODO get rid of className?
    className: 'status-panel'
    tagName: 'h5'
    template: Handlebars.compile $('#status_panel_view-template').html()

    initialize: ->
        log_initial '(initializing status panel view'
        @model.on 'change', => @render()
    render: ->
        log_render '(rendering) status panel view'
        @.$el.html @template(connection_status.toJSON())
        return @

# Machine view
module 'MachineView', ->
    # Container
    class @Container extends Backbone.View
        className: 'machine-view'
        template: Handlebars.compile $('#machine_view-container-template').html()

        initialize: ->
            log_initial '(initializing) machine view: container'

            # Sparkline for CPU
            @cpu_sparkline =
                data: []
                total_points: 30
                update_interval: 750
            @cpu_sparkline.data[i] = 0 for i in [0...@cpu_sparkline.total_points]

            # Performance graph (flot)
            @performance_graph =
                data: []
                total_points: 75
                update_interval: 750
                options:
                    series:
                        shadowSize: 0
                    yaxis:
                        min: 0
                        max: 10000
                    xaxis:
                        show: true
                        min: 0
                        max: 75
                plot: null
            @performance_graph.data[i] = 0 for i in [0..@performance_graph.total_points]

            @model.on 'change', @update_meters

            setInterval @update_sparklines, @cpu_sparkline.update_interval
            setInterval @update_graphs, @performance_graph.update_interval

        render: =>
            log_render '(rendering) machine view: container'

            @.$el.html @template @model.toJSON()

            return @

        # Update the sparkline data and render a new sparkline for the view
        update_sparklines: =>
            @cpu_sparkline.data = @cpu_sparkline.data.slice(1)
            @cpu_sparkline.data.push @model.get 'cpu'
            $('.cpu-graph', @el).sparkline(@cpu_sparkline.data)

        # Update the performance data and render a graph for the view
        update_graphs: =>
            @performance_graph.data = @performance_graph.data.slice(1)
            @performance_graph.data.push @model.get 'iops'
            # TODO: consider making this a utility function: enumerate
            # Maps the graph data y-values to x-values (zips them up) and sets the data
            data = _.map @performance_graph.data, (val, i) -> [i, val]


            # If the plot isn't in the DOM yet, create the initial plot
            if not @performance_graph.plot?
                @performance_graph.plot = $.plot($('#performance-graph'), [data], @performance_graph.options)
                window.graph = @performance_graph
            # Otherwise, set the updated data and draw the plot again
            else
                @performance_graph.plot.setData [data]
                @performance_graph.plot.draw() if not pause_live_data

        # Update the meters
        update_meters: =>
            $('.meter > span').each ->
                $(this)
                    .data('origWidth', $(this).width())
                    .width(0)
                    .animate width: $(this).data('origWidth'), 1200

# Datacenter view
module 'DatacenterView', ->
    # Container
    class @Container extends Backbone.View
        className: 'datacenter-view'
        template: Handlebars.compile $('#datacenter_view-container-template').html()

        initialize: ->
            log_initial '(initializing) datacenter view: container'

        render: =>
            log_render('(rendering) datacenter view: container')
            # Add a list of machines that belong to this datacenter to the json
            json = @model.toJSON()
            # Filter all the machines for those belonging to this datacenter, then return an array of their JSON representations
            json.machines = _.map _.filter(machines.models, (machine) => return machine.get('datacenter_uuid') == @model.id),
                (machine) -> machine.toJSON()
            @.$el.html @template json

            return @

# Sidebar view
module 'Sidebar', ->
    # Sidebar.Container
    class @Container extends Backbone.View
        className: 'sidebar-container'
        template: Handlebars.compile $('#sidebar-container-template').html()
        max_recent_events: 5
        resolve_issues_route: '#resolve_issues'

        initialize: =>
            log_initial '(initializing) sidebar view: container'

            # Create and store a list of critical issue views, sliced by type of issue
            @critical_issue_views = []

            critical_issues = _.groupBy _.filter(issues.models, (issue) -> return issue.get('critical')),
                (issue) -> issue.get('type')
            for type, issue_set of critical_issues
                @critical_issue_views.push new Sidebar.CriticalIssue
                    type: type
                    num: issue_set.length

            @other_issues = new Sidebar.OtherIssues()

            @reset_event_views()
            events.on 'add', (model, collection) =>
                @reset_event_views()
                @render()

            # Set up bindings for the router
            $(window.app_events).on 'on_ready', =>
                window.app.on 'all', => @render()

        render: (route) =>
            @.$el.html @template
                show_resolve_issues: window.location.hash isnt @resolve_issues_route
                machines_active: 8     # TODO: replace bullshit with reality
                machines_total: 10     # TODO: replace bullshit with reality
                datacenters_active: 4  # TODO: replace bullshit with reality
                datacenters_total: 5   # TODO: replace bullshit with reality

            # Render each critical issue summary and add it to the list of critical issues
            for view in @critical_issue_views
                $('.critical-issues', @el).append view.render().el

            # Render a view for all other issues
            $('.other-issues', @el).html @other_issues.render().el

            # Render each event view and add it to the list of recent events
            for view in @event_views
                $('.recent-events', @el).append view.render().el

            return @

        reset_event_views: =>
            # Wipe out any existing event views
            @event_views = []

            # Add an event view for the most recent events.
            for event in events.models[0...@max_recent_events]
                @event_views.push new Sidebar.Event model: event

    # Sidebar.CriticalIssue
    class @CriticalIssue extends Backbone.View
        className: 'critical issue'
        template: Handlebars.compile $('#sidebar-critical_issue-template').html()

        initialize: =>
            log_initial '(initializing) sidebar view: critical issues'
            @num = @options.num
            @type = @options.type

            plural = @num > 1

            switch @type
                when 'master_down'
                    if plural then @message = "#{@num} masters are down."
                    else @message = "#{@num} master is down."
                when 'metadata_conflict'
                    if plural then @message = "#{@num} metadata conflicts."
                    else @message = "#{@num} metadata conflict."
                else
                    if plural then @message = "#{@num} critical issues."
                    else @message = "#{@num} critical issues."

        render: =>
            @.$el.html @template
                message: @message

            return @

    # Sidebar.OtherIssues
    class @OtherIssues extends Backbone.View
        className: 'other issue'
        template: Handlebars.compile $('#sidebar-other_issues-template').html()

        initialize: ->
            log_initial '(initializing) sidebar view: other issues'

        render: =>
            num = (issues.filter (issue) -> return issue.get('critical')).length

            @.$el.html @template
                message: if num > 1 then "#{num} non-critical issues." else "#{num} other issue."

            return @

    # Sidebar.Event
    class @Event extends Backbone.View
        className: 'event'
        template: Handlebars.compile $('#sidebar-event-template').html()

        initialize: =>
            log_initial '(initializing) sidebar view: event'

        render: =>
            @.$el.html @template @model.toJSON()
            $('abbr.timeago', @el).timeago()
            return @

# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Container
    class @Container extends Backbone.View
        className: 'resolve-issues'
        template: Handlebars.compile $('#resolve_issues-container-template').html()

        initialize: ->
            log_initial '(initializing) resolve issues view: container'

        render: ->
            @.$el.html @template({})

            issue_views = []
            for issue in issues.models
                issue_views.push new ResolveIssuesView.Issue
                    model: issue

            $issues = $('.issues', @el).empty()
            $issues.append view.render().el for view in issue_views

            return @

    # ResolveIssuesView.Issue
    class @Issue extends Backbone.View
        className: 'issue-container'
        template: Handlebars.compile $('#resolve_issues-issue-template').html()

        initialize: ->
            log_initial '(initializing) resolve issues view: issue'

        render: ->
            switch @model.get('type')
                when 'master_down'
                    model = random_model_from namespaces
                    json =
                        master_down: true
                        shard: 'a-k'
                        namespace: model.get('name')
                        namespace_uuid: model.get('id')
                        datetime: @model.get('datetime')
                        details: [
                            'Write availability is lost for shard a-k on namespace bobloblaw.'
                            "Reason: <strong>#{random_model_from(machines).get('name')}</strong> is unreachable."
                        ]
                        resolution:
                            summary: 'a new master must be chosen.'
                            action: 'Choose a new master: '
                            options: ['usa_1', 'usa_2']
                            recommended: 'usa_1'
                else
                    json =
                        title: "Unknown issue: #{ @model.get('type') }"

            if @model.get('critical')
                json = _.extend json, critical: true

            @.$el.html @template json
            $('abbr.timeago', @el).timeago()

            return @

# Events view
module 'EventsView', ->
    # EventsView.Container
    class @Container extends Backbone.View
        className: 'events-view'
        template: Handlebars.compile $('#events-container-template').html()

        initialize: ->
            log_initial '(initializing) events view: container'

            @reset_event_views()
            events.on 'add', (model, collection) =>
                @reset_event_views()
                @render()

        render: ->
            @.$el.html @template({})

            # Render each event view and add it to the list of events
            for view in @event_views
                $('.events', @el).append view.render().el
            return @

        reset_event_views: =>
            # Wipe out any existing event views
            @event_views = []

            # Add an event view for the most recent events.
            events.forEach (event) =>
                @event_views.push new EventsView.Event model: event

    # EventsView.Event
    class @Event extends Backbone.View
        className: 'event'
        template: Handlebars.compile $('#events-event-template').html()

        initialize: ->
            log_initial '(initializing) events view: event'

        render: =>
            @.$el.html @template @model.toJSON()

            return @


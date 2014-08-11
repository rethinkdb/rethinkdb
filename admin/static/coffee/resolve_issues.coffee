# Copyright 2010-2012 RethinkDB, all rights reserved.
# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Container
    class @Container extends UIComponents.AbstractList
        id: 'resolve-issues'
        className: 'section'

        template: Handlebars.templates['resolve_issues-container-template']

        initialize: ->
            @issue_list = new ResolveIssuesView.IssueList()

        render: =>
            @.$el.html @template()
            @.$('.issue-list').html @issue_list.render().el
            return @

    class @IssueList extends UIComponents.AbstractList
        template: Handlebars.templates['resolve_issues-issue_list-template']

        initialize: ->
            super issues, ResolveIssuesView.Issue, '.issues'

    class @DeclareMachineDeadModal extends UIComponents.AbstractModal
        template: Handlebars.templates['declare_machine_dead-modal-template']
        alert_tmpl: Handlebars.templates['resolve_issues-resolved-template']
        template_issue_error: Handlebars.templates['fail_solve_issue-template']

        class: 'declare-machine-dead'

        initialize: ->
            log_initial '(initializing) modal dialog: declare machine dead'
            super @template

        render: (_machine_to_kill) ->
            log_render '(rendering) declare machine dead dialog'
            @machine_to_kill = _machine_to_kill
            super
                machine_name: @machine_to_kill.get("name")
                modal_title: "Permanently remove the server"
                btn_primary_text: 'Remove'

        on_submit: ->
            super

            if @$('.verification').val().toLowerCase() is 'remove'
                $.ajax
                    url: "ajax/semilattice/machines/#{@machine_to_kill.id}"
                    type: 'DELETE'
                    contentType: 'application/json'
                    success: @on_success
                    error: @on_error
            else
                @.$('.error_verification').slideDown 'fast'
                @reset_buttons()

        on_success_with_error: =>
            @.$('.error_answer').html @template_issue_error

            if @.$('.error_answer').css('display') is 'none'
                @.$('.error_answer').slideDown('fast')
            else
                @.$('.error_answer').css('display', 'none')
                @.$('.error_answer').fadeIn()
            @reset_buttons()


        on_success: (response) =>
            if (response)
                @on_success_with_error()
                return

            # notify the user that we succeeded
            $('#issue-alerts').append @alert_tmpl
                machine_dead:
                    machine_name: @machine_to_kill.get("name")

            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: 'ajax/issues'
                contentType: 'application/json'
                success: =>
                    set_issues()
                    super


    class @ResolveNameConflictModal extends Backbone.View
        alert_tmpl_: Handlebars.templates['resolve_issues-resolved-template']

        render: (uuid, type) =>
            @type = type
            rename_modal = new UIComponents.RenameItemModal(uuid, type, @on_success_, { hide_alert: true })
            rename_modal.render()
            return @

        on_success_: =>
            # notify the user that we succeeded
            $('#issue-alerts').append @alert_tmpl_
                name_conflict:
                    type: @type

            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: 'ajax/issues'
                contentType: 'application/json'
                success: set_issues

    class @ResolveVClockModal extends UIComponents.AbstractModal
        template: Handlebars.templates['resolve_vclock-modal-template']
        alert_tmpl: Handlebars.templates['resolve_issues-resolved-template']
        class: 'resolve-vclock-modal'

        initialize: ->
            log_initial '(initializing) modal dialog: resolve vclock'
            super

        render: (_final_value, _resolution_url) ->
            @final_value = _final_value
            @resolution_url = _resolution_url
            log_render '(rendering) resolve vclock'
            super
                final_value: JSON.stringify @final_value
                modal_title: 'Resolve configuration conflict'
                btn_primary_text: 'Resolve'

        on_submit: ->
            super
            $.ajax
                url: @resolution_url
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(@final_value)
                success: @on_success
                error: @on_error

        on_success: (response) ->
            # notify the user that we succeeded
            $('#issue-alerts').append @alert_tmpl
                vclock_conflict:
                    final_value: @final_value

            $.ajax
                url: 'ajax/issues'
                contentType: 'application/json'
                success: =>
                    set_issues()
                    super

    class @ResolveUnsatisfiableGoal extends UIComponents.AbstractModal
        template: Handlebars.templates['resolve_unsatisfiable_goals_modal-template']
        alert_tmpl: Handlebars.templates['resolve_issues-resolved-template']
        class: 'resolve-vclock-modal'

        initialize: (namespace, datacenters_with_issues, can_be_solved)->
            log_initial '(initializing) modal dialog: resolve vclock'
            @namespace = namespace
            @datacenters_with_issues = datacenters_with_issues
            @can_be_solved = can_be_solved # Whether we are going to completly solve the issue or not
            super

        render: (data)->
            log_render '(rendering) resolve unsatisfiable goal'
            super
                namespace_id: @namespace.get('id')
                namespace_name: @namespace.get('name')
                datacenters_with_issues: @datacenters_with_issues
                modal_title: "Lower number of replicas"

        on_submit: ->
            super
            replica_affinities_to_send = {}
            ack_expectations_to_send = {}

            for datacenter in @datacenters_with_issues
                if datacenter.num_replicas > datacenter.num_machines
                    replica_affinities_to_send[datacenter.datacenter_id] = datacenter.num_machines

                    ack_expectations_to_send[datacenter.datacenter_id] = @namespace.get('ack_expectations')[datacenter.datacenter_id]
                    if ack_expectations_to_send[datacenter.datacenter_id].expectation > replica_affinities_to_send[datacenter.datacenter_id]
                        ack_expectations_to_send[datacenter.datacenter_id].expectation = replica_affinities_to_send[datacenter.datacenter_id]

                    if @namespace.get('primary_uuid') is datacenter.datacenter_id # Since primary is a replica, we lower it by 1 if we are dealing with the primary datacenter
                        replica_affinities_to_send[datacenter.datacenter_id]--

            $.ajax
                processData: false
                url: "ajax/semilattice/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
                contentType: 'application/json'
                type: 'POST'
                data: JSON.stringify
                    replica_affinities: replica_affinities_to_send
                    ack_expectations: ack_expectations_to_send
                success: @on_success
                error: @on_error


        on_success: ->
            # notify the user that we succeeded
            $('#issue-alerts').append @alert_tmpl
                unsatisfiable_goals: true
                can_be_solved: @can_be_solved

            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: 'ajax/issues'
                contentType: 'application/json'
                success: =>
                    set_issues()
                    super

    # ResolveIssuesView.Issue
    class @Issue extends Backbone.View
        className: 'issue-container'
        templates:
            'MACHINE_DOWN': Handlebars.templates['resolve_issues-machine_down-template']
            'NAME_CONFLICT_ISSUE': Handlebars.templates['resolve_issues-name_conflict-template']
            'LOGFILE_WRITE_ERROR': Handlebars.templates['resolve_issues-logfile_write-template']
            'VCLOCK_CONFLICT': Handlebars.templates['resolve_issues-vclock_conflict-template']
            'UNSATISFIABLE_GOALS': Handlebars.templates['resolve_issues-unsatisfiable_goals-template']
            'MACHINE_GHOST': Handlebars.templates['resolve_issues-machine_ghost-template']
            'PORT_CONFLICT': Handlebars.templates['resolve_issues-port_conflict-template']
            'OUTDATED_INDEX_ISSUE': Handlebars.templates['resolve_issues-outdated_index_issue-template']

        unknown_issue_template: Handlebars.templates['resolve_issues-unknown-template']

        render_machine_down: (_template) =>
            machine = machines.get(@model.get('victim'))

            masters = []
            replicas = []

            # Look at all namespaces in the cluster and determine whether this machine had a master or replicas for them
            namespaces.each (namespace) ->
                for machine_uuid, role_summary of namespace.get('blueprint').peers_roles
                    if machine_uuid is machine.get('id')
                        for shard, role of role_summary
                            if role is 'role_primary'
                                masters.push
                                    name: namespace.get('name')
                                    uuid: namespace.get('id')
                                    shard: human_readable_shard shard
                            if role is 'role_secondary'
                                replicas.push
                                    name: namespace.get('name')
                                    uuid: namespace.get('id')
                                    shard: human_readable_shard shard

            json =
                name: machine.get('name')
                masters: if _.isEmpty(masters) then null else masters
                replicas: if _.isEmpty(replicas) then null else replicas
                no_responsibilities: if (_.isEmpty(replicas) and _.isEmpty(masters)) then true else false
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')

            # Remove the old handler before we rerender
            @.$('.solve-issue').off "click"

            @.$el.html _template(json)

            # Declare machine dead handler
            @.$('.solve-issue').click =>
                declare_dead_modal = new ResolveIssuesView.DeclareMachineDeadModal
                declare_dead_modal.render machine

        render_name_conflict_issue: (_template) ->
            json =
                name: @model.get('contested_name')
                type: @model.get('contested_type')
                num_contestants: @model.get('contestants').length
                contestants: _.map(@model.get('contestants'), (uuid) =>
                    uuid: uuid
                    type: @model.get('contested_type')
                )
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')

            @.$el.html _template(json)

            # bind rename handlers
            _.each(@model.get('contestants'), (uuid) =>
                @.$("a#rename_" + uuid).click (e) =>
                    e.preventDefault()
                    rename_modal = new ResolveIssuesView.ResolveNameConflictModal()
                    rename_modal.render uuid, @model.get('contested_type')
            )

        render_logfile_write_issue: (_template) ->
            _machine = machines.get(@model.get('location'))
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                machine_name: if _machine then _machine.get('name') else "N/A"
                machine_uuid: @model.get('location')

            # Remove the old handler before we rerender
            @.$('.solve-issue').off "click"

            @.$el.html _template(json)

            # Declare machine dead handler
            @.$('.solve-issue').click =>
                declare_dead_modal = new ResolveIssuesView.DeclareMachineDeadModal
                declare_dead_modal.render machines.get(_machine)

        render_vclock_conflict: (_template) ->
            get_resolution_url = =>
                if @model.get('field') is 'auth_key'
                    return 'ajax/auth/auth_key/resolve'
                else if @model.get('object_type') is 'namespace'
                    return 'ajax/semilattice/rdb_'+@model.get('object_type') + 's/' + @model.get('object_id') + '/' + @model.get('field') + '/resolve'
                else
                    return 'ajax/semilattice/'+@model.get('object_type') + 's/' + @model.get('object_id') + '/' + @model.get('field') + '/resolve'

            # Remove the old handler before we rerender
            if @contestants?
                _.each @contestants, (contestant) =>
                    @.$('#resolve_' + contestant.contestant_id).off 'click'
         
            if @model.get('field') is 'blueprint'
                @contestants = []
            else
                # grab possible conflicting values
                $.ajax
                    url: get_resolution_url()
                    type: 'GET'
                    contentType: 'application/json'
                    async: false
                    success: (response) =>
                        @contestants = _.map response, (x, index) -> { value: JSON.stringify(x[1]), contestant_id: index }

            # renderevsky
            switch @model.get('object_type')
                when 'machine'
                    object_type_url = 'servers'
                    object_type = 'server'
                    if machines.get(@model.get('object_id'))?
                        name = machines.get(@model.get('object_id')).get('name')
                    else
                        name = 'Unknown machine'
                when 'datacenter'
                    object_type_url = 'datacenters'
                    object_type = 'datacenter'
                    if @model.get('object_id') is universe_datacenter.get('id')
                        name = universe_datacenter.get('name')
                    else if datacenters.get(@model.get('object_id'))?
                        name = datacenters.get(@model.get('object_id')).get('name')
                    else
                        name = "Unknown datacenter"
                when 'database'
                    object_type_url = 'databases'
                    object_type = 'database'
                    if databases.get(@model.get('object_id'))?
                        name = databases.get(@model.get('object_id')).get('name')
                    else
                        name = 'Unknown database'
                when 'namespace'
                    object_type = 'table'
                    object_type_url = 'tables'
                    if namespaces.get(@model.get('object_id'))?
                        name = namespaces.get(@model.get('object_id')).get('name')
                    else
                        name = 'Unknown table'
                when 'auth_key'
                    object_type = 'the cluster'

            
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                object_type: object_type
                object_id: @model.get('object_id')
                object_name: name
                object_type_url: object_type_url
                field: @model.get('field')
                name_contest: @model.get('field') is 'name'
                contestants: @contestants

            @.$el.html _template(json)

            # bind resolution events
            _.each @contestants, (contestant) =>
                @.$('#resolve_' + contestant.contestant_id).click (event) =>
                    event.preventDefault()
                    resolve_modal = new ResolveIssuesView.ResolveVClockModal
                    resolve_modal.render JSON.parse(contestant.value), get_resolution_url()

        render_unsatisfiable_goals: (_template) ->
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                namespace_id: @model.get('namespace_id')
                namespace_name: namespaces.get(@model.get('namespace_id')).get('name')
                datacenters_with_issues: []

            namespace = namespaces.get(@model.get('namespace_id'))
            if @model.get('primary_datacenter') isnt universe_datacenter.get('id') and not datacenters.get(@model.get('primary_datacenter'))?
                json.no_primary = true
            else
                # known issues = issue where replica > number of machines in a datacenter (not universe)
                number_machines_universe_can_use_if_no_known_issues = machines.length
                if @model.get('primary_datacenter') is universe_datacenter.get('id')
                    number_machines_universe_can_use_if_no_known_issues-- #-1 because there is a primary somewhere

                # Find the datacenters in which we are sure that there is a unsatisfiable goal ( replicas > number of machines in the datacenter )
                for datacenter_id of @model.get('replica_affinities')
                    # Compute the number of replicas required
                    number_replicas = @model.get('replica_affinities')[datacenter_id]
                    if datacenter_id is @model.get('primary_datacenter')
                        number_replicas++

                    #If the datacenter was removed
                    if not datacenters.get(datacenter_id)? and datacenter_id isnt universe_datacenter.get('id')
                        if @model.get('replica_affinities')[datacenter_id] isnt 0
                            # We have a datacenter that was removed, but the replicas_affinities of this table was not cleaned.
                            json.datacenters_with_issues.push
                                datacenter_removed: true
                                datacenter_name: 'A removed datacenter'
                                datacenter_id: datacenter_id
                                datacenter_id_small: datacenter_id.slice 30
                                num_replicas: number_replicas
                                num_machines: 0
                    else
                        # We won't add universe here because if universe has too many replicas, it's an issue in which there might two solutions. We will deal with this case later
                        if @model.get('actual_machines_in_datacenters')[datacenter_id]?
                            number_machines_in_datacenter = @model.get('actual_machines_in_datacenters')[datacenter_id]
                        else
                            number_machines_in_datacenter = 0
                        if datacenter_id isnt universe_datacenter.get('id') and number_replicas > number_machines_in_datacenter
                            if datacenter_id is @model.get('primary_datacenter') and number_machines_in_datacenter is 0
                                json.primary_empty = true
                            else
                                datacenter_name = datacenters.get(datacenter_id).get('name') # That's safe, datacenters.get(datacenter_id) is defined

                                json.datacenters_with_issues.push
                                    datacenter_id: datacenter_id
                                    datacenter_name: datacenter_name
                                    num_replicas: number_replicas
                                    num_machines: number_machines_in_datacenter
                                    change_ack: namespace.get('ack_expectations')[datacenter_id].expectation > number_machines_in_datacenter

                        # We substract the number of machines used by the datacenter if we solve the issue
                        if datacenter_id isnt universe_datacenter.get('id')
                            number_machines_universe_can_use_if_no_known_issues -= Math.min number_replicas, number_machines_in_datacenter

                # The primary might not have a value in @model.get('replica_affinities'), in which case we just missed it, so let's check
                if not @model.get('replica_affinities')[@model.get('primary_datacenter')]? and @model.get('primary_datacenter') isnt universe_datacenter.get('id')
                    number_replicas = 1 # No secondaries, and is primary
                    found_machine = false
                    for machine in machines.models
                        if machine.get('datacenter_uuid') is @model.get('primary_datacenter')
                            found_machine = true
                            break
                    if found_machine is false
                        # We didn't find any machine in the datacenter
                        datacenter_id = @model.get('primary_datacenter')

                        if @model.get('actual_machines_in_datacenters')[datacenter_id]?
                            number_machines_in_datacenter = @model.get('actual_machines_in_datacenters')[datacenter_id]
                        else
                            number_machines_in_datacenter = 0

                        if datacenter_id is @model.get('primary_datacenter') and number_machines_in_datacenter is 0
                            json.primary_empty = true
                        else
                            json.datacenters_with_issues.push
                                datacenter_id: datacenter_id
                                datacenter_name: datacenters.get(datacenter_id).get('name') # Safe since it cannot be universe
                                num_replicas: number_replicas
                                num_machines: 0
                                change_ack: namespace.get('ack_expectations')[datacenter_id].expectation > number_machines_in_datacenter


                number_machines_requested_by_universe = @model.get('replica_affinities')[universe_datacenter.get('id')]
                if @model.get('primary_datacenter') is universe_datacenter.get('id')
                    number_machines_requested_by_universe++
                # If universe has some responsabilities and that the user is asking for too many replicas across the whole cluster
                if @model.get('replica_affinities')[universe_datacenter.get('id')]? and number_machines_universe_can_use_if_no_known_issues < number_machines_requested_by_universe
                    number_replicas = @model.get('replica_affinities')[universe_datacenter.get('id')]
                    if datacenter_id is @model.get('primary_datacenter')
                        number_replicas++

                    extra_replicas_accross_cluster = @model.get('replica_affinities')[universe_datacenter.get('id')] - number_machines_universe_can_use_if_no_known_issues

                    # We might still be able to solve the issue if the table is just using universe. Let's check that
                    num_active_datacenters = 0
                    for datacenter_id of @model.get('replica_affinities')
                        if @model.get('replica_affinities')[datacenter_id] > 0
                            num_active_datacenters++
                    # Because the primary might ask for no secondary, we may have missed one datacenter. Let's make sure that we don't
                    if (not @model.get('replica_affinities')[@model.get('primary_datacenter')]?) or @model.get('replica_affinities')[@model.get('primary_datacenter')] is 0
                        num_active_datacenters++
                    # If only universe has too many replicas
                    if num_active_datacenters is 1
                        json.datacenters_with_issues.push
                            is_universe: true
                            datacenter_id: universe_datacenter.get('id')
                            num_replicas: number_replicas
                            num_machines: machines.length
                            change_ack: namespace.get('ack_expectations')[universe_datacenter.get('id')].expectation > machines.length
                    else
                        # We have an unsatisfiable goals now, but we cannot tell the user where the problem comes from
                        json.extra_replicas_accross_cluster =
                            value: extra_replicas_accross_cluster
                            datacenters_that_can_help: []
                        # Let's add to datacenters_that_can_help all the datacenters that have some responsabilities (so servers that could potentially work for universe instead of its datacenter)
                        for datacenter_id of @model.get('replica_affinities')
                            num_replicas_requested = @model.get('replica_affinities')[datacenter_id]
                            if @model.get('primary_datacenter') is datacenter_id
                                num_replicas_requested++
                            if num_replicas_requested > 0 and @model.get('actual_machines_in_datacenters')[datacenter_id] > 0 and datacenters.get(datacenter_id)? and datacenter_id isnt universe_datacenter.get('id')
                                datacenter_name = datacenters.get(datacenter_id)?.get('name')
                                json.extra_replicas_accross_cluster.datacenters_that_can_help.push
                                    datacenter_id: datacenter_id
                                    datacenter_name: (datacenter_name if datacenter_name?)
                                    num_replicas_requested: num_replicas_requested

                        # Let's add universe at the beginning of the list of datacenters that require some replicas
                        num_replicas_universe_request = @model.get('replica_affinities')[universe_datacenter.get('id')]
                        if @model.get('primary_datacenter') is universe_datacenter.get('id')
                            num_replicas_universe_request += 1
                        json.extra_replicas_accross_cluster.datacenters_that_can_help.unshift
                            datacenter_id: universe_datacenter.get('id')
                            is_universe: true
                            num_replicas_requested: num_replicas_universe_request


                json.can_solve_issue = json.datacenters_with_issues.length > 0

            @.$el.html _template(json)

            # bind resolution events
            if json.can_solve_issue
                @.$('.solve-issue').click (event) =>
                    event.preventDefault()
                    resolve_modal = new ResolveIssuesView.ResolveUnsatisfiableGoal namespace, json.datacenters_with_issues, not json.extra_replicas_accross_cluster?
                    resolve_modal.render()
            else
                that = @
                @.$('.try-solve-issue').click ->
                    window.router.navigate '#tables/'+that.model.get('namespace_id'), {trigger: true}

        render_machine_ghost: (_template) ->
            # render
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                machine_id: @model.get('ghost')
            @.$el.html _template(json)


        render_outdated_index_issue: (_template) ->
            # render
            namespaces_concerned = []
            for namespace_id, indexes of @model.get('indexes')
                namespaces_concerned.push
                    db: databases.get(namespaces.get(namespace_id).get('database')).get 'name'
                    name: namespaces.get(namespace_id).get('name')
                    indexes: indexes
                    count: indexes.length

            namespaces_concerned.sort (left, right) ->
                if left.db < right.db
                    return -1
                else if left.db > right.db
                    return 1
                else
                    if left.name < right.name
                        return -1
                    else if left.name > right.name
                        return 1
                return 0

            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                description: @model.get('description')
                namespaces: namespaces_concerned
            @.$el.html _template(json)

        rebuild_index: (ev) ->
            ev.preventDefault()
            #TODO
            console.log $(ev.target).data('db')
            console.log $(ev.target).data('table')
            console.log $(ev.target).data('index')

        render_port_conflict: (_template) ->
            # render
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                machine_id: @model.get('location')
                description: @model.get('description')
            @.$el.html _template(json)

        render_unknown_issue: (_template) ->
            json =
                issue_type: @model.get('type')
                critical: @model.get('critical')
                raw_data: JSON.stringify(@model, undefined, 2)
                datetime: iso_date_from_unix_time @model.get('time')
            @.$el.html _template(json)

        render: ->
            _template = @templates[@model.get('type')]
            switch @model.get('type')
                when 'MACHINE_DOWN'
                    @render_machine_down _template
                when 'NAME_CONFLICT_ISSUE'
                    @render_name_conflict_issue _template
                when 'LOGFILE_WRITE_ERROR'
                    @render_logfile_write_issue _template
                when 'VCLOCK_CONFLICT'
                    @render_vclock_conflict _template
                when 'UNSATISFIABLE_GOALS'
                    @render_unsatisfiable_goals _template
                when 'MACHINE_GHOST'
                    @render_machine_ghost _template
                when 'PORT_CONFLICT'
                    @render_port_conflict _template
                when 'OUTDATED_INDEX_ISSUE'
                    @render_outdated_index_issue _template
                else
                    @render_unknown_issue @unknown_issue_template

            @.$('abbr.timeago').timeago()

            return @


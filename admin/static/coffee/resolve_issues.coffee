# Copyright 2010-2012 RethinkDB, all rights reserved.
# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Container
    class @Container extends UIComponents.AbstractList
        id: 'resolve-issues'
        className: 'section'

        template: Handlebars.compile $('#resolve_issues-container-template').html()

        initialize: ->
            @issue_list = new ResolveIssuesView.IssueList()

        render: =>
            @.$el.html @template()
            @.$('.issue-list').html @issue_list.render().el
            return @

    class @IssueList extends UIComponents.AbstractList
        template: Handlebars.compile $('#resolve_issues-issue_list-template').html()

        initialize: ->
            super issues, ResolveIssuesView.Issue, '.issues'

    class @DeclareMachineDeadModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#declare_machine_dead-modal-template').html()
        alert_tmpl: Handlebars.compile $('#resolve_issues-resolved-template').html()
        template_issue_error: Handlebars.compile $('#fail_solve_issue-template').html()

        class: 'declare-machine-dead'

        initialize: ->
            log_initial '(initializing) modal dialog: declare machine dead'
            super @template

        render: (_machine_to_kill) ->
            log_render '(rendering) declare machine dead dialog'
            @machine_to_kill = _machine_to_kill
            super
                machine_name: @machine_to_kill.get("name")
                modal_title: "Declare server dead"
                btn_primary_text: 'Declare Dead'

        on_submit: ->
            super

            $.ajax
                url: "/ajax/semilattice/machines/#{@machine_to_kill.id}"
                type: 'DELETE'
                contentType: 'application/json'
                success: @on_success
                error: @on_error

        on_success_with_error: =>
            @.$('.error_answer').html @template_issue_error

            if @.$('.error_answer').css('display') is 'none'
                @.$('.error_answer').slideDown('fast')
            else
                @.$('.error_answer').css('display', 'none')
                @.$('.error_answer').fadeIn()
            @reset_buttons()


        on_success: (response) ->
            if (response)
                @on_success_with_error()
                return

            # notify the user that we succeeded
            $('#issue-alerts').append @alert_tmpl
                machine_dead:
                    machine_name: @machine_to_kill.get("name")

            #TODO Remove this synchronous request and use proper callbacks.
            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: '/ajax/issues'
                contentType: 'application/json'
                success: set_issues
                async: false

            super

            # We clean data now to have data fresher than if we were waiting for the next call to ajax/
            # remove from bluprints
            for namespace in namespaces.models
                blueprint = namespace.get('blueprint')
                if @machine_to_kill.get("id") of blueprint.peers_roles
                    delete blueprint.peers_roles[@machine_to_kill.get('id')]
                    namespace.set('blueprint', blueprint)

            # remove the dead machine from the models (this must be last)
            machines.remove(@machine_to_kill.id)

    class @ResolveNameConflictModal extends Backbone.View
        alert_tmpl_: Handlebars.compile $('#resolve_issues-resolved-template').html()

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
                url: '/ajax/issues'
                contentType: 'application/json'
                success: set_issues
                async: false

            return @


    class @ResolveVClockModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#resolve_vclock-modal-template').html()
        alert_tmpl: Handlebars.compile $('#resolve_issues-resolved-template').html()
        class: 'resolve-vclock-modal'

        initialize: ->
            log_initial '(initializing) modal dialog: resolve vclock'
            super

        render: (_final_value, _resolution_url) ->
            @final_value = _final_value
            @resolution_url = _resolution_url
            log_render '(rendering) resolve vclock'
            super
                final_value: @final_value
                modal_title: 'Resolve configuration conflict'
                btn_primary_text: 'Resolve'

        on_submit: ->
            super
            $.ajax
                url: "/ajax/semilattice/" + @resolution_url
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
                url: '/ajax/issues'
                contentType: 'application/json'
                success: set_issues
                async: false



    class @ResolveUnsatisfiableGoal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#resolve_unsatisfiable_goals_modal-template').html()
        alert_tmpl: Handlebars.compile $('#resolve_issues-resolved-template').html()
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
                    if ack_expectations_to_send[datacenter.datacenter_id] > replica_affinities_to_send[datacenter.datacenter_id]
                        ack_expectations_to_send[datacenter.datacenter_id] = replica_affinities_to_send[datacenter.datacenter_id]

                    if @namespace.get('primary_uuid') is datacenter.datacenter_id # Since primary is a replica, we lower it by 1 if we are dealing with the primary datacenter
                        replica_affinities_to_send[datacenter.datacenter_id]--

            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@namespace.get("protocol")}_namespaces/#{@namespace.id}"
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
                url: '/ajax/issues'
                contentType: 'application/json'
                success: set_issues
                async: false

            super

    # ResolveIssuesView.Issue
    class @Issue extends Backbone.View
        className: 'issue-container'
        templates:
            'MACHINE_DOWN': Handlebars.compile $('#resolve_issues-machine_down-template').html()
            'NAME_CONFLICT_ISSUE': Handlebars.compile $('#resolve_issues-name_conflict-template').html()
            'LOGFILE_WRITE_ERROR': Handlebars.compile $('#resolve_issues-logfile_write-template').html()
            'VCLOCK_CONFLICT': Handlebars.compile $('#resolve_issues-vclock_conflict-template').html()
            'UNSATISFIABLE_GOALS': Handlebars.compile $('#resolve_issues-unsatisfiable_goals-template').html()
            'MACHINE_GHOST': Handlebars.compile $('#resolve_issues-machine_ghost-template').html()
            'PORT_CONFLICT': Handlebars.compile $('#resolve_issues-port_conflict-template').html()

        unknown_issue_template: Handlebars.compile $('#resolve_issues-unknown-template').html()


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
                if @model.get('object_type') is 'namespace'
                    return 'rdb_'+@model.get('object_type') + 's/' + @model.get('object_id') + '/' + @model.get('field') + '/resolve'
                else
                    return @model.get('object_type') + 's/' + @model.get('object_id') + '/' + @model.get('field') + '/resolve'

            # grab possible conflicting values
            $.ajax
                url: '/ajax/semilattice/' + get_resolution_url()
                type: 'GET'
                contentType: 'application/json'
                async: false
                success: (response) =>
                    @contestants = _.map response, (x, index) -> { value: x[1], contestant_id: index }

            # renderevsky
            if @model.get('object_id') is universe_datacenter.get('id')
                datacenter_name = universe_datacenter.get('name')
            else if datacenters.get(@model.get('object_id'))?
                datacenter_name = datacenters.get(@model.get('object_id')).get('name')
            else
                datacenter_name = "Unknown datacenter"
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                object_type: @model.get('object_type')
                object_id: @model.get('object_id')
                object_name: datacenter_name
                field: @model.get('field')
                name_contest: @model.get('field') is 'name'
                contestants: @contestants

            # Remove the old handler before we rerender
            @.$('#resolve_' + contestant.contestant_id).off 'click'

            @.$el.html _template(json)

            # bind resolution events
            _.each @contestants, (contestant) =>
                @.$('#resolve_' + contestant.contestant_id).click (event) =>
                    event.preventDefault()
                    resolve_modal = new ResolveIssuesView.ResolveVClockModal
                    resolve_modal.render contestant.value, get_resolution_url()

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
                if @model.get('primary_datacenter') is universe_datacenter
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
                        if datacenter_id isnt universe_datacenter.get('id') and number_replicas > @model.get('actual_machines_in_datacenters')[datacenter_id]
                            datacenter_name = datacenters.get(datacenter_id).get('name') # That's safe, datacenters.get(datacenter_id) is defined

                            json.datacenters_with_issues.push
                                datacenter_id: datacenter_id
                                datacenter_name: datacenter_name
                                num_replicas: number_replicas
                                num_machines: @model.get('actual_machines_in_datacenters')[datacenter_id]
                                change_ack: namespace.get('ack_expectations')[datacenter_id] > @model.get('actual_machines_in_datacenters')[datacenter_id]

                        # We substract the number of machines used by the datacenter if we solve the issue
                        if datacenter_id isnt universe_datacenter.get('id')
                            number_machines_universe_can_use_if_no_known_issues -= Math.min number_replicas, @model.get('actual_machines_in_datacenters')[datacenter_id]

                number_machines_requested_by_universe = @model.get('replica_affinities')[universe_datacenter.get('id')]
                if @model.get('primary_datacenter') is universe_datacenter.get('id')
                    number_machines_requested_by_universe++
                # If universe has some responsabilities and that the user is asking for too many replicas across the whole cluster
                if @model.get('replica_affinities')[universe_datacenter.get('id')]? and number_machines_universe_can_use_if_no_known_issues < number_machines_requested_by_universe
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
                            change_ack: namespace.get('ack_expectations')[universe_datacenter.get('id')] > machines.length
                    else
                        # We have an unsatisfiable goals now, but we cannot tell the user where the problem comes from
                        json.extra_replicas_accross_cluster =
                            value: extra_replicas_accross_cluster
                            datacenters_that_can_help: []
                        # Let's add to datacenters_that_can_help all the datacenters that have some responsabilities (so servers that could potentially work for universe instead of its datacenter)
                        for datacenter_id of @model.get('replica_affinities')
                            if @model.get('replica_affinities')[datacenter_id] > 0 and @model.get('actual_machines_in_datacenters')[datacenter_id] > 0 and datacenters.get(datacenter_id)? and datacenter_id isnt universe_datacenter.get('id')
                                datacenter_name = datacenters.get(datacenter_id)?.get('name')
                                num_replicas_requested = @model.get('replica_affinities')[datacenter_id]
                                if @model.get('primary_datacenter') is datacenter_id
                                    num_replicas_requested++
                                json.extra_replicas_accross_cluster.datacenters_that_can_help.push
                                    datacenter_id: datacenter_id
                                    datacenter_name: (datacenter_name if datacenter_name?)
                                    num_replicas_requested: num_replicas_requested

                        # Let's add universe at the beginning
                        json.extra_replicas_accross_cluster.datacenters_that_can_help.unshift
                            datacenter_id: universe_datacenter.get('id')
                            is_universe: true
                            num_replicas_requested: @model.get('replica_affinities')[universe_datacenter.get('id')]


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

        render_port_conflict: (_template) ->
            # render
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                machine_id: @model.get('location')
                machine_name: machines.get(@model.get('location')).get('name')
                description: @model.get('description')
            @.$el.html _template(json)

        render_unknown_issue: (_template) ->
            json =
                issue_type: @model.get('type')
                critical: @model.get('critical')
                raw_data: @model.toJSON()
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
                else
                    @render_unknown_issue @unknown_issue_template

            @.$('abbr.timeago').timeago()

            return @


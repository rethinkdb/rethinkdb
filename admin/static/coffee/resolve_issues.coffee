# Resolve issues view
module 'ResolveIssuesView', ->
    # ResolveIssuesView.Container
    class @Container extends Backbone.View
        className: 'resolve-issues'
        template_outer: Handlebars.compile $('#resolve_issues-container-outer-template').html()
        template_inner: Handlebars.compile $('#resolve_issues-container-inner-template').html()


        events:
            'click .close': 'remove_parent_alert'

        initialize: =>
            log_initial '(initializing) resolve issues view: container'
            issues.on 'all', (model, collection) => @render_issues()

        render: ->
            @.$el.html @template_outer
            @render_issues()

            return @

        remove_parent_alert: (event) ->
            event.preventDefault()
            element = $(event.target).parent()
            element.slideUp 'fast', -> element.remove()


        # we're adding an inner render function to avoid rerendering
        # everything (for example we need to not render the alert,
        # otherwise it disappears)
        render_issues: ->
            @.$('#resolve_issues-container-inner-placeholder').html @template_inner
                issues_exist: if issues.length > 0 then true else false

            # TODO: We should use a collection instead. It's ok for now since there are not a lot of issues, but still we should change it later.
            previous_issues_views = if @issue_views? then @issue_views else []
            @issue_views = [] 
            issues.each (issue) =>
                @issue_views.push new ResolveIssuesView.Issue
                    model: issue

            for view in previous_issues_views
                view.destroy()

            $issues = @.$('.issues').empty()
            $issues.append view.render().el for view in @issue_views

            return @

        destroy: =>
            @unbind()
            for view in @issue_views
                view.destroy()

    class @DeclareMachineDeadModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#declare_machine_dead-modal-template').html()
        alert_tmpl: Handlebars.compile $('#declared_machine_dead-alert-template').html()
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
                modal_title: "Declare machine dead"
                btn_primary_text: 'Kill'

        on_submit: ->
            super

            ###
            data = 
                semilattice:
                    machines: {}

            data.semilattice.machines[@machine_to_kill.get('id')] = null
            $.ajax
                url: "/ajax"
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(data)
                success: @on_success
                error: @on_error
            ###
 
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

            super

            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: '/ajax/issues'
                contentType: 'application/json'
                success: set_issues
                async: false
            
            # rerender issue view (just the issues, not the whole thing)
            #window.app.resolve_issues.render_issues()
            
            # notify the user that we succeeded
            $('#user-alert-space').append @alert_tmpl
                machine_name: @machine_to_kill.get("name")


            # We clean data now to have data fresher than if we were waiting for the next call to ajax/
            # remove from bluprints
            for namespace in namespaces.models
                blueprint = namespace.get('blueprint')
                if @machine_to_kill.get("id") of blueprint.peers_roles
                    delete blueprint.peers_roles[@machine_to_kill.get('id')]
                    namespace.set('blueprint', blueprint)

            # remove the dead machine from the models (this must be last)
            machines.remove(@machine_to_kill.id)
            
    class @ResolveVClockModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#resolve_vclock-modal-template').html()
        alert_tmpl: Handlebars.compile $('#resolved_vclock-alert-template').html()
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
                url: "/ajax/" + @resolution_url
                type: 'POST'
                contentType: 'application/json'
                data: JSON.stringify(@final_value)
                success: @on_success
                error: @on_error

        on_success: (response) ->
            super
            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: '/ajax/issues'
                contentType: 'application/json'
                success: set_issues
                async: false

            # rerender issue view (just the issues, not the whole thing)
            window.app.resolve_issues.render_issues()

            # notify the user that we succeeded
            $('#user-alert-space').append @alert_tmpl
                final_value: @final_value


    class @ResolveUnsatisfiableGoal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#resolve_unsatisfiable_goals_modal-template').html()
        alert_tmpl: Handlebars.compile $('#resolved_issues_unsatisfiable_goals-template').html()
        class: 'resolve-vclock-modal'

        initialize: (namespace, datacenters_with_issues)->
            log_initial '(initializing) modal dialog: resolve vclock'
            @namespace = namespace
            @datacenters_with_issues = datacenters_with_issues
            super

        render: (data)->
            log_render '(rendering) resolve unsatisfiable goal'
            super 
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
            super
            # Grab the new set of issues (so we don't have to wait)
            $.ajax
                url: '/ajax/issues'
                success: set_issues
                async: false

            # rerender issue view (just the issues, not the whole thing)
            #window.app.resolve_issues_view.render_issues()

            # notify the user that we succeeded
            $('#user-alert-space').append @alert_tmpl

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

        unknown_issue_template: Handlebars.compile $('#resolve_issues-unknown-template').html()

        initialize: ->
            log_initial '(initializing) resolve issues view: issue'

        render_machine_down: (_template) ->
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

            @.$el.html _template(json)

            # Declare machine dead handler
            @.$('p a.btn').off "click"
            @.$('p a.btn').click =>
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
                    rename_modal = new UIComponents.RenameItemModal(uuid, @model.get('contested_type'), (response) =>
                        # Grab the new set of issues (so we don't have to wait)
                        $.ajax
                            url: '/ajax/issues'
                            contentType: 'application/json'
                            success: set_issues
                            async: false

                        # rerender issue view (just the issues, not the whole thing)
                        window.app.resolve_issues_view.render_issues()
                    )
                    rename_modal.render()
            )

        render_logfile_write_issue: (_template) ->
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                machine_name: machines.get(@model.get('location')).get('name')
                machine_uuid: @model.get('location')
            @.$el.html _template(json)
            # Declare machine dead handler
            @.$('p a.btn').off "click"
            @.$('p a.btn').click =>
                declare_dead_modal = new ResolveIssuesView.DeclareMachineDeadModal
                declare_dead_modal.render machines.get(@model.get('location'))

            @.$el.html _template(json)



        render_vclock_conflict: (_template) ->
            get_resolution_url = =>
                return @model.get('object_type') + 's/' + @model.get('object_id') + '/' + @model.get('field') + '/resolve'

            # grab possible conflicting values
            $.ajax
                url: '/ajax/' + get_resolution_url()
                type: 'GET'
                contentType: 'application/json'
                async: false
                success: (response) =>
                    @contestants = _.map response, (x, index) -> { value: x[1], contestant_id: index }

            # renderevsky
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                object_type: @model.get('object_type')
                object_id: @model.get('object_id')
                object_name: datacenters.get(@model.get('object_id')).get('name')
                field: @model.get('field')
                name_contest: @model.get('field') is 'name'
                contestants: @contestants
            @.$el.html _template(json)

            # bind resolution events
            _.each @contestants, (contestant) =>
                @.$('#resolve_' + contestant.contestant_id).off 'click' #TODO turn off later?
                @.$('#resolve_' + contestant.contestant_id).click (event) =>
                    event.preventDefault()
                    resolve_modal = new ResolveIssuesView.ResolveVClockModal
                    resolve_modal.render contestant.value, get_resolution_url()

        render_unsatisfiable_goals: (_template) ->
            # render
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                namespace_id: @model.get('namespace_id')
                namespace_name: namespaces.get(@model.get('namespace_id')).get('name')
                datacenters_with_issues: []

            namespace = namespaces.get(@model.get('namespace_id'))
            
            for datacenter_id of @model.get('replica_affinities')
                number_replicas = if datacenter_id is @model.get('primary_datacenter') then @model.get('replica_affinities')[datacenter_id]+1 else @model.get('replica_affinities')[datacenter_id]
                
                if number_replicas > @model.get('actual_machines_in_datacenters')[datacenter_id]
                    json.datacenters_with_issues.push
                        datacenter_id: datacenter_id
                        datacenter_name: datacenters.get(datacenter_id).get('name')
                        num_replicas: number_replicas
                        num_machines: @model.get('actual_machines_in_datacenters')[datacenter_id]

            @.$el.html _template(json)
            # bind resolution events
            @.$('.solve_unsatisfiable_goals').click (event) =>
                event.preventDefault()
                resolve_modal = new ResolveIssuesView.ResolveUnsatisfiableGoal namespace, json.datacenters_with_issues
                resolve_modal.render()

        render_machine_ghost: (_template) ->
            # render
            json =
                datetime: iso_date_from_unix_time @model.get('time')
                critical: @model.get('critical')
                machine_id: @model.get('ghost')
                machine_name: @model.get('ghost')
            @.$el.html _template(json)

        render_unknown_issue: (_template) ->
            json =
                issue_type: @model.get('type')
                critical: @model.get('critical')
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
                else
                    @render_unknown_issue @unknown_issue_template

            @.$('abbr.timeago').timeago()

            return @


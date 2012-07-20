# Helper stuff
Handlebars.registerPartial 'shard_name', $('#shard_name-partial').html()

# Namespace view
module 'NamespaceView', ->
    # Hardcoded!
    MAX_SHARD_COUNT = 32

    # Shards view
    # The sharding view consists of several lists. Here's the hierarchy:
    #   @Sharding
    #   |-- @Shard
    #   |   `-- @ShardDatacenterList: primary and secondary replicas organized by datacenter
    #   |       `-- @ShardDatacenter
    #   |           `-- @ShardMachineList: machines with primary and secondary replicas
    #   |               `-- @ShardMachine
    class @Pinning extends UIComponents.AbstractList
        className: 'namespace-shards'
        template: Handlebars.compile $('#namespace_view-sharding-template').html()
        error_update_forbidden_msg: Handlebars.compile $('#namespace_view-sharding_alert-template').html()

        events: ->
            'click .change-sharding-scheme': 'change_sharding_scheme'

        initialize: ->
            super @model.get('computed_shards'), NamespaceView.Shard, 'table.shards tbody',
                element_args:
                    namespace: @model
            @render()

            @should_be_hidden = false
            @check_has_unsatisfiable_goals()
            issues.on 'all', @check_has_unsatisfiable_goals
            @model.on 'change:replica_affinities', @render
            @model.on 'change:shards', @render
            @model.on 'blueprint', @render



        check_has_unsatisfiable_goals: =>
            if @should_be_hidden
                should_be_hidden_new = false
                for issue in issues.models 
                    if issue.get('type') is 'UNSATISFIABLE_GOALS' and issue.get('namespace_id') is @model.get('id') # If unsatisfiable goals, the user should not change shards
                        should_be_hidden_new = true
                        break
                    if issue.get('type') is 'MACHINE_DOWN' # If a machine connected (even with a role nothing) is down, the user should not change shards
                        if machines.get(issue.get('victim')).get('datacenter_uuid') of @model.get('replica_affinities')
                            should_be_hidden_new = true
                            break

                if not should_be_hidden_new
                    @should_be_hidden = false
                    @render()
            else
                for issue in issues.models
                    if issue.get('type') is 'UNSATISFIABLE_GOALS' and issue.get('namespace_id') is @model.get('id')
                        @should_be_hidden = true
                        @render()
                        break
                    if issue.get('type') is 'MACHINE_DOWN'
                        if machines.get(issue.get('victim')).get('datacenter_uuid') of @model.get('replica_affinities')
                            @should_be_hidden = true
                            @render()
                            break

        render: =>
            super
            @.$el.toggleClass('namespace-shards-blackout', @should_be_hidden)
            @.$('.blackout').toggleClass('blackout-active', @should_be_hidden)
            @.$('.alert_for_sharding').toggle @should_be_hidden
            @.$('.alert_for_sharding').html @error_update_forbidden_msg if @should_be_hidden

            return @

        change_sharding_scheme: (event) =>
            event.preventDefault()
            @.$('.sharding').hide()
            view = new NamespaceView.ModifyShards @model.get('id')
            @.$('.change-shards').html view.render().el

        destroy: ->
            super
            issues.off()
            @model.off()

    class @Shard extends Backbone.View
        tagName: 'div'
        template: Handlebars.compile $('#namespace_view-shard-template').html()
        summary_template: Handlebars.compile $('#namespace_view-shard-summary-template').html()

        initialize: ->
            @namespace = @options.args.namespace

            @datacenter_list = new NamespaceView.ShardDatacenterList datacenters, NamespaceView.ShardDatacenter, 'div.datacenters',
                filter: (datacenter) =>
                    return true
                ###
                filter: (datacenter) =>
                    for datacenter_uuid of @model.get('secondary_uuids')
                        return true if datacenter.get('id') is datacenter_uuid
                    if @model.get('primary_uuid')
                        return true if datacenter.get('id') is machines.get(@model.get('primary_uuid')).get('datacenter_uuid')
                ###
                element_args:
                    shard: @model
                    namespace: @namespace

            @namespace.on 'change:key_distr_sorted', @render_summary
            @namespace.on 'change:blueprint', @reset_datacenter_list #TODO bind to peers_roles

        render: =>
            @.el = @template({})
            @.$el = $(@template({}))
            
            @render_summary()
            @.$('.datacenter-list').html @datacenter_list.render().el
            return @

        render_summary: =>
            @.$('.shard.summary').html @summary_template
                name: human_readable_shard @model.get('shard_boundaries')
                shard_stats:
                    rows_approx: @namespace.compute_shard_rows_approximation(@model.get('shard_boundaries'))

        reset_datacenter_list: =>
            @datacenter_list.render()

        destroy: =>
            @datacenter_list.destroy()
            @namespace.off 'change:key_distr_sorted', @render_summary
            @namespace.off 'change:blueprint', @reset_datacenter_list

    class @ShardDatacenterList extends UIComponents.AbstractList
        template: Handlebars.compile $('#namespace_view-shard_datacenter_list-template').html()

    class @ShardDatacenter extends UIComponents.CollapsibleListElement
        template: Handlebars.compile $('#namespace_view-shard_datacenter-template').html()
        summary_template: Handlebars.compile $('#namespace_view-shard_datacenter-summary-template').html()

        initialize: ->
            super

            @namespace = @options.args.namespace

            @machine_list = new NamespaceView.ShardMachineList machines, NamespaceView.ShardMachine, 'ul.machines',
                filter: (machine) =>
                    shard = @options.args.shard
                    return false if machine.get('datacenter_uuid') isnt @model.get('id')
                    for datacenter_uuid, machine_uuids of shard.get('secondary_uuids')
                        return true if machine.get('id') in machine_uuids
                    if shard.get('primary_uuid')
                        return true if machine.get('id') is shard.get('primary_uuid')
                sort: (a, b) =>
                    a_is_master = a.model.get('id') is @options.args.shard.get('primary_uuid')
                    b_is_master = b.model.get('id') is @options.args.shard.get('primary_uuid')
                    a_name = a.model.get('name')
                    b_name = b.model.get('name')

                    return -1 if a_is_master > b_is_master
                    return 1 if a_is_master < b_is_master

                    return -1 if a_name < b_name
                    return 1 if a_name > b_name
                    return 0
                element_args:
                    shard: @options.args.shard
                    namespace: @options.args.namespace
                    datacenter: @model

            @model.on 'change', @render_summary
            directory.on 'all', @render_summary
            @namespace.on 'change:replica_affinities', @reset_list
            @namespace.on 'change:secondary_pinnings', @reset_list
            @namespace.on 'change:blueprint', @reset_list

        reset_list: =>
            @machine_list.reset_element_views()
            @machine_list.render()
            @render()

        render: =>
            if @machine_list.element_views.length > 0 # We don't display empty datacenter here.
                @.$el.html @template({})
                @render_summary()
                @.$('.machine-list').html @machine_list.render().el

            super

            return @

        render_summary: =>
            json = _.extend @model.toJSON(),
                status: DataUtils.get_datacenter_reachability(@model.get('id'))

            @.$('.datacenter.summary').html @summary_template json

        destroy: =>
            @machine_list.destroy()

            @model.off 'change', @render_summary
            directory.off 'all', @render_summary
            @namespace.off 'change:replica_affinities', @reset_list
            @namespace.off 'change:secondary_pinnings', @reset_list
            @namespace.off 'change:blueprint', @reset_list

    class @ShardMachineList extends UIComponents.AbstractList
        template: Handlebars.compile $('#namespace_view-shard_machine_list-template').html()

        initialize: =>
            super
            @namespace = @options.element_args.namespace
            @namespace.on 'change:primary_pinnings', @render
            @namespace.on 'change:replica_affinities', @render
            


        destroy: =>
            super

            @namespace.off 'change:primary_pinnings', @render
            @namespace.off 'change:replica_affinities', @render

    class @ShardMachine extends Backbone.View
        template: Handlebars.compile $('#namespace_view-shard_machine-template').html()
        alert_tmpl: Handlebars.compile $('#edit_machines-alert-template').html()
        tagName: 'li'

        events: ->
            'click .change-machine': 'change_machine'
            'click .make-master': 'make_master'

        initialize: ->
            @shard = @options.args.shard
            @namespace = @options.args.namespace
            @datacenter = @options.args.datacenter

            @shard.on 'change:primary_uuid', @render
            @shard.on 'change:secondary_uuids', @render
            @model.on 'change:name', @render
            directory.on 'all', @render
            #@namespace.on 'change:primary_pinnings', => @list.render()


            @set_machine_dialog = new NamespaceView.SetMachineModal @namespace

        change_machine: (event) =>
            event.preventDefault()
            @set_machine_dialog.set_data
                shard_name: @shard.shard
                available_machines: @.get_available_machines_in_datacenter()
                old_pin: JSON.stringify _.without @get_currently_pinned(), @model.get('id')
                
            @set_machine_dialog.render()

        make_master: (event) =>
            event.preventDefault()

            # New primary pinning for this shard should be this machine
            post_data = {}
            post_data[@shard.get('shard_boundaries')] = @model.get('id')

            # Present a confirmation dialog to make sure the user actually wants this
            confirmation_modal = new UIComponents.ConfirmationDialogModal
            confirmation_modal.render("Are you sure you want to make machine #{@model.get('name')} the master for this shard in datacenter #{@datacenter.get('name')}?",
                "/ajax/semilattice/memcached_namespaces/#{@namespace.get('id')}/primary_pinnings",
                JSON.stringify(post_data),
                (response) =>
                    # Set the link's text to a loading state
                    $link = @.$('a.make-master')
                    $link.text $link.data('loading-text')

                    clear_modals()
                    $('#user-alert-space').append (@alert_tmpl {})
                    
                    # Trigger a manual refresh of the data
                    collect_server_data_once(false)
            )

        get_available_machines_in_datacenter: =>
            machines_in_datacenter = DataUtils.get_datacenter_machines(@model.get('datacenter_uuid'))
            # Machines that are unused (neither primary nor secondary replicas for this shard)
            available_machines = _.filter machines_in_datacenter, (machine) =>
                return false if machine.get('id') is @shard.get('primary_uuid')
                for datacenter_uuid, machine_uuids of @shard.get('secondary_uuids')
                    return false if machine.get('id') in machine_uuids
                return true
            return _.map available_machines, (machine) ->
                name: machine.get('name')
                id: machine.get('id')

        get_currently_pinned: =>
            # Grab pinned machines for this shard
            for shard, pins of @namespace.get('secondary_pinnings')
                return pins if JSON.stringify(@shard.get('shard_boundaries')) is JSON.stringify(shard)

        render: =>
            @.$el.html @template _.extend @model.toJSON(),
                status: DataUtils.get_machine_reachability(@model.get('id'))
                is_master: @model.get('id') is @options.args.shard.get('primary_uuid')
                cannot_be_master: @options.args.datacenter.get('id') isnt @options.args.namespace.get('primary_uuid')
                backfill_progress: DataUtils.get_backfill_progress(@namespace.get('id'), @shard.get('shard_boundaries'), @model.get('id'))
                no_available_machines: @get_available_machines_in_datacenter().length is 0

            ###
            # Popover to change the machine
            @.$('a[rel=popover]').popover
                html: true
                trigger: 'manual'
                content: @change_machine_popover
                    available_machines: @get_available_machines_in_datacenter()
            ###

            @.delegateEvents()
            return @

        destroy: =>
            @shard.off()
            @model.off()
            directory.off()

    class @SetMachineModal extends UIComponents.AbstractModal
        template: Handlebars.compile $('#shard-set_machine-template').html()
        alert_tmpl: Handlebars.compile $('#shard-set_machine-alert-template').html()
        class: 'set-machine-modal'

        initialize: (namespace) ->
            log_initial '(initializing) modal dialog: set datacenter'
            @namespace = namespace
            @data = 
                modal_title: 'Set new machine'
                btn_primary_text: 'Commit'
                machines: []
            super

        set_data: (data) =>
            for key of data
                @data[key] = data[key]

        render: (_machines_list) ->
            log_render '(rendering) set machine dialog'
            super @data

        on_submit: (response) =>
            super
            form_data = form_data_as_object($('form', @$modal))
            post_data = {}
            old_pin = JSON.parse form_data.old_pin
            old_pin.push form_data.server_to_pin
            post_data[form_data.shard_name] = old_pin
            $.ajax
                processData: false
                url: "/ajax/semilattice/#{@namespace.get("protocol")}_namespaces/#{@namespace.get('id')}/secondary_pinnings"
                type: 'POST'
                data: JSON.stringify(post_data)
                success: @on_success
                error: @on_error

        on_success: (response) =>
            super

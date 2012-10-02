# Helper stuff
Handlebars.registerPartial 'shard_name', $('#shard_name-partial').html()

# Namespace view
module 'NamespaceView', ->
    class @Pinning extends Backbone.View
        className: 'assignments_container modal overwrite_modal'
        template: Handlebars.compile $('#namespace_view-assignments_container-template').html()

        initialize: =>
            @datacenter_views = []
            @datacenter_displayed = {}

            for datacenter in datacenters.models
                @datacenter_views.push new NamespaceView.DatacenterViewRead
                    model: datacenter
                    namespace: @model
                @datacenter_displayed[datacenter.get('id')] = true
            @datacenter_views.sort (a,b) ->
                if b.model.get('name') > a.model.get('name')
                    return 1
                else if b.model.get('name') < a.model.get('name')
                    -1
                else
                    return 0

            @universe_datacenter_view = new NamespaceView.DatacenterViewRead
                model:universe_datacenter
                namespace: @model
            
            datacenters.on 'all', @compute_datacenters

        compute_datacenters: =>
            need_render = false
            for datacenter in datacenters.models
                if not datacenter.get('id') of @datacenter_displayed
                    @datacenter_views.push new NamespaceView.DatacenterViewRead
                        model: datacenter
                        namespace: @model
                    @datacenter_dispayed[datacenter.get('id')] = true
                    need_render = true

            for datacenter_id of @datacenter_displayed
                if not datacenters.get(datacenter_id)?
                    @datacenter_displayed[datacenter_id] = undefined
                    
                    for datacenter, i in @datacenter_views
                        if datacenter.model.get('id') is datacenter_id
                            @datacenters_views[i].destroy()
                            @datacenters_views.splice i, 1
                            need_render = true
                            break

            if need_render is true
                @datacenter_views.sort (a,b) ->
                    if b.model.get('name') > a.model.get('name')
                        return 1
                    else if b.model.get('name') < a.model.get('name')
                        -1
                    else
                        return 0
            @render()

        render: =>
            @.$el.html @template

            for datacenter_view in @datacenter_views
                @.$('.datacenter_ul').append datacenter_view.render().$el
            @.$('.datacenter_ul').append @universe_datacenter_view.render().$el
            return @

        destroy: =>
            datacenters.off 'all', @compute_datacenters

            for datacenter_view in @datacenter_views
                datacenter_view.destroy()

    class @DatacenterViewRead extends Backbone.View
        tagName: 'li'
        className: 'datacenter_li'
        template: Handlebars.compile $('#namespace_view-datacenter_view-template').html()
        initialize: (data) =>
            @model = data.model
            @namespace = data.namespace

            @machines_views = []
            @machines_displayed = {}
            for machine in machines.models
                if machine.get('datacenter_uuid') is @model.get('id')
                    @machines_views.push new NamespaceView.MachineViewRead
                        model: machine
                        namespace: @namespace
                    @machines_displayed[machine.get('id')] = true
            @machines_views.sort (a,b) ->
                if b.model.get('name') > a.model.get('name')
                    return 1
                else if b.model.get('name') < a.model.get('name')
                    -1
                else
                    return 0

            @model.on 'change:name', @render
            machines.on 'all', @compute_machines

        compute_machines: =>
            need_render = false
            for machine in machines.models
                if not machine.get('id') of @machines_displayed and machine.get('datacenter_uuid') is @model.get('id')
                    @machines_views.push new NamespaceView.machineViewRead
                        model: machine
                        namespace: @model
                    @machines_displayed[machine.get('id')] = true
                    need_render = true

            for machine_id of @machines_displayed
                if not machines.get(machine_id)?
                    @machines_displayed[machine_id] = undefined
                    for machine, i in @machines_views
                        if machine.model.get('id') is machine_id
                            @machines_views[i].destroy()
                            @machines_views.splice i, 1
                            need_render = true
                            break

            if need_render is true
                @machines_views.sort (a,b) -> 
                    if b.model.get('name') > a.model.get('name')
                        return 1
                    else if b.model.get('name') < a.model.get('name')
                        -1
                    else
                        return 0

                @render()

        render: =>
            @.$el.html @template
                name: if @model.get('id') is universe_datacenter.get('id') then 'Unassigned machines' else @model.get('name')
            
            for machine_view in @machines_views
                @.$('.machine_ul').append machine_view.render().$el

            return @

        destroy: =>
            @model.off 'change:name', @render
            machines.off 'all', @compute_machines

            for machine_view in @machines_views
                machine_view.destroy()

    class @MachineViewRead extends Backbone.View
        tagName: 'li'
        className: 'machine_li'
        template: Handlebars.compile $('#namespace_view-machine_view-template').html()
        initialize: (data) =>
            @model = data.model
            @namespace = data.namespace
            @compute_shards()

            @namespace.on 'change:key_distr', @compute_shards
            @namespace.on 'change:shards', @compute_shards
            @namespace.on 'change:blueprint', @compute_shards
            @model.on 'change:name', @render()

        compute_shards: =>
            @shards = []
            for machine_id, peer_roles of @namespace.get('blueprint').peers_roles
                if machine_id is @model.get('id')
                    for shard, role of peer_roles
                        keys = @namespace.compute_shard_rows_approximation shard
                        if role is 'role_nothing'
                            role_pretty = 'nothing'
                        else if role is 'role_secondary'
                            role_pretty = 'secondary'
                        else if role is 'role_primary'
                            role_pretty = 'primary'
                        @shards.push
                            name: human_readable_shard shard
                            keys: keys if typeof keys is 'string'
                            role: role_pretty
            @render()


        render: =>
            @.$el.html @template
                name: @model.get('name')
                shards: @shards

            return @

        destroy: =>
            @namespace.off 'change:key_distr', @compute_shards
            @namespace.off 'change:shards', @compute_shards
            @namespace.off 'change:blueprint', @compute_shards
            @model.off 'change:name', @render()



# Namespace view
module 'NamespaceView', ->
    # Container for the entire namespace view
    class @Container extends Backbone.View
        className: 'namespace-view'
        template: Handlebars.compile $('#namespace_view-container-template').html()
        events: ->
            'click a.rename-namespace': 'rename_namespace'
            'click #user-alert-space a.close': 'hide_user_alert'

        initialize: (id) =>
            log_initial '(initializing) namespace view: container'
            @namespace_uuid = id

        hide_user_alert: (event) ->
            event.preventDefault()
            @.$('#user-alert-space .alert').remove()

        rename_namespace: (event) ->
            event.preventDefault()
            rename_modal = new UIComponents.RenameItemModal @model.get('id'), 'namespace'
            rename_modal.render()

        wait_for_model_noop: =>
            return true

        wait_for_model: =>
            @model = namespaces.get(@namespace_uuid)
            if not @model
                namespaces.off 'all', @render
                namespaces.on 'all', @render
                return false

            # Model is finally ready, unbind necessary handlers
            namespaces.off 'all', @render

            # Some additional setup
            @profile = new NamespaceView.Profile(model: @model)
            @replicas = new NamespaceView.Replicas(model: @model)
            @shards = new NamespaceView.Sharding(model: @model)

            # We no longer need all this logic in wait_for_model, so
            # switch it to noop for the callers
            @wait_for_model = @wait_for_model_noop

            return true

        render_empty: =>
            @.$el.text 'Namespace ' + @namespace_uuid + ' is not available.'
            return @

        render: =>
            log_render '(rendering) namespace view: container'

            if @wait_for_model() is false
                return @render_empty()

            json = @model.toJSON()
            @.$el.html @template json

            # Add the replica and shards views
            @.$('.profile-holder').html @profile.render().el
            @.$('.section.replication').html @replicas.render().el
            @.$('.section.sharding').html @shards.render().el

            return @

    # Profile view
    class @Profile extends Backbone.View
        className: 'namespace-profile'
        template: Handlebars.compile $('#namespace_view-profile-template').html()

        initialize: ->
            # @model is a namespace.  somebody is supposed to pass model: namespace to the constructor.
            @model.on 'all', @render
            directory.on 'all', @render
            progress_list.on 'all', @render
            log_initial '(initializing) namespace view: replica'

        render: =>
            json = @model.toJSON()
            json = _.extend json, DataUtils.get_namespace_status(@model.get('id'))
            json = _.extend json,
                old_alert_html: @.$('#user-alert-space').html()
            @.$el.html @template json

            return @


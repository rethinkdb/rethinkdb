# Copyright 2010-2012 RethinkDB, all rights reserved.
# This file extends the UIComponents module with commonly used progressbars.

module 'UIComponents', ->
    # Progress bar that has different states
    class @OperationProgressBar extends Backbone.View
        # These are the possible states for the ProgressBar
        states: ['none', 'starting', 'processing', 'finished']
        stage: 'none'

        # The initialize function takes one optional argument:
        #   - template: optional custom template
        initialize: (template) ->
            if template?
                @template = template
            else
                @template = Handlebars.templates['progressbar-template']
            @timeout = null
            @percent = 0

        reset_percent: =>
            @percent = 0

        # The render function takes a number of arguments:
        #   - current_value: current value of the operation or status being monitored
        #   - max_value: maximum value of the the operation or status being monitored
        #   - additional_info: optional argument, hash that can include a
        #     number of details to describe the state of the operation, etc.
        #       * new_value: the new desired maximum value as a result of the operation
        #       * got_response: flag indicating that the goals have been set
        #         and we're ready to start processing
        #   - cb: callback to execute once we remove the progress bar
        render: (current_value, max_value, additional_info, cb) =>
            if current_value isnt max_value and @timeout?
                # We are in a finished state, but current_value != max_value,
                # so the server sent us an update and we start processing again
                clearTimeout @timeout
                @timeout = null
                @stage = 'processing'

            # State machine
            if @stage is 'none' and additional_info.new_value?
                @stage = 'starting'
                if @timeout? # If there is a timeout, we have to clear it, we are not done yet
                    clearTimeout @timeout
                    @timeout = null
            if @stage is 'starting' and additional_info.got_response?
                @stage = 'processing'
                if @timeout? # If there is a timeout, we have to clear it, we are not done yet
                    clearTimeout @timeout
                    @timeout = null
            if @stage is 'processing' and current_value is max_value
                @stage = 'finished'
                if @timeout? # If there is a timeout, we have to clear it, we are not done yet
                    clearTimeout @timeout
                    @timeout = null

            data = _.extend additional_info,
                current_value: current_value
                max_value: max_value

            # Depending on the current state, choose the correct template data
            if @stage is 'starting'
                data = _.extend data,
                    operation_active: true
                    starting: true

            if @stage is 'processing'
                if current_value is max_value
                    percent_complete = 0
                else
                    percent_complete = Math.floor current_value/max_value*100

                # Make sure we never go back
                if additional_info.check is true
                    if percent_complete < @percent
                        percent_complete = @percent
                    else
                        @percent = percent_complete

                data = _.extend data,
                    operation_active: true
                    processing: true
                    percent_complete: percent_complete

            if @stage is 'finished'
                data = _.extend data,
                    operation_active: true
                    finished: true
                    percent_complete: 100

                if not @timeout? # If there is already a timeout, no need to add another one
                    @timeout = setTimeout =>
                        @stage = 'none'
                        @render current_value, max_value, {}
                        @timeout = null
                        if cb?
                            cb()
                    , 2000
            
            @.$el.html @template data

            return @
       
        skip_to_processing: =>
            @stage = 'processing'
            if @timeout? # If there is a timeout, we have to clear it, we are not done yet
                clearTimeout @timeout
                @timeout = null
        set_none_state: =>
            @stage = 'none'

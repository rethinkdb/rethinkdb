# This file extends the UIComponents module with commonly used progressbars.

module 'UIComponents', ->
    # Progress bar that has different states
    class @OperationProgressBar extends Backbone.View
        template: Handlebars.compile $('#progressbar-template').html()
        # These are the possible states for the ProgressBar
        states: ['none', 'starting', 'processing', 'finished']
        stage: 'none'

        # The initialize function takes one optional argument:
        #   - template: optional custom template
        initialize: (template) ->
            @template = template if template?

        # The render function takes a number of arguments:
        #   - current_value: current value of the operation or status being monitored
        #   - max_value: maximum value of the the operation or status being monitored
        #   - additional_info: optional argument, hash that can include a
        #     number of details to describe the state of the operation, etc.
        #       * new_value: the new desired maximum value as a result of the operation
        #       * got_response: flag indicating that the goals have been set
        #         and we're ready to start processing
        render: (current_value, max_value, additional_info) =>
            # State machine
            if @stage is 'none' and additional_info.new_value?
                @stage = 'starting'
            if @stage is 'starting' and additional_info.got_response?
                @stage = 'processing'
            if @stage is 'processing' and current_value is max_value
                @stage = 'finished'

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
                    percent_complete = current_value / max_value * 100 
                data = _.extend data,
                    operation_active: true
                    processing: true
                    percent_complete: percent_complete

            if @stage is 'finished'
                data = _.extend data,
                    operation_active: true
                    finished: true
                    percent_complete: 100

                setTimeout =>
                    @render current_value, max_value, {}
                    @stage = 'none'
                , 2000
            
            @.$el.html @template data

            return @

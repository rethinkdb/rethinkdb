# Copyright 2010-2015 RethinkDB

# This file extends the UIComponents module with commonly used modal
# dialog boxes.

models = require('../models.coffee')
util = require('../util.coffee')
app = require('../app.coffee')
driver = app.driver
system_db = app.system_db

r = require('rethinkdb')

# Modal that allows for form submission
class AbstractModal extends Backbone.View
    template_outer: require('../../handlebars/abstract-modal-outer.hbs')
    error_template: require('../../handlebars/error_input.hbs')

    events:
        'click .cancel': 'cancel_modal'
        'click .close_modal': 'cancel_modal'
        'click .btn-primary': 'abstract_submit'
        'keypress input': 'check_keypress_is_enter'
        'click .alert .close': 'close_error'
        'click .change-route': 'reroute'


    close_error: (event) ->
        event.preventDefault()
        $(event.currentTarget).parent().slideUp('fast', -> $(this).remove())


    initialize: ->
        @$container = $('#modal-dialog')
        @custom_buttons = []

    # Render and show the modal.
    #   validator_options: object that defines form behavior and validation rules
    #   template_json: json to pass to the template for the modal
    render: (template_data) =>

        # Add the modal generated from the template to the container, and show it
        template_data = {} if not template_data?
        template_data = _.extend template_data,
            modal_class: @class
        @$container.html(@template_outer template_data).addClass('visible')
        $('.modal-body', @$container).html(@template template_data)

        # Note: Bootstrap's modal JS moves the modal from the container element to the end of the body tag in the DOM
        @$modal = $('.modal', @$container).modal
            'show': true
            'backdrop': true
            'keyboard': true
        .on 'hidden', =>
            # Removes the modal dialog from the DOM
            @hide_modal()

        # Define @el to be the modal (the root of the view), make sure events perform on it
        @setElement @$modal
        @delegateEvents()

        for btn in @custom_buttons
            @$('.custom_btn_placeholder').append("<button class='btn #{ btn.class_str }' data-loading-text='#{ btn.data_loading_text }'>#{ btn.main_text }</button>")
            @$('.custom_btn_placeholder > .' + btn.class_str).click (e) =>
                btn.on_click(e)
            @$('.custom_btn_placeholder > .' + btn.class_str).button()

    hide_modal: =>
        @$container.removeClass('visible')
        @$modal.modal('hide') if @$modal?

    cancel_modal: (e) ->
        @hide_modal()
        e.preventDefault()

    reroute: (e) ->
        @hide_modal()

    check_keypress_is_enter: (event) =>
        if event.which is 13
            event.preventDefault()
            @abstract_submit(event)

    abstract_submit: (event) ->
        event.preventDefault()
        @on_submit(event)

    reset_buttons: =>
        @$('.btn-primary').button('reset')
        @$('.cancel').button('reset')

    # This is meant to be called by the overriding class
    on_success: (response) =>
        @reset_buttons()
        @remove()

    on_submit: (event) =>
        @$('.btn-primary').button('loading')
        @$('.cancel').button('loading')

    on_error: (error) =>
        @$('.alert_modal').html @error_template
            ajax_fail: true
            error: (error if error? and error isnt '')

        if @$('.alert_modal_content').css('display') is 'none'
            @$('.alert_modal_content').slideDown('fast')
        else
            @$('.alert_modal_content').css('display', 'none')
            @$('.alert_modal_content').fadeIn()
        @reset_buttons()

    add_custom_button: (main_text, class_str, data_loading_text, on_click) =>
        @custom_buttons.push
            main_text: main_text
            class_str: class_str
            data_loading_text: data_loading_text
            on_click: on_click

    find_custom_button: (class_str) =>
        @$('.custom_btn_placeholder > .' + class_str)

    remove: =>
        @hide_modal()
        super()

# This is for doing user confirmation easily
class ConfirmationDialogModal extends AbstractModal
    template: require('../../handlebars/confirmation_dialog.hbs')
    class: 'confirmation-modal'

    render: (message, _url, _data, _on_success) ->
        @url = _url
        @data = _data
        @on_user_success = _on_success

        super
            message: message
            modal_title: 'Confirmation'
            btn_secondary_text: 'No'
            btn_primary_text: 'Yes'
        @$('.btn-primary').focus()

    on_submit: ->
        super
        $.ajax
            processData: false
            url: @url
            type: 'POST'
            contentType: 'application/json'
            data: @data
            success: @on_success
            error: @on_error

    on_success: (response) ->
        super
        @on_user_success(response)

# Rename common items (tables, databases, servers)
# The modal takes a few arguments:
#   - item_id: id of the element to rename
#   - item_type: type of the element to rename
#   - on_success: function to perform on successful rename
#   - options:
#     * hide_alert: hide the alert shown in the user space on success
class RenameItemModal extends AbstractModal
    template: require('../../handlebars/rename_item-modal.hbs')
    alert_tmpl: require('../../handlebars/renamed_item-alert.hbs')
    error_template: require('../../handlebars/error_input.hbs')
    class: 'rename-item-modal'

    initialize: (options) =>
        super
        if @model instanceof models.Table
            @item_type = 'table'
        else if @model instanceof models.Database
            @item_type = 'database'
        else if @model instanceof models.Server
            @item_type = 'server'
        else if options.item_type?
            @item_type = options.item_type
        else
            throw "Rename *what* kind of item?"

    render: ->
        super
            type: @item_type
            old_name: @model.get('name')
            id: @model.get('id')
            modal_title: "Rename #{@item_type}"
            btn_primary_text: "Rename #{@item_type}"

        @$('#focus_new_name').focus()


    on_submit: =>
        super
        @old_name = @model.get('name')
        @formdata = util.form_data_as_object($('form', @$modal))

        no_error = true
        if @formdata.new_name is ''
            no_error = false
            $('.alert_modal').html @error_template
                empty_name: true
        else if /^[a-zA-Z0-9_]+$/.test(@formdata.new_name) is false
            no_error = false
            $('.alert_modal').html @error_template
                special_char_detected: true
                type: @item_type

        # Check if already use
        if no_error is true
            if @item_type == "table"
                query = r.db(system_db).table('table_config').get(@model.get('id')).update
                    name: @formdata.new_name
            else if @item_type == "database"
                query = r.db(system_db).table('db_config').get(@model.get('id')).update
                    name: @formdata.new_name
            else if @item_type == "server"
                query = r.db(system_db).table('server_config').get(@model.get('id')).update
                    name: @formdata.new_name

            driver.run_once query, (err, result) =>
                if err?
                    @on_error err
                else if result?.first_error?
                    @on_error new Error result.first_error
                else
                    if result?.replaced is 1
                        @on_success()
                    else
                        @on_error(new Error("The value returned for `replaced` was not 1."))
        else
            $('.alert_modal_content').slideDown 'fast'
            @reset_buttons()

    on_success: (response) ->
        super
        old_name = @model.get 'name'
        @model.set
            name: @formdata.new_name

        # Unless an alerts should be suppressed, show an alert
        if not @options?.hide_alert
            # notify the user that we succeeded
            $('#user-alert-space').html @alert_tmpl
                type: @item_type
                old_name: old_name
                new_name: @model.get 'name'

        # Call custom success function
        if typeof @options?.on_success is 'function'
            @options.on_success @model


exports.AbstractModal = AbstractModal
exports.ConfirmationDialogModal = ConfirmationDialogModal
exports.RenameItemModal = RenameItemModal

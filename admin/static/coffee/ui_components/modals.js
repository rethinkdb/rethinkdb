// Copyright 2010-2015 RethinkDB

// This file extends the UIComponents module with commonly used modal
// dialog boxes.

const models = require('../models.coffee')
const util = require('../util.coffee')
const app = require('../app.coffee')
const { driver } = app
const { system_db } = app

const r = require('rethinkdb')

// Modal that allows for form submission
class AbstractModal extends Backbone.View {
  constructor (...args) {
    this.render = this.render.bind(this)
    this.hide_modal = this.hide_modal.bind(this)
    this.check_keypress_is_enter = this.check_keypress_is_enter.bind(this)
    this.reset_buttons = this.reset_buttons.bind(this)
    this.on_success = this.on_success.bind(this)
    this.on_submit = this.on_submit.bind(this)
    this.on_error = this.on_error.bind(this)
    this.add_custom_button = this.add_custom_button.bind(this)
    this.find_custom_button = this.find_custom_button.bind(this)
    this.remove = this.remove.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template_outer = require('../../handlebars/abstract-modal-outer.hbs')
    this.prototype.error_template = require('../../handlebars/error_input.hbs')

    this.prototype.events = {
      'click .cancel'      : 'cancel_modal',
      'click .close_modal' : 'cancel_modal',
      'click .btn-primary' : 'abstract_submit',
      'keypress input'     : 'check_keypress_is_enter',
      'click .alert .close': 'close_error',
      'click .change-route': 'reroute',
    }
  }

  close_error (event) {
    event.preventDefault()
    return $(event.currentTarget).parent().slideUp('fast', function () {
      return $(this).remove()
    })
  }

  initialize () {
    this.$container = $('#modal-dialog')
    return this.custom_buttons = []
  }

  // Render and show the modal.
  //   validator_options: object that defines form behavior and validation rules
  //   template_json: json to pass to the template for the modal
  render (template_data) {
    // Add the modal generated from the template to the container, and show it
    if (template_data == null) {
      template_data = {}
    }
    template_data = _.extend(template_data, { modal_class: this.class })
    this.$container.html(this.template_outer(template_data)).addClass('visible')
    $('.modal-body', this.$container).html(this.template(template_data))

    // Note: Bootstrap's modal JS moves the modal from the container element to the end of the body tag in the DOM
    this.$modal = $('.modal', this.$container)
      .modal({
        show    : true,
        backdrop: true,
        keyboard: true,
      })
      .on('hidden', () => {
        // Removes the modal dialog from the DOM
        return this.hide_modal()
      })

    // Define @el to be the modal (the root of the view), make sure events perform on it
    this.setElement(this.$modal)
    this.delegateEvents()

    return Array.from(this.custom_buttons).map(
      btn =>
        (this
          .$('.custom_btn_placeholder')
          .append(
            `<button class='btn ${btn.class_str}' data-loading-text='${btn.data_loading_text}'>${btn.main_text}</button>`
          ), this.$(`.custom_btn_placeholder > .${btn.class_str}`).click(e => {
            return btn.on_click(e)
          }), this.$(`.custom_btn_placeholder > .${btn.class_str}`).button())
    )
  }

  hide_modal () {
    this.$container.removeClass('visible')
    if (this.$modal != null) {
      return this.$modal.modal('hide')
    }
  }

  cancel_modal (e) {
    this.hide_modal()
    return e.preventDefault()
  }

  reroute (e) {
    return this.hide_modal()
  }

  check_keypress_is_enter (event) {
    if (event.which === 13) {
      event.preventDefault()
      return this.abstract_submit(event)
    }
  }

  abstract_submit (event) {
    event.preventDefault()
    return this.on_submit(event)
  }

  reset_buttons () {
    this.$('.btn-primary').button('reset')
    return this.$('.cancel').button('reset')
  }

  // This is meant to be called by the overriding class
  on_success (response) {
    this.reset_buttons()
    return this.remove()
  }

  on_submit (event) {
    this.$('.btn-primary').button('loading')
    return this.$('.cancel').button('loading')
  }

  on_error (error) {
    this.$('.alert_modal').html(
      this.error_template({
        ajax_fail: true,
        error    : error != null && error !== '' ? error : undefined,
      })
    )

    if (this.$('.alert_modal_content').css('display') === 'none') {
      this.$('.alert_modal_content').slideDown('fast')
    } else {
      this.$('.alert_modal_content').css('display', 'none')
      this.$('.alert_modal_content').fadeIn()
    }
    return this.reset_buttons()
  }

  add_custom_button (main_text, class_str, data_loading_text, on_click) {
    return this.custom_buttons.push({
      main_text,
      class_str,
      data_loading_text,
      on_click,
    })
  }

  find_custom_button (class_str) {
    return this.$(`.custom_btn_placeholder > .${class_str}`)
  }

  remove () {
    this.hide_modal()
    return super.remove()
  }
}
AbstractModal.initClass()

// This is for doing user confirmation easily
class ConfirmationDialogModal extends AbstractModal {
  static initClass () {
    this.prototype.template = require('../../handlebars/confirmation_dialog.hbs')
    this.prototype.class = 'confirmation-modal'
  }

  render (message, _url, _data, _on_success) {
    this.url = _url
    this.data = _data
    this.on_user_success = _on_success

    super.render({
      message,
      modal_title       : 'Confirmation',
      btn_secondary_text: 'No',
      btn_primary_text  : 'Yes',
    })
    return this.$('.btn-primary').focus()
  }

  on_submit (...args) {
    super.on_submit(...args)
    return $.ajax({
      processData: false,
      url        : this.url,
      type       : 'POST',
      contentType: 'application/json',
      data       : this.data,
      success    : this.on_success,
      error      : this.on_error,
    })
  }

  on_success (response) {
    super.on_success(...arguments)
    return this.on_user_success(response)
  }
}
ConfirmationDialogModal.initClass()

// Rename common items (tables, databases, servers)
// The modal takes a few arguments:
//   - item_id: id of the element to rename
//   - item_type: type of the element to rename
//   - on_success: function to perform on successful rename
//   - options:
//     * hide_alert: hide the alert shown in the user space on success
class RenameItemModal extends AbstractModal {
  constructor (...args) {
    this.initialize = this.initialize.bind(this)
    this.on_submit = this.on_submit.bind(this)
    super(...args)
  }

  static initClass () {
    this.prototype.template = require('../../handlebars/rename_item-modal.hbs')
    this.prototype.alert_tmpl = require('../../handlebars/renamed_item-alert.hbs')
    this.prototype.error_template = require('../../handlebars/error_input.hbs')
    this.prototype.class = 'rename-item-modal'
  }

  initialize ({ item_type }) {
    super.initialize(...arguments)
    if (this.model instanceof models.Table) {
      return this.item_type = 'table'
    } else if (this.model instanceof models.Database) {
      return this.item_type = 'database'
    } else if (this.model instanceof models.Server) {
      return this.item_type = 'server'
    } else if (item_type != null) {
      return this.item_type = item_type
    } else {
      throw 'Rename *what* kind of item?'
    }
  }

  render () {
    super.render({
      type            : this.item_type,
      old_name        : this.model.get('name'),
      id              : this.model.get('id'),
      modal_title     : `Rename ${this.item_type}`,
      is_database     : this.item_type === 'database',
      btn_primary_text: `Rename ${this.item_type}`,
    })

    return this.$('#focus_new_name').focus()
  }

  on_submit (...args) {
    super.on_submit(...args)
    this.old_name = this.model.get('name')
    this.formdata = util.form_data_as_object($('form', this.$modal))

    let no_error = true
    if (this.formdata.new_name === '') {
      no_error = false
      $('.alert_modal').html(
        this.error_template({
          empty_name: true,
        })
      )
    } else if (/^[a-zA-Z0-9_]+$/.test(this.formdata.new_name) === false) {
      no_error = false
      $('.alert_modal').html(
        this.error_template({
          special_char_detected: true,
          type                 : this.item_type,
        })
      )
    }

    // Check if already use
    if (no_error === true) {
      let query
      if (this.item_type === 'table') {
        query = r.db(system_db).table('table_config').get(this.model.get('id')).update({
          name: this.formdata.new_name,
        })
      } else if (this.item_type === 'database') {
        query = r.db(system_db).table('db_config').get(this.model.get('id')).update({
          name: this.formdata.new_name,
        })
      } else if (this.item_type === 'server') {
        query = r.db(system_db).table('server_config').get(this.model.get('id')).update({
          name: this.formdata.new_name,
        })
      }

      return driver.run_once(query, (err, result) => {
        if (err != null) {
          return this.on_error(err)
        } else if (__guard__(result, ({ first_error }) => first_error) != null) {
          return this.on_error(new Error(result.first_error))
        } else {
          if (__guard__(result, ({ replaced }) => replaced) === 1) {
            return this.on_success()
          } else {
            return this.on_error(new Error('The value returned for `replaced` was not 1.'))
          }
        }
      })
    } else {
      $('.alert_modal_content').slideDown('fast')
      return this.reset_buttons()
    }
  }

  on_success (response) {
    super.on_success(...arguments)
    const old_name = this.model.get('name')
    this.model.set({
      name: this.formdata.new_name,
    })

    // Unless an alerts should be suppressed, show an alert
    if (!__guard__(this.options, ({ hide_alert }) => hide_alert)) {
      // notify the user that we succeeded
      $('#user-alert-space').html(
        this.alert_tmpl({
          type    : this.item_type,
          old_name,
          new_name: this.model.get('name'),
        })
      )
    }

    // Call custom success function
    if (typeof __guard__(this.options, ({ on_success }) => on_success) === 'function') {
      return this.options.on_success(this.model)
    }
  }
}
RenameItemModal.initClass()

exports.AbstractModal = AbstractModal
exports.ConfirmationDialogModal = ConfirmationDialogModal
exports.RenameItemModal = RenameItemModal

function __guard__ (value, transform) {
  return typeof value !== 'undefined' && value !== null ? transform(value) : undefined
}

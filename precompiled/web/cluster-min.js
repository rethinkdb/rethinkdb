window.VERSION = '2.0.3';
var capitalize, form_data_as_object, humanize_table_status;

Handlebars.registerHelper('comma_separated', function(context, block) {
  var i, out, _i, _ref;
  out = "";
  for (i = _i = 0, _ref = context.length; 0 <= _ref ? _i < _ref : _i > _ref; i = 0 <= _ref ? ++_i : --_i) {
    out += block(context[i]);
    if (i !== context.length - 1) {
      out += ", ";
    }
  }
  return out;
});

Handlebars.registerHelper('comma_separated_simple', function(context) {
  var i, out, _i, _ref;
  out = "";
  for (i = _i = 0, _ref = context.length; 0 <= _ref ? _i < _ref : _i > _ref; i = 0 <= _ref ? ++_i : --_i) {
    out += context[i];
    if (i !== context.length - 1) {
      out += ", ";
    }
  }
  return out;
});

Handlebars.registerHelper('links_to_servers', function(servers, safety) {
  var i, out, _i, _ref;
  out = "";
  for (i = _i = 0, _ref = servers.length; 0 <= _ref ? _i < _ref : _i > _ref; i = 0 <= _ref ? ++_i : --_i) {
    if (servers[i].exists) {
      out += '<a href="#servers/' + servers[i].id + '" class="links_to_other_view">' + servers[i].name + '</a>';
    } else {
      out += servers[i].name;
    }
    if (i !== servers.length - 1) {
      out += ", ";
    }
  }
  if ((safety != null) && safety === false) {
    return out;
  }
  return new Handlebars.SafeString(out);
});

Handlebars.registerHelper('links_to_datacenters_inline_for_replica', function(datacenters) {
  var i, out, _i, _ref;
  out = "";
  for (i = _i = 0, _ref = datacenters.length; 0 <= _ref ? _i < _ref : _i > _ref; i = 0 <= _ref ? ++_i : --_i) {
    out += '<strong>' + datacenters[i].name + '</strong>';
    if (i !== datacenters.length - 1) {
      out += ", ";
    }
  }
  return new Handlebars.SafeString(out);
});

Handlebars.registerHelper('pluralize_noun', function(noun, num, capitalize) {
  var ends_with_y, result;
  if (noun == null) {
    return 'NOUN_NULL';
  }
  ends_with_y = noun.substr(-1) === 'y';
  if (num === 1) {
    result = noun;
  } else {
    if (ends_with_y && (noun !== 'key')) {
      result = noun.slice(0, noun.length - 1) + "ies";
    } else if (noun.substr(-1) === 's') {
      result = noun + "es";
    } else if (noun.substr(-1) === 'x') {
      result = noun + "es";
    } else {
      result = noun + "s";
    }
  }
  if (capitalize === true) {
    result = result.charAt(0).toUpperCase() + result.slice(1);
  }
  return result;
});

Handlebars.registerHelper('pluralize_verb_to_have', function(num) {
  if (num === 1) {
    return 'has';
  } else {
    return 'have';
  }
});

Handlebars.registerHelper('pluralize_verb', function(verb, num) {
  if (num === 1) {
    return verb + 's';
  } else {
    return verb;
  }
});

capitalize = function(str) {
  if (str != null) {
    return str.charAt(0).toUpperCase() + str.slice(1);
  } else {
    return "NULL";
  }
};

Handlebars.registerHelper('capitalize', capitalize);

Handlebars.registerHelper('humanize_uuid', function(str) {
  if (str != null) {
    return str.substr(0, 6);
  } else {
    return "NULL";
  }
});

Handlebars.registerHelper('humanize_server_connectivity', function(status) {
  var connectivity, success;
  if (status == null) {
    status = 'N/A';
  }
  success = status === 'connected' ? 'success' : 'failure';
  connectivity = "<span class='label label-" + success + "'>" + (capitalize(status)) + "</span>";
  return new Handlebars.SafeString(connectivity);
});

humanize_table_status = function(status) {
  if (!status) {
    return "";
  } else if (status.all_replicas_ready || status.ready_for_writes) {
    return "Ready";
  } else if (status.ready_for_reads) {
    return 'Reads only';
  } else if (status.ready_for_outdated_reads) {
    return 'Outdated reads';
  } else {
    return 'Unavailable';
  }
};

Handlebars.registerHelper('humanize_table_readiness', function(status, num, denom) {
  var label, value;
  if (status === void 0) {
    label = 'failure';
    value = 'unknown';
  } else if (status.all_replicas_ready) {
    label = 'success';
    value = "" + (humanize_table_status(status)) + " " + num + "/" + denom;
  } else if (status.ready_for_writes) {
    label = 'partial-success';
    value = "" + (humanize_table_status(status)) + " " + num + "/" + denom;
  } else {
    label = 'failure';
    value = humanize_table_status(status);
  }
  return new Handlebars.SafeString("<div class='status label label-" + label + "'>" + value + "</div>");
});

Handlebars.registerHelper('humanize_table_status', humanize_table_status);

Handlebars.registerHelper('approximate_count', function(num) {
  var approx, result;
  if (num === 0) {
    return '0';
  } else if (num <= 5) {
    return '5';
  } else if (num <= 10) {
    return '10';
  } else {
    approx = Math.round(num / Math.pow(10, num.toString().length - 2)) * Math.pow(10, num.toString().length - 2);
    if (approx < 100) {
      return (Math.floor(approx / 10) * 10).toString();
    } else if (approx < 1000) {
      return (Math.floor(approx / 100) * 100).toString();
    } else if (approx < 1000000) {
      result = (approx / 1000).toString();
      if (result.length === 1) {
        result = result + '.0';
      }
      return result + 'K';
    } else if (approx < 1000000000) {
      result = (approx / 1000000).toString();
      if (result.length === 1) {
        result = result + '.0';
      }
      return result + 'M';
    } else {
      result = (approx / 1000000000).toString();
      if (result.length === 1) {
        result = result + '.0';
      }
      return result + 'B';
    }
  }
});

Handlebars.registerHelper('print_safe', function(str) {
  if (str != null) {
    return new Handlebars.SafeString(str);
  } else {
    return "";
  }
});

Handlebars.registerHelper('inc', function(num) {
  return num + 1;
});

Handlebars.registerPartial('backfill_progress_summary', $('#backfill_progress_summary-partial').html());

Handlebars.registerPartial('backfill_progress_details', $('#backfill_progress_details-partial').html());

Handlebars.registerHelper('if_defined', function(condition, options) {
  if (typeof condition !== 'undefined') {
    return options.fn(this);
  } else {
    return options.inverse(this);
  }
});

form_data_as_object = function(form) {
  var formarray, formdata, x, _i, _len;
  formarray = form.serializeArray();
  formdata = {};
  for (_i = 0, _len = formarray.length; _i < _len; _i++) {
    x = formarray[_i];
    formdata[x.name] = x.value;
  }
  return formdata;
};


/*
    Taken from "Namespacing and modules with Coffeescript"
    https://github.com/jashkenas/coffee-script/wiki/Easy-modules-with-coffeescript

    Introduces module function that allows namespaces by enclosing classes in anonymous functions.

    Usage:
    ------------------------------
        @module "foo", ->
          @module "bar", ->
            class @Amazing
              toString: "ain't it"
    ------------------------------

    Or, more simply:
    ------------------------------
        @module "foo.bar", ->
          class @Amazing
            toString: "ain't it"
    ------------------------------

    Which can then be accessed with:
    ------------------------------
        x = new foo.bar.Amazing
    ------------------------------
 */

this.module = function(names, fn) {
  var space, _name;
  if (typeof names === 'string') {
    names = names.split('.');
  }
  space = this[_name = names.shift()] || (this[_name] = {});
  space.module || (space.module = this.module);
  if (names.length) {
    return space.module(names, fn);
  } else {
    return fn.call(space);
  }
};
var IsDisconnected, Settings,
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('MainView', function() {
  this.MainContainer = (function(_super) {
    __extends(MainContainer, _super);

    function MainContainer() {
      this.remove = __bind(this.remove, this);
      this.toggle_options = __bind(this.toggle_options, this);
      this.hide_options = __bind(this.hide_options, this);
      this.render = __bind(this.render, this);
      this.fetch_data = __bind(this.fetch_data, this);
      this.fetch_ajax_data = __bind(this.fetch_ajax_data, this);
      this.start_router = __bind(this.start_router, this);
      this.initialize = __bind(this.initialize, this);
      return MainContainer.__super__.constructor.apply(this, arguments);
    }

    MainContainer.prototype.template = Handlebars.templates['body-structure-template'];

    MainContainer.prototype.id = 'main_view';

    MainContainer.prototype.initialize = function() {
      this.fetch_ajax_data();
      this.databases = new Databases;
      this.tables = new Tables;
      this.servers = new Servers;
      this.issues = new Issues;
      this.dashboard = new Dashboard;
      this.alert_update_view = new MainView.AlertUpdates;
      this.options_view = new MainView.OptionsView({
        alert_update_view: this.alert_update_view
      });
      this.options_state = 'hidden';
      this.navbar = new TopBar.NavBarView({
        databases: this.databases,
        tables: this.tables,
        servers: this.servers,
        options_view: this.options_view,
        container: this
      });
      return this.topbar = new TopBar.Container({
        model: this.dashboard,
        issues: this.issues
      });
    };

    MainContainer.prototype.start_router = function() {
      this.router = new BackboneCluster({
        navbar: this.navbar
      });
      Backbone.history.start();
      return this.navbar.init_typeahead();
    };

    MainContainer.prototype.fetch_ajax_data = function() {
      return $.ajax({
        contentType: 'application/json',
        url: 'ajax/me',
        dataType: 'json',
        success: (function(_this) {
          return function(response) {
            return _this.fetch_data(response);
          };
        })(this),
        error: (function(_this) {
          return function(response) {
            return _this.fetch_data(null);
          };
        })(this)
      });
    };

    MainContainer.prototype.fetch_data = function(server_uuid) {
      var query;
      query = r.expr({
        tables: r.db(system_db).table('table_config').merge({
          id: r.row("id")
        }).pluck('db', 'name', 'id').coerceTo("ARRAY"),
        servers: r.db(system_db).table('server_config').merge({
          id: r.row("id")
        }).pluck('name', 'id').coerceTo("ARRAY"),
        issues: driver.queries.issues_with_ids(),
        num_issues: r.db(system_db).table('current_issues').count(),
        num_servers: r.db(system_db).table('server_config').count(),
        num_available_servers: r.db(system_db).table('server_status').filter(function(server) {
          return server("status").eq("connected");
        }).count(),
        num_tables: r.db(system_db).table('table_config').count(),
        num_available_tables: r.db(system_db).table('table_status')('status').filter(function(status) {
          return status("all_replicas_ready");
        }).count(),
        me: r.db(system_db).table('server_status').get(server_uuid)('name')
      });
      return this.timer = driver.run(query, 5000, (function(_this) {
        return function(error, result) {
          var server, table, _i, _j, _len, _len1, _ref, _ref1;
          if (error != null) {
            return console.log(error);
          } else {
            _ref = result.tables;
            for (_i = 0, _len = _ref.length; _i < _len; _i++) {
              table = _ref[_i];
              _this.tables.add(new Table(table), {
                merge: true
              });
              delete result.tables;
            }
            _ref1 = result.servers;
            for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
              server = _ref1[_j];
              _this.servers.add(new Server(server), {
                merge: true
              });
              delete result.servers;
            }
            _this.issues.set(result.issues);
            delete result.issues;
            return _this.dashboard.set(result);
          }
        };
      })(this));
    };

    MainContainer.prototype.render = function() {
      this.$el.html(this.template());
      this.$('#updates_container').html(this.alert_update_view.render().$el);
      this.$('#options_container').html(this.options_view.render().$el);
      this.$('#topbar').html(this.topbar.render().$el);
      this.$('#navbar-container').html(this.navbar.render().$el);
      return this;
    };

    MainContainer.prototype.hide_options = function() {
      return this.$('#options_container').slideUp('fast');
    };

    MainContainer.prototype.toggle_options = function(event) {
      event.preventDefault();
      if (this.options_state === 'visible') {
        this.options_state = 'hidden';
        return this.hide_options(event);
      } else {
        this.options_state = 'visible';
        return this.$('#options_container').slideDown('fast');
      }
    };

    MainContainer.prototype.remove = function() {
      driver.stop_timer(this.timer);
      this.alert_update_view.remove();
      this.options_view.remove();
      return this.navbar.remove();
    };

    return MainContainer;

  })(Backbone.View);
  this.OptionsView = (function(_super) {
    __extends(OptionsView, _super);

    function OptionsView() {
      this.turn_updates_off = __bind(this.turn_updates_off, this);
      this.turn_updates_on = __bind(this.turn_updates_on, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return OptionsView.__super__.constructor.apply(this, arguments);
    }

    OptionsView.prototype.className = 'options_background';

    OptionsView.prototype.template = Handlebars.templates['options_view-template'];

    OptionsView.prototype.events = {
      'click label[for=updates_yes]': 'turn_updates_on',
      'click label[for=updates_no]': 'turn_updates_off'
    };

    OptionsView.prototype.initialize = function(data) {
      return this.alert_update_view = data.alert_update_view;
    };

    OptionsView.prototype.render = function() {
      var _ref;
      this.$el.html(this.template({
        check_update: ((_ref = window.localStorage) != null ? _ref.check_updates : void 0) != null ? JSON.parse(window.localStorage.check_updates) : true,
        version: window.VERSION
      }));
      return this;
    };

    OptionsView.prototype.turn_updates_on = function(event) {
      window.localStorage.check_updates = JSON.stringify(true);
      window.localStorage.removeItem('ignore_version');
      return this.alert_update_view.check();
    };

    OptionsView.prototype.turn_updates_off = function(event) {
      window.localStorage.check_updates = JSON.stringify(false);
      return this.alert_update_view.hide();
    };

    return OptionsView;

  })(Backbone.View);
  return this.AlertUpdates = (function(_super) {
    __extends(AlertUpdates, _super);

    function AlertUpdates() {
      this.deactivate_update = __bind(this.deactivate_update, this);
      this.render = __bind(this.render, this);
      this.compare_version = __bind(this.compare_version, this);
      this.render_updates = __bind(this.render_updates, this);
      this.check = __bind(this.check, this);
      this.hide = __bind(this.hide, this);
      this.close = __bind(this.close, this);
      this.initialize = __bind(this.initialize, this);
      return AlertUpdates.__super__.constructor.apply(this, arguments);
    }

    AlertUpdates.prototype.has_update_template = Handlebars.templates['has_update-template'];

    AlertUpdates.prototype.className = 'settings alert';

    AlertUpdates.prototype.events = {
      'click .no_update_btn': 'deactivate_update',
      'click .close': 'close'
    };

    AlertUpdates.prototype.initialize = function() {
      var check_updates, err;
      if (window.localStorage != null) {
        try {
          check_updates = JSON.parse(window.localStorage.check_updates);
          if (check_updates !== false) {
            return this.check();
          }
        } catch (_error) {
          err = _error;
          window.localStorage.check_updates = JSON.stringify(true);
          return this.check();
        }
      } else {
        return this.check();
      }
    };

    AlertUpdates.prototype.close = function(event) {
      event.preventDefault();
      if (this.next_version != null) {
        window.localStorage.ignore_version = JSON.stringify(this.next_version);
      }
      return this.hide();
    };

    AlertUpdates.prototype.hide = function() {
      return this.$el.slideUp('fast');
    };

    AlertUpdates.prototype.check = function() {
      return $.getJSON("http://update.rethinkdb.com/update_for/" + window.VERSION + "?callback=?", this.render_updates);
    };

    AlertUpdates.prototype.render_updates = function(data) {
      var err, ignored_version;
      if (data.status === 'need_update') {
        try {
          ignored_version = JSON.parse(window.localStorage.ignore_version);
        } catch (_error) {
          err = _error;
          ignored_version = null;
        }
        if ((!ignored_version) || this.compare_version(ignored_version, data.last_version) < 0) {
          this.next_version = data.last_version;
          this.$el.html(this.has_update_template({
            last_version: data.last_version,
            link_changelog: data.link_changelog
          }));
          return this.$el.slideDown('fast');
        }
      }
    };

    AlertUpdates.prototype.compare_version = function(v1, v2) {
      var index, v1_array, v1_array_str, v2_array, v2_array_str, value, _i, _j, _k, _len, _len1, _len2;
      v1_array_str = v1.split('.');
      v2_array_str = v2.split('.');
      v1_array = [];
      for (_i = 0, _len = v1_array_str.length; _i < _len; _i++) {
        value = v1_array_str[_i];
        v1_array.push(parseInt(value));
      }
      v2_array = [];
      for (_j = 0, _len1 = v2_array_str.length; _j < _len1; _j++) {
        value = v2_array_str[_j];
        v2_array.push(parseInt(value));
      }
      for (index = _k = 0, _len2 = v1_array.length; _k < _len2; index = ++_k) {
        value = v1_array[index];
        if (value < v2_array[index]) {
          return -1;
        } else if (value > v2_array[index]) {
          return 1;
        }
      }
      return 0;
    };

    AlertUpdates.prototype.render = function() {
      return this;
    };

    AlertUpdates.prototype.deactivate_update = function() {
      this.$el.slideUp('fast');
      if (window.localStorage != null) {
        return window.localStorage.check_updates = JSON.stringify(false);
      }
    };

    return AlertUpdates;

  })(Backbone.View);
});

Settings = (function(_super) {
  __extends(Settings, _super);

  function Settings() {
    this.render = __bind(this.render, this);
    this.change_settings = __bind(this.change_settings, this);
    this.initialize = __bind(this.initialize, this);
    this.close = __bind(this.close, this);
    return Settings.__super__.constructor.apply(this, arguments);
  }

  Settings.prototype.settings_template = Handlebars.templates['settings-template'];

  Settings.prototype.events = {
    'click .check_updates_btn': 'change_settings',
    'click .close': 'close'
  };

  Settings.prototype.close = function(event) {
    event.preventDefault();
    this.$el.parent().hide();
    return this.$el.remove();
  };

  Settings.prototype.initialize = function(args) {
    var _ref;
    this.alert_view = args.alert_view;
    if (((_ref = window.localStorage) != null ? _ref.check_updates : void 0) != null) {
      return this.check_updates = JSON.parse(window.localStorage.check_updates);
    } else {
      return this.check_updates = true;
    }
  };

  Settings.prototype.change_settings = function(event) {
    var update;
    update = this.$(event.target).data('update');
    if (update === 'on') {
      this.check_updates = true;
      if (window.localStorage != null) {
        window.localStorage.check_updates = JSON.stringify(true);
      }
      this.alert_view.check();
    } else if (update === 'off') {
      this.check_updates = false;
      this.alert_view.hide();
      if (window.localStorage != null) {
        window.localStorage.check_updates = JSON.stringify(false);
        window.localStorage.removeItem('ignore_version');
      }
    }
    return this.render();
  };

  Settings.prototype.render = function() {
    this.$el.html(this.settings_template({
      check_value: this.check_updates ? 'off' : 'on'
    }));
    this.delegateEvents();
    return this;
  };

  return Settings;

})(Backbone.View);

IsDisconnected = (function(_super) {
  __extends(IsDisconnected, _super);

  function IsDisconnected() {
    this.display_fail = __bind(this.display_fail, this);
    this.animate_loading = __bind(this.animate_loading, this);
    this.render = __bind(this.render, this);
    this.initialize = __bind(this.initialize, this);
    return IsDisconnected.__super__.constructor.apply(this, arguments);
  }

  IsDisconnected.prototype.el = 'body';

  IsDisconnected.prototype.className = 'is_disconnected_view';

  IsDisconnected.prototype.template = Handlebars.templates['is_disconnected-template'];

  IsDisconnected.prototype.message = Handlebars.templates['is_disconnected_message-template'];

  IsDisconnected.prototype.initialize = function() {
    this.render();
    return setInterval(function() {
      return driver.run_once(r.expr(1));
    }, 2000);
  };

  IsDisconnected.prototype.render = function() {
    this.$('#modal-dialog > .modal').css('z-index', '1');
    this.$('.modal-backdrop').remove();
    this.$el.append(this.template());
    this.$('.is_disconnected').modal({
      'show': true,
      'backdrop': 'static'
    });
    return this.animate_loading();
  };

  IsDisconnected.prototype.animate_loading = function() {
    if (this.$('.three_dots_connecting')) {
      if (this.$('.three_dots_connecting').html() === '...') {
        this.$('.three_dots_connecting').html('');
      } else {
        this.$('.three_dots_connecting').append('.');
      }
      return setTimeout(this.animate_loading, 300);
    }
  };

  IsDisconnected.prototype.display_fail = function() {
    return this.$('.animation_state').fadeOut('slow', (function(_this) {
      return function() {
        $('.reconnecting_state').html(_this.message);
        return $('.animation_state').fadeIn('slow');
      };
    })(this));
  };

  return IsDisconnected;

})(Backbone.View);
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('UIComponents', function() {
  this.AbstractModal = (function(_super) {
    __extends(AbstractModal, _super);

    function AbstractModal() {
      this.remove = __bind(this.remove, this);
      this.find_custom_button = __bind(this.find_custom_button, this);
      this.add_custom_button = __bind(this.add_custom_button, this);
      this.on_error = __bind(this.on_error, this);
      this.on_submit = __bind(this.on_submit, this);
      this.on_success = __bind(this.on_success, this);
      this.reset_buttons = __bind(this.reset_buttons, this);
      this.check_keypress_is_enter = __bind(this.check_keypress_is_enter, this);
      this.hide_modal = __bind(this.hide_modal, this);
      this.render = __bind(this.render, this);
      return AbstractModal.__super__.constructor.apply(this, arguments);
    }

    AbstractModal.prototype.template_outer = Handlebars.templates['abstract-modal-outer-template'];

    AbstractModal.prototype.error_template = Handlebars.templates['error_input-template'];

    AbstractModal.prototype.events = {
      'click .cancel': 'cancel_modal',
      'click .close_modal': 'cancel_modal',
      'click .btn-primary': 'abstract_submit',
      'keypress input': 'check_keypress_is_enter',
      'click .alert .close': 'close_error',
      'click .change-route': 'reroute'
    };

    AbstractModal.prototype.close_error = function(event) {
      event.preventDefault();
      return $(event.currentTarget).parent().slideUp('fast', function() {
        return $(this).remove();
      });
    };

    AbstractModal.prototype.initialize = function() {
      this.$container = $('#modal-dialog');
      return this.custom_buttons = [];
    };

    AbstractModal.prototype.render = function(template_data) {
      var btn, _i, _len, _ref, _results;
      if (template_data == null) {
        template_data = {};
      }
      template_data = _.extend(template_data, {
        modal_class: this["class"]
      });
      this.$container.html(this.template_outer(template_data)).addClass('visible');
      $('.modal-body', this.$container).html(this.template(template_data));
      this.$modal = $('.modal', this.$container).modal({
        'show': true,
        'backdrop': true,
        'keyboard': true
      }).on('hidden', (function(_this) {
        return function() {
          return _this.hide_modal();
        };
      })(this));
      this.setElement(this.$modal);
      this.delegateEvents();
      _ref = this.custom_buttons;
      _results = [];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        btn = _ref[_i];
        this.$('.custom_btn_placeholder').append("<button class='btn " + btn.class_str + "' data-loading-text='" + btn.data_loading_text + "'>" + btn.main_text + "</button>");
        this.$('.custom_btn_placeholder > .' + btn.class_str).click((function(_this) {
          return function(e) {
            return btn.on_click(e);
          };
        })(this));
        _results.push(this.$('.custom_btn_placeholder > .' + btn.class_str).button());
      }
      return _results;
    };

    AbstractModal.prototype.hide_modal = function() {
      this.$container.removeClass('visible');
      if (this.$modal != null) {
        return this.$modal.modal('hide');
      }
    };

    AbstractModal.prototype.cancel_modal = function(e) {
      this.hide_modal();
      return e.preventDefault();
    };

    AbstractModal.prototype.reroute = function(e) {
      return this.hide_modal();
    };

    AbstractModal.prototype.check_keypress_is_enter = function(event) {
      if (event.which === 13) {
        event.preventDefault();
        return this.abstract_submit(event);
      }
    };

    AbstractModal.prototype.abstract_submit = function(event) {
      event.preventDefault();
      return this.on_submit(event);
    };

    AbstractModal.prototype.reset_buttons = function() {
      this.$('.btn-primary').button('reset');
      return this.$('.cancel').button('reset');
    };

    AbstractModal.prototype.on_success = function(response) {
      this.reset_buttons();
      return this.remove();
    };

    AbstractModal.prototype.on_submit = function(event) {
      this.$('.btn-primary').button('loading');
      return this.$('.cancel').button('loading');
    };

    AbstractModal.prototype.on_error = function(error) {
      this.$('.alert_modal').html(this.error_template({
        ajax_fail: true,
        error: ((error != null) && error !== '' ? error : void 0)
      }));
      if (this.$('.alert_modal_content').css('display') === 'none') {
        this.$('.alert_modal_content').slideDown('fast');
      } else {
        this.$('.alert_modal_content').css('display', 'none');
        this.$('.alert_modal_content').fadeIn();
      }
      return this.reset_buttons();
    };

    AbstractModal.prototype.add_custom_button = function(main_text, class_str, data_loading_text, on_click) {
      return this.custom_buttons.push({
        main_text: main_text,
        class_str: class_str,
        data_loading_text: data_loading_text,
        on_click: on_click
      });
    };

    AbstractModal.prototype.find_custom_button = function(class_str) {
      return this.$('.custom_btn_placeholder > .' + class_str);
    };

    AbstractModal.prototype.remove = function() {
      this.hide_modal();
      return AbstractModal.__super__.remove.call(this);
    };

    return AbstractModal;

  })(Backbone.View);
  this.ConfirmationDialogModal = (function(_super) {
    __extends(ConfirmationDialogModal, _super);

    function ConfirmationDialogModal() {
      return ConfirmationDialogModal.__super__.constructor.apply(this, arguments);
    }

    ConfirmationDialogModal.prototype.template = Handlebars.templates['confirmation_dialog-template'];

    ConfirmationDialogModal.prototype["class"] = 'confirmation-modal';

    ConfirmationDialogModal.prototype.render = function(message, _url, _data, _on_success) {
      this.url = _url;
      this.data = _data;
      this.on_user_success = _on_success;
      ConfirmationDialogModal.__super__.render.call(this, {
        message: message,
        modal_title: 'Confirmation',
        btn_secondary_text: 'No',
        btn_primary_text: 'Yes'
      });
      return this.$('.btn-primary').focus();
    };

    ConfirmationDialogModal.prototype.on_submit = function() {
      ConfirmationDialogModal.__super__.on_submit.apply(this, arguments);
      return $.ajax({
        processData: false,
        url: this.url,
        type: 'POST',
        contentType: 'application/json',
        data: this.data,
        success: this.on_success,
        error: this.on_error
      });
    };

    ConfirmationDialogModal.prototype.on_success = function(response) {
      ConfirmationDialogModal.__super__.on_success.apply(this, arguments);
      return this.on_user_success(response);
    };

    return ConfirmationDialogModal;

  })(this.AbstractModal);
  return this.RenameItemModal = (function(_super) {
    __extends(RenameItemModal, _super);

    function RenameItemModal() {
      this.initialize = __bind(this.initialize, this);
      return RenameItemModal.__super__.constructor.apply(this, arguments);
    }

    RenameItemModal.prototype.template = Handlebars.templates['rename_item-modal-template'];

    RenameItemModal.prototype.alert_tmpl = Handlebars.templates['renamed_item-alert-template'];

    RenameItemModal.prototype.error_template = Handlebars.templates['error_input-template'];

    RenameItemModal.prototype["class"] = 'rename-item-modal';

    RenameItemModal.prototype.initialize = function(model, options) {
      RenameItemModal.__super__.initialize.call(this);
      if (this.model instanceof Table) {
        return this.item_type = 'table';
      } else if (this.model instanceof Database) {
        return this.item_type = 'database';
      } else if (this.model instanceof Server) {
        return this.item_type = 'server';
      }
    };

    RenameItemModal.prototype.render = function() {
      RenameItemModal.__super__.render.call(this, {
        type: this.item_type,
        old_name: this.model.get('name'),
        id: this.model.get('id'),
        modal_title: "Rename " + this.item_type,
        btn_primary_text: "Rename " + this.item_type
      });
      return this.$('#focus_new_name').focus();
    };

    RenameItemModal.prototype.on_submit = function() {
      var no_error, query;
      RenameItemModal.__super__.on_submit.apply(this, arguments);
      this.old_name = this.model.get('name');
      this.formdata = form_data_as_object($('form', this.$modal));
      no_error = true;
      if (this.formdata.new_name === '') {
        no_error = false;
        $('.alert_modal').html(this.error_template({
          empty_name: true
        }));
      } else if (/^[a-zA-Z0-9_]+$/.test(this.formdata.new_name) === false) {
        no_error = false;
        $('.alert_modal').html(this.error_template({
          special_char_detected: true,
          type: this.item_type
        }));
      }
      if (no_error === true) {
        if (this.model instanceof Table) {
          query = r.db(system_db).table('table_config').get(this.model.get('id')).update({
            name: this.formdata.new_name
          });
        } else if (this.model instanceof Database) {
          query = r.db(system_db).table('db_config').get(this.model.get('id')).update({
            name: this.formdata.new_name
          });
        } else if (this.model instanceof Server) {
          query = r.db(system_db).table('server_config').get(this.model.get('id')).update({
            name: this.formdata.new_name
          });
        }
        return driver.run_once(query, (function(_this) {
          return function(err, result) {
            if (err != null) {
              return _this.on_error(err);
            } else if ((result != null ? result.first_error : void 0) != null) {
              return _this.on_error(new Error(result.first_error));
            } else {
              if ((result != null ? result.replaced : void 0) === 1) {
                return _this.on_success();
              } else {
                return _this.on_error(new Error("The value returned for `replaced` was not 1."));
              }
            }
          };
        })(this));
      } else {
        $('.alert_modal_content').slideDown('fast');
        return this.reset_buttons();
      }
    };

    RenameItemModal.prototype.on_success = function(response) {
      var old_name, _ref, _ref1;
      RenameItemModal.__super__.on_success.apply(this, arguments);
      old_name = this.model.get('name');
      this.model.set({
        name: this.formdata.new_name
      });
      if (!((_ref = this.options) != null ? _ref.hide_alert : void 0)) {
        $('#user-alert-space').html(this.alert_tmpl({
          type: this.item_type,
          old_name: old_name,
          new_name: this.model.get('name')
        }));
      }
      if (typeof ((_ref1 = this.options) != null ? _ref1.on_success : void 0) === 'function') {
        return this.options.on_success(this.model);
      }
    };

    return RenameItemModal;

  })(this.AbstractModal);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('UIComponents', function() {
  return this.OperationProgressBar = (function(_super) {
    __extends(OperationProgressBar, _super);

    function OperationProgressBar() {
      this.set_none_state = __bind(this.set_none_state, this);
      this.skip_to_processing = __bind(this.skip_to_processing, this);
      this.get_stage = __bind(this.get_stage, this);
      this.render = __bind(this.render, this);
      this.reset_percent = __bind(this.reset_percent, this);
      return OperationProgressBar.__super__.constructor.apply(this, arguments);
    }

    OperationProgressBar.prototype.states = ['none', 'starting', 'processing', 'finished'];

    OperationProgressBar.prototype.initialize = function(template) {
      this.stage = 'none';
      if (template != null) {
        this.template = template;
      } else {
        this.template = Handlebars.templates['progressbar-template'];
      }
      this.timeout = null;
      return this.percent = 0;
    };

    OperationProgressBar.prototype.reset_percent = function() {
      return this.percent = 0;
    };

    OperationProgressBar.prototype.render = function(current_value, max_value, additional_info, cb) {
      var data, percent_complete;
      if (current_value !== max_value && (this.timeout != null)) {
        clearTimeout(this.timeout);
        this.timeout = null;
        this.stage = 'processing';
      }
      if (this.stage === 'none' && (additional_info.new_value != null)) {
        this.stage = 'starting';
        if (this.timeout != null) {
          clearTimeout(this.timeout);
          this.timeout = null;
        }
      }
      if (this.stage === 'starting' && (additional_info.got_response != null)) {
        this.stage = 'processing';
        if (this.timeout != null) {
          clearTimeout(this.timeout);
          this.timeout = null;
        }
      }
      if (this.stage === 'processing' && current_value === max_value) {
        this.stage = 'finished';
        if (this.timeout != null) {
          clearTimeout(this.timeout);
          this.timeout = null;
        }
      }
      data = _.extend(additional_info, {
        current_value: current_value,
        max_value: max_value
      });
      if (this.stage === 'starting') {
        data = _.extend(data, {
          operation_active: true,
          starting: true
        });
      }
      if (this.stage === 'processing') {
        if (current_value === max_value) {
          percent_complete = 0;
        } else {
          percent_complete = Math.floor(current_value / max_value * 100);
        }
        if (additional_info.check === true) {
          if (percent_complete < this.percent) {
            percent_complete = this.percent;
          } else {
            this.percent = percent_complete;
          }
        }
        data = _.extend(data, {
          operation_active: true,
          processing: true,
          percent_complete: percent_complete
        });
      }
      if (this.stage === 'finished') {
        data = _.extend(data, {
          operation_active: true,
          finished: true,
          percent_complete: 100
        });
        if (this.timeout == null) {
          this.timeout = setTimeout((function(_this) {
            return function() {
              _this.stage = 'none';
              _this.render(current_value, max_value, {});
              _this.timeout = null;
              if (cb != null) {
                return cb();
              }
            };
          })(this), 2000);
        }
      }
      if (data.current_value !== data.current_value) {
        data.current_value = "Unknown";
      }
      if (data.max_value !== data.max_value) {
        data.max_value = "Unknown";
      }
      this.$el.html(this.template(data));
      return this;
    };

    OperationProgressBar.prototype.get_stage = function() {
      return this.stage;
    };

    OperationProgressBar.prototype.skip_to_processing = function() {
      this.stage = 'processing';
      if (this.timeout != null) {
        clearTimeout(this.timeout);
        return this.timeout = null;
      }
    };

    OperationProgressBar.prototype.set_none_state = function() {
      return this.stage = 'none';
    };

    return OperationProgressBar;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('TablesView', function() {
  this.DatabasesContainer = (function(_super) {
    __extends(DatabasesContainer, _super);

    function DatabasesContainer() {
      this.remove = __bind(this.remove, this);
      this.fetch_data = __bind(this.fetch_data, this);
      this.fetch_data_once = __bind(this.fetch_data_once, this);
      this.render_message = __bind(this.render_message, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      this.update_delete_tables_button = __bind(this.update_delete_tables_button, this);
      this.delete_tables = __bind(this.delete_tables, this);
      this.add_database = __bind(this.add_database, this);
      this.query_callback = __bind(this.query_callback, this);
      return DatabasesContainer.__super__.constructor.apply(this, arguments);
    }

    DatabasesContainer.prototype.id = 'databases_container';

    DatabasesContainer.prototype.template = {
      main: Handlebars.templates['databases_container-template'],
      error: Handlebars.templates['error-query-template'],
      alert_message: Handlebars.templates['alert_message-template']
    };

    DatabasesContainer.prototype.events = {
      'click .add_database': 'add_database',
      'click .remove-tables': 'delete_tables',
      'click .checkbox-container': 'update_delete_tables_button',
      'click .close': 'remove_parent_alert'
    };

    DatabasesContainer.prototype.query_callback = function(error, result) {
      var _ref;
      this.loading = false;
      if (error != null) {
        if (((_ref = this.error) != null ? _ref.msg : void 0) !== error.msg) {
          this.error = error;
          return this.$el.html(this.template.error({
            url: '#tables',
            error: error.message
          }));
        }
      } else {
        return this.databases.set(_.map(_.sortBy(result, function(db) {
          return db.name;
        }), function(database) {
          return new Database(database);
        }), {
          merge: true
        });
      }
    };

    DatabasesContainer.prototype.add_database = function() {
      return this.add_database_dialog.render();
    };

    DatabasesContainer.prototype.delete_tables = function(event) {
      var selected_tables;
      if (!$(event.currentTarget).is(':disabled')) {
        selected_tables = [];
        this.$('.checkbox-table:checked').each(function() {
          return selected_tables.push({
            table: JSON.parse($(this).data('table')),
            database: JSON.parse($(this).data('database'))
          });
        });
        return this.remove_tables_dialog.render(selected_tables);
      }
    };

    DatabasesContainer.prototype.remove_parent_alert = function(event) {
      var element;
      event.preventDefault();
      element = $(event.target).parent();
      return element.slideUp('fast', function() {
        return element.remove();
      });
    };

    DatabasesContainer.prototype.update_delete_tables_button = function() {
      if (this.$('.checkbox-table:checked').length > 0) {
        return this.$('.remove-tables').prop('disabled', false);
      } else {
        return this.$('.remove-tables').prop('disabled', true);
      }
    };

    DatabasesContainer.prototype.initialize = function() {
      if (window.view_data_backup.tables_view_databases == null) {
        window.view_data_backup.tables_view_databases = new Databases;
        this.loading = true;
      } else {
        this.loading = false;
      }
      this.databases = window.view_data_backup.tables_view_databases;
      this.databases_list = new TablesView.DatabasesListView({
        collection: this.databases,
        container: this
      });
      this.query = r.db(system_db).table('db_config').filter(function(db) {
        return db("name").ne(system_db);
      }).map(function(db) {
        return {
          name: db("name"),
          id: db("id"),
          tables: r.db(system_db).table('table_status').orderBy(function(table) {
            return table("name");
          }).filter({
            db: db("name")
          }).merge(function(table) {
            return {
              shards: table("shards").count(),
              replicas: table("shards").map(function(shard) {
                return shard('replicas').count();
              }).sum(),
              replicas_ready: table('shards').map(function(shard) {
                return shard('replicas').filter(function(replica) {
                  return replica('state').eq('ready');
                }).count();
              }).sum(),
              status: table('status'),
              id: table('id')
            };
          })
        };
      });
      this.fetch_data();
      this.add_database_dialog = new Modals.AddDatabaseModal(this.databases);
      return this.remove_tables_dialog = new Modals.RemoveTableModal({
        collection: this.databases
      });
    };

    DatabasesContainer.prototype.render = function() {
      this.$el.html(this.template.main({}));
      this.$('.databases_list').html(this.databases_list.render().$el);
      return this;
    };

    DatabasesContainer.prototype.render_message = function(message) {
      return this.$('#user-alert-space').append(this.template.alert_message({
        message: message
      }));
    };

    DatabasesContainer.prototype.fetch_data_once = function() {
      return this.timer = driver.run_once(this.query, this.query_callback);
    };

    DatabasesContainer.prototype.fetch_data = function() {
      return this.timer = driver.run(this.query, 5000, this.query_callback);
    };

    DatabasesContainer.prototype.remove = function() {
      driver.stop_timer(this.timer);
      this.add_database_dialog.remove();
      this.remove_tables_dialog.remove();
      return DatabasesContainer.__super__.remove.call(this);
    };

    return DatabasesContainer;

  })(Backbone.View);
  this.DatabasesListView = (function(_super) {
    __extends(DatabasesListView, _super);

    function DatabasesListView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return DatabasesListView.__super__.constructor.apply(this, arguments);
    }

    DatabasesListView.prototype.className = 'database_list';

    DatabasesListView.prototype.template = {
      no_databases: Handlebars.templates['no_databases-template'],
      loading_databases: Handlebars.templates['loading_databases-template']
    };

    DatabasesListView.prototype.initialize = function(data) {
      this.container = data.container;
      this.databases_view = [];
      this.collection.each((function(_this) {
        return function(database) {
          var view;
          view = new TablesView.DatabaseView({
            model: database
          });
          _this.databases_view.push(view);
          return _this.$el.append(view.render().$el);
        };
      })(this));
      if (this.container.loading) {
        this.$el.html(this.template.loading_databases());
      } else if (this.collection.length === 0) {
        this.$el.html(this.template.no_databases());
      }
      this.listenTo(this.collection, 'add', (function(_this) {
        return function(database) {
          var added, new_view, position, view, _i, _len, _ref;
          new_view = new TablesView.DatabaseView({
            model: database
          });
          if (_this.databases_view.length === 0) {
            _this.databases_view.push(new_view);
            _this.$el.html(new_view.render().$el);
          } else {
            added = false;
            _ref = _this.databases_view;
            for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
              view = _ref[position];
              if (view.model.get('name') > database.get('name')) {
                added = true;
                _this.databases_view.splice(position, 0, new_view);
                if (position === 0) {
                  _this.$el.prepend(new_view.render().$el);
                } else {
                  _this.$('.database_container').eq(position - 1).after(new_view.render().$el);
                }
                break;
              }
            }
            if (added === false) {
              _this.databases_view.push(new_view);
              _this.$el.append(new_view.render().$el);
            }
          }
          if (_this.databases_view.length === 1) {
            return _this.$('.no-databases').remove();
          }
        };
      })(this));
      return this.listenTo(this.collection, 'remove', (function(_this) {
        return function(database) {
          var position, view, _i, _len, _ref;
          _ref = _this.databases_view;
          for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
            view = _ref[position];
            if (view.model === database) {
              database.destroy();
              view.remove();
              _this.databases_view.splice(position, 1);
              break;
            }
          }
          if (_this.collection.length === 0) {
            return _this.$el.html(_this.template.no_databases());
          }
        };
      })(this));
    };

    DatabasesListView.prototype.render = function() {
      return this;
    };

    DatabasesListView.prototype.remove = function() {
      var view, _i, _len, _ref;
      this.stopListening();
      _ref = this.databases_view;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        view = _ref[_i];
        view.remove();
      }
      return DatabasesListView.__super__.remove.call(this);
    };

    return DatabasesListView;

  })(Backbone.View);
  this.DatabaseView = (function(_super) {
    __extends(DatabaseView, _super);

    function DatabaseView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.update_collection = __bind(this.update_collection, this);
      this.initialize = __bind(this.initialize, this);
      this.add_table = __bind(this.add_table, this);
      this.delete_database = __bind(this.delete_database, this);
      return DatabaseView.__super__.constructor.apply(this, arguments);
    }

    DatabaseView.prototype.className = 'database_container section';

    DatabaseView.prototype.template = {
      main: Handlebars.templates['database-template'],
      empty: Handlebars.templates['empty_list-template']
    };

    DatabaseView.prototype.events = {
      'click button.add-table': 'add_table',
      'click button.delete-database': 'delete_database'
    };

    DatabaseView.prototype.delete_database = function() {
      if (this.delete_database_dialog != null) {
        this.delete_database_dialog.remove();
      }
      this.delete_database_dialog = new Modals.DeleteDatabaseModal;
      return this.delete_database_dialog.render(this.model);
    };

    DatabaseView.prototype.add_table = function() {
      if (this.add_table_dialog != null) {
        this.add_table_dialog.remove();
      }
      this.add_table_dialog = new Modals.AddTableModal({
        db_id: this.model.get('id'),
        db_name: this.model.get('name'),
        tables: this.model.get('tables')
      });
      return this.add_table_dialog.render();
    };

    DatabaseView.prototype.initialize = function() {
      this.$el.html(this.template.main(this.model.toJSON()));
      this.tables_views = [];
      this.collection = new Tables();
      this.update_collection();
      this.model.on('change', this.update_collection);
      this.collection.each((function(_this) {
        return function(table) {
          var view;
          view = new TablesView.TableView({
            model: table
          });
          _this.tables_views.push(view);
          return _this.$('.tables_container').append(view.render().$el);
        };
      })(this));
      if (this.collection.length === 0) {
        this.$('.tables_container').html(this.template.empty({
          element: "table",
          container: "database"
        }));
      }
      this.listenTo(this.collection, 'add', (function(_this) {
        return function(table) {
          var added, new_view, position, view, _i, _len, _ref;
          new_view = new TablesView.TableView({
            model: table
          });
          if (_this.tables_views.length === 0) {
            _this.tables_views.push(new_view);
            _this.$('.tables_container').html(new_view.render().$el);
          } else {
            added = false;
            _ref = _this.tables_views;
            for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
              view = _ref[position];
              if (view.model.get('name') > table.get('name')) {
                added = true;
                _this.tables_views.splice(position, 0, new_view);
                if (position === 0) {
                  _this.$('.tables_container').prepend(new_view.render().$el);
                } else {
                  _this.$('.table_container').eq(position - 1).after(new_view.render().$el);
                }
                break;
              }
            }
            if (added === false) {
              _this.tables_views.push(new_view);
              _this.$('.tables_container').append(new_view.render().$el);
            }
          }
          if (_this.tables_views.length === 1) {
            return _this.$('.no_element').remove();
          }
        };
      })(this));
      return this.listenTo(this.collection, 'remove', (function(_this) {
        return function(table) {
          var position, view, _i, _len, _ref;
          _ref = _this.tables_views;
          for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
            view = _ref[position];
            if (view.model === table) {
              table.destroy();
              view.remove();
              _this.tables_views.splice(position, 1);
              break;
            }
          }
          if (_this.collection.length === 0) {
            return _this.$('.tables_container').html(_this.template.empty({
              element: "table",
              container: "database"
            }));
          }
        };
      })(this));
    };

    DatabaseView.prototype.update_collection = function() {
      return this.collection.set(_.map(this.model.get("tables"), function(table) {
        return new Table(table);
      }), {
        merge: true
      });
    };

    DatabaseView.prototype.render = function() {
      return this;
    };

    DatabaseView.prototype.remove = function() {
      var view, _i, _len, _ref;
      this.stopListening();
      _ref = this.tables_views;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        view = _ref[_i];
        view.remove();
      }
      if (this.delete_database_dialog != null) {
        this.delete_database_dialog.remove();
      }
      if (this.add_table_dialog != null) {
        this.add_table_dialog.remove();
      }
      return DatabaseView.__super__.remove.call(this);
    };

    return DatabaseView;

  })(Backbone.View);
  return this.TableView = (function(_super) {
    __extends(TableView, _super);

    function TableView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return TableView.__super__.constructor.apply(this, arguments);
    }

    TableView.prototype.className = 'table_container';

    TableView.prototype.template = Handlebars.templates['table-template'];

    TableView.prototype.initialize = function() {
      return this.listenTo(this.model, 'change', this.render);
    };

    TableView.prototype.render = function() {
      this.$el.html(this.template({
        id: this.model.get('id'),
        db_json: JSON.stringify(this.model.get('db')),
        name_json: JSON.stringify(this.model.get('name')),
        db: this.model.get('db'),
        name: this.model.get('name'),
        shards: this.model.get('shards'),
        replicas: this.model.get('replicas'),
        replicas_ready: this.model.get('replicas_ready'),
        status: this.model.get('status')
      }));
      return this;
    };

    TableView.prototype.remove = function() {
      this.stopListening();
      return TableView.__super__.remove.call(this);
    };

    return TableView;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('TableView', function() {
  var MAX_SHARD_COUNT;
  MAX_SHARD_COUNT = 32;
  return this.ShardDistribution = (function(_super) {
    __extends(ShardDistribution, _super);

    function ShardDistribution() {
      this.render_data_distribution = __bind(this.render_data_distribution, this);
      this.display_outdated = __bind(this.display_outdated, this);
      this.render = __bind(this.render, this);
      this.set_distribution = __bind(this.set_distribution, this);
      this.initialize = __bind(this.initialize, this);
      return ShardDistribution.__super__.constructor.apply(this, arguments);
    }

    ShardDistribution.prototype.className = 'shards_container';

    ShardDistribution.prototype.template = {
      main: Handlebars.templates['shard_distribution_container']
    };

    ShardDistribution.prototype.initialize = function(data) {
      if (this.collection != null) {
        this.listenTo(this.collection, 'update', this.render_data_distribution);
      }
      this.listenTo(this.model, 'change:info_unavailable', this.set_warnings);
      return this.render_data_distribution;
    };

    ShardDistribution.prototype.set_warnings = function() {
      if (this.model.get('info_unavailable')) {
        return this.$('.unavailable-error').show();
      } else {
        return this.$('.unavailable-error').hide();
      }
    };

    ShardDistribution.prototype.set_distribution = function(shards) {
      this.collection = shards;
      this.listenTo(this.collection, 'update', this.render_data_distribution);
      return this.render_data_distribution();
    };

    ShardDistribution.prototype.render = function() {
      this.$el.html(this.template.main(this.model.toJSON()));
      this.init_chart = false;
      setTimeout((function(_this) {
        return function() {
          return _this.render_data_distribution();
        };
      })(this), 0);
      this.set_warnings();
      return this;
    };

    ShardDistribution.prototype.prettify_num = function(num) {
      if (num > 1000000000) {
        num = ("" + num).slice(0, -9) + "M";
      }
      if (num > 1000000 && num < 1000000000) {
        return num = ("" + num).slice(0, -6) + "m";
      } else if (num > 1000 && num < 1000000) {
        return num = ("" + num).slice(0, -3) + "k";
      } else {
        return num;
      }
    };

    ShardDistribution.prototype.display_outdated = function() {
      if (this.$('.outdated_distribution').css('display') === 'none') {
        return this.$('.outdated_distribution').slideDown('fast');
      }
    };

    ShardDistribution.prototype.render_data_distribution = function() {
      var axe_legend, bar_width, bars, bars_container, extra_container, extra_data, legends, lines, margin_bar, margin_height, margin_width, margin_width_inner, max_keys, min_keys, min_margin_width_inner, rules, svg, svg_height, svg_width, ticks, width_of_all_bars, y, _ref, _ref1;
      $('.tooltip').remove();
      this.$('.loading').slideUp('fast');
      this.$('.outdated_distribution').slideUp('fast');
      this.$('.shard-diagram').show();
      max_keys = (_ref = d3.max(this.collection.models, function(shard) {
        return shard.get('num_keys');
      })) != null ? _ref : 100;
      min_keys = (_ref1 = d3.min(this.collection.models, function(shard) {
        return shard.get('num_keys');
      })) != null ? _ref1 : 0;
      svg_width = 328;
      svg_height = 270;
      margin_width = 20;
      margin_height = 20;
      min_margin_width_inner = 20;
      if (this.collection.length === 1) {
        bar_width = 100;
        margin_bar = 20;
      } else if (this.collection.length === 2) {
        bar_width = 80;
        margin_bar = 20;
      } else {
        bar_width = Math.floor(0.8 * (328 - margin_width * 2 - min_margin_width_inner * 2) / this.collection.length);
        margin_bar = Math.floor(0.2 * (328 - margin_width * 2 - min_margin_width_inner * 2) / this.collection.length);
      }
      width_of_all_bars = bar_width * this.collection.length + margin_bar * (this.collection.length - 1);
      margin_width_inner = Math.floor((svg_width - margin_width * 2 - width_of_all_bars) / 2);
      y = d3.scale.linear().domain([0, max_keys]).range([1, svg_height - margin_height * 2.5]);
      svg = d3.select('.shard-diagram').attr('width', svg_width).attr('height', svg_height);
      bars_container = svg.select('.bars_container');
      bars = bars_container.selectAll('rect').data(this.collection.map(function(shard, index) {
        return {
          index: index,
          num_keys: shard.get('num_keys')
        };
      }));
      bars.enter().append('rect').attr('class', 'rect').attr('x', function(d, i) {
        return margin_width + margin_width_inner + bar_width * i + margin_bar * i;
      }).attr('y', function(d) {
        return svg_height - y(d.num_keys) - margin_height - 1;
      }).attr('width', bar_width).attr('height', function(d) {
        return y(d.num_keys);
      }).attr('title', function(d) {
        return "Shard " + (d.index + 1) + "<br />~" + d.num_keys + " keys";
      });
      bars.transition().duration(600).attr('x', function(d, i) {
        return margin_width + margin_width_inner + bar_width * i + margin_bar * i;
      }).attr('y', function(d) {
        return svg_height - y(d.num_keys) - margin_height - 1;
      }).attr('width', bar_width).attr('height', function(d) {
        return y(d.num_keys);
      });
      bars.exit().remove();
      extra_data = [];
      extra_data.push({
        x1: margin_width,
        x2: margin_width,
        y1: margin_height,
        y2: svg_height - margin_height
      });
      extra_data.push({
        x1: margin_width,
        x2: svg_width - margin_width,
        y1: svg_height - margin_height,
        y2: svg_height - margin_height
      });
      extra_container = svg.select('.extra_container');
      lines = extra_container.selectAll('.line').data(extra_data);
      lines.enter().append('line').attr('class', 'line').attr('x1', function(d) {
        return d.x1;
      }).attr('x2', function(d) {
        return d.x2;
      }).attr('y1', function(d) {
        return d.y1;
      }).attr('y2', function(d) {
        return d.y2;
      });
      lines.exit().remove();
      axe_legend = [];
      axe_legend.push({
        x: margin_width,
        y: Math.floor(margin_height / 2 + 2),
        string: 'Docs',
        anchor: 'middle'
      });
      axe_legend.push({
        x: Math.floor(svg_width / 2),
        y: svg_height,
        string: 'Shards',
        anchor: 'middle'
      });
      legends = extra_container.selectAll('.legend').data(axe_legend);
      legends.enter().append('text').attr('class', 'legend').attr('x', function(d) {
        return d.x;
      }).attr('y', function(d) {
        return d.y;
      }).attr('text-anchor', function(d) {
        return d.anchor;
      }).text(function(d) {
        return d.string;
      });
      legends.exit().remove();
      ticks = extra_container.selectAll('.cache').data(y.ticks(5));
      ticks.enter().append('rect').attr('class', 'cache').attr('width', function(d, i) {
        if (i === 0) {
          return 0;
        }
        return 4;
      }).attr('height', 18).attr('x', margin_width - 2).attr('y', function(d) {
        return svg_height - margin_height - y(d) - 14;
      }).attr('fill', '#fff');
      ticks.transition().duration(600).attr('y', function(d) {
        return svg_height - margin_height - y(d) - 14;
      });
      ticks.exit().remove();
      rules = extra_container.selectAll('.rule').data(y.ticks(5));
      rules.enter().append('text').attr('class', 'rule').attr('x', margin_width).attr('y', function(d) {
        return svg_height - margin_height - y(d);
      }).attr('text-anchor', "middle").text((function(_this) {
        return function(d, i) {
          if (i !== 0) {
            return _this.prettify_num(d);
          } else {
            return '';
          }
        };
      })(this));
      rules.transition().duration(600).attr('y', function(d) {
        return svg_height - margin_height - y(d);
      });
      rules.exit().remove();
      return this.$('rect').tooltip({
        trigger: 'hover'
      });
    };

    return ShardDistribution;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('TableView', function() {
  this.ShardAssignmentsView = (function(_super) {
    __extends(ShardAssignmentsView, _super);

    function ShardAssignmentsView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.set_assignments = __bind(this.set_assignments, this);
      this.initialize = __bind(this.initialize, this);
      return ShardAssignmentsView.__super__.constructor.apply(this, arguments);
    }

    ShardAssignmentsView.prototype.template = Handlebars.templates['shard_assignments-template'];

    ShardAssignmentsView.prototype.initialize = function(data) {
      this.listenTo(this.model, 'change:info_unavailable', this.set_warnings);
      if (data.collection != null) {
        this.collection = data.collection;
      }
      return this.assignments_view = [];
    };

    ShardAssignmentsView.prototype.set_warnings = function() {
      if (this.model.get('info_unavailable')) {
        return this.$('.unavailable-error').show();
      } else {
        return this.$('.unavailable-error').hide();
      }
    };

    ShardAssignmentsView.prototype.set_assignments = function(assignments) {
      this.collection = assignments;
      return this.render();
    };

    ShardAssignmentsView.prototype.render = function() {
      this.$el.html(this.template(this.model.toJSON()));
      if (this.collection != null) {
        this.collection.each((function(_this) {
          return function(assignment) {
            var view;
            view = new TableView.ShardAssignmentView({
              model: assignment,
              container: _this
            });
            _this.assignments_view.push(view);
            return _this.$('.assignments_list').append(view.render().$el);
          };
        })(this));
        this.listenTo(this.collection, 'add', (function(_this) {
          return function(assignment) {
            var added, new_view, position, view, _i, _len, _ref;
            new_view = new TableView.ShardAssignmentView({
              model: assignment,
              container: _this
            });
            if (_this.assignments_view.length === 0) {
              _this.assignments_view.push(new_view);
              return _this.$('.assignments_list').html(new_view.render().$el);
            } else {
              added = false;
              _ref = _this.assignments_view;
              for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
                view = _ref[position];
                if (ShardAssignments.prototype.comparator(view.model, assignment) > 0) {
                  added = true;
                  _this.assignments_view.splice(position, 0, new_view);
                  if (position === 0) {
                    _this.$('.assignments_list').prepend(new_view.render().$el);
                  } else {
                    _this.$('.assignment_container').eq(position - 1).after(new_view.render().$el);
                  }
                  break;
                }
              }
              if (added === false) {
                _this.assignments_view.push(new_view);
                return _this.$('.assignments_list').append(new_view.render().$el);
              }
            }
          };
        })(this));
        this.listenTo(this.collection, 'remove', (function(_this) {
          return function(assignment) {
            var position, view, _i, _len, _ref, _results;
            _ref = _this.assignments_view;
            _results = [];
            for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
              view = _ref[position];
              if (view.model === assignment) {
                assignment.destroy();
                view.remove();
                _this.assignments_view.splice(position, 1);
                break;
              } else {
                _results.push(void 0);
              }
            }
            return _results;
          };
        })(this));
      }
      this.set_warnings();
      return this;
    };

    ShardAssignmentsView.prototype.remove = function() {
      var view, _i, _len, _ref;
      this.stopListening();
      _ref = this.assignments_view;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        view = _ref[_i];
        view.model.destroy();
        view.remove();
      }
      return ShardAssignmentsView.__super__.remove.call(this);
    };

    return ShardAssignmentsView;

  })(Backbone.View);
  return this.ShardAssignmentView = (function(_super) {
    __extends(ShardAssignmentView, _super);

    function ShardAssignmentView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ShardAssignmentView.__super__.constructor.apply(this, arguments);
    }

    ShardAssignmentView.prototype.template = Handlebars.templates['shard_assignment-template'];

    ShardAssignmentView.prototype.className = 'assignment_container';

    ShardAssignmentView.prototype.initialize = function() {
      return this.listenTo(this.model, 'change', this.render);
    };

    ShardAssignmentView.prototype.render = function() {
      this.$el.html(this.template(this.model.toJSON()));
      return this;
    };

    ShardAssignmentView.prototype.remove = function() {
      this.stopListening();
      return ShardAssignmentView.__super__.remove.call(this);
    };

    return ShardAssignmentView;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; },
  __indexOf = [].indexOf || function(item) { for (var i = 0, l = this.length; i < l; i++) { if (i in this && this[i] === item) return i; } return -1; };

module('TableView', function() {
  this.TableContainer = (function(_super) {
    __extends(TableContainer, _super);

    function TableContainer() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.fetch_data = __bind(this.fetch_data, this);
      this.handle_guaranteed_response = __bind(this.handle_guaranteed_response, this);
      this.handle_failable_misc_response = __bind(this.handle_failable_misc_response, this);
      this.handle_failable_index_response = __bind(this.handle_failable_index_response, this);
      this.failable_misc_query = __bind(this.failable_misc_query, this);
      this.failable_index_query = __bind(this.failable_index_query, this);
      this.guaranteed_query = __bind(this.guaranteed_query, this);
      this.clear_cached_error_messages = __bind(this.clear_cached_error_messages, this);
      this.add_error_message = __bind(this.add_error_message, this);
      this.maybe_refetch_data = __bind(this.maybe_refetch_data, this);
      this.initialize = __bind(this.initialize, this);
      return TableContainer.__super__.constructor.apply(this, arguments);
    }

    TableContainer.prototype.template = {
      not_found: Handlebars.templates['element_view-not_found-template'],
      error: Handlebars.templates['error-query-template']
    };

    TableContainer.prototype.className = 'table-view';

    TableContainer.prototype.initialize = function(id) {
      this.id = id;
      this.table_found = true;
      this.indexes = null;
      this.distribution = new Distribution;
      this.guaranteed_timer = null;
      this.failable_index_timer = null;
      this.failable_misc_timer = null;
      this.current_errors = {
        index: [],
        misc: []
      };
      this.model = new Table({
        id: id,
        info_unavailable: false,
        index_unavailable: false
      });
      this.listenTo(this.model, 'change:index_unavailable', ((function(_this) {
        return function() {
          return _this.maybe_refetch_data();
        };
      })(this)));
      this.table_view = new TableView.TableMainView({
        model: this.model,
        indexes: this.indexes,
        distribution: this.distribution,
        shards_assignments: this.shards_assignments
      });
      return this.fetch_data();
    };

    TableContainer.prototype.maybe_refetch_data = function() {
      if (this.model.get('info_unavailable') && !this.model.get('index_unavailable')) {
        return this.fetch_once();
      }
    };

    TableContainer.prototype.fetch_once = _.debounce((function() {
      driver.run_once(TableContainer.failable_misc_query, TableContainer.failable_misc_handler);
      return driver.run_once(TableContainer.guaranteed_query, TableContainer.guaranted_handler);
    }), 2000, true);

    TableContainer.prototype.add_error_message = function(type, error) {
      if (__indexOf.call(this.current_errors[type], error) < 0) {
        console.log("" + type + " error: " + error);
        return this.current_errors[type].push(error);
      }
    };

    TableContainer.prototype.clear_cached_error_messages = function(type) {
      if (this.current_errors[type].length > 0) {
        return this.current_errors[type] = [];
      }
    };

    TableContainer.prototype.guaranteed_query = function() {
      return r["do"](r.db(system_db).table('server_config').coerceTo('array'), r.db(system_db).table('table_status').get(this.id), r.db(system_db).table('table_config').get(this.id), function(server_config, table_status, table_config) {
        return r.branch(table_status.eq(null), null, table_status.merge({
          max_shards: 32,
          num_shards: table_config("shards").count(),
          num_servers: server_config.count(),
          num_default_servers: server_config.filter(function(server) {
            return server('tags').contains('default');
          }).count(),
          num_available_shards: table_status("shards").count(function(row) {
            return row('primary_replica').ne(null);
          }),
          num_replicas: table_config("shards").concatMap(function(shard) {
            return shard('replicas');
          }).count(),
          num_available_replicas: table_status("shards").concatMap(function(shard) {
            return shard('replicas').filter({
              state: "ready"
            });
          }).count(),
          num_replicas_per_shard: table_config("shards").map(function(shard) {
            return shard('replicas').count();
          }).max(),
          status: table_status('status'),
          id: table_status("id")
        }).without('shards'));
      });
    };

    TableContainer.prototype.failable_index_query = function() {
      return r["do"](r.db(system_db).table('table_config').get(this.id), function(table_config) {
        return r.db(table_config("db")).table(table_config("name"), {
          useOutdated: true
        }).indexStatus().pluck('index', 'ready', 'blocks_processed', 'blocks_total').merge(function(index) {
          return {
            id: index("index"),
            db: table_config("db"),
            table: table_config("name")
          };
        });
      });
    };

    TableContainer.prototype.failable_misc_query = function() {
      return r["do"](r.db(system_db).table('table_status').get(this.id), r.db(system_db).table('table_config').get(this.id), r.db(system_db).table('server_config').map(function(x) {
        return [x('name'), x('id')];
      }).coerceTo('ARRAY').coerceTo('OBJECT'), function(table_status, table_config, server_name_to_id) {
        return table_status.merge({
          distribution: r.db(table_status('db')).table(table_status('name'), {
            useOutdated: true
          }).info()('doc_count_estimates').map(r.range(), function(num_keys, position) {
            return {
              num_keys: num_keys,
              id: position
            };
          }).coerceTo('array'),
          total_keys: r.db(table_status('db')).table(table_status('name'), {
            useOutdated: true
          }).info()('doc_count_estimates').sum(),
          shards_assignments: table_config("shards").map(r.range(), function(shard, position) {
            return {
              id: position.add(1),
              num_keys: r.db(table_status('db')).table(table_status('name'), {
                useOutdated: true
              }).info()('doc_count_estimates')(position),
              primary: {
                id: server_name_to_id(shard("primary_replica")),
                name: shard("primary_replica")
              },
              replicas: shard("replicas").filter(function(replica) {
                return replica.ne(shard("primary_replica"));
              }).map(function(name) {
                return {
                  id: server_name_to_id(name),
                  name: name
                };
              })
            };
          }).coerceTo('array')
        });
      });
    };

    TableContainer.prototype.handle_failable_index_response = function(error, result) {
      var _ref;
      if (error != null) {
        this.add_error_message('index', error.msg);
        this.model.set('index_unavailable', true);
        return;
      }
      this.model.set('index_unavailable', false);
      this.clear_cached_error_messages('index');
      if (this.indexes != null) {
        this.indexes.set(_.map(result, function(index) {
          return new Index(index);
        }));
      } else {
        this.indexes = new Indexes(_.map(result, function(index) {
          return new Index(index);
        }));
      }
      if ((_ref = this.table_view) != null) {
        _ref.set_indexes(this.indexes);
      }
      this.model.set({
        indexes: result
      });
      if (this.table_view == null) {
        this.table_view = new TableView.TableMainView({
          model: this.model,
          indexes: this.indexes,
          distribution: this.distribution,
          shards_assignments: this.shards_assignments
        });
        return this.render();
      }
    };

    TableContainer.prototype.handle_failable_misc_response = function(error, result) {
      var position, replica, shard, shards_assignments, _i, _j, _len, _len1, _ref, _ref1, _ref2, _ref3;
      if (error != null) {
        this.add_error_message('misc', error.msg);
        this.model.set('info_unavailable', true);
        return;
      }
      this.model.set('info_unavailable', false);
      this.clear_cached_error_messages('misc');
      if (this.distribution != null) {
        this.distribution.set(_.map(result.distribution, function(shard) {
          return new Shard(shard);
        }));
        this.distribution.trigger('update');
      } else {
        this.distribution = new Distribution(_.map(result.distribution, function(shard) {
          return new Shard(shard);
        }));
        if ((_ref = this.table_view) != null) {
          _ref.set_distribution(this.distribution);
        }
      }
      shards_assignments = [];
      _ref1 = result.shards_assignments;
      for (_i = 0, _len = _ref1.length; _i < _len; _i++) {
        shard = _ref1[_i];
        shards_assignments.push({
          id: "start_shard_" + shard.id,
          shard_id: shard.id,
          num_keys: shard.num_keys,
          start_shard: true
        });
        shards_assignments.push({
          id: "shard_primary_" + shard.id,
          primary: true,
          shard_id: shard.id,
          data: shard.primary
        });
        _ref2 = shard.replicas;
        for (position = _j = 0, _len1 = _ref2.length; _j < _len1; position = ++_j) {
          replica = _ref2[position];
          shards_assignments.push({
            id: "shard_replica_" + shard.id + "_" + position,
            replica: true,
            replica_position: position,
            shard_id: shard.id,
            data: replica
          });
        }
        shards_assignments.push({
          id: "end_shard_" + shard.id,
          shard_id: shard.id,
          end_shard: true
        });
      }
      if (this.shards_assignments != null) {
        this.shards_assignments.set(_.map(shards_assignments, function(shard) {
          return new ShardAssignment(shard);
        }));
      } else {
        this.shards_assignments = new ShardAssignments(_.map(shards_assignments, function(shard) {
          return new ShardAssignment(shard);
        }));
        if ((_ref3 = this.table_view) != null) {
          _ref3.set_assignments(this.shards_assignments);
        }
      }
      this.model.set(result);
      if (this.table_view == null) {
        this.table_view = new TableView.TableMainView({
          model: this.model,
          indexes: this.indexes,
          distribution: this.distribution,
          shards_assignments: this.shards_assignments
        });
        return this.render();
      }
    };

    TableContainer.prototype.handle_guaranteed_response = function(error, result) {
      var rerender;
      if (error != null) {
        this.error = error;
        return this.render();
      } else {
        rerender = this.error != null;
        this.error = null;
        if (result === null) {
          rerender = rerender || this.table_found;
          this.table_found = false;
          this.indexes = null;
          this.render();
        } else {
          rerender = rerender || !this.table_found;
          this.table_found = true;
          this.model.set(result);
        }
        if (rerender) {
          return this.render();
        }
      }
    };

    TableContainer.prototype.fetch_data = function() {
      this.failable_index_timer = driver.run(this.failable_index_query(), 1000, this.handle_failable_index_response);
      this.failable_misc_timer = driver.run(this.failable_misc_query(), 10000, this.handle_failable_misc_response);
      return this.guaranteed_timer = driver.run(this.guaranteed_query(), 5000, this.handle_guaranteed_response);
    };

    TableContainer.prototype.render = function() {
      var _ref;
      if (this.error != null) {
        this.$el.html(this.template.error({
          error: (_ref = this.error) != null ? _ref.message : void 0,
          url: '#tables/' + this.id
        }));
      } else {
        if (this.table_found) {
          this.$el.html(this.table_view.render().$el);
        } else {
          this.$el.html(this.template.not_found({
            id: this.id,
            type: 'table',
            type_url: 'tables',
            type_all_url: 'tables'
          }));
        }
      }
      return this;
    };

    TableContainer.prototype.remove = function() {
      var _ref;
      driver.stop_timer(this.guaranteed_timer);
      driver.stop_timer(this.failable_index_timer);
      driver.stop_timer(this.failable_misc_timer);
      if ((_ref = this.table_view) != null) {
        _ref.remove();
      }
      return TableContainer.__super__.remove.call(this);
    };

    return TableContainer;

  })(Backbone.View);
  this.TableMainView = (function(_super) {
    __extends(TableMainView, _super);

    function TableMainView() {
      this.remove = __bind(this.remove, this);
      this.rename_table = __bind(this.rename_table, this);
      this.change_pinning = __bind(this.change_pinning, this);
      this.change_shards = __bind(this.change_shards, this);
      this.render = __bind(this.render, this);
      this.set_assignments = __bind(this.set_assignments, this);
      this.set_distribution = __bind(this.set_distribution, this);
      this.set_indexes = __bind(this.set_indexes, this);
      this.initialize = __bind(this.initialize, this);
      return TableMainView.__super__.constructor.apply(this, arguments);
    }

    TableMainView.prototype.className = 'namespace-view';

    TableMainView.prototype.template = {
      main: Handlebars.templates['table_container-template'],
      alert: Handlebars.templates['modify_shards-alert-template']
    };

    TableMainView.prototype.events = {
      'click .close': 'close_alert',
      'click .change_shards-link': 'change_shards',
      'click .operations .rename': 'rename_table',
      'click .operations .delete': 'delete_table'
    };

    TableMainView.prototype.initialize = function(data, options) {
      this.indexes = data.indexes;
      this.distribution = data.distribution;
      this.shards_assignments = data.shards_assignments;
      this.title = new TableView.Title({
        model: this.model
      });
      this.profile = new TableView.Profile({
        model: this.model
      });
      this.secondary_indexes_view = new TableView.SecondaryIndexesView({
        collection: this.indexes,
        model: this.model
      });
      this.shard_distribution = new TableView.ShardDistribution({
        collection: this.distribution,
        model: this.model
      });
      this.server_assignments = new TableView.ShardAssignmentsView({
        model: this.model,
        collection: this.shards_assignments
      });
      this.reconfigure = new TableView.ReconfigurePanel({
        model: this.model
      });
      this.stats = new Stats;
      this.stats_timer = driver.run(r.db(system_db).table('stats').get(["table", this.model.get('id')])["do"](function(stat) {
        return {
          keys_read: stat('query_engine')('read_docs_per_sec'),
          keys_set: stat('query_engine')('written_docs_per_sec')
        };
      }), 1000, this.stats.on_result);
      return this.performance_graph = new Vis.OpsPlot(this.stats.get_stats, {
        width: 564,
        height: 210,
        seconds: 73,
        type: 'table'
      });
    };

    TableMainView.prototype.set_indexes = function(indexes) {
      if (this.indexes == null) {
        this.indexes = indexes;
        return this.secondary_indexes_view.set_indexes(this.indexes);
      }
    };

    TableMainView.prototype.set_distribution = function(distribution) {
      this.distribution = distribution;
      return this.shard_distribution.set_distribution(this.distribution);
    };

    TableMainView.prototype.set_assignments = function(shards_assignments) {
      this.shards_assignments = shards_assignments;
      return this.server_assignments.set_assignments(this.shards_assignments);
    };

    TableMainView.prototype.render = function() {
      this.$el.html(this.template.main({
        namespace_id: this.model.get('id')
      }));
      this.$('.main_title').html(this.title.render().$el);
      this.$('.profile').html(this.profile.render().$el);
      this.$('.performance-graph').html(this.performance_graph.render().$el);
      this.$('.sharding').html(this.shard_distribution.render().el);
      this.$('.server-assignments').html(this.server_assignments.render().el);
      this.$('.secondary_indexes').html(this.secondary_indexes_view.render().el);
      this.$('.reconfigure-panel').html(this.reconfigure.render().el);
      return this;
    };

    TableMainView.prototype.close_alert = function(event) {
      event.preventDefault();
      return $(event.currentTarget).parent().slideUp('fast', function() {
        return $(this).remove();
      });
    };

    TableMainView.prototype.change_shards = function(event) {
      event.preventDefault();
      return this.$('#namespace-sharding-link').tab('show');
    };

    TableMainView.prototype.change_pinning = function(event) {
      event.preventDefault();
      this.$('#namespace-pinning-link').tab('show');
      return $(event.currentTarget).parent().parent().slideUp('fast', function() {
        return $(this).remove();
      });
    };

    TableMainView.prototype.rename_table = function(event) {
      event.preventDefault();
      if (this.rename_modal != null) {
        this.rename_modal.remove();
      }
      this.rename_modal = new UIComponents.RenameItemModal({
        model: this.model
      });
      return this.rename_modal.render();
    };

    TableMainView.prototype.delete_table = function(event) {
      event.preventDefault();
      if (this.remove_table_dialog != null) {
        this.remove_table_dialog.remove();
      }
      this.remove_table_dialog = new Modals.RemoveTableModal;
      return this.remove_table_dialog.render([
        {
          table: this.model.get('name'),
          database: this.model.get('db')
        }
      ]);
    };

    TableMainView.prototype.remove = function() {
      this.title.remove();
      this.profile.remove();
      this.shard_distribution.remove();
      this.server_assignments.remove();
      this.performance_graph.remove();
      this.secondary_indexes_view.remove();
      this.reconfigure.remove();
      driver.stop_timer(this.stats_timer);
      if (this.remove_table_dialog != null) {
        this.remove_table_dialog.remove();
      }
      if (this.rename_modal != null) {
        this.rename_modal.remove();
      }
      return TableMainView.__super__.remove.call(this);
    };

    return TableMainView;

  })(Backbone.View);
  this.ReconfigurePanel = (function(_super) {
    __extends(ReconfigurePanel, _super);

    function ReconfigurePanel() {
      this.render = __bind(this.render, this);
      this.render_status = __bind(this.render_status, this);
      this.fetch_progress = __bind(this.fetch_progress, this);
      this.remove = __bind(this.remove, this);
      this.launch_modal = __bind(this.launch_modal, this);
      this.initialize = __bind(this.initialize, this);
      return ReconfigurePanel.__super__.constructor.apply(this, arguments);
    }

    ReconfigurePanel.prototype.className = 'reconfigure-panel';

    ReconfigurePanel.prototype.templates = {
      main: Handlebars.templates['reconfigure'],
      status: Handlebars.templates['replica_status-template']
    };

    ReconfigurePanel.prototype.events = {
      'click .reconfigure.btn': 'launch_modal'
    };

    ReconfigurePanel.prototype.initialize = function(obj) {
      this.model = obj.model;
      this.listenTo(this.model, 'change:num_shards', this.render);
      this.listenTo(this.model, 'change:num_replicas_per_shard', this.render);
      this.listenTo(this.model, 'change:num_available_replicas', this.render_status);
      this.progress_bar = new UIComponents.OperationProgressBar(this.templates.status);
      return this.timer = null;
    };

    ReconfigurePanel.prototype.launch_modal = function() {
      if (this.reconfigure_modal != null) {
        this.reconfigure_modal.remove();
      }
      this.reconfigure_modal = new Modals.ReconfigureModal({
        model: new Reconfigure({
          parent: this,
          id: this.model.get('id'),
          db: this.model.get('db'),
          name: this.model.get('name'),
          total_keys: this.model.get('total_keys'),
          shards: [],
          max_shards: this.model.get('max_shards'),
          num_shards: this.model.get('num_shards'),
          num_servers: this.model.get('num_servers'),
          num_default_servers: this.model.get('num_default_servers'),
          num_replicas_per_shard: this.model.get('num_replicas_per_shard')
        })
      });
      return this.reconfigure_modal.render();
    };

    ReconfigurePanel.prototype.remove = function() {
      if (this.reconfigure_modal != null) {
        this.reconfigure_modal.remove();
      }
      if (this.timer != null) {
        driver.stop_timer(this.timer);
        this.timer = null;
      }
      this.progress_bar.remove();
      return ReconfigurePanel.__super__.remove.call(this);
    };

    ReconfigurePanel.prototype.fetch_progress = function() {
      var query;
      query = r.db(system_db).table('table_status').get(this.model.get('id'))('shards')('replicas').concatMap(function(x) {
        return x;
      })["do"](function(replicas) {
        return {
          num_available_replicas: replicas.filter({
            state: 'ready'
          }).count(),
          num_replicas: replicas.count()
        };
      });
      if (this.timer != null) {
        driver.stop_timer(this.timer);
        this.timer = null;
      }
      return this.timer = driver.run(query, 1000, (function(_this) {
        return function(error, result) {
          if (error != null) {
            console.log("Nothing bad - Could not fetch replicas statuses");
            return console.log(error);
          } else {
            _this.model.set(result);
            return _this.render_status();
          }
        };
      })(this));
    };

    ReconfigurePanel.prototype.render_status = function() {
      if (this.model.get('num_available_replicas') < this.model.get('num_replicas')) {
        if (this.timer == null) {
          this.fetch_progress();
          return;
        }
        if (this.progress_bar.get_stage() === 'none') {
          this.progress_bar.skip_to_processing();
        }
      } else {
        if (this.timer != null) {
          driver.stop_timer(this.timer);
          this.timer = null;
        }
      }
      return this.progress_bar.render(this.model.get('num_available_replicas'), this.model.get('num_replicas'), {
        got_response: true
      });
    };

    ReconfigurePanel.prototype.render = function() {
      this.$el.html(this.templates.main(this.model.toJSON()));
      if (this.model.get('num_available_replicas') < this.model.get('num_replicas')) {
        if (this.progress_bar.get_stage() === 'none') {
          this.progress_bar.skip_to_processing();
        }
      }
      this.$('.backfill-progress').html(this.progress_bar.render(this.model.get('num_available_replicas'), this.model.get('num_replicas'), {
        got_response: true
      }).$el);
      return this;
    };

    return ReconfigurePanel;

  })(Backbone.View);
  this.ReconfigureDiffView = (function(_super) {
    __extends(ReconfigureDiffView, _super);

    function ReconfigureDiffView() {
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ReconfigureDiffView.__super__.constructor.apply(this, arguments);
    }

    ReconfigureDiffView.prototype.className = 'reconfigure-diff';

    ReconfigureDiffView.prototype.template = Handlebars.templates['reconfigure-diff'];

    ReconfigureDiffView.prototype.initialize = function() {
      return this.listenTo(this.model, 'change:shards', this.render);
    };

    ReconfigureDiffView.prototype.render = function() {
      this.$el.html(this.template(this.model.toJSON()));
      return this;
    };

    return ReconfigureDiffView;

  })(Backbone.View);
  this.Title = (function(_super) {
    __extends(Title, _super);

    function Title() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return Title.__super__.constructor.apply(this, arguments);
    }

    Title.prototype.className = 'namespace-info-view';

    Title.prototype.template = Handlebars.templates['table_title-template'];

    Title.prototype.initialize = function() {
      return this.listenTo(this.model, 'change:name', this.render);
    };

    Title.prototype.render = function() {
      this.$el.html(this.template({
        name: this.model.get('name'),
        db: this.model.get('db')
      }));
      return this;
    };

    Title.prototype.remove = function() {
      this.stopListening();
      return Title.__super__.remove.call(this);
    };

    return Title;

  })(Backbone.View);
  this.Profile = (function(_super) {
    __extends(Profile, _super);

    function Profile() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return Profile.__super__.constructor.apply(this, arguments);
    }

    Profile.prototype.template = Handlebars.templates['table_profile-template'];

    Profile.prototype.initialize = function() {
      this.listenTo(this.model, 'change', this.render);
      return this.indicator = new TableView.TableStatusIndicator({
        model: this.model
      });
    };

    Profile.prototype.render = function() {
      var obj, _ref;
      obj = {
        status: this.model.get('status'),
        total_keys: this.model.get('total_keys'),
        num_shards: this.model.get('num_shards'),
        num_available_shards: this.model.get('num_available_shards'),
        num_replicas: this.model.get('num_replicas'),
        num_available_replicas: this.model.get('num_available_replicas')
      };
      if (((_ref = obj.status) != null ? _ref.ready_for_writes : void 0) && !(obj != null ? obj.status.all_replicas_ready : void 0)) {
        obj.parenthetical = true;
      }
      this.$el.html(this.template(obj));
      this.$('.availability').prepend(this.indicator.$el);
      return this;
    };

    Profile.prototype.remove = function() {
      this.indicator.remove();
      return Profile.__super__.remove.call(this);
    };

    return Profile;

  })(Backbone.View);
  this.TableStatusIndicator = (function(_super) {
    __extends(TableStatusIndicator, _super);

    function TableStatusIndicator() {
      this.render = __bind(this.render, this);
      this.status_to_class = __bind(this.status_to_class, this);
      this.initialize = __bind(this.initialize, this);
      return TableStatusIndicator.__super__.constructor.apply(this, arguments);
    }

    TableStatusIndicator.prototype.template = Handlebars.templates['table_status_indicator'];

    TableStatusIndicator.prototype.initialize = function() {
      var status_class;
      status_class = this.status_to_class(this.model.get('status'));
      this.setElement(this.template({
        status_class: status_class
      }));
      this.render();
      return this.listenTo(this.model, 'change:status', this.render);
    };

    TableStatusIndicator.prototype.status_to_class = function(status) {
      if (!status) {
        return "unknown";
      }
      if (status.all_replicas_ready) {
        return "ready";
      } else if (status.ready_for_writes) {
        return "ready-but-with-caveats";
      } else {
        return "not-ready";
      }
    };

    TableStatusIndicator.prototype.render = function() {
      var stat_class;
      stat_class = this.status_to_class(this.model.get('status'));
      return this.$el.attr('class', "ionicon-table-status " + stat_class);
    };

    return TableStatusIndicator;

  })(Backbone.View);
  this.SecondaryIndexesView = (function(_super) {
    __extends(SecondaryIndexesView, _super);

    function SecondaryIndexesView() {
      this.remove = __bind(this.remove, this);
      this.create_index = __bind(this.create_index, this);
      this.on_fail_to_connect = __bind(this.on_fail_to_connect, this);
      this.handle_keypress = __bind(this.handle_keypress, this);
      this.hide_add_index = __bind(this.hide_add_index, this);
      this.show_add_index = __bind(this.show_add_index, this);
      this.render = __bind(this.render, this);
      this.render_feedback = __bind(this.render_feedback, this);
      this.render_error = __bind(this.render_error, this);
      this.hook = __bind(this.hook, this);
      this.set_indexes = __bind(this.set_indexes, this);
      this.fetch_progress = __bind(this.fetch_progress, this);
      this.set_fetch_progress = __bind(this.set_fetch_progress, this);
      this.initialize = __bind(this.initialize, this);
      return SecondaryIndexesView.__super__.constructor.apply(this, arguments);
    }

    SecondaryIndexesView.prototype.template = Handlebars.templates['table-secondary_indexes-template'];

    SecondaryIndexesView.prototype.alert_message_template = Handlebars.templates['secondary_indexes-alert_msg-template'];

    SecondaryIndexesView.prototype.error_template = Handlebars.templates['secondary_indexes-error-template'];

    SecondaryIndexesView.prototype.events = {
      'click .create_link': 'show_add_index',
      'click .create_btn': 'create_index',
      'keydown .new_index_name': 'handle_keypress',
      'click .cancel_btn': 'hide_add_index',
      'click .reconnect_link': 'init_connection',
      'click .close_hide': 'hide_alert'
    };

    SecondaryIndexesView.prototype.error_interval = 5 * 1000;

    SecondaryIndexesView.prototype.normal_interval = 10 * 1000;

    SecondaryIndexesView.prototype.short_interval = 1000;

    SecondaryIndexesView.prototype.initialize = function(data) {
      this.indexes_view = [];
      this.interval_progress = null;
      this.collection = data.collection;
      this.model = data.model;
      this.adding_index = false;
      this.listenTo(this.model, 'change:index_unavailable', this.set_warnings);
      this.render();
      if (this.collection != null) {
        return this.hook();
      }
    };

    SecondaryIndexesView.prototype.set_warnings = function() {
      if (this.model.get('index_unavailable')) {
        return this.$('.unavailable-error').show();
      } else {
        return this.$('.unavailable-error').hide();
      }
    };

    SecondaryIndexesView.prototype.set_fetch_progress = function(index) {
      if (this.interval_progress == null) {
        this.fetch_progress();
        return this.interval_progress = setInterval(this.fetch_progress, 1000);
      }
    };

    SecondaryIndexesView.prototype.fetch_progress = function() {
      var build_in_progress, index, query, _i, _len, _ref;
      build_in_progress = false;
      _ref = this.collection.models;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        index = _ref[_i];
        if (index.get('ready') !== true) {
          build_in_progress = true;
          break;
        }
      }
      if (build_in_progress && (this.model.get('db') != null)) {
        query = r.db(this.model.get('db')).table(this.model.get('name')).indexStatus().pluck('index', 'ready', 'blocks_processed', 'blocks_total').merge((function(_this) {
          return function(index) {
            return {
              id: index("index"),
              db: _this.model.get("db"),
              table: _this.model.get("name")
            };
          };
        })(this));
        return driver.run_once(query, (function(_this) {
          return function(error, result) {
            var all_ready, _j, _len1;
            if (error != null) {
              console.log("Nothing bad - Could not fetch secondary indexes statuses");
              return console.log(error);
            } else {
              all_ready = true;
              for (_j = 0, _len1 = result.length; _j < _len1; _j++) {
                index = result[_j];
                if (index.ready !== true) {
                  all_ready = false;
                }
                _this.collection.add(new Index(index, {
                  merge: true
                }));
              }
              if (all_ready === true) {
                clearInterval(_this.interval_progress);
                return _this.interval_progress = null;
              }
            }
          };
        })(this));
      } else {
        clearInterval(this.interval_progress);
        return this.interval_progress = null;
      }
    };

    SecondaryIndexesView.prototype.set_indexes = function(indexes) {
      this.collection = indexes;
      return this.hook();
    };

    SecondaryIndexesView.prototype.hook = function() {
      this.collection.each((function(_this) {
        return function(index) {
          var view;
          view = new TableView.SecondaryIndexView({
            model: index,
            container: _this
          });
          _this.indexes_view.push(view);
          return _this.$('.list_secondary_indexes').append(view.render().$el);
        };
      })(this));
      if (this.collection.length === 0) {
        this.$('.no_index').show();
      } else {
        this.$('.no_index').hide();
      }
      this.listenTo(this.collection, 'add', (function(_this) {
        return function(index) {
          var added, new_view, position, view, _i, _len, _ref;
          new_view = new TableView.SecondaryIndexView({
            model: index,
            container: _this
          });
          if (_this.indexes_view.length === 0) {
            _this.indexes_view.push(new_view);
            _this.$('.list_secondary_indexes').html(new_view.render().$el);
          } else {
            added = false;
            _ref = _this.indexes_view;
            for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
              view = _ref[position];
              if (view.model.get('index') > index.get('index')) {
                added = true;
                _this.indexes_view.splice(position, 0, new_view);
                if (position === 0) {
                  _this.$('.list_secondary_indexes').prepend(new_view.render().$el);
                } else {
                  _this.$('.index_container').eq(position - 1).after(new_view.render().$el);
                }
                break;
              }
            }
            if (added === false) {
              _this.indexes_view.push(new_view);
              _this.$('.list_secondary_indexes').append(new_view.render().$el);
            }
          }
          if (_this.indexes_view.length === 1) {
            return _this.$('.no_index').hide();
          }
        };
      })(this));
      return this.listenTo(this.collection, 'remove', (function(_this) {
        return function(index) {
          var view, _i, _len, _ref;
          _ref = _this.indexes_view;
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            view = _ref[_i];
            if (view.model === index) {
              index.destroy();
              (function(view) {
                view.remove();
                return _this.indexes_view.splice(_this.indexes_view.indexOf(view), 1);
              })(view);
              break;
            }
          }
          if (_this.collection.length === 0) {
            return _this.$('.no_index').show();
          }
        };
      })(this));
    };

    SecondaryIndexesView.prototype.render_error = function(args) {
      this.$('.alert_error_content').html(this.error_template(args));
      this.$('.main_alert_error').show();
      return this.$('.main_alert').hide();
    };

    SecondaryIndexesView.prototype.render_feedback = function(args) {
      if (this.$('.main_alert').css('display') === 'none') {
        this.$('.alert_content').html(this.alert_message_template(args));
        this.$('.main_alert').show();
      } else {
        this.$('.main_alert').fadeOut('fast', (function(_this) {
          return function() {
            _this.$('.alert_content').html(_this.alert_message_template(args));
            return _this.$('.main_alert').fadeIn('fast');
          };
        })(this));
      }
      return this.$('.main_alert_error').hide();
    };

    SecondaryIndexesView.prototype.render = function() {
      this.$el.html(this.template({
        adding_index: this.adding_index
      }));
      this.set_warnings();
      return this;
    };

    SecondaryIndexesView.prototype.show_add_index = function(event) {
      event.preventDefault();
      this.$('.add_index_li').show();
      this.$('.create_container').hide();
      return this.$('.new_index_name').focus();
    };

    SecondaryIndexesView.prototype.hide_add_index = function() {
      this.$('.add_index_li').hide();
      this.$('.create_container').show();
      return this.$('.new_index_name').val('');
    };

    SecondaryIndexesView.prototype.handle_keypress = function(event) {
      if (event.which === 13) {
        event.preventDefault();
        return this.create_index();
      } else if (event.which === 27) {
        event.preventDefault();
        return this.hide_add_index();
      }
    };

    SecondaryIndexesView.prototype.on_fail_to_connect = function() {
      this.render_error({
        connect_fail: true
      });
      return this;
    };

    SecondaryIndexesView.prototype.create_index = function() {
      var index_name;
      this.$('.create_btn').prop('disabled', 'disabled');
      this.$('.cancel_btn').prop('disabled', 'disabled');
      index_name = this.$('.new_index_name').val();
      return ((function(_this) {
        return function(index_name) {
          var query;
          query = r.db(_this.model.get('db')).table(_this.model.get('name')).indexCreate(index_name);
          return driver.run_once(query, function(error, result) {
            var that;
            _this.$('.create_btn').prop('disabled', false);
            _this.$('.cancel_btn').prop('disabled', false);
            that = _this;
            if (error != null) {
              return _this.render_error({
                create_fail: true,
                message: error.msg.replace('\n', '<br/>')
              });
            } else {
              _this.collection.add(new Index({
                id: index_name,
                index: index_name,
                db: _this.model.get('db'),
                table: _this.model.get('name')
              }));
              _this.render_feedback({
                create_ok: true,
                name: index_name
              });
              return _this.hide_add_index();
            }
          });
        };
      })(this))(index_name);
    };

    SecondaryIndexesView.prototype.hide_alert = function(event) {
      var _ref;
      if ((event != null) && (((_ref = this.$(event.target)) != null ? _ref.data('name') : void 0) != null)) {
        this.deleting_secondary_index = null;
      }
      event.preventDefault();
      return $(event.target).parent().hide();
    };

    SecondaryIndexesView.prototype.remove = function() {
      var view, _i, _len, _ref;
      this.stopListening();
      _ref = this.indexes_view;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        view = _ref[_i];
        view.remove();
      }
      return SecondaryIndexesView.__super__.remove.call(this);
    };

    return SecondaryIndexesView;

  })(Backbone.View);
  return this.SecondaryIndexView = (function(_super) {
    __extends(SecondaryIndexView, _super);

    function SecondaryIndexView() {
      this.remove = __bind(this.remove, this);
      this.delete_index = __bind(this.delete_index, this);
      this.confirm_delete = __bind(this.confirm_delete, this);
      this.render_progress_bar = __bind(this.render_progress_bar, this);
      this.render = __bind(this.render, this);
      this.update = __bind(this.update, this);
      this.initialize = __bind(this.initialize, this);
      return SecondaryIndexView.__super__.constructor.apply(this, arguments);
    }

    SecondaryIndexView.prototype.template = Handlebars.templates['table-secondary_index-template'];

    SecondaryIndexView.prototype.progress_template = Handlebars.templates['simple_progressbar-template'];

    SecondaryIndexView.prototype.events = {
      'click .delete_link': 'confirm_delete',
      'click .delete_index_btn': 'delete_index',
      'click .cancel_delete_btn': 'cancel_delete'
    };

    SecondaryIndexView.prototype.tagName = 'li';

    SecondaryIndexView.prototype.className = 'index_container';

    SecondaryIndexView.prototype.initialize = function(data) {
      this.container = data.container;
      this.model.on('change:blocks_processed', this.update);
      return this.model.on('change:ready', this.update);
    };

    SecondaryIndexView.prototype.update = function(args) {
      if (this.model.get('ready') === false) {
        return this.render_progress_bar();
      } else {
        if (this.progress_bar != null) {
          return this.render_progress_bar();
        } else {
          return this.render();
        }
      }
    };

    SecondaryIndexView.prototype.render = function() {
      this.$el.html(this.template({
        is_empty: this.model.get('name') === '',
        name: this.model.get('index'),
        ready: this.model.get('ready')
      }));
      if (this.model.get('ready') === false) {
        this.render_progress_bar();
        this.container.set_fetch_progress();
      }
      return this;
    };

    SecondaryIndexView.prototype.render_progress_bar = function() {
      var blocks_processed, blocks_total;
      blocks_processed = this.model.get('blocks_processed');
      blocks_total = this.model.get('blocks_total');
      if (this.progress_bar != null) {
        if (this.model.get('ready') === true) {
          return this.progress_bar.render(100, 100, {
            got_response: true,
            check: true
          }, (function(_this) {
            return function() {
              return _this.render();
            };
          })(this));
        } else {
          return this.progress_bar.render(blocks_processed, blocks_total, {
            got_response: true,
            check: true
          }, (function(_this) {
            return function() {
              return _this.render();
            };
          })(this));
        }
      } else {
        this.progress_bar = new UIComponents.OperationProgressBar(this.progress_template);
        return this.$('.progress_li').html(this.progress_bar.render(0, Infinity, {
          new_value: true,
          check: true
        }).$el);
      }
    };

    SecondaryIndexView.prototype.confirm_delete = function(event) {
      event.preventDefault();
      return this.$('.alert_confirm_delete').show();
    };

    SecondaryIndexView.prototype.delete_index = function() {
      var query;
      this.$('.btn').prop('disabled', 'disabled');
      query = r.db(this.model.get('db')).table(this.model.get('table')).indexDrop(this.model.get('index'));
      return driver.run_once(query, (function(_this) {
        return function(error, result) {
          _this.$('.btn').prop('disabled', false);
          if (error != null) {
            return _this.container.render_error({
              delete_fail: true,
              message: error.msg.replace('\n', '<br/>')
            });
          } else if (result.dropped === 1) {
            _this.container.render_feedback({
              delete_ok: true,
              name: _this.model.get('index')
            });
            return _this.model.destroy();
          } else {
            return _this.container.render_error({
              delete_fail: true,
              message: "Result was not {dropped: 1}"
            });
          }
        };
      })(this));
    };

    SecondaryIndexView.prototype.cancel_delete = function() {
      return this.$('.alert_confirm_delete').hide();
    };

    SecondaryIndexView.prototype.remove = function() {
      var _ref;
      if ((_ref = this.progress_bar) != null) {
        _ref.remove();
      }
      return SecondaryIndexView.__super__.remove.call(this);
    };

    return SecondaryIndexView;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('ServersView', function() {
  this.ServersContainer = (function(_super) {
    __extends(ServersContainer, _super);

    function ServersContainer() {
      this.remove = __bind(this.remove, this);
      this.fetch_servers = __bind(this.fetch_servers, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ServersContainer.__super__.constructor.apply(this, arguments);
    }

    ServersContainer.prototype.id = 'servers_container';

    ServersContainer.prototype.template = {
      main: Handlebars.templates['servers_container-template']
    };

    ServersContainer.prototype.initialize = function() {
      if (window.view_data_backup.servers_view_servers == null) {
        window.view_data_backup.servers_view_servers = new Servers;
        this.loading = true;
      } else {
        this.loading = false;
      }
      this.servers = window.view_data_backup.servers_view_servers;
      this.servers_list = new ServersView.ServersListView({
        collection: this.servers
      });
      return this.fetch_servers();
    };

    ServersContainer.prototype.render = function() {
      this.$el.html(this.template.main({}));
      this.$('.servers_list').html(this.servers_list.render().$el);
      return this;
    };

    ServersContainer.prototype.fetch_servers = function() {
      var query;
      query = r["do"](r.db(system_db).table('server_config').map(function(x) {
        return [x('id'), x];
      }).coerceTo('ARRAY').coerceTo('OBJECT'), r.db(system_db).table('table_config').coerceTo('array'), r.db(system_db).table('table_config').coerceTo('array').concatMap(function(table) {
        return table('shards');
      }), function(server_config, table_config, table_config_shards) {
        return r.db(system_db).table('server_status').merge(function(server) {
          return {
            id: server("id"),
            tags: server_config(server('id'))('tags'),
            primary_count: table_config.concatMap(function(table) {
              return table("shards");
            }).count(function(shard) {
              return shard("primary_replica").eq(server("name"));
            }),
            secondary_count: table_config_shards.filter(function(shard) {
              return shard("primary_replica").ne(server("name"));
            }).map(function(shard) {
              return shard("replicas").count(function(replica) {
                return replica.eq(server("name"));
              });
            }).sum()
          };
        });
      });
      return this.timer = driver.run(query, 5000, (function(_this) {
        return function(error, result) {
          var ids, index, server, toDestroy, _i, _j, _k, _len, _len1, _len2, _ref;
          if (error != null) {
            console.log(error);
            return;
          }
          ids = {};
          for (index = _i = 0, _len = result.length; _i < _len; index = ++_i) {
            server = result[index];
            _this.servers.add(new Server(server));
            ids[server.id] = true;
          }
          toDestroy = [];
          _ref = _this.servers.models;
          for (_j = 0, _len1 = _ref.length; _j < _len1; _j++) {
            server = _ref[_j];
            if (ids[server.get('id')] !== true) {
              toDestroy.push(server);
            }
          }
          for (_k = 0, _len2 = toDestroy.length; _k < _len2; _k++) {
            server = toDestroy[_k];
            server.destroy();
          }
          return _this.render();
        };
      })(this));
    };

    ServersContainer.prototype.remove = function() {
      driver.stop_timer(this.timer);
      this.servers_list.remove();
      return ServersContainer.__super__.remove.call(this);
    };

    return ServersContainer;

  })(Backbone.View);
  this.ServersListView = (function(_super) {
    __extends(ServersListView, _super);

    function ServersListView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ServersListView.__super__.constructor.apply(this, arguments);
    }

    ServersListView.prototype.className = 'servers_view';

    ServersListView.prototype.tagName = 'tbody';

    ServersListView.prototype.template = {
      loading_servers: Handlebars.templates['loading_servers']
    };

    ServersListView.prototype.initialize = function() {
      this.servers_view = [];
      this.collection.each((function(_this) {
        return function(server) {
          var view;
          view = new ServersView.ServerView({
            model: server
          });
          _this.servers_view.push(view);
          return _this.$el.append(view.render().$el);
        };
      })(this));
      this.listenTo(this.collection, 'add', (function(_this) {
        return function(server) {
          var added, new_view, position, view, _i, _len, _ref;
          new_view = new ServersView.ServerView({
            model: server
          });
          if (_this.servers_view.length === 0) {
            _this.servers_view.push(new_view);
            return _this.$el.html(new_view.render().$el);
          } else {
            added = false;
            _ref = _this.servers_view;
            for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
              view = _ref[position];
              if (view.model.get('name') > server.get('name')) {
                added = true;
                _this.servers_view.splice(position, 0, new_view);
                if (position === 0) {
                  _this.$el.prepend(new_view.render().$el);
                } else {
                  _this.$('.server_container').eq(position - 1).after(new_view.render().$el);
                }
                break;
              }
            }
            if (added === false) {
              _this.servers_view.push(new_view);
              return _this.$el.append(new_view.render().$el);
            }
          }
        };
      })(this));
      return this.listenTo(this.collection, 'remove', (function(_this) {
        return function(server) {
          var view, _i, _len, _ref, _results;
          _ref = _this.servers_view;
          _results = [];
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            view = _ref[_i];
            if (view.model === server) {
              server.destroy();
              view.remove();
              break;
            } else {
              _results.push(void 0);
            }
          }
          return _results;
        };
      })(this));
    };

    ServersListView.prototype.render = function() {
      var server_view, _i, _len, _ref;
      if (this.servers_view.length === 0) {
        this.$el.append(this.template.loading_servers());
      } else {
        _ref = this.servers_view;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          server_view = _ref[_i];
          this.$el.append(server_view.render().$el);
        }
      }
      return this;
    };

    ServersListView.prototype.remove = function() {
      var view, _i, _len, _ref;
      this.stopListening();
      _ref = this.servers_view;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        view = _ref[_i];
        view.remove();
      }
      return ServersListView.__super__.remove.call(this);
    };

    return ServersListView;

  })(Backbone.View);
  return this.ServerView = (function(_super) {
    __extends(ServerView, _super);

    function ServerView() {
      this.remove = __bind(this.remove, this);
      this.initialize = __bind(this.initialize, this);
      return ServerView.__super__.constructor.apply(this, arguments);
    }

    ServerView.prototype.className = 'server_container';

    ServerView.prototype.tagName = 'tr';

    ServerView.prototype.template = Handlebars.templates['server-template'];

    ServerView.prototype.initialize = function() {
      return this.listenTo(this.model, 'change', this.render);
    };

    ServerView.prototype.render = function() {
      this.$el.html(this.template(this.model.toJSON()));
      return this;
    };

    ServerView.prototype.remove = function() {
      return this.stopListening();
    };

    return ServerView;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('ServerView', function() {
  this.ServerContainer = (function(_super) {
    __extends(ServerContainer, _super);

    function ServerContainer() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.fetch_server = __bind(this.fetch_server, this);
      this.initialize = __bind(this.initialize, this);
      return ServerContainer.__super__.constructor.apply(this, arguments);
    }

    ServerContainer.prototype.template = {
      error: Handlebars.templates['error-query-template'],
      not_found: Handlebars.templates['element_view-not_found-template']
    };

    ServerContainer.prototype.initialize = function(id) {
      this.id = id;
      this.server_found = true;
      this.server = new Server({
        id: id
      });
      this.responsibilities = new Responsibilities;
      this.server_view = new ServerView.ServerMainView({
        model: this.server,
        collection: this.responsibilities
      });
      return this.fetch_server();
    };

    ServerContainer.prototype.fetch_server = function() {
      var query;
      query = r["do"](r.db(system_db).table('server_config').get(this.id), r.db(system_db).table('server_status').get(this.id), function(server_config, server_status) {
        return r.branch(server_status.eq(null), null, server_status.merge(function(server_status) {
          return {
            tags: server_config('tags'),
            responsibilities: r.db(system_db).table('table_status').orderBy(function(table) {
              return table('db').add('.').add(table('name'));
            }).map(function(table) {
              return table.merge(function(table) {
                return {
                  shards: table("shards").map(r.range(), function(shard, index) {
                    return shard.merge({
                      index: index.add(1),
                      num_shards: table('shards').count(),
                      role: r.branch(server_status('name').eq(shard('primary_replica')), 'primary', 'secondary')
                    });
                  }).filter(function(shard) {
                    return shard('replicas')('server').contains(server_status('name'));
                  }).coerceTo('array')
                };
              });
            }).filter(function(table) {
              return table("shards").isEmpty().not();
            }).coerceTo("ARRAY")
          };
        }).merge({
          id: server_status('id')
        }));
      });
      return this.timer = driver.run(query, 5000, (function(_this) {
        return function(error, result) {
          var rerender, responsibilities, shard, table, _i, _j, _len, _len1, _ref, _ref1;
          if (error != null) {
            _this.error = error;
            return _this.render();
          } else {
            rerender = _this.error != null;
            _this.error = null;
            if (result === null) {
              rerender = rerender || _this.server_found;
              _this.server_found = false;
            } else {
              rerender = rerender || !_this.server_found;
              _this.server_found = true;
              responsibilities = [];
              _ref = result.responsibilities;
              for (_i = 0, _len = _ref.length; _i < _len; _i++) {
                table = _ref[_i];
                responsibilities.push(new Responsibility({
                  type: "table",
                  is_table: true,
                  db: table.db,
                  table: table.name,
                  table_id: table.id,
                  id: table.db + "." + table.name
                }));
                _ref1 = table.shards;
                for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
                  shard = _ref1[_j];
                  responsibilities.push(new Responsibility({
                    is_shard: true,
                    db: table.db,
                    table: table.name,
                    index: shard.index,
                    num_shards: shard.num_shards,
                    role: shard.role,
                    id: table.db + "." + table.name + "." + shard.index
                  }));
                }
              }
              if (_this.responsibilities == null) {
                _this.responsibilities = new Responsibilities(responsibilities);
              } else {
                _this.responsibilities.set(responsibilities);
              }
              delete result.responsibilities;
              _this.server.set(result);
            }
            if (rerender) {
              return _this.render();
            }
          }
        };
      })(this));
    };

    ServerContainer.prototype.render = function() {
      var _ref;
      if (this.error != null) {
        this.$el.html(this.template.error({
          error: (_ref = this.error) != null ? _ref.message : void 0,
          url: '#servers/' + this.id
        }));
      } else {
        if (this.server_found) {
          this.$el.html(this.server_view.render().$el);
        } else {
          this.$el.html(this.template.not_found({
            id: this.id,
            type: 'server',
            type_url: 'servers',
            type_all_url: 'servers'
          }));
        }
      }
      return this;
    };

    ServerContainer.prototype.remove = function() {
      var _ref;
      driver.stop_timer(this.timer);
      if ((_ref = this.server_view) != null) {
        _ref.remove();
      }
      return ServerContainer.__super__.remove.call(this);
    };

    return ServerContainer;

  })(Backbone.View);
  this.ServerMainView = (function(_super) {
    __extends(ServerMainView, _super);

    function ServerMainView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      this.rename_server = __bind(this.rename_server, this);
      return ServerMainView.__super__.constructor.apply(this, arguments);
    }

    ServerMainView.prototype.template = {
      main: Handlebars.templates['full_server-template']
    };

    ServerMainView.prototype.events = {
      'click .close': 'close_alert',
      'click .operations .rename': 'rename_server'
    };

    ServerMainView.prototype.rename_server = function(event) {
      event.preventDefault();
      if (this.rename_modal != null) {
        this.rename_modal.remove();
      }
      this.rename_modal = new UIComponents.RenameItemModal({
        model: this.model
      });
      return this.rename_modal.render();
    };

    ServerMainView.prototype.close_alert = function(event) {
      event.preventDefault();
      return $(event.currentTarget).parent().slideUp('fast', function() {
        return $(this).remove();
      });
    };

    ServerMainView.prototype.initialize = function() {
      this.title = new ServerView.Title({
        model: this.model
      });
      this.profile = new ServerView.Profile({
        model: this.model,
        collection: this.collection
      });
      this.stats = new Stats;
      this.stats_timer = driver.run(r.db(system_db).table('stats').get(['server', this.model.get('id')])["do"](function(stat) {
        return {
          keys_read: stat('query_engine')('read_docs_per_sec'),
          keys_set: stat('query_engine')('written_docs_per_sec')
        };
      }), 1000, this.stats.on_result);
      this.performance_graph = new Vis.OpsPlot(this.stats.get_stats, {
        width: 564,
        height: 210,
        seconds: 73,
        type: 'server'
      });
      return this.responsibilities = new ServerView.ResponsibilitiesList({
        collection: this.collection
      });
    };

    ServerMainView.prototype.render = function() {
      this.$el.html(this.template.main());
      this.$('.main_title').html(this.title.render().$el);
      this.$('.profile').html(this.profile.render().$el);
      this.$('.performance-graph').html(this.performance_graph.render().$el);
      this.$('.responsibilities').html(this.responsibilities.render().$el);
      this.logs = new LogView.LogsContainer({
        server_id: this.model.get('id'),
        limit: 5,
        query: driver.queries.server_logs
      });
      this.$('.recent-log-entries').html(this.logs.render().$el);
      return this;
    };

    ServerMainView.prototype.remove = function() {
      driver.stop_timer(this.stats_timer);
      this.title.remove();
      this.profile.remove();
      this.responsibilities.remove();
      if (this.rename_modal != null) {
        this.rename_modal.remove();
      }
      return this.logs.remove();
    };

    return ServerMainView;

  })(Backbone.View);
  this.Title = (function(_super) {
    __extends(Title, _super);

    function Title() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return Title.__super__.constructor.apply(this, arguments);
    }

    Title.prototype.className = 'server-info-view';

    Title.prototype.template = Handlebars.templates['server_view_title-template'];

    Title.prototype.initialize = function() {
      return this.listenTo(this.model, 'change:name', this.render);
    };

    Title.prototype.render = function() {
      this.$el.html(this.template({
        name: this.model.get('name')
      }));
      return this;
    };

    Title.prototype.remove = function() {
      this.stopListening();
      return Title.__super__.remove.call(this);
    };

    return Title;

  })(Backbone.View);
  this.Profile = (function(_super) {
    __extends(Profile, _super);

    function Profile() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return Profile.__super__.constructor.apply(this, arguments);
    }

    Profile.prototype.className = 'server-info-view';

    Profile.prototype.template = Handlebars.templates['server_view_profile-template'];

    Profile.prototype.initialize = function() {
      this.listenTo(this.model, 'change', this.render);
      this.listenTo(this.collection, 'add', this.render);
      return this.listenTo(this.collection, 'remove', this.render);
    };

    Profile.prototype.render = function() {
      var last_seen, main_ip, uptime, version, _ref;
      if (this.model.get('status') !== 'connected') {
        if ((this.model.get('connection') != null) && (this.model.get('connection').time_disconnected != null)) {
          last_seen = $.timeago(this.model.get('connection').time_disconnected).slice(0, -4);
        } else {
          last_seen = "unknown time";
        }
        uptime = null;
        version = "unknown";
      } else {
        last_seen = null;
        uptime = $.timeago(this.model.get('connection').time_connected).slice(0, -4);
        version = (_ref = this.model.get('process').version) != null ? _ref.split(' ')[1].split('-')[0] : void 0;
      }
      if (this.model.get('network') != null) {
        main_ip = this.model.get('network').hostname;
      } else {
        main_ip = "";
      }
      this.$el.html(this.template({
        main_ip: main_ip,
        tags: this.model.get('tags'),
        uptime: uptime,
        version: version,
        num_shards: this.collection.length,
        status: this.model.get('status'),
        last_seen: last_seen,
        system_db: system_db
      }));
      this.$('.tag-row .tags, .tag-row .admonition').tooltip({
        for_dataexplorer: false,
        trigger: 'hover',
        placement: 'bottom'
      });
      return this;
    };

    Profile.prototype.remove = function() {
      this.stopListening();
      return Profile.__super__.remove.call(this);
    };

    return Profile;

  })(Backbone.View);
  this.ResponsibilitiesList = (function(_super) {
    __extends(ResponsibilitiesList, _super);

    function ResponsibilitiesList() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ResponsibilitiesList.__super__.constructor.apply(this, arguments);
    }

    ResponsibilitiesList.prototype.template = Handlebars.templates['responsibilities-template'];

    ResponsibilitiesList.prototype.initialize = function() {
      this.responsibilities_view = [];
      this.$el.html(this.template);
      this.collection.each((function(_this) {
        return function(responsibility) {
          var view;
          view = new ServerView.ResponsibilityView({
            model: responsibility,
            container: _this
          });
          _this.responsibilities_view.push(view);
          return _this.$('.responsibilities_list').append(view.render().$el);
        };
      })(this));
      if (this.responsibilities_view.length > 0) {
        this.$('.no_element').hide();
      }
      this.listenTo(this.collection, 'add', (function(_this) {
        return function(responsibility) {
          var added, new_view, position, view, _i, _len, _ref;
          new_view = new ServerView.ResponsibilityView({
            model: responsibility,
            container: _this
          });
          if (_this.responsibilities_view.length === 0) {
            _this.responsibilities_view.push(new_view);
            _this.$('.responsibilities_list').html(new_view.render().$el);
          } else {
            added = false;
            _ref = _this.responsibilities_view;
            for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
              view = _ref[position];
              if (Responsibilities.prototype.comparator(view.model, responsibility) > 0) {
                added = true;
                _this.responsibilities_view.splice(position, 0, new_view);
                if (position === 0) {
                  _this.$('.responsibilities_list').prepend(new_view.render().$el);
                } else {
                  _this.$('.responsibility_container').eq(position - 1).after(new_view.render().$el);
                }
                break;
              }
            }
            if (added === false) {
              _this.responsibilities_view.push(new_view);
              _this.$('.responsibilities_list').append(new_view.render().$el);
            }
          }
          if (_this.responsibilities_view.length > 0) {
            return _this.$('.no_element').hide();
          }
        };
      })(this));
      return this.listenTo(this.collection, 'remove', (function(_this) {
        return function(responsibility) {
          var position, view, _i, _len, _ref;
          _ref = _this.responsibilities_view;
          for (position = _i = 0, _len = _ref.length; _i < _len; position = ++_i) {
            view = _ref[position];
            if (view.model === responsibility) {
              responsibility.destroy();
              view.remove();
              _this.responsibilities_view.splice(position, 1);
              break;
            }
          }
          if (_this.responsibilities_view.length === 0) {
            return _this.$('.no_element').show();
          }
        };
      })(this));
    };

    ResponsibilitiesList.prototype.render = function() {
      return this;
    };

    ResponsibilitiesList.prototype.remove = function() {
      var view, _i, _len, _ref;
      this.stopListening();
      _ref = this.responsibilities_view;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        view = _ref[_i];
        view.model.destroy();
        view.remove();
      }
      return ResponsibilitiesList.__super__.remove.call(this);
    };

    return ResponsibilitiesList;

  })(Backbone.View);
  return this.ResponsibilityView = (function(_super) {
    __extends(ResponsibilityView, _super);

    function ResponsibilityView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ResponsibilityView.__super__.constructor.apply(this, arguments);
    }

    ResponsibilityView.prototype.className = 'responsibility_container';

    ResponsibilityView.prototype.template = Handlebars.templates['responsibility-template'];

    ResponsibilityView.prototype.initialize = function() {
      return this.listenTo(this.model, 'change', this.render);
    };

    ResponsibilityView.prototype.render = function() {
      this.$el.html(this.template(this.model.toJSON()));
      return this;
    };

    ResponsibilityView.prototype.remove = function() {
      this.stopListening();
      return ResponsibilityView.__super__.remove.call(this);
    };

    return ResponsibilityView;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('DashboardView', function() {
  this.DashboardContainer = (function(_super) {
    __extends(DashboardContainer, _super);

    function DashboardContainer() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.fetch_data = __bind(this.fetch_data, this);
      this.initialize = __bind(this.initialize, this);
      return DashboardContainer.__super__.constructor.apply(this, arguments);
    }

    DashboardContainer.prototype.id = 'dashboard_container';

    DashboardContainer.prototype.template = {
      error: Handlebars.templates['error-query-template']
    };

    DashboardContainer.prototype.initialize = function() {
      if (window.view_data_backup.dashboard_view_dashboard == null) {
        window.view_data_backup.dashboard_view_dashboard = new Dashboard;
      }
      this.dashboard = window.view_data_backup.dashboard_view_dashboard;
      this.dashboard_view = new DashboardView.DashboardMainView({
        model: this.dashboard
      });
      return this.fetch_data();
    };

    DashboardContainer.prototype.fetch_data = function() {
      var A, dashboard_callback, query;
      A = driver.admin();
      query = r["do"](A.table_status.coerceTo('array'), A.table_config_id.coerceTo('array'), A.server_config.coerceTo('array'), A.server_status.coerceTo('array'), A.jobs.coerceTo('array'), A.current_issues.coerceTo('array'), function(table_status, table_config_id, server_config, server_status, jobs, current_issues) {
        var Q;
        Q = driver.queries;
        return r.expr({
          num_primaries: Q.num_primaries(table_config_id),
          num_connected_primaries: Q.num_connected_primaries(table_status),
          num_replicas: Q.num_replicas(table_config_id),
          num_connected_replicas: Q.num_connected_replicas(table_status),
          tables_with_primaries_not_ready: Q.tables_with_primaries_not_ready(table_config_id, table_status),
          tables_with_replicas_not_ready: Q.tables_with_replicas_not_ready(table_config_id, table_status),
          num_tables: table_config_id.count(),
          num_servers: server_status.count(),
          num_connected_servers: Q.num_connected_servers(server_status),
          disconnected_servers: Q.disconnected_servers(server_status),
          num_disconnected_tables: Q.num_disconnected_tables(table_status),
          num_tables_w_missing_replicas: Q.num_tables_w_missing_replicas(table_status),
          num_sindex_issues: Q.num_sindex_issues(current_issues),
          num_sindexes_constructing: Q.num_sindexes_constructing(jobs)
        });
      });
      dashboard_callback = (function(_this) {
        return function(error, result) {
          var rerender;
          if (error != null) {
            console.log(error);
            _this.error = error;
            return _this.render();
          } else {
            rerender = _this.error != null;
            _this.error = null;
            _this.dashboard.set(result);
            if (rerender) {
              return _this.render();
            }
          }
        };
      })(this);
      return this.main_timer = driver.run(query, 5000, dashboard_callback);
    };

    DashboardContainer.prototype.render = function() {
      var _ref;
      if (this.error != null) {
        this.$el.html(this.template.error({
          error: (_ref = this.error) != null ? _ref.message : void 0,
          url: '#'
        }));
      } else {
        this.$el.html(this.dashboard_view.render().$el);
      }
      return this;
    };

    DashboardContainer.prototype.remove = function() {
      driver.stop_timer(this.main_timer);
      if (this.dashboard_view) {
        this.dashboard_view.remove();
      }
      return DashboardContainer.__super__.remove.call(this);
    };

    return DashboardContainer;

  })(Backbone.View);
  this.DashboardMainView = (function(_super) {
    __extends(DashboardMainView, _super);

    function DashboardMainView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return DashboardMainView.__super__.constructor.apply(this, arguments);
    }

    DashboardMainView.prototype.template = Handlebars.templates['dashboard_view-template'];

    DashboardMainView.prototype.id = 'dashboard_main_view';

    DashboardMainView.prototype.events = {
      'click .view-logs': 'show_all_logs'
    };

    DashboardMainView.prototype.initialize = function() {
      this.cluster_status_availability = new DashboardView.ClusterStatusAvailability({
        model: this.model
      });
      this.cluster_status_redundancy = new DashboardView.ClusterStatusRedundancy({
        model: this.model
      });
      this.cluster_status_connectivity = new DashboardView.ClusterStatusConnectivity({
        model: this.model
      });
      this.cluster_status_sindexes = new DashboardView.ClusterStatusSindexes({
        model: this.model
      });
      this.stats = new Stats;
      this.stats_timer = driver.run(r.db(system_db).table('stats').get(['cluster'])["do"](function(stat) {
        return {
          keys_read: stat('query_engine')('read_docs_per_sec'),
          keys_set: stat('query_engine')('written_docs_per_sec')
        };
      }), 1000, this.stats.on_result);
      this.cluster_performance = new Vis.OpsPlot(this.stats.get_stats, {
        width: 833,
        height: 300,
        seconds: 119,
        type: 'cluster'
      });
      return this.logs = new LogView.LogsContainer({
        limit: 5,
        query: driver.queries.all_logs
      });
    };

    DashboardMainView.prototype.show_all_logs = function() {
      return main_view.router.navigate('#logs', {
        trigger: true
      });
    };

    DashboardMainView.prototype.render = function() {
      this.$el.html(this.template({}));
      this.$('.availability').html(this.cluster_status_availability.render().$el);
      this.$('.redundancy').html(this.cluster_status_redundancy.render().$el);
      this.$('.connectivity').html(this.cluster_status_connectivity.render().$el);
      this.$('.sindexes').html(this.cluster_status_sindexes.render().$el);
      this.$('#cluster_performance_container').html(this.cluster_performance.render().$el);
      this.$('.recent-log-entries-container').html(this.logs.render().$el);
      return this;
    };

    DashboardMainView.prototype.remove = function() {
      driver.stop_timer(this.stats_timer);
      this.cluster_status_availability.remove();
      this.cluster_status_redundancy.remove();
      this.cluster_status_connectivity.remove();
      this.cluster_performance.remove();
      this.cluster_status_sindexes.remove();
      this.logs.remove();
      return DashboardMainView.__super__.remove.call(this);
    };

    return DashboardMainView;

  })(Backbone.View);
  this.ClusterStatusAvailability = (function(_super) {
    __extends(ClusterStatusAvailability, _super);

    function ClusterStatusAvailability() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.hide_popup = __bind(this.hide_popup, this);
      this.show_popup = __bind(this.show_popup, this);
      this.initialize = __bind(this.initialize, this);
      return ClusterStatusAvailability.__super__.constructor.apply(this, arguments);
    }

    ClusterStatusAvailability.prototype.className = 'cluster-status-availability ';

    ClusterStatusAvailability.prototype.template = Handlebars.templates['dashboard_availability-template'];

    ClusterStatusAvailability.prototype.events = {
      'click .show_details': 'show_popup',
      'click .close': 'hide_popup'
    };

    ClusterStatusAvailability.prototype.initialize = function() {
      this.listenTo(this.model, 'change:num_primaries', this.render);
      this.listenTo(this.model, 'change:num_connected_primaries', this.render);
      $(window).on('mouseup', this.hide_popup);
      this.$el.on('click', this.stop_propagation);
      this.display_popup = false;
      return this.margin = {};
    };

    ClusterStatusAvailability.prototype.stop_propagation = function(event) {
      return event.stopPropagation();
    };

    ClusterStatusAvailability.prototype.show_popup = function(event) {
      if (event != null) {
        event.preventDefault();
        this.margin.top = event.pageY - 60 - 13;
        this.margin.left = event.pageX + 12;
      }
      this.$('.popup_container').show();
      this.$('.popup_container').css('margin', this.margin.top + 'px 0px 0px ' + this.margin.left + 'px');
      return this.display_popup = true;
    };

    ClusterStatusAvailability.prototype.hide_popup = function(event) {
      event.preventDefault();
      this.display_popup = false;
      return this.$('.popup_container').hide();
    };

    ClusterStatusAvailability.prototype.render = function() {
      var template_model;
      template_model = {
        status_is_ok: this.model.get('num_connected_primaries') === this.model.get('num_primaries'),
        num_primaries: this.model.get('num_primaries'),
        num_connected_primaries: this.model.get('num_connected_primaries'),
        num_disconnected_primaries: this.model.get('num_primaries') - this.model.get('num_connected_primaries'),
        num_disconnected_tables: this.model.get('num_disconnected_tables'),
        num_tables_w_missing_replicas: this.model.get('num_tables_w_missing_replicas'),
        num_tables: this.model.get('num_tables'),
        tables_with_primaries_not_ready: this.model.get('tables_with_primaries_not_ready')
      };
      this.$el.html(this.template(template_model));
      if (this.display_popup === true && this.model.get('num_connected_primaries') !== this.model.get('num_primaries')) {
        this.show_popup();
      }
      return this;
    };

    ClusterStatusAvailability.prototype.remove = function() {
      this.stopListening();
      $(window).off('mouseup', this.remove_popup);
      return ClusterStatusAvailability.__super__.remove.call(this);
    };

    return ClusterStatusAvailability;

  })(Backbone.View);
  this.ClusterStatusRedundancy = (function(_super) {
    __extends(ClusterStatusRedundancy, _super);

    function ClusterStatusRedundancy() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.hide_popup = __bind(this.hide_popup, this);
      this.show_popup = __bind(this.show_popup, this);
      this.initialize = __bind(this.initialize, this);
      return ClusterStatusRedundancy.__super__.constructor.apply(this, arguments);
    }

    ClusterStatusRedundancy.prototype.className = 'cluster-status-redundancy';

    ClusterStatusRedundancy.prototype.template = Handlebars.templates['dashboard_redundancy-template'];

    ClusterStatusRedundancy.prototype.events = {
      'click .show_details': 'show_popup',
      'click .close': 'hide_popup'
    };

    ClusterStatusRedundancy.prototype.initialize = function() {
      this.listenTo(this.model, 'change:num_replicas', this.render);
      this.listenTo(this.model, 'change:num_connected_replicas', this.render);
      $(window).on('mouseup', this.hide_popup);
      this.$el.on('click', this.stop_propagation);
      this.display_popup = false;
      return this.margin = {};
    };

    ClusterStatusRedundancy.prototype.stop_propagation = function(event) {
      return event.stopPropagation();
    };

    ClusterStatusRedundancy.prototype.show_popup = function(event) {
      if (event != null) {
        event.preventDefault();
        this.margin.top = event.pageY - 60 - 13;
        this.margin.left = event.pageX + 12;
      }
      this.$('.popup_container').show();
      this.$('.popup_container').css('margin', this.margin.top + 'px 0px 0px ' + this.margin.left + 'px');
      return this.display_popup = true;
    };

    ClusterStatusRedundancy.prototype.hide_popup = function(event) {
      event.preventDefault();
      this.display_popup = false;
      return this.$('.popup_container').hide();
    };

    ClusterStatusRedundancy.prototype.render = function() {
      this.$el.html(this.template({
        status_is_ok: this.model.get('num_connected_replicas') === this.model.get('num_replicas'),
        num_replicas: this.model.get('num_replicas'),
        num_connected_replicas: this.model.get('num_available_replicas'),
        num_disconnected_replicas: this.model.get('num_replicas') - this.model.get('num_connected_replicas'),
        num_disconnected_tables: this.model.get('num_disconnected_tables'),
        num_tables_w_missing_replicas: this.model.get('num_tables_w_missing_replicas'),
        num_tables: this.model.get('num_tables'),
        tables_with_replicas_not_ready: this.model.get('tables_with_replicas_not_ready')
      }));
      if (this.display_popup === true && this.model.get('num_connected_replicas') !== this.model.get('num_replicas')) {
        this.show_popup();
      }
      return this;
    };

    ClusterStatusRedundancy.prototype.remove = function() {
      this.stopListening();
      $(window).off('mouseup', this.remove_popup);
      return ClusterStatusRedundancy.__super__.remove.call(this);
    };

    return ClusterStatusRedundancy;

  })(Backbone.View);
  this.ClusterStatusConnectivity = (function(_super) {
    __extends(ClusterStatusConnectivity, _super);

    function ClusterStatusConnectivity() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.hide_popup = __bind(this.hide_popup, this);
      this.show_popup = __bind(this.show_popup, this);
      this.initialize = __bind(this.initialize, this);
      return ClusterStatusConnectivity.__super__.constructor.apply(this, arguments);
    }

    ClusterStatusConnectivity.prototype.className = 'cluster-status-connectivity ';

    ClusterStatusConnectivity.prototype.template = Handlebars.templates['dashboard_connectivity-template'];

    ClusterStatusConnectivity.prototype.events = {
      'click .show_details': 'show_popup',
      'click .close': 'hide_popup'
    };

    ClusterStatusConnectivity.prototype.initialize = function() {
      this.listenTo(this.model, 'change:num_servers', this.render);
      this.listenTo(this.model, 'change:num_connected_servers', this.render);
      $(window).on('mouseup', this.hide_popup);
      this.$el.on('click', this.stop_propagation);
      this.display_popup = false;
      return this.margin = {};
    };

    ClusterStatusConnectivity.prototype.stop_propagation = function(event) {
      return event.stopPropagation();
    };

    ClusterStatusConnectivity.prototype.show_popup = function(event) {
      if (event != null) {
        event.preventDefault();
        this.margin.top = event.pageY - 60 - 13;
        this.margin.left = event.pageX + 12;
      }
      this.$('.popup_container').show();
      this.$('.popup_container').css('margin', this.margin.top + 'px 0px 0px ' + this.margin.left + 'px');
      return this.display_popup = true;
    };

    ClusterStatusConnectivity.prototype.hide_popup = function(event) {
      event.preventDefault();
      this.display_popup = false;
      return this.$('.popup_container').hide();
    };

    ClusterStatusConnectivity.prototype.render = function() {
      var template_model;
      template_model = {
        status_is_ok: this.model.get('num_connected_servers') === this.model.get('num_servers'),
        num_servers: this.model.get('num_servers'),
        num_disconnected_servers: this.model.get('num_servers') - this.model.get('num_connected_servers'),
        num_connected_servers: this.model.get('num_connected_servers'),
        disconnected_servers: this.model.get('disconnected_servers')
      };
      this.$el.html(this.template(template_model));
      if (this.display_popup === true && this.model.get('num_connected_servers') !== this.model.get('num_servers')) {
        this.show_popup();
      }
      return this;
    };

    ClusterStatusConnectivity.prototype.remove = function() {
      this.stopListening();
      $(window).off('mouseup', this.remove_popup);
      return ClusterStatusConnectivity.__super__.remove.call(this);
    };

    return ClusterStatusConnectivity;

  })(Backbone.View);
  return this.ClusterStatusSindexes = (function(_super) {
    __extends(ClusterStatusSindexes, _super);

    function ClusterStatusSindexes() {
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ClusterStatusSindexes.__super__.constructor.apply(this, arguments);
    }

    ClusterStatusSindexes.prototype.className = 'cluster-status-sindexes';

    ClusterStatusSindexes.prototype.template = Handlebars.templates['dashboard_sindexes'];

    ClusterStatusSindexes.prototype.initialize = function() {
      this.listenTo(this.model, 'change:num_sindex_issues', this.render);
      return this.listenTo(this.model, 'change:num_sindexes_constructing', this.render);
    };

    ClusterStatusSindexes.prototype.render = function() {
      var constructing, constructing_class, issue_class, issues, section_class, template_model;
      issues = this.model.get('num_sindex_issues') > 1;
      constructing = this.model.get('num_sindexes_constructing');
      if (issues || constructing) {
        section_class = 'problems-detected';
        issue_class = issues ? 'bad' : 'good';
        constructing_class = constructing ? 'bad' : 'good';
      } else {
        section_class = 'no-problems-detected';
        issue_class = 'good';
        constructing_class = 'good';
      }
      template_model = {
        section_class: section_class,
        issue_class: issue_class,
        constructing_class: constructing_class,
        num_sindex_issues: this.model.get('num_sindex_issues'),
        num_sindexes_constructing: this.model.get('num_sindexes_constructing')
      };
      this.$el.html(this.template(template_model));
      return this;
    };

    return ClusterStatusSindexes;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('DataExplorerView', function() {
  var DriverHandler, HistoryView, OptionsView, ProfileView, QueryResult, RawView, ResultView, ResultViewWrapper, TableView, TreeView, Utils;
  this.defaults = {
    current_query: null,
    query_result: null,
    cursor_timed_out: true,
    view: 'tree',
    history_state: 'hidden',
    last_keys: [],
    last_columns_size: {},
    options_state: 'hidden',
    options: {
      suggestions: true,
      electric_punctuation: false,
      profiler: false,
      query_limit: 40
    },
    history: [],
    focus_on_codemirror: true
  };
  Object.freeze(this.defaults);
  Object.freeze(this.defaults.options);
  this.state = _.extend({}, this.defaults);
  QueryResult = (function() {
    _.extend(QueryResult.prototype, Backbone.Events);

    function QueryResult(options) {
      this.slice = __bind(this.slice, this);
      this.drop_before = __bind(this.drop_before, this);
      this.force_end_gracefully = __bind(this.force_end_gracefully, this);
      this.size = __bind(this.size, this);
      this.set_error = __bind(this.set_error, this);
      this.fetch_next = __bind(this.fetch_next, this);
      this.discard = __bind(this.discard, this);
      this.set = __bind(this.set, this);
      var event, handler, _ref;
      this.has_profile = options.has_profile;
      this.current_query = options.current_query;
      this.raw_query = options.raw_query;
      this.driver_handler = options.driver_handler;
      this.ready = false;
      this.position = 0;
      if (options.events != null) {
        _ref = options.events;
        for (event in _ref) {
          if (!__hasProp.call(_ref, event)) continue;
          handler = _ref[event];
          this.on(event, handler);
        }
      }
    }

    QueryResult.prototype.set = function(error, result) {
      var value;
      if (error != null) {
        return this.set_error(error);
      } else if (!this.discard_results) {
        if (this.has_profile) {
          this.profile = result.profile;
          value = result.value;
        } else {
          this.profile = null;
          value = result;
        }
        if ((value != null) && typeof value._next === 'function' && !(value instanceof Array)) {
          this.type = 'cursor';
          this.results = [];
          this.results_offset = 0;
          this.cursor = value;
          this.is_feed = this.cursor.toString() !== '[object Cursor]';
          this.missing = 0;
          this.ended = false;
        } else {
          this.type = 'value';
          this.value = value;
          this.ended = true;
        }
        this.ready = true;
        return this.trigger('ready', this);
      }
    };

    QueryResult.prototype.discard = function() {
      var _base, _ref;
      this.trigger('discard', this);
      this.off();
      this.type = 'discarded';
      this.discard_results = true;
      delete this.profile;
      delete this.value;
      delete this.results;
      delete this.results_offset;
      if ((_ref = this.cursor) != null) {
        if (typeof (_base = _ref.close())["catch"] === "function") {
          _base["catch"](function() {
            return null;
          });
        }
      }
      return delete this.cursor;
    };

    QueryResult.prototype.fetch_next = function() {
      var error;
      if (!this.ended) {
        try {
          return this.driver_handler.cursor_next(this.cursor, {
            end: (function(_this) {
              return function() {
                _this.ended = true;
                return _this.trigger('end', _this);
              };
            })(this),
            error: (function(_this) {
              return function(error) {
                if (!_this.ended) {
                  return _this.set_error(error);
                }
              };
            })(this),
            row: (function(_this) {
              return function(row) {
                if (_this.discard_results) {
                  return;
                }
                _this.results.push(row);
                return _this.trigger('add', _this, row);
              };
            })(this)
          });
        } catch (_error) {
          error = _error;
          return this.set_error(error);
        }
      }
    };

    QueryResult.prototype.set_error = function(error) {
      this.type = 'error';
      this.error = error;
      this.trigger('error', this, error);
      this.discard_results = true;
      return this.ended = true;
    };

    QueryResult.prototype.size = function() {
      switch (this.type) {
        case 'value':
          if (this.value instanceof Array) {
            return this.value.length;
          } else {
            return 1;
          }
          break;
        case 'cursor':
          return this.results.length + this.results_offset;
      }
    };

    QueryResult.prototype.force_end_gracefully = function() {
      var _ref;
      if (this.is_feed) {
        this.ended = true;
        if ((_ref = this.cursor) != null) {
          _ref.close()["catch"](function() {
            return null;
          });
        }
        return this.trigger('end', this);
      }
    };

    QueryResult.prototype.drop_before = function(n) {
      if (n > this.results_offset) {
        this.results = this.results.slice(n - this.results_offset);
        return this.results_offset = n;
      }
    };

    QueryResult.prototype.slice = function(from, to) {
      if (from < 0) {
        from = this.results.length + from;
      } else {
        from = from - this.results_offset;
      }
      from = Math.max(0, from);
      if (to != null) {
        if (to < 0) {
          to = this.results.length + to;
        } else {
          to = to - this.results_offset;
        }
        to = Math.min(this.results.length, to);
        return this.results.slice(from, +to + 1 || 9e9);
      } else {
        return this.results.slice(from);
      }
    };

    return QueryResult;

  })();
  this.Container = (function(_super) {
    __extends(Container, _super);

    function Container() {
      this.remove = __bind(this.remove, this);
      this.display_full = __bind(this.display_full, this);
      this.display_normal = __bind(this.display_normal, this);
      this.toggle_size = __bind(this.toggle_size, this);
      this.handle_gutter_click = __bind(this.handle_gutter_click, this);
      this.error_on_connect = __bind(this.error_on_connect, this);
      this.clear_query = __bind(this.clear_query, this);
      this.separate_queries = __bind(this.separate_queries, this);
      this.evaluate = __bind(this.evaluate, this);
      this.execute_portion = __bind(this.execute_portion, this);
      this.toggle_executing = __bind(this.toggle_executing, this);
      this.execute_query = __bind(this.execute_query, this);
      this.abort_query = __bind(this.abort_query, this);
      this.extract_database_used = __bind(this.extract_database_used, this);
      this.extend_description = __bind(this.extend_description, this);
      this.mouseout_suggestion = __bind(this.mouseout_suggestion, this);
      this.mouseover_suggestion = __bind(this.mouseover_suggestion, this);
      this.select_suggestion = __bind(this.select_suggestion, this);
      this.write_suggestion = __bind(this.write_suggestion, this);
      this.remove_highlight_suggestion = __bind(this.remove_highlight_suggestion, this);
      this.highlight_suggestion = __bind(this.highlight_suggestion, this);
      this.move_suggestion = __bind(this.move_suggestion, this);
      this.show_or_hide_arrow = __bind(this.show_or_hide_arrow, this);
      this.hide_suggestion_and_description = __bind(this.hide_suggestion_and_description, this);
      this.hide_description = __bind(this.hide_description, this);
      this.hide_suggestion = __bind(this.hide_suggestion, this);
      this.show_description = __bind(this.show_description, this);
      this.show_suggestion_without_moving = __bind(this.show_suggestion_without_moving, this);
      this.show_suggestion = __bind(this.show_suggestion, this);
      this.create_safe_regex = __bind(this.create_safe_regex, this);
      this.create_suggestion = __bind(this.create_suggestion, this);
      this.count_group_level = __bind(this.count_group_level, this);
      this.extract_data_from_query = __bind(this.extract_data_from_query, this);
      this.get_last_key = __bind(this.get_last_key, this);
      this.last_element_type_if_incomplete = __bind(this.last_element_type_if_incomplete, this);
      this.handle_keypress = __bind(this.handle_keypress, this);
      this.count_not_closed_brackets = __bind(this.count_not_closed_brackets, this);
      this.count_char = __bind(this.count_char, this);
      this.move_cursor = __bind(this.move_cursor, this);
      this.remove_next = __bind(this.remove_next, this);
      this.insert_next = __bind(this.insert_next, this);
      this.get_previous_char = __bind(this.get_previous_char, this);
      this.get_next_char = __bind(this.get_next_char, this);
      this.pair_char = __bind(this.pair_char, this);
      this.handle_click = __bind(this.handle_click, this);
      this.on_blur = __bind(this.on_blur, this);
      this.init_after_dom_rendered = __bind(this.init_after_dom_rendered, this);
      this.render = __bind(this.render, this);
      this.handle_mousedown = __bind(this.handle_mousedown, this);
      this.handle_mouseup = __bind(this.handle_mouseup, this);
      this.handle_mousemove = __bind(this.handle_mousemove, this);
      this.fetch_data = __bind(this.fetch_data, this);
      this.initialize = __bind(this.initialize, this);
      this.save_current_query = __bind(this.save_current_query, this);
      this.clear_history = __bind(this.clear_history, this);
      this.save_query = __bind(this.save_query, this);
      this.set_docs = __bind(this.set_docs, this);
      this.set_doc_description = __bind(this.set_doc_description, this);
      this.expand_types = __bind(this.expand_types, this);
      this.convert_type = __bind(this.convert_type, this);
      this.activate_overflow = __bind(this.activate_overflow, this);
      this.deactivate_overflow = __bind(this.deactivate_overflow, this);
      this.adjust_collapsible_panel_height = __bind(this.adjust_collapsible_panel_height, this);
      this.move_arrow = __bind(this.move_arrow, this);
      this.hide_collapsible_panel = __bind(this.hide_collapsible_panel, this);
      this.toggle_options = __bind(this.toggle_options, this);
      this.toggle_history = __bind(this.toggle_history, this);
      this.toggle_pressed_buttons = __bind(this.toggle_pressed_buttons, this);
      this.clear_history_view = __bind(this.clear_history_view, this);
      this.start_resize_history = __bind(this.start_resize_history, this);
      this.mouse_up_description = __bind(this.mouse_up_description, this);
      this.stop_propagation = __bind(this.stop_propagation, this);
      this.mouse_down_description = __bind(this.mouse_down_description, this);
      return Container.__super__.constructor.apply(this, arguments);
    }

    Container.prototype.id = 'dataexplorer';

    Container.prototype.template = Handlebars.templates['dataexplorer_view-template'];

    Container.prototype.input_query_template = Handlebars.templates['dataexplorer_input_query-template'];

    Container.prototype.description_template = Handlebars.templates['dataexplorer-description-template'];

    Container.prototype.template_suggestion_name = Handlebars.templates['dataexplorer_suggestion_name_li-template'];

    Container.prototype.description_with_example_template = Handlebars.templates['dataexplorer-description_with_example-template'];

    Container.prototype.alert_connection_fail_template = Handlebars.templates['alert-connection_fail-template'];

    Container.prototype.databases_suggestions_template = Handlebars.templates['dataexplorer-databases_suggestions-template'];

    Container.prototype.tables_suggestions_template = Handlebars.templates['dataexplorer-tables_suggestions-template'];

    Container.prototype.reason_dataexplorer_broken_template = Handlebars.templates['dataexplorer-reason_broken-template'];

    Container.prototype.query_error_template = Handlebars.templates['dataexplorer-query_error-template'];

    Container.prototype.line_height = 13;

    Container.prototype.size_history = 50;

    Container.prototype.max_size_stack = 100;

    Container.prototype.max_size_query = 1000;

    Container.prototype.delay_toggle_abort = 70;

    Container.prototype.events = {
      'mouseup .CodeMirror': 'handle_click',
      'mousedown .suggestion_name_li': 'select_suggestion',
      'mouseover .suggestion_name_li': 'mouseover_suggestion',
      'mouseout .suggestion_name_li': 'mouseout_suggestion',
      'click .clear_query': 'clear_query',
      'click .execute_query': 'execute_query',
      'click .abort_query': 'abort_query',
      'click .change_size': 'toggle_size',
      'click #rerun_query': 'execute_query',
      'click .close': 'close_alert',
      'click .clear_queries_link': 'clear_history_view',
      'click .close_queries_link': 'toggle_history',
      'click .toggle_options_link': 'toggle_options',
      'mousedown .nano_border_bottom': 'start_resize_history',
      'mousedown .suggestion_description': 'mouse_down_description',
      'click .suggestion_description': 'stop_propagation',
      'mouseup .suggestion_description': 'mouse_up_description',
      'mousedown .suggestion_full_container': 'mouse_down_description',
      'click .suggestion_full_container': 'stop_propagation',
      'mousedown .CodeMirror': 'mouse_down_description',
      'click .CodeMirror': 'stop_propagation'
    };

    Container.prototype.mouse_down_description = function(event) {
      this.keep_suggestions_on_blur = true;
      return this.stop_propagation(event);
    };

    Container.prototype.stop_propagation = function(event) {
      return event.stopPropagation();
    };

    Container.prototype.mouse_up_description = function(event) {
      this.keep_suggestions_on_blur = false;
      return this.stop_propagation(event);
    };

    Container.prototype.start_resize_history = function(event) {
      return this.history_view.start_resize(event);
    };

    Container.prototype.clear_history_view = function(event) {
      var that;
      that = this;
      this.clear_history();
      return this.history_view.clear_history(event);
    };

    Container.prototype.toggle_pressed_buttons = function() {
      if (this.history_view.state === 'visible') {
        this.state.history_state = 'visible';
        this.$('.clear_queries_link').fadeIn('fast');
        this.$('.close_queries_link').addClass('active');
      } else {
        this.state.history_state = 'hidden';
        this.$('.clear_queries_link').fadeOut('fast');
        this.$('.close_queries_link').removeClass('active');
      }
      if (this.options_view.state === 'visible') {
        this.state.options_state = 'visible';
        return this.$('.toggle_options_link').addClass('active');
      } else {
        this.state.options_state = 'hidden';
        return this.$('.toggle_options_link').removeClass('active');
      }
    };

    Container.prototype.toggle_history = function(args) {
      var that;
      that = this;
      this.deactivate_overflow();
      if (args.no_animation === true) {
        this.history_view.state = 'visible';
        this.$('.content').html(this.history_view.render(true).$el);
        this.move_arrow({
          type: 'history',
          move_arrow: 'show'
        });
        this.adjust_collapsible_panel_height({
          no_animation: true,
          is_at_bottom: true
        });
      } else if (this.options_view.state === 'visible') {
        this.options_view.state = 'hidden';
        this.move_arrow({
          type: 'history',
          move_arrow: 'animate'
        });
        this.options_view.$el.fadeOut('fast', function() {
          that.$('.content').html(that.history_view.render(false).$el);
          that.history_view.state = 'visible';
          that.history_view.$el.fadeIn('fast');
          that.adjust_collapsible_panel_height({
            is_at_bottom: true
          });
          return that.toggle_pressed_buttons();
        });
      } else if (this.history_view.state === 'hidden') {
        this.history_view.state = 'visible';
        this.$('.content').html(this.history_view.render(true).$el);
        this.history_view.delegateEvents();
        this.move_arrow({
          type: 'history',
          move_arrow: 'show'
        });
        this.adjust_collapsible_panel_height({
          is_at_bottom: true
        });
      } else if (this.history_view.state === 'visible') {
        this.history_view.state = 'hidden';
        this.hide_collapsible_panel('history');
      }
      return this.toggle_pressed_buttons();
    };

    Container.prototype.toggle_options = function(args) {
      var that;
      that = this;
      this.deactivate_overflow();
      if ((args != null ? args.no_animation : void 0) === true) {
        this.options_view.state = 'visible';
        this.$('.content').html(this.options_view.render(true).$el);
        this.options_view.delegateEvents();
        this.move_arrow({
          type: 'options',
          move_arrow: 'show'
        });
        this.adjust_collapsible_panel_height({
          no_animation: true,
          is_at_bottom: true
        });
      } else if (this.history_view.state === 'visible') {
        this.history_view.state = 'hidden';
        this.move_arrow({
          type: 'options',
          move_arrow: 'animate'
        });
        this.history_view.$el.fadeOut('fast', function() {
          that.$('.content').html(that.options_view.render(false).$el);
          that.options_view.state = 'visible';
          that.options_view.$el.fadeIn('fast');
          that.adjust_collapsible_panel_height();
          that.toggle_pressed_buttons();
          that.$('.profiler_enabled').css('visibility', 'hidden');
          return that.$('.profiler_enabled').hide();
        });
      } else if (this.options_view.state === 'hidden') {
        this.options_view.state = 'visible';
        this.$('.content').html(this.options_view.render(true).$el);
        this.options_view.delegateEvents();
        this.move_arrow({
          type: 'options',
          move_arrow: 'show'
        });
        this.adjust_collapsible_panel_height({
          cb: args != null ? args.cb : void 0
        });
      } else if (this.options_view.state === 'visible') {
        this.options_view.state = 'hidden';
        this.hide_collapsible_panel('options');
      }
      return this.toggle_pressed_buttons();
    };

    Container.prototype.hide_collapsible_panel = function(type) {
      var that;
      that = this;
      this.deactivate_overflow();
      this.$('.nano').animate({
        height: 0
      }, 200, function() {
        that.activate_overflow();
        if ((type === 'history' && that.history_view.state === 'hidden') || (type === 'options' && that.options_view.state === 'hidden')) {
          that.$('.nano_border').hide();
          that.$('.arrow_dataexplorer').hide();
          return that.$(this).css('visibility', 'hidden');
        }
      });
      this.$('.nano_border').slideUp('fast');
      return this.$('.arrow_dataexplorer').slideUp('fast');
    };

    Container.prototype.move_arrow = function(args) {
      var margin_right;
      if (args.type === 'options') {
        margin_right = 74;
      } else if (args.type === 'history') {
        margin_right = 154;
      }
      if (args.move_arrow === 'show') {
        this.$('.arrow_dataexplorer').css('margin-right', margin_right);
        this.$('.arrow_dataexplorer').show();
      } else if (args.move_arrow === 'animate') {
        this.$('.arrow_dataexplorer').animate({
          'margin-right': margin_right
        }, 200);
      }
      return this.$('.nano_border').show();
    };

    Container.prototype.adjust_collapsible_panel_height = function(args) {
      var duration, size, that;
      that = this;
      if ((args != null ? args.size : void 0) != null) {
        size = args.size;
      } else {
        if ((args != null ? args.extra : void 0) != null) {
          size = Math.min(this.$('.content > div').height() + args.extra, this.history_view.height_history);
        } else {
          size = Math.min(this.$('.content > div').height(), this.history_view.height_history);
        }
      }
      this.deactivate_overflow();
      duration = Math.max(150, size);
      duration = Math.min(duration, 250);
      this.$('.nano').css('visibility', 'visible');
      if ((args != null ? args.no_animation : void 0) === true) {
        this.$('.nano').height(size);
        this.$('.nano > .content').scrollTop(this.$('.nano > .content > div').height());
        this.$('.nano').css('visibility', 'visible');
        this.$('.arrow_dataexplorer').show();
        this.$('.nano_border').show();
        if ((args != null ? args.no_animation : void 0) === true) {
          this.$('.nano').nanoScroller({
            preventPageScrolling: true
          });
        }
        return this.activate_overflow();
      } else {
        this.$('.nano').animate({
          height: size
        }, duration, function() {
          that.$(this).css('visibility', 'visible');
          that.$('.arrow_dataexplorer').show();
          that.$('.nano_border').show();
          that.$(this).nanoScroller({
            preventPageScrolling: true
          });
          that.activate_overflow();
          if ((args != null) && args.delay_scroll === true && args.is_at_bottom === true) {
            that.$('.nano > .content').animate({
              scrollTop: that.$('.nano > .content > div').height()
            }, 200);
          }
          if ((args != null ? args.cb : void 0) != null) {
            return args.cb();
          }
        });
        if ((args != null) && args.delay_scroll !== true && args.is_at_bottom === true) {
          return that.$('.nano > .content').animate({
            scrollTop: that.$('.nano > .content > div').height()
          }, 200);
        }
      }
    };

    Container.prototype.deactivate_overflow = function() {
      if ($(window).height() >= $(document).height()) {
        return $('body').css('overflow', 'hidden');
      }
    };

    Container.prototype.activate_overflow = function() {
      return $('body').css('overflow', 'auto');
    };

    Container.prototype.displaying_full_view = false;

    Container.prototype.close_alert = function(event) {
      event.preventDefault();
      return $(event.currentTarget).parent().slideUp('fast', function() {
        return $(this).remove();
      });
    };

    Container.prototype.map_state = {
      '': ''
    };

    Container.prototype.descriptions = {};

    Container.prototype.suggestions = {};

    Container.prototype.types = {
      value: ['number', 'bool', 'string', 'array', 'object', 'time', 'binary', 'line', 'point', 'polygon'],
      any: ['number', 'bool', 'string', 'array', 'object', 'stream', 'selection', 'table', 'db', 'r', 'error', 'binary', 'line', 'point', 'polygon'],
      geometry: ['line', 'point', 'polygon'],
      sequence: ['table', 'selection', 'stream', 'array'],
      grouped_stream: ['stream', 'array']
    };

    Container.prototype.convert_type = function(type) {
      if (this.types[type] != null) {
        return this.types[type];
      } else {
        return [type];
      }
    };

    Container.prototype.expand_types = function(ar) {
      var element, result, _i, _len;
      result = [];
      if (_.isArray(ar)) {
        for (_i = 0, _len = ar.length; _i < _len; _i++) {
          element = ar[_i];
          result.concat(this.convert_type(element));
        }
      } else {
        result.concat(this.convert_type(element));
      }
      return result;
    };

    Container.prototype.set_doc_description = function(command, tag, suggestions) {
      var dont_need_parenthesis, full_tag, pair, parent_value, parent_values, parents, return_values, returns, _i, _j, _len, _len1, _ref, _ref1;
      if (command['body'] != null) {
        if (tag === 'getField') {
          dont_need_parenthesis = false;
          full_tag = tag + '(';
        } else {
          dont_need_parenthesis = !(new RegExp(tag + '\\(')).test(command['body']);
          if (dont_need_parenthesis) {
            full_tag = tag;
          } else {
            full_tag = tag + '(';
          }
        }
        this.descriptions[full_tag] = (function(_this) {
          return function(grouped_data) {
            var _ref;
            return {
              name: tag,
              args: (_ref = /.*(\(.*\))/.exec(command['body'])) != null ? _ref[1] : void 0,
              description: _this.description_with_example_template({
                description: command['description'],
                example: command['example'],
                grouped_data: grouped_data === true && full_tag !== 'group(' && full_tag !== 'ungroup('
              })
            };
          };
        })(this);
      }
      parents = {};
      returns = [];
      _ref1 = (_ref = command.io) != null ? _ref : [];
      for (_i = 0, _len = _ref1.length; _i < _len; _i++) {
        pair = _ref1[_i];
        parent_values = pair[0] === null ? '' : pair[0];
        return_values = pair[1];
        parent_values = this.convert_type(parent_values);
        return_values = this.convert_type(return_values);
        returns = returns.concat(return_values);
        for (_j = 0, _len1 = parent_values.length; _j < _len1; _j++) {
          parent_value = parent_values[_j];
          parents[parent_value] = true;
        }
      }
      if (full_tag !== '(') {
        for (parent_value in parents) {
          if (suggestions[parent_value] == null) {
            suggestions[parent_value] = [];
          }
          suggestions[parent_value].push(full_tag);
        }
      }
      return this.map_state[full_tag] = returns;
    };

    Container.prototype.ignored_commands = {
      'connect': true,
      'close': true,
      'reconnect': true,
      'use': true,
      'runp': true,
      'next': true,
      'collect': true,
      'run': true,
      'EventEmitter\'s methods': true
    };

    Container.prototype.set_docs = function(data) {
      var command, key, relations, state, tag;
      for (key in data) {
        command = data[key];
        tag = command['name'];
        if (tag in this.ignored_commands) {
          continue;
        }
        if (tag === '()') {
          tag = '';
          this.set_doc_description(command, tag, this.suggestions);
          tag = 'getField';
        } else if (tag === 'toJsonString, toJSON') {
          tag = 'toJsonString';
          this.set_doc_description(command, tag, this.suggestions);
          tag = 'toJSON';
        }
        this.set_doc_description(command, tag, this.suggestions);
      }
      relations = data['types'];
      for (state in this.suggestions) {
        this.suggestions[state].sort();
      }
      if (DataExplorerView.Container.prototype.focus_on_codemirror === true) {
        return window.router.current_view.handle_keypress();
      }
    };

    Container.prototype.save_query = function(args) {
      var broken_query, query;
      query = args.query;
      broken_query = args.broken_query;
      query = query.replace(/^\s*$[\n\r]{1,}/gm, '');
      query = query.replace(/\s*$/, '');
      if (window.localStorage != null) {
        if (this.state.history.length === 0 || this.state.history[this.state.history.length - 1].query !== query && this.regex.white.test(query) === false) {
          this.state.history.push({
            query: query,
            broken_query: broken_query
          });
          if (this.state.history.length > this.size_history) {
            window.localStorage.rethinkdb_history = JSON.stringify(this.state.history.slice(this.state.history.length - this.size_history));
          } else {
            window.localStorage.rethinkdb_history = JSON.stringify(this.state.history);
          }
          return this.history_view.add_query({
            query: query,
            broken_query: broken_query
          });
        }
      }
    };

    Container.prototype.clear_history = function() {
      this.state.history.length = 0;
      return window.localStorage.rethinkdb_history = JSON.stringify(this.state.history);
    };

    Container.prototype.save_current_query = function() {
      if (window.localStorage != null) {
        return window.localStorage.current_query = JSON.stringify(this.codemirror.getValue());
      }
    };

    Container.prototype.initialize = function(args) {
      var err, _ref, _ref1, _ref2;
      this.TermBaseConstructor = r.expr(1).constructor.__super__.constructor.__super__.constructor;
      this.state = args.state;
      this.executing = false;
      if (window.localStorage != null) {
        try {
          this.state.options = JSON.parse(window.localStorage.options);
          _.defaults(this.state.options, DataExplorerView.defaults.options);
        } catch (_error) {
          err = _error;
          window.localStorage.removeItem('options');
        }
        window.localStorage.options = JSON.stringify(this.state.options);
      }
      if (typeof ((_ref = window.localStorage) != null ? _ref.current_query : void 0) === 'string') {
        try {
          this.state.current_query = JSON.parse(window.localStorage.current_query);
        } catch (_error) {
          err = _error;
          window.localStorage.removeItem('current_query');
        }
      }
      if (((_ref1 = window.localStorage) != null ? _ref1.rethinkdb_history : void 0) != null) {
        try {
          this.state.history = JSON.parse(window.localStorage.rethinkdb_history);
        } catch (_error) {
          err = _error;
          window.localStorage.removeItem('rethinkdb_history');
        }
      }
      this.query_has_changed = ((_ref2 = this.state.query_result) != null ? _ref2.current_query : void 0) !== this.state.current_query;
      this.history_displayed_id = 0;
      this.unsafe_to_safe_regexstr = [[/\\/g, '\\\\'], [/\(/g, '\\('], [/\)/g, '\\)'], [/\^/g, '\\^'], [/\$/g, '\\$'], [/\*/g, '\\*'], [/\+/g, '\\+'], [/\?/g, '\\?'], [/\./g, '\\.'], [/\|/g, '\\|'], [/\{/g, '\\{'], [/\}/g, '\\}'], [/\[/g, '\\[']];
      this.results_view_wrapper = new ResultViewWrapper({
        container: this,
        view: this.state.view
      });
      this.options_view = new OptionsView({
        container: this,
        options: this.state.options
      });
      this.history_view = new HistoryView({
        container: this,
        history: this.state.history
      });
      this.driver_handler = new DriverHandler({
        container: this
      });
      $(window).mousemove(this.handle_mousemove);
      $(window).mouseup(this.handle_mouseup);
      $(window).mousedown(this.handle_mousedown);
      this.keep_suggestions_on_blur = false;
      this.databases_available = {};
      return this.fetch_data();
    };

    Container.prototype.fetch_data = function() {
      var query;
      query = r.db(system_db).table('table_config').pluck('db', 'name').group('db').ungroup().map(function(group) {
        return [
          group("group"), group("reduction")("name").orderBy(function(x) {
            return x;
          })
        ];
      }).coerceTo("OBJECT").merge(r.object(system_db, r.db(system_db).tableList().coerceTo("ARRAY")));
      return this.timer = driver.run(query, 5000, (function(_this) {
        return function(error, result) {
          if (error != null) {
            console.log("Error: Could not fetch databases and tables");
            return console.log(error);
          } else {
            return _this.databases_available = result;
          }
        };
      })(this));
    };

    Container.prototype.handle_mousemove = function(event) {
      this.results_view_wrapper.handle_mousemove(event);
      return this.history_view.handle_mousemove(event);
    };

    Container.prototype.handle_mouseup = function(event) {
      this.results_view_wrapper.handle_mouseup(event);
      return this.history_view.handle_mouseup(event);
    };

    Container.prototype.handle_mousedown = function(event) {
      this.keep_suggestions_on_blur = false;
      return this.hide_suggestion_and_description();
    };

    Container.prototype.render = function() {
      var _ref;
      this.$el.html(this.template());
      this.$('.input_query_full_container').html(this.input_query_template());
      if ((typeof navigator !== "undefined" && navigator !== null ? navigator.appName : void 0) === 'Microsoft Internet Explorer') {
        this.$('.reason_dataexplorer_broken').html(this.reason_dataexplorer_broken_template({
          is_internet_explorer: true
        }));
        this.$('.reason_dataexplorer_broken').slideDown('fast');
        this.$('.button_query').prop('disabled', true);
      } else if ((typeof DataView === "undefined" || DataView === null) || (typeof Uint8Array === "undefined" || Uint8Array === null)) {
        this.$('.reason_dataexplorer_broken').html(this.reason_dataexplorer_broken_template);
        this.$('.reason_dataexplorer_broken').slideDown('fast');
        this.$('.button_query').prop('disabled', true);
      } else if (window.r == null) {
        this.$('.reason_dataexplorer_broken').html(this.reason_dataexplorer_broken_template({
          no_driver: true
        }));
        this.$('.reason_dataexplorer_broken').slideDown('fast');
        this.$('.button_query').prop('disabled', true);
      }
      if (((_ref = this.state) != null ? _ref.query_result : void 0) != null) {
        this.results_view_wrapper.set_query_result({
          query_result: this.state.query_result
        });
      }
      this.$('.results_container').html(this.results_view_wrapper.render({
        query_has_changed: this.query_has_changed
      }).$el);
      return this;
    };

    Container.prototype.init_after_dom_rendered = function() {
      this.codemirror = CodeMirror.fromTextArea(document.getElementById('input_query'), {
        mode: {
          name: 'javascript'
        },
        onKeyEvent: this.handle_keypress,
        lineNumbers: true,
        lineWrapping: true,
        matchBrackets: true,
        tabSize: 2
      });
      this.codemirror.on('blur', this.on_blur);
      this.codemirror.on('gutterClick', this.handle_gutter_click);
      this.codemirror.setSize('100%', 'auto');
      if (this.state.current_query != null) {
        this.codemirror.setValue(this.state.current_query);
      }
      this.codemirror.focus();
      this.state.focus_on_codemirror = true;
      this.codemirror.setCursor(this.codemirror.lineCount(), 0);
      if (this.codemirror.getValue() === '') {
        this.handle_keypress();
      }
      this.results_view_wrapper.init_after_dom_rendered();
      this.draft = this.codemirror.getValue();
      if (this.state.history_state === 'visible') {
        this.toggle_history({
          no_animation: true
        });
      }
      if (this.state.options_state === 'visible') {
        return this.toggle_options({
          no_animation: true
        });
      }
    };

    Container.prototype.on_blur = function() {
      this.state.focus_on_codemirror = false;
      if (this.keep_suggestions_on_blur === false) {
        return this.hide_suggestion_and_description();
      }
    };

    Container.prototype.current_suggestions = [];

    Container.prototype.current_highlighted_suggestion = -1;

    Container.prototype.current_conpleted_query = '';

    Container.prototype.query_first_part = '';

    Container.prototype.query_last_part = '';

    Container.prototype.mouse_type_event = {
      click: true,
      dblclick: true,
      mousedown: true,
      mouseup: true,
      mouseover: true,
      mouseout: true,
      mousemove: true
    };

    Container.prototype.char_breakers = {
      '.': true,
      '}': true,
      ')': true,
      ',': true,
      ';': true,
      ']': true
    };

    Container.prototype.handle_click = function(event) {
      return this.handle_keypress(null, event);
    };

    Container.prototype.pair_char = function(event, stack) {
      var char_to_insert, last_element_incomplete_type, last_key, next_char, num_not_closed_bracket, num_quote, opening_char, previous_char;
      if ((event != null ? event.which : void 0) != null) {
        if (this.codemirror.getSelection() !== '' && event.type === 'keypress') {
          char_to_insert = String.fromCharCode(event.which);
          if ((char_to_insert != null) && char_to_insert === '"' || char_to_insert === "'") {
            this.codemirror.replaceSelection(char_to_insert + this.codemirror.getSelection() + char_to_insert);
            event.preventDefault();
            return true;
          }
        }
        if (event.which === 8) {
          if (event.type !== 'keydown') {
            return true;
          }
          previous_char = this.get_previous_char();
          if (previous_char === null) {
            return true;
          }
          if (previous_char in this.matching_opening_bracket) {
            next_char = this.get_next_char();
            if (next_char === this.matching_opening_bracket[previous_char]) {
              num_not_closed_bracket = this.count_not_closed_brackets(previous_char);
              if (num_not_closed_bracket <= 0) {
                this.remove_next();
                return true;
              }
            }
          } else if (previous_char === '"' || previous_char === "'") {
            next_char = this.get_next_char();
            if (next_char === previous_char && this.get_previous_char(2) !== '\\') {
              num_quote = this.count_char(char_to_insert);
              if (num_quote % 2 === 0) {
                this.remove_next();
                return true;
              }
            }
          }
          return true;
        }
        if (event.type !== 'keypress') {
          return true;
        }
        char_to_insert = String.fromCharCode(event.which);
        if (char_to_insert != null) {
          if (this.codemirror.getSelection() !== '') {
            if (char_to_insert in this.matching_opening_bracket || char_to_insert in this.matching_closing_bracket) {
              this.codemirror.replaceSelection('');
            } else {
              return true;
            }
          }
          last_element_incomplete_type = this.last_element_type_if_incomplete(stack);
          if (char_to_insert === '"' || char_to_insert === "'") {
            num_quote = this.count_char(char_to_insert);
            next_char = this.get_next_char();
            if (next_char === char_to_insert) {
              if (num_quote % 2 === 0) {
                if (last_element_incomplete_type === 'string' || last_element_incomplete_type === 'object_key') {
                  this.move_cursor(1);
                  event.preventDefault();
                  return true;
                } else {
                  return true;
                }
              } else {
                return true;
              }
            } else {
              if (num_quote % 2 === 0) {
                last_key = this.get_last_key(stack);
                if (last_element_incomplete_type === 'string') {
                  return true;
                } else if (last_element_incomplete_type === 'object_key' && (last_key !== '' && this.create_safe_regex(char_to_insert).test(last_key) === true)) {
                  return true;
                } else {
                  this.insert_next(char_to_insert);
                }
              } else {
                return true;
              }
            }
          } else if (last_element_incomplete_type !== 'string') {
            next_char = this.get_next_char();
            if (char_to_insert in this.matching_opening_bracket) {
              num_not_closed_bracket = this.count_not_closed_brackets(char_to_insert);
              if (num_not_closed_bracket >= 0) {
                this.insert_next(this.matching_opening_bracket[char_to_insert]);
                return true;
              }
              return true;
            } else if (char_to_insert in this.matching_closing_bracket) {
              opening_char = this.matching_closing_bracket[char_to_insert];
              num_not_closed_bracket = this.count_not_closed_brackets(opening_char);
              if (next_char === char_to_insert) {
                if (num_not_closed_bracket <= 0) {
                  this.move_cursor(1);
                  event.preventDefault();
                }
                return true;
              }
            }
          }
        }
      }
      return false;
    };

    Container.prototype.get_next_char = function() {
      var cursor_end;
      cursor_end = this.codemirror.getCursor();
      cursor_end.ch++;
      return this.codemirror.getRange(this.codemirror.getCursor(), cursor_end);
    };

    Container.prototype.get_previous_char = function(less_value) {
      var cursor_end, cursor_start;
      cursor_start = this.codemirror.getCursor();
      cursor_end = this.codemirror.getCursor();
      if (less_value != null) {
        cursor_start.ch -= less_value;
        cursor_end.ch -= less_value - 1;
      } else {
        cursor_start.ch--;
      }
      if (cursor_start.ch < 0) {
        return null;
      }
      return this.codemirror.getRange(cursor_start, cursor_end);
    };

    Container.prototype.insert_next = function(str) {
      this.codemirror.replaceRange(str, this.codemirror.getCursor());
      return this.move_cursor(-1);
    };

    Container.prototype.remove_next = function() {
      var end_cursor;
      end_cursor = this.codemirror.getCursor();
      end_cursor.ch++;
      return this.codemirror.replaceRange('', this.codemirror.getCursor(), end_cursor);
    };

    Container.prototype.move_cursor = function(move_value) {
      var cursor;
      cursor = this.codemirror.getCursor();
      cursor.ch += move_value;
      if (cursor.ch < 0) {
        cursor.ch = 0;
      }
      return this.codemirror.setCursor(cursor);
    };

    Container.prototype.count_char = function(char_to_count) {
      var char, i, is_parsing_string, query, result, result_inline_comment, result_multiple_line_comment, string_delimiter, to_skip, _i, _len;
      query = this.codemirror.getValue();
      is_parsing_string = false;
      to_skip = 0;
      result = 0;
      for (i = _i = 0, _len = query.length; _i < _len; i = ++_i) {
        char = query[i];
        if (to_skip > 0) {
          to_skip--;
          continue;
        }
        if (is_parsing_string === true) {
          if (char === string_delimiter && (query[i - 1] != null) && query[i - 1] !== '\\') {
            is_parsing_string = false;
            if (char === char_to_count) {
              result++;
            }
          }
        } else {
          if (char === char_to_count) {
            result++;
          }
          if (char === '\'' || char === '"') {
            is_parsing_string = true;
            string_delimiter = char;
            continue;
          }
          result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
          if (result_inline_comment != null) {
            to_skip = result_inline_comment[0].length - 1;
            continue;
          }
          result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
          if (result_multiple_line_comment != null) {
            to_skip = result_multiple_line_comment[0].length - 1;
            continue;
          }
        }
      }
      return result;
    };

    Container.prototype.matching_opening_bracket = {
      '(': ')',
      '{': '}',
      '[': ']'
    };

    Container.prototype.matching_closing_bracket = {
      ')': '(',
      '}': '{',
      ']': '['
    };

    Container.prototype.count_not_closed_brackets = function(opening_char) {
      var char, i, is_parsing_string, query, result, result_inline_comment, result_multiple_line_comment, string_delimiter, to_skip, _i, _len;
      query = this.codemirror.getValue();
      is_parsing_string = false;
      to_skip = 0;
      result = 0;
      for (i = _i = 0, _len = query.length; _i < _len; i = ++_i) {
        char = query[i];
        if (to_skip > 0) {
          to_skip--;
          continue;
        }
        if (is_parsing_string === true) {
          if (char === string_delimiter && (query[i - 1] != null) && query[i - 1] !== '\\') {
            is_parsing_string = false;
          }
        } else {
          if (char === opening_char) {
            result++;
          } else if (char === this.matching_opening_bracket[opening_char]) {
            result--;
          }
          if (char === '\'' || char === '"') {
            is_parsing_string = true;
            string_delimiter = char;
            continue;
          }
          result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
          if (result_inline_comment != null) {
            to_skip = result_inline_comment[0].length - 1;
            continue;
          }
          result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
          if (result_multiple_line_comment != null) {
            to_skip = result_multiple_line_comment[0].length - 1;
            continue;
          }
        }
      }
      return result;
    };

    Container.prototype.handle_keypress = function(editor, event) {
      var cached_query, current_name, cursor, end_body, i, index, lines, move_outside, new_extra_suggestions, new_highlighted_suggestion, new_suggestions, next_char, previous_char, query, query_after_cursor, query_before_cursor, regex, result, result_last_char_is_white, result_non_white_char_after_cursor, show_suggestion, stack, start_search, string_delimiter, suggestion, _i, _j, _k, _l, _len, _len1, _len2, _len3, _len4, _m, _ref, _ref1, _ref10, _ref11, _ref12, _ref13, _ref14, _ref2, _ref3, _ref4, _ref5, _ref6, _ref7, _ref8, _ref9;
      if (this.ignored_next_keyup === true) {
        if ((event != null ? event.type : void 0) === 'keyup' && (event != null ? event.which : void 0) !== 9) {
          this.ignored_next_keyup = false;
        }
        return true;
      }
      this.state.focus_on_codemirror = true;
      if ((event != null ? event.type : void 0) === 'mouseup') {
        this.hide_suggestion_and_description();
      }
      this.state.current_query = this.codemirror.getValue();
      this.save_current_query();
      if ((event != null ? event.which : void 0) != null) {
        if (event.type !== 'keydown' && ((event.ctrlKey === true || event.metaKey === true) && event.which === 32)) {
          return true;
        }
        if (event.which === 27 || (event.type === 'keydown' && ((event.ctrlKey === true || event.metaKey === true) && event.which === 32) && (this.$('.suggestion_description').css('display') !== 'none' || this.$('.suggestion_name_list').css('display') !== 'none'))) {
          event.preventDefault();
          this.hide_suggestion_and_description();
          query_before_cursor = this.codemirror.getRange({
            line: 0,
            ch: 0
          }, this.codemirror.getCursor());
          query_after_cursor = this.codemirror.getRange(this.codemirror.getCursor(), {
            line: this.codemirror.lineCount() + 1,
            ch: 0
          });
          stack = this.extract_data_from_query({
            size_stack: 0,
            query: query_before_cursor,
            position: 0
          });
          if (stack === null) {
            this.ignore_tab_keyup = false;
            this.hide_suggestion_and_description();
            return false;
          }
          this.current_highlighted_suggestion = -1;
          this.current_highlighted_extra_suggestion = -1;
          this.$('.suggestion_name_list').empty();
          this.query_last_part = query_after_cursor;
          this.current_suggestions = [];
          this.current_element = '';
          this.current_extra_suggestion = '';
          this.written_suggestion = null;
          this.cursor_for_auto_completion = this.codemirror.getCursor();
          this.description = null;
          result = {
            status: null
          };
          this.create_suggestion({
            stack: stack,
            query: query_before_cursor,
            result: result
          });
          result.suggestions = this.uniq(result.suggestions);
          this.grouped_data = this.count_group_level(stack).count_group > 0;
          if (((_ref = result.suggestions) != null ? _ref.length : void 0) > 0) {
            _ref1 = result.suggestions;
            for (i = _i = 0, _len = _ref1.length; _i < _len; i = ++_i) {
              suggestion = _ref1[i];
              if (suggestion !== 'ungroup(' || this.grouped_data === true) {
                result.suggestions.sort();
                this.current_suggestions.push(suggestion);
                this.$('.suggestion_name_list').append(this.template_suggestion_name({
                  id: i,
                  suggestion: suggestion
                }));
              }
            }
          } else if (result.description != null) {
            this.description = result.description;
          }
          return true;
        } else if (event.which === 13 && (event.shiftKey === false && event.ctrlKey === false && event.metaKey === false)) {
          if (event.type === 'keydown') {
            if (this.current_highlighted_suggestion > -1) {
              event.preventDefault();
              this.handle_keypress();
              return true;
            }
            previous_char = this.get_previous_char();
            if (previous_char in this.matching_opening_bracket) {
              next_char = this.get_next_char();
              if (this.matching_opening_bracket[previous_char] === next_char) {
                cursor = this.codemirror.getCursor();
                this.insert_next('\n');
                this.codemirror.indentLine(cursor.line + 1, 'smart');
                this.codemirror.setCursor(cursor);
                return false;
              }
            }
          }
        } else if ((event.which === 9 && event.ctrlKey === false) || (event.type === 'keydown' && ((event.ctrlKey === true || event.metaKey === true) && event.which === 32) && (this.$('.suggestion_description').css('display') === 'none' && this.$('.suggestion_name_list').css('display') === 'none'))) {
          event.preventDefault();
          if (event.type !== 'keydown') {
            return false;
          } else {
            if (((_ref2 = this.current_suggestions) != null ? _ref2.length : void 0) > 0) {
              if (this.$('.suggestion_name_list').css('display') === 'none') {
                this.show_suggestion();
                return true;
              } else {
                if (this.written_suggestion === null) {
                  cached_query = this.query_first_part + this.current_element + this.query_last_part;
                } else {
                  cached_query = this.query_first_part + this.written_suggestion + this.query_last_part;
                }
                if (cached_query !== this.codemirror.getValue()) {
                  this.current_element = this.codemirror.getValue().slice(this.query_first_part.length, this.codemirror.getValue().length - this.query_last_part.length);
                  regex = this.create_safe_regex(this.current_element);
                  new_suggestions = [];
                  new_highlighted_suggestion = -1;
                  _ref3 = this.current_suggestions;
                  for (index = _j = 0, _len1 = _ref3.length; _j < _len1; index = ++_j) {
                    suggestion = _ref3[index];
                    if (index < this.current_highlighted_suggestion) {
                      new_highlighted_suggestion = new_suggestions.length;
                    }
                    if (regex.test(suggestion) === true) {
                      new_suggestions.push(suggestion);
                    }
                  }
                  this.current_suggestions = new_suggestions;
                  this.current_highlighted_suggestion = new_highlighted_suggestion;
                  if (this.current_suggestions.length > 0) {
                    this.$('.suggestion_name_list').empty();
                    _ref4 = this.current_suggestions;
                    for (i = _k = 0, _len2 = _ref4.length; _k < _len2; i = ++_k) {
                      suggestion = _ref4[i];
                      this.$('.suggestion_name_list').append(this.template_suggestion_name({
                        id: i,
                        suggestion: suggestion
                      }));
                    }
                    this.ignored_next_keyup = true;
                  } else {
                    this.hide_suggestion_and_description();
                  }
                }
                if (event.shiftKey) {
                  this.current_highlighted_suggestion--;
                  if (this.current_highlighted_suggestion < -1) {
                    this.current_highlighted_suggestion = this.current_suggestions.length - 1;
                  } else if (this.current_highlighted_suggestion < 0) {
                    this.show_suggestion_without_moving();
                    this.remove_highlight_suggestion();
                    this.write_suggestion({
                      suggestion_to_write: this.current_element
                    });
                    this.ignore_tab_keyup = true;
                    return true;
                  }
                } else {
                  this.current_highlighted_suggestion++;
                  if (this.current_highlighted_suggestion >= this.current_suggestions.length) {
                    this.show_suggestion_without_moving();
                    this.remove_highlight_suggestion();
                    this.write_suggestion({
                      suggestion_to_write: this.current_element
                    });
                    this.ignore_tab_keyup = true;
                    this.current_highlighted_suggestion = -1;
                    return true;
                  }
                }
                if (this.current_suggestions[this.current_highlighted_suggestion] != null) {
                  this.show_suggestion_without_moving();
                  this.highlight_suggestion(this.current_highlighted_suggestion);
                  this.write_suggestion({
                    suggestion_to_write: this.current_suggestions[this.current_highlighted_suggestion]
                  });
                  this.ignore_tab_keyup = true;
                  return true;
                }
              }
            } else if (this.description != null) {
              if (this.$('.suggestion_description').css('display') === 'none') {
                this.show_description();
                return true;
              }
              if ((this.extra_suggestions != null) && this.extra_suggestions.length > 0 && this.extra_suggestion.start_body === this.extra_suggestion.start_body) {
                if (((_ref5 = this.extra_suggestion) != null ? (_ref6 = _ref5.body) != null ? (_ref7 = _ref6[0]) != null ? _ref7.type : void 0 : void 0 : void 0) === 'string') {
                  if (this.extra_suggestion.body[0].complete === true) {
                    this.extra_suggestions = [];
                  } else {
                    current_name = this.extra_suggestion.body[0].name.replace(/^\s*('|")/, '').replace(/('|")\s*$/, '');
                    regex = this.create_safe_regex(current_name);
                    new_extra_suggestions = [];
                    _ref8 = this.extra_suggestions;
                    for (_l = 0, _len3 = _ref8.length; _l < _len3; _l++) {
                      suggestion = _ref8[_l];
                      if (regex.test(suggestion) === true) {
                        new_extra_suggestions.push(suggestion);
                      }
                    }
                    this.extra_suggestions = new_extra_suggestions;
                  }
                }
                if (this.extra_suggestions.length > 0) {
                  query = this.codemirror.getValue();
                  start_search = this.extra_suggestion.start_body;
                  if (((_ref9 = this.extra_suggestion.body) != null ? (_ref10 = _ref9[0]) != null ? _ref10.name.length : void 0 : void 0) != null) {
                    start_search += this.extra_suggestion.body[0].name.length;
                  }
                  end_body = query.indexOf(')', start_search);
                  this.query_last_part = '';
                  if (end_body !== -1) {
                    this.query_last_part = query.slice(end_body);
                  }
                  this.query_first_part = query.slice(0, this.extra_suggestion.start_body);
                  lines = this.query_first_part.split('\n');
                  if (event.shiftKey === true) {
                    this.current_highlighted_extra_suggestion--;
                  } else {
                    this.current_highlighted_extra_suggestion++;
                  }
                  if (this.current_highlighted_extra_suggestion >= this.extra_suggestions.length) {
                    this.current_highlighted_extra_suggestion = -1;
                  } else if (this.current_highlighted_extra_suggestion < -1) {
                    this.current_highlighted_extra_suggestion = this.extra_suggestions.length - 1;
                  }
                  suggestion = '';
                  if (this.current_highlighted_extra_suggestion === -1) {
                    if (this.current_extra_suggestion != null) {
                      if (/^\s*'/.test(this.current_extra_suggestion) === true) {
                        suggestion = this.current_extra_suggestion + "'";
                      } else if (/^\s*"/.test(this.current_extra_suggestion) === true) {
                        suggestion = this.current_extra_suggestion + '"';
                      }
                    }
                  } else {
                    if (this.state.options.electric_punctuation === false) {
                      move_outside = true;
                    }
                    if (/^\s*'/.test(this.current_extra_suggestion) === true) {
                      string_delimiter = "'";
                    } else if (/^\s*"/.test(this.current_extra_suggestion) === true) {
                      string_delimiter = '"';
                    } else {
                      string_delimiter = "'";
                      move_outside = true;
                    }
                    suggestion = string_delimiter + this.extra_suggestions[this.current_highlighted_extra_suggestion] + string_delimiter;
                  }
                  this.write_suggestion({
                    move_outside: move_outside,
                    suggestion_to_write: suggestion
                  });
                  this.ignore_tab_keyup = true;
                }
              }
            }
          }
        }
        if (event.which === 13 && (event.shiftKey || event.ctrlKey || event.metaKey)) {
          this.hide_suggestion_and_description();
          event.preventDefault();
          if (event.type !== 'keydown') {
            return true;
          }
          this.execute_query();
          return true;
        } else if ((event.ctrlKey || event.metaKey) && event.which === 86 && event.type === 'keydown') {
          this.last_action_is_paste = true;
          this.num_released_keys = 0;
          if (event.metaKey) {
            this.num_released_keys++;
          }
          this.hide_suggestion_and_description();
          return true;
        } else if (event.type === 'keyup' && this.last_action_is_paste === true && (event.which === 17 || event.which === 91)) {
          this.num_released_keys++;
          if (this.num_released_keys === 2) {
            this.last_action_is_paste = false;
          }
          this.hide_suggestion_and_description();
          return true;
        } else if (event.type === 'keyup' && this.last_action_is_paste === true && event.which === 86) {
          this.num_released_keys++;
          if (this.num_released_keys === 2) {
            this.last_action_is_paste = false;
          }
          this.hide_suggestion_and_description();
          return true;
        } else if (event.type === 'keyup' && event.altKey && event.which === 38) {
          if (this.history_displayed_id < this.state.history.length) {
            this.history_displayed_id++;
            this.codemirror.setValue(this.state.history[this.state.history.length - this.history_displayed_id].query);
            event.preventDefault();
            return true;
          }
        } else if (event.type === 'keyup' && event.altKey && event.which === 40) {
          if (this.history_displayed_id > 1) {
            this.history_displayed_id--;
            this.codemirror.setValue(this.state.history[this.state.history.length - this.history_displayed_id].query);
            event.preventDefault();
            return true;
          } else if (this.history_displayed_id === 1) {
            this.history_displayed_id--;
            this.codemirror.setValue(this.draft);
            this.codemirror.setCursor(this.codemirror.lineCount(), 0);
          }
        } else if (event.type === 'keyup' && event.altKey && event.which === 33) {
          this.history_displayed_id = this.state.history.length;
          this.codemirror.setValue(this.state.history[this.state.history.length - this.history_displayed_id].query);
          event.preventDefault();
          return true;
        } else if (event.type === 'keyup' && event.altKey && event.which === 34) {
          this.history_displayed_id = this.history.length;
          this.codemirror.setValue(this.state.history[this.state.history.length - this.history_displayed_id].query);
          this.codemirror.setCursor(this.codemirror.lineCount(), 0);
          event.preventDefault();
          return true;
        }
      }
      if (this.$('.suggestion_name_li_hl').length > 0) {
        if ((event != null ? event.which : void 0) === 13) {
          event.preventDefault();
          this.handle_keypress();
          return true;
        }
      }
      if (this.history_displayed_id !== 0 && (event != null)) {
        if (event.ctrlKey || event.shiftKey || event.altKey || event.which === 16 || event.which === 17 || event.which === 18 || event.which === 20 || (event.which === 91 && event.type !== 'keypress') || event.which === 92 || event.type in this.mouse_type_event) {
          return false;
        }
      }
      if ((event != null) && (event.which === 16 || event.which === 17 || event.which === 18 || event.which === 20 || (event.which === 91 && event.type !== 'keypress') || event.which === 92)) {
        return false;
      }
      if ((event == null) || (event.which !== 37 && event.which !== 38 && event.which !== 39 && event.which !== 40 && event.which !== 33 && event.which !== 34 && event.which !== 35 && event.which !== 36 && event.which !== 0)) {
        this.history_displayed_id = 0;
        this.draft = this.codemirror.getValue();
      }
      if (this.codemirror.getValue().length > this.max_size_query) {
        return void 0;
      }
      query_before_cursor = this.codemirror.getRange({
        line: 0,
        ch: 0
      }, this.codemirror.getCursor());
      query_after_cursor = this.codemirror.getRange(this.codemirror.getCursor(), {
        line: this.codemirror.lineCount() + 1,
        ch: 0
      });
      stack = this.extract_data_from_query({
        size_stack: 0,
        query: query_before_cursor,
        position: 0
      });
      if (stack === null) {
        this.ignore_tab_keyup = false;
        this.hide_suggestion_and_description();
        return false;
      }
      if (this.state.options.electric_punctuation === true) {
        this.pair_char(event, stack);
      }
      if (((event != null ? event.type : void 0) != null) && event.type !== 'keyup' && event.which !== 9 && event.type !== 'mouseup') {
        return false;
      }
      if ((event != null ? event.which : void 0) === 16) {
        return false;
      }
      if (this.ignore_tab_keyup === true && (event != null ? event.which : void 0) === 9) {
        if (event.type === 'keyup') {
          this.ignore_tab_keyup = false;
        }
        return true;
      }
      this.current_highlighted_suggestion = -1;
      this.current_highlighted_extra_suggestion = -1;
      this.$('.suggestion_name_list').empty();
      this.query_last_part = query_after_cursor;
      if (this.codemirror.getSelection() !== '') {
        this.hide_suggestion_and_description();
        if ((event != null) && event.which === 13 && (event.shiftKey || event.ctrlKey || event.metaKey)) {
          this.hide_suggestion_and_description();
          if (event.type !== 'keydown') {
            return true;
          }
          this.execute_query();
          return true;
        }
        if ((event != null ? event.type : void 0) !== 'mouseup') {
          return false;
        } else {
          return true;
        }
      }
      this.current_suggestions = [];
      this.current_element = '';
      this.current_extra_suggestion = '';
      this.written_suggestion = null;
      this.cursor_for_auto_completion = this.codemirror.getCursor();
      this.description = null;
      result = {
        status: null
      };
      result_non_white_char_after_cursor = this.regex.get_first_non_white_char.exec(query_after_cursor);
      if (result_non_white_char_after_cursor !== null && !(((_ref11 = result_non_white_char_after_cursor[1]) != null ? _ref11[0] : void 0) in this.char_breakers || ((_ref12 = result_non_white_char_after_cursor[1]) != null ? _ref12.match(/^((\/\/)|(\/\*))/) : void 0) !== null)) {
        result.status = 'break_and_look_for_description';
        this.hide_suggestion();
      } else {
        result_last_char_is_white = this.regex.last_char_is_white.exec(query_before_cursor[query_before_cursor.length - 1]);
        if (result_last_char_is_white !== null) {
          result.status = 'break_and_look_for_description';
          this.hide_suggestion();
        }
      }
      this.create_suggestion({
        stack: stack,
        query: query_before_cursor,
        result: result
      });
      result.suggestions = this.uniq(result.suggestions);
      this.grouped_data = this.count_group_level(stack).count_group > 0;
      if (((_ref13 = result.suggestions) != null ? _ref13.length : void 0) > 0) {
        show_suggestion = false;
        _ref14 = result.suggestions;
        for (i = _m = 0, _len4 = _ref14.length; _m < _len4; i = ++_m) {
          suggestion = _ref14[i];
          if (suggestion !== 'ungroup(' || this.grouped_data === true) {
            show_suggestion = true;
            this.current_suggestions.push(suggestion);
            this.$('.suggestion_name_list').append(this.template_suggestion_name({
              id: i,
              suggestion: suggestion
            }));
          }
        }
        if (this.state.options.suggestions === true && show_suggestion === true) {
          this.show_suggestion();
        } else {
          this.hide_suggestion();
        }
        this.hide_description();
      } else if (result.description != null) {
        this.hide_suggestion();
        this.description = result.description;
        if (this.state.options.suggestions === true && (event != null ? event.type : void 0) !== 'mouseup') {
          this.show_description();
        } else {
          this.hide_description();
        }
      } else {
        this.hide_suggestion_and_description();
      }
      if ((event != null ? event.which : void 0) === 9) {
        if (this.last_element_type_if_incomplete(stack) !== 'string' && this.regex.white_or_empty.test(this.codemirror.getLine(this.codemirror.getCursor().line).slice(0, this.codemirror.getCursor().ch)) !== true) {
          return true;
        } else {
          return false;
        }
      }
      return true;
    };

    Container.prototype.uniq = function(ar) {
      var element, hash, key, result, _i, _len;
      if ((ar == null) || ar.length === 0) {
        return ar;
      }
      result = [];
      hash = {};
      for (_i = 0, _len = ar.length; _i < _len; _i++) {
        element = ar[_i];
        hash[element] = true;
      }
      for (key in hash) {
        result.push(key);
      }
      result.sort();
      return result;
    };

    Container.prototype.regex = {
      anonymous: /^(\s)*function\s*\(([a-zA-Z0-9,\s]*)\)(\s)*{/,
      loop: /^(\s)*(for|while)(\s)*\(([^\)]*)\)(\s)*{/,
      method: /^(\s)*([a-zA-Z0-9]*)\(/,
      row: /^(\s)*row\(/,
      method_var: /^(\s)*(\d*[a-zA-Z][a-zA-Z0-9]*)\./,
      "return": /^(\s)*return(\s)*/,
      object: /^(\s)*{(\s)*/,
      array: /^(\s)*\[(\s)*/,
      white: /^(\s)+$/,
      white_or_empty: /^(\s)*$/,
      white_replace: /\s/g,
      white_start: /^(\s)+/,
      comma: /^(\s)*,(\s)*/,
      semicolon: /^(\s)*;(\s)*/,
      number: /^[0-9]+\.?[0-9]*/,
      inline_comment: /^(\s)*\/\/.*\n/,
      multiple_line_comment: /^(\s)*\/\*[^(\*\/)]*\*\//,
      get_first_non_white_char: /\s*(\S+)/,
      last_char_is_white: /.*(\s+)$/
    };

    Container.prototype.stop_char = {
      opening: {
        '(': ')',
        '{': '}',
        '[': ']'
      },
      closing: {
        ')': '(',
        '}': '{',
        ']': '['
      }
    };

    Container.prototype.last_element_type_if_incomplete = function(stack) {
      var element;
      if ((stack == null) || stack.length === 0) {
        return '';
      }
      element = stack[stack.length - 1];
      if (element.body != null) {
        return this.last_element_type_if_incomplete(element.body);
      } else {
        if (element.complete === false) {
          return element.type;
        } else {
          return '';
        }
      }
    };

    Container.prototype.get_last_key = function(stack) {
      var element;
      if ((stack == null) || stack.length === 0) {
        return '';
      }
      element = stack[stack.length - 1];
      if (element.body != null) {
        return this.get_last_key(element.body);
      } else {
        if (element.complete === false && (element.key != null)) {
          return element.key;
        } else {
          return '';
        }
      }
    };

    Container.prototype.extract_data_from_query = function(args) {
      var arg, body, body_start, char, context, element, entry_start, i, is_parsing_string, keys_values, list_args, new_context, new_element, new_start, position, position_opening_parenthesis, query, result_inline_comment, result_multiple_line_comment, result_regex, result_regex_row, result_white, size_stack, stack, stack_stop_char, start, string_delimiter, to_skip, _i, _j, _len, _len1, _ref, _ref1, _ref2;
      size_stack = args.size_stack;
      query = args.query;
      context = args.context != null ? DataUtils.deep_copy(args.context) : {};
      position = args.position;
      stack = [];
      element = {
        type: null,
        context: context,
        complete: false
      };
      start = 0;
      is_parsing_string = false;
      to_skip = 0;
      for (i = _i = 0, _len = query.length; _i < _len; i = ++_i) {
        char = query[i];
        if (to_skip > 0) {
          to_skip--;
          continue;
        }
        if (is_parsing_string === true) {
          if (char === string_delimiter && (query[i - 1] != null) && query[i - 1] !== '\\') {
            is_parsing_string = false;
            if (element.type === 'string') {
              element.name = query.slice(start, i + 1);
              element.complete = true;
              stack.push(element);
              size_stack++;
              if (size_stack > this.max_size_stack) {
                return null;
              }
              element = {
                type: null,
                context: context,
                complete: false
              };
              start = i + 1;
            }
          }
        } else {
          if (char === '\'' || char === '"') {
            is_parsing_string = true;
            string_delimiter = char;
            if (element.type === null) {
              element.type = 'string';
              start = i;
            }
            continue;
          }
          if (element.type === null) {
            result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
            if (result_inline_comment != null) {
              to_skip = result_inline_comment[0].length - 1;
              start += result_inline_comment[0].length;
              continue;
            }
            result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
            if (result_multiple_line_comment != null) {
              to_skip = result_multiple_line_comment[0].length - 1;
              start += result_multiple_line_comment[0].length;
              continue;
            }
            if (start === i) {
              result_white = this.regex.white_start.exec(query.slice(i));
              if (result_white != null) {
                to_skip = result_white[0].length - 1;
                start += result_white[0].length;
                continue;
              }
              result_regex = this.regex.anonymous.exec(query.slice(i));
              if (result_regex !== null) {
                element.type = 'anonymous_function';
                list_args = (_ref = result_regex[2]) != null ? _ref.split(',') : void 0;
                element.args = [];
                new_context = DataUtils.deep_copy(context);
                for (_j = 0, _len1 = list_args.length; _j < _len1; _j++) {
                  arg = list_args[_j];
                  arg = arg.replace(/(^\s*)|(\s*$)/gi, "");
                  new_context[arg] = true;
                  element.args.push(arg);
                }
                element.context = new_context;
                to_skip = result_regex[0].length;
                body_start = i + result_regex[0].length;
                stack_stop_char = ['{'];
                continue;
              }
              result_regex = this.regex.loop.exec(query.slice(i));
              if (result_regex !== null) {
                element.type = 'loop';
                element.context = context;
                to_skip = result_regex[0].length;
                body_start = i + result_regex[0].length;
                stack_stop_char = ['{'];
                continue;
              }
              result_regex = this.regex["return"].exec(query.slice(i));
              if (result_regex !== null) {
                element.type = 'return';
                element.complete = true;
                to_skip = result_regex[0].length - 1;
                stack.push(element);
                size_stack++;
                if (size_stack > this.max_size_stack) {
                  return null;
                }
                element = {
                  type: null,
                  context: context,
                  complete: false
                };
                start = i + result_regex[0].length;
                continue;
              }
              result_regex = this.regex.object.exec(query.slice(i));
              if (result_regex !== null) {
                element.type = 'object';
                element.next_key = null;
                element.body = [];
                element.current_key_start = i + result_regex[0].length;
                to_skip = result_regex[0].length - 1;
                stack_stop_char = ['{'];
                continue;
              }
              result_regex = this.regex.array.exec(query.slice(i));
              if (result_regex !== null) {
                element.type = 'array';
                element.next_key = null;
                element.body = [];
                entry_start = i + result_regex[0].length;
                to_skip = result_regex[0].length - 1;
                stack_stop_char = ['['];
                continue;
              }
              if (char === '.') {
                new_start = i + 1;
              } else {
                new_start = i;
              }
              result_regex = this.regex.method.exec(query.slice(new_start));
              if (result_regex !== null) {
                result_regex_row = this.regex.row.exec(query.slice(new_start));
                if (result_regex_row !== null) {
                  position_opening_parenthesis = result_regex_row[0].indexOf('(');
                  element.type = 'function';
                  element.name = 'row';
                  stack.push(element);
                  size_stack++;
                  if (size_stack > this.max_size_stack) {
                    return null;
                  }
                  element = {
                    type: 'function',
                    name: '(',
                    position: position + 3 + 1,
                    context: context,
                    complete: 'false'
                  };
                  stack_stop_char = ['('];
                  start += position_opening_parenthesis;
                  to_skip = result_regex[0].length - 1 + new_start - i;
                  continue;
                } else {
                  if (((_ref1 = stack[stack.length - 1]) != null ? _ref1.type : void 0) === 'function' || ((_ref2 = stack[stack.length - 1]) != null ? _ref2.type : void 0) === 'var') {
                    element.type = 'function';
                    element.name = result_regex[0];
                    element.position = position + new_start;
                    start += new_start - i;
                    to_skip = result_regex[0].length - 1 + new_start - i;
                    stack_stop_char = ['('];
                    continue;
                  } else {
                    position_opening_parenthesis = result_regex[0].indexOf('(');
                    if (position_opening_parenthesis !== -1 && result_regex[0].slice(0, position_opening_parenthesis) in context) {
                      element.real_type = this.types.value;
                      element.type = 'var';
                      element.name = result_regex[0].slice(0, position_opening_parenthesis);
                      stack.push(element);
                      size_stack++;
                      if (size_stack > this.max_size_stack) {
                        return null;
                      }
                      element = {
                        type: 'function',
                        name: '(',
                        position: position + position_opening_parenthesis + 1,
                        context: context,
                        complete: 'false'
                      };
                      stack_stop_char = ['('];
                      start = position_opening_parenthesis;
                      to_skip = result_regex[0].length - 1;
                      continue;
                    }

                    /*
                     * This last condition is a special case for r(expr)
                    else if position_opening_parenthesis isnt -1 and result_regex[0].slice(0, position_opening_parenthesis) is 'r'
                        element.type = 'var'
                        element.name = 'r'
                        element.real_type = @types.value
                        element.position = position+new_start
                        start += new_start-i
                        to_skip = result_regex[0].length-1+new_start-i
                        stack_stop_char = ['(']
                        continue
                     */
                  }
                }
              }
              result_regex = this.regex.method_var.exec(query.slice(new_start));
              if (result_regex !== null) {
                if (result_regex[0].slice(0, result_regex[0].length - 1) in context) {
                  element.type = 'var';
                  element.real_type = this.types.value;
                } else {
                  element.type = 'function';
                }
                element.position = position + new_start;
                element.name = result_regex[0].slice(0, result_regex[0].length - 1).replace(/\s/, '');
                element.complete = true;
                to_skip = element.name.length - 1 + new_start - i;
                stack.push(element);
                size_stack++;
                if (size_stack > this.max_size_stack) {
                  return null;
                }
                element = {
                  type: null,
                  context: context,
                  complete: false
                };
                start = new_start + to_skip + 1;
                start -= new_start - i;
                continue;
              }
              result_regex = this.regex.comma.exec(query.slice(i));
              if (result_regex !== null) {
                element.complete = true;
                stack.push({
                  type: 'separator',
                  complete: true,
                  name: query.slice(i, result_regex[0].length)
                });
                size_stack++;
                if (size_stack > this.max_size_stack) {
                  return null;
                }
                element = {
                  type: null,
                  context: context,
                  complete: false
                };
                start = i + result_regex[0].length - 1 + 1;
                to_skip = result_regex[0].length - 1;
                continue;
              }
              result_regex = this.regex.semicolon.exec(query.slice(i));
              if (result_regex !== null) {
                element.complete = true;
                stack.push({
                  type: 'separator',
                  complete: true,
                  name: query.slice(i, result_regex[0].length)
                });
                size_stack++;
                if (size_stack > this.max_size_stack) {
                  return null;
                }
                element = {
                  type: null,
                  context: context,
                  complete: false
                };
                start = i + result_regex[0].length - 1 + 1;
                to_skip = result_regex[0].length - 1;
                continue;
              }
            } else {
              if (char === ';') {
                start = i + 1;
              }
            }
          } else {
            result_regex = this.regex.comma.exec(query.slice(i));
            if (result_regex !== null && stack_stop_char.length < 1) {
              stack.push({
                type: 'separator',
                complete: true,
                name: query.slice(i, result_regex[0].length),
                position: position + i
              });
              size_stack++;
              if (size_stack > this.max_size_stack) {
                return null;
              }
              element = {
                type: null,
                context: context,
                complete: false
              };
              start = i + result_regex[0].length - 1;
              to_skip = result_regex[0].length - 1;
              continue;
            } else if (element.type === 'anonymous_function') {
              if (char in this.stop_char.opening) {
                stack_stop_char.push(char);
              } else if (char in this.stop_char.closing) {
                if (stack_stop_char[stack_stop_char.length - 1] === this.stop_char.closing[char]) {
                  stack_stop_char.pop();
                  if (stack_stop_char.length === 0) {
                    element.body = this.extract_data_from_query({
                      size_stack: size_stack,
                      query: query.slice(body_start, i),
                      context: element.context,
                      position: position + body_start
                    });
                    if (element.body === null) {
                      return null;
                    }
                    element.complete = true;
                    stack.push(element);
                    size_stack++;
                    if (size_stack > this.max_size_stack) {
                      return null;
                    }
                    element = {
                      type: null,
                      context: context
                    };
                    start = i + 1;
                  }
                }
              }
            } else if (element.type === 'loop') {
              if (char in this.stop_char.opening) {
                stack_stop_char.push(char);
              } else if (char in this.stop_char.closing) {
                if (stack_stop_char[stack_stop_char.length - 1] === this.stop_char.closing[char]) {
                  stack_stop_char.pop();
                  if (stack_stop_char.length === 0) {
                    element.body = this.extract_data_from_query({
                      size_stack: size_stack,
                      query: query.slice(body_start, i),
                      context: element.context,
                      position: position + body_start
                    });
                    if (element.body) {
                      return null;
                    }
                    element.complete = true;
                    stack.push(element);
                    size_stack++;
                    if (size_stack > this.max_size_stack) {
                      return null;
                    }
                    element = {
                      type: null,
                      context: context
                    };
                    start = i + 1;
                  }
                }
              }
            } else if (element.type === 'function') {
              if (char in this.stop_char.opening) {
                stack_stop_char.push(char);
              } else if (char in this.stop_char.closing) {
                if (stack_stop_char[stack_stop_char.length - 1] === this.stop_char.closing[char]) {
                  stack_stop_char.pop();
                  if (stack_stop_char.length === 0) {
                    element.body = this.extract_data_from_query({
                      size_stack: size_stack,
                      query: query.slice(start + element.name.length, i),
                      context: element.context,
                      position: position + start + element.name.length
                    });
                    if (element.body === null) {
                      return null;
                    }
                    element.complete = true;
                    stack.push(element);
                    size_stack++;
                    if (size_stack > this.max_size_stack) {
                      return null;
                    }
                    element = {
                      type: null,
                      context: context
                    };
                    start = i + 1;
                  }
                }
              }
            } else if (element.type === 'object') {
              keys_values = [];
              if (char in this.stop_char.opening) {
                stack_stop_char.push(char);
              } else if (char in this.stop_char.closing) {
                if (stack_stop_char[stack_stop_char.length - 1] === this.stop_char.closing[char]) {
                  stack_stop_char.pop();
                  if (stack_stop_char.length === 0) {
                    if (element.next_key != null) {
                      body = this.extract_data_from_query({
                        size_stack: size_stack,
                        query: query.slice(element.current_value_start, i),
                        context: element.context,
                        position: position + element.current_value_start
                      });
                      if (body === null) {
                        return null;
                      }
                      new_element = {
                        type: 'object_key',
                        key: element.next_key,
                        key_complete: true,
                        complete: false,
                        body: body
                      };
                      element.body[element.body.length - 1] = new_element;
                    }
                    element.next_key = null;
                    element.complete = true;
                    stack.push(element);
                    size_stack++;
                    if (size_stack > this.max_size_stack) {
                      return null;
                    }
                    element = {
                      type: null,
                      context: context
                    };
                    start = i + 1;
                    continue;
                  }
                }
              }
              if (element.next_key == null) {
                if (stack_stop_char.length === 1 && char === ':') {
                  new_element = {
                    type: 'object_key',
                    key: query.slice(element.current_key_start, i),
                    key_complete: true
                  };
                  if (element.body.length === 0) {
                    element.body.push(new_element);
                    size_stack++;
                    if (size_stack > this.max_size_stack) {
                      return null;
                    }
                  } else {
                    element.body[element.body.length - 1] = new_element;
                  }
                  element.next_key = query.slice(element.current_key_start, i);
                  element.current_value_start = i + 1;
                }
              } else {
                result_regex = this.regex.comma.exec(query.slice(i));
                if (stack_stop_char.length === 1 && result_regex !== null) {
                  body = this.extract_data_from_query({
                    size_stack: size_stack,
                    query: query.slice(element.current_value_start, i),
                    context: element.context,
                    position: element.current_value_start
                  });
                  if (body === null) {
                    return null;
                  }
                  new_element = {
                    type: 'object_key',
                    key: element.next_key,
                    key_complete: true,
                    body: body
                  };
                  element.body[element.body.length - 1] = new_element;
                  to_skip = result_regex[0].length - 1;
                  element.next_key = null;
                  element.current_key_start = i + result_regex[0].length;
                }
              }
            } else if (element.type === 'array') {
              if (char in this.stop_char.opening) {
                stack_stop_char.push(char);
              } else if (char in this.stop_char.closing) {
                if (stack_stop_char[stack_stop_char.length - 1] === this.stop_char.closing[char]) {
                  stack_stop_char.pop();
                  if (stack_stop_char.length === 0) {
                    body = this.extract_data_from_query({
                      size_stack: size_stack,
                      query: query.slice(entry_start, i),
                      context: element.context,
                      position: position + entry_start
                    });
                    if (body === null) {
                      return null;
                    }
                    new_element = {
                      type: 'array_entry',
                      complete: true,
                      body: body
                    };
                    if (new_element.body.length > 0) {
                      element.body.push(new_element);
                      size_stack++;
                      if (size_stack > this.max_size_stack) {
                        return null;
                      }
                    }
                    continue;
                  }
                }
              }
              if (stack_stop_char.length === 1 && char === ',') {
                body = this.extract_data_from_query({
                  size_stack: size_stack,
                  query: query.slice(entry_start, i),
                  context: element.context,
                  position: position + entry_start
                });
                if (body === null) {
                  return null;
                }
                new_element = {
                  type: 'array_entry',
                  complete: true,
                  body: body
                };
                if (new_element.body.length > 0) {
                  element.body.push(new_element);
                  size_stack++;
                  if (size_stack > this.max_size_stack) {
                    return null;
                  }
                }
                entry_start = i + 1;
              }
            }
          }
        }
      }
      if (element.type !== null) {
        element.complete = false;
        if (element.type === 'function') {
          element.body = this.extract_data_from_query({
            size_stack: size_stack,
            query: query.slice(start + element.name.length),
            context: element.context,
            position: position + start + element.name.length
          });
          if (element.body === null) {
            return null;
          }
        } else if (element.type === 'anonymous_function') {
          element.body = this.extract_data_from_query({
            size_stack: size_stack,
            query: query.slice(body_start),
            context: element.context,
            position: position + body_start
          });
          if (element.body === null) {
            return null;
          }
        } else if (element.type === 'loop') {
          element.body = this.extract_data_from_query({
            size_stack: size_stack,
            query: query.slice(body_start),
            context: element.context,
            position: position + body_start
          });
          if (element.body === null) {
            return null;
          }
        } else if (element.type === 'string') {
          element.name = query.slice(start);
        } else if (element.type === 'object') {
          if (element.next_key == null) {
            new_element = {
              type: 'object_key',
              key: query.slice(element.current_key_start),
              key_complete: false,
              complete: false
            };
            element.body.push(new_element);
            size_stack++;
            if (size_stack > this.max_size_stack) {
              return null;
            }
            element.next_key = query.slice(element.current_key_start);
          } else {
            body = this.extract_data_from_query({
              size_stack: size_stack,
              query: query.slice(element.current_value_start),
              context: element.context,
              position: position + element.current_value_start
            });
            if (body === null) {
              return null;
            }
            new_element = {
              type: 'object_key',
              key: element.next_key,
              key_complete: true,
              complete: false,
              body: body
            };
            element.body[element.body.length - 1] = new_element;
            element.next_key = null;
          }
        } else if (element.type === 'array') {
          body = this.extract_data_from_query({
            size_stack: size_stack,
            query: query.slice(entry_start),
            context: element.context,
            position: position + entry_start
          });
          if (body === null) {
            return null;
          }
          new_element = {
            type: 'array_entry',
            complete: false,
            body: body
          };
          if (new_element.body.length > 0) {
            element.body.push(new_element);
            size_stack++;
            if (size_stack > this.max_size_stack) {
              return null;
            }
          }
        }
        stack.push(element);
        size_stack++;
        if (size_stack > this.max_size_stack) {
          return null;
        }
      } else if (start !== i) {
        if (query.slice(start) in element.context) {
          element.name = query.slice(start);
          element.type = 'var';
          element.real_type = this.types.value;
          element.complete = true;
        } else if (this.regex.number.test(query.slice(start)) === true) {
          element.type = 'number';
          element.name = query.slice(start);
          element.complete = true;
        } else if (query[start] === '.') {
          element.type = 'function';
          element.position = position + start;
          element.name = query.slice(start + 1);
          element.complete = false;
        } else {
          element.name = query.slice(start);
          element.position = position + start;
          element.complete = false;
        }
        stack.push(element);
        size_stack++;
        if (size_stack > this.max_size_stack) {
          return null;
        }
      }
      return stack;
    };

    Container.prototype.count_group_level = function(stack) {
      var count_group, element, i, parse_body, parse_level, _i, _ref;
      count_group = 0;
      if (stack.length > 0) {
        parse_level = true;
        element = stack[stack.length - 1];
        if ((element.body != null) && element.body.length > 0 && element.complete === false) {
          parse_body = this.count_group_level(element.body);
          count_group += parse_body.count_group;
          parse_level = parse_body.parse_level;
          if (element.body[0].type === 'return') {
            parse_level = false;
          }
          if (element.body[element.body.length - 1].type === 'function') {
            parse_level = false;
          }
        }
        if (parse_level === true) {
          for (i = _i = _ref = stack.length - 1; _i >= 0; i = _i += -1) {
            if (stack[i].type === 'function' && stack[i].name === 'ungroup(') {
              count_group -= 1;
            } else if (stack[i].type === 'function' && stack[i].name === 'group(') {
              count_group += 1;
            }
          }
        }
      }
      return {
        count_group: count_group,
        parse_level: parse_level
      };
    };

    Container.prototype.create_suggestion = function(args) {
      var element, i, query, result, stack, state, suggestion, type, _i, _j, _k, _l, _len, _len1, _len2, _len3, _len4, _m, _n, _ref, _ref1, _ref10, _ref11, _ref12, _ref13, _ref14, _ref15, _ref16, _ref17, _ref2, _ref3, _ref4, _ref5, _ref6, _ref7, _ref8, _ref9, _results;
      stack = args.stack;
      query = args.query;
      result = args.result;
      if (result.status === null && stack.length === 0) {
        result.suggestions = [];
        result.status = 'done';
        this.query_first_part = '';
        if (this.suggestions[''] != null) {
          _ref = this.suggestions[''];
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            suggestion = _ref[_i];
            result.suggestions.push(suggestion);
          }
        }
      }
      _results = [];
      for (i = _j = _ref1 = stack.length - 1; _j >= 0; i = _j += -1) {
        element = stack[i];
        if ((element.body != null) && element.body.length > 0 && element.complete === false) {
          this.create_suggestion({
            stack: element.body,
            query: args != null ? args.query : void 0,
            result: args.result
          });
        }
        if (result.status === 'done') {
          continue;
        }
        if (result.status === null) {
          if (element.complete === true) {
            if (element.type === 'function') {
              if (element.complete === true || element.name === '') {
                result.suggestions = null;
                result.status = 'look_for_description';
                break;
              } else {
                result.suggestions = null;
                result.description = element.name;
                this.extra_suggestion = {
                  start_body: element.position + element.name.length,
                  body: element.body
                };
                if (((_ref2 = element.body) != null ? (_ref3 = _ref2[0]) != null ? (_ref4 = _ref3.name) != null ? _ref4.length : void 0 : void 0 : void 0) != null) {
                  this.cursor_for_auto_completion.ch -= element.body[0].name.length;
                  this.current_extra_suggestion = element.body[0].name;
                }
                result.status = 'done';
              }
            } else if (element.type === 'anonymous_function' || element.type === 'separator' || element.type === 'object' || element.type === 'object_key' || element.type === 'return' || 'element.type' === 'array') {
              result.suggestions = null;
              result.status = 'look_for_description';
              break;
            }
          } else {
            if (element.type === 'function') {
              if (element.body != null) {
                result.suggestions = null;
                result.description = element.name;
                this.extra_suggestion = {
                  start_body: element.position + element.name.length,
                  body: element.body
                };
                if (((_ref5 = element.body) != null ? (_ref6 = _ref5[0]) != null ? (_ref7 = _ref6.name) != null ? _ref7.length : void 0 : void 0 : void 0) != null) {
                  this.cursor_for_auto_completion.ch -= element.body[0].name.length;
                  this.current_extra_suggestion = element.body[0].name;
                }
                result.status = 'done';
                break;
              } else {
                result.suggestions = [];
                result.suggestions_regex = this.create_safe_regex(element.name);
                result.description = null;
                this.query_first_part = query.slice(0, element.position + 1);
                this.current_element = element.name;
                this.cursor_for_auto_completion.ch -= element.name.length;
                this.current_query;
                if (i !== 0) {
                  result.status = 'look_for_state';
                } else {
                  result.state = '';
                }
              }
            } else if (element.type === 'anonymous_function' || element.type === 'object_key' || element.type === 'string' || element.type === 'separator' || element.type === 'array') {
              result.suggestions = null;
              result.status = 'look_for_description';
              break;
            } else if (element.type === null) {
              result.suggestions = [];
              result.status = 'look_for_description';
              break;
            }
          }
        } else if (result.status === 'look_for_description') {
          if (element.type === 'function') {
            result.description = element.name;
            this.extra_suggestion = {
              start_body: element.position + element.name.length,
              body: element.body
            };
            if (((_ref8 = element.body) != null ? (_ref9 = _ref8[0]) != null ? (_ref10 = _ref9.name) != null ? _ref10.length : void 0 : void 0 : void 0) != null) {
              this.cursor_for_auto_completion.ch -= element.body[0].name.length;
              this.current_extra_suggestion = element.body[0].name;
            }
            result.suggestions = null;
            result.status = 'done';
          } else {
            break;
          }
        }
        if (result.status === 'break_and_look_for_description') {
          if (element.type === 'function' && element.complete === false && element.name.indexOf('(') !== -1) {
            result.description = element.name;
            this.extra_suggestion = {
              start_body: element.position + element.name.length,
              body: element.body
            };
            if (((_ref11 = element.body) != null ? (_ref12 = _ref11[0]) != null ? (_ref13 = _ref12.name) != null ? _ref13.length : void 0 : void 0 : void 0) != null) {
              this.cursor_for_auto_completion.ch -= element.body[0].name.length;
              this.current_extra_suggestion = element.body[0].name;
            }
            result.suggestions = null;
            _results.push(result.status = 'done');
          } else {
            if (element.type !== 'function') {
              break;
            } else {
              result.status = 'look_for_description';
              break;
            }
          }
        } else if (result.status === 'look_for_state') {
          if (element.type === 'function' && element.complete === true) {
            result.state = element.name;
            if (this.map_state[element.name] != null) {
              _ref14 = this.map_state[element.name];
              for (_k = 0, _len1 = _ref14.length; _k < _len1; _k++) {
                state = _ref14[_k];
                if (this.suggestions[state] != null) {
                  _ref15 = this.suggestions[state];
                  for (_l = 0, _len2 = _ref15.length; _l < _len2; _l++) {
                    suggestion = _ref15[_l];
                    if (result.suggestions_regex.test(suggestion) === true) {
                      result.suggestions.push(suggestion);
                    }
                  }
                }
              }
            }
            _results.push(result.status = 'done');
          } else if (element.type === 'var' && element.complete === true) {
            result.state = element.real_type;
            _ref16 = result.state;
            for (_m = 0, _len3 = _ref16.length; _m < _len3; _m++) {
              type = _ref16[_m];
              if (this.suggestions[type] != null) {
                _ref17 = this.suggestions[type];
                for (_n = 0, _len4 = _ref17.length; _n < _len4; _n++) {
                  suggestion = _ref17[_n];
                  if (result.suggestions_regex.test(suggestion) === true) {
                    result.suggestions.push(suggestion);
                  }
                }
              }
            }
            _results.push(result.status = 'done');
          } else {
            _results.push(void 0);
          }
        } else {
          _results.push(void 0);
        }
      }
      return _results;
    };

    Container.prototype.create_safe_regex = function(str) {
      var char, _i, _len, _ref;
      _ref = this.unsafe_to_safe_regexstr;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        char = _ref[_i];
        str = str.replace(char[0], char[1]);
      }
      return new RegExp('^(' + str + ')', 'i');
    };

    Container.prototype.show_suggestion = function() {
      var margin;
      this.move_suggestion();
      margin = (parseInt(this.$('.CodeMirror-cursor').css('top').replace('px', '')) + this.line_height) + 'px';
      this.$('.suggestion_full_container').css('margin-top', margin);
      this.$('.arrow').css('margin-top', margin);
      this.$('.suggestion_name_list').show();
      return this.$('.arrow').show();
    };

    Container.prototype.show_suggestion_without_moving = function() {
      this.$('.arrow').show();
      return this.$('.suggestion_name_list').show();
    };

    Container.prototype.show_description = function() {
      var margin;
      if (this.descriptions[this.description] != null) {
        margin = (parseInt(this.$('.CodeMirror-cursor').css('top').replace('px', '')) + this.line_height) + 'px';
        this.$('.suggestion_full_container').css('margin-top', margin);
        this.$('.arrow').css('margin-top', margin);
        this.$('.suggestion_description').html(this.description_template(this.extend_description(this.description)));
        this.$('.suggestion_description').show();
        this.move_suggestion();
        return this.show_or_hide_arrow();
      } else {
        return this.hide_description();
      }
    };

    Container.prototype.hide_suggestion = function() {
      this.$('.suggestion_name_list').hide();
      return this.show_or_hide_arrow();
    };

    Container.prototype.hide_description = function() {
      this.$('.suggestion_description').hide();
      return this.show_or_hide_arrow();
    };

    Container.prototype.hide_suggestion_and_description = function() {
      this.hide_suggestion();
      return this.hide_description();
    };

    Container.prototype.show_or_hide_arrow = function() {
      if (this.$('.suggestion_name_list').css('display') === 'none' && this.$('.suggestion_description').css('display') === 'none') {
        return this.$('.arrow').hide();
      } else {
        return this.$('.arrow').show();
      }
    };

    Container.prototype.move_suggestion = function() {
      var margin_left, margin_left_bloc, max_margin;
      margin_left = parseInt(this.$('.CodeMirror-cursor').css('left').replace('px', '')) + 23;
      this.$('.arrow').css('margin-left', margin_left);
      if (margin_left < 200) {
        return this.$('.suggestion_full_container').css('left', '0px');
      } else {
        max_margin = this.$('.CodeMirror-scroll').width() - 418;
        margin_left_bloc = Math.min(max_margin, Math.floor(margin_left / 200) * 200);
        if (margin_left > max_margin + 418 - 150 - 23) {
          return this.$('.suggestion_full_container').css('left', (max_margin - 34) + 'px');
        } else if (margin_left_bloc > max_margin - 150 - 23) {
          return this.$('.suggestion_full_container').css('left', (max_margin - 34 - 150) + 'px');
        } else {
          return this.$('.suggestion_full_container').css('left', (margin_left_bloc - 100) + 'px');
        }
      }
    };

    Container.prototype.highlight_suggestion = function(id) {
      this.remove_highlight_suggestion();
      this.$('.suggestion_name_li').eq(id).addClass('suggestion_name_li_hl');
      this.$('.suggestion_description').html(this.description_template(this.extend_description(this.current_suggestions[id])));
      return this.$('.suggestion_description').show();
    };

    Container.prototype.remove_highlight_suggestion = function() {
      return this.$('.suggestion_name_li').removeClass('suggestion_name_li_hl');
    };

    Container.prototype.write_suggestion = function(args) {
      var ch, move_outside, suggestion_to_write;
      suggestion_to_write = args.suggestion_to_write;
      move_outside = args.move_outside === true;
      ch = this.cursor_for_auto_completion.ch + suggestion_to_write.length;
      if (this.state.options.electric_punctuation === true) {
        if (suggestion_to_write[suggestion_to_write.length - 1] === '(' && this.count_not_closed_brackets('(') >= 0) {
          this.codemirror.setValue(this.query_first_part + suggestion_to_write + ')' + this.query_last_part);
          this.written_suggestion = suggestion_to_write + ')';
        } else {
          this.codemirror.setValue(this.query_first_part + suggestion_to_write + this.query_last_part);
          this.written_suggestion = suggestion_to_write;
          if ((move_outside === false) && (suggestion_to_write[suggestion_to_write.length - 1] === '"' || suggestion_to_write[suggestion_to_write.length - 1] === "'")) {
            ch--;
          }
        }
        this.codemirror.focus();
        return this.codemirror.setCursor({
          line: this.cursor_for_auto_completion.line,
          ch: ch
        });
      } else {
        this.codemirror.setValue(this.query_first_part + suggestion_to_write + this.query_last_part);
        this.written_suggestion = suggestion_to_write;
        if ((move_outside === false) && (suggestion_to_write[suggestion_to_write.length - 1] === '"' || suggestion_to_write[suggestion_to_write.length - 1] === "'")) {
          ch--;
        }
        this.codemirror.focus();
        return this.codemirror.setCursor({
          line: this.cursor_for_auto_completion.line,
          ch: ch
        });
      }
    };

    Container.prototype.select_suggestion = function(event) {
      var suggestion_to_write;
      suggestion_to_write = this.$(event.target).html();
      this.write_suggestion({
        suggestion_to_write: suggestion_to_write
      });
      this.hide_suggestion();
      return setTimeout((function(_this) {
        return function() {
          _this.handle_keypress();
          return _this.codemirror.focus();
        };
      })(this), 0);
    };

    Container.prototype.mouseover_suggestion = function(event) {
      return this.highlight_suggestion(event.target.dataset.id);
    };

    Container.prototype.mouseout_suggestion = function(event) {
      return this.hide_description();
    };

    Container.prototype.extend_description = function(fn) {
      var data, database_used, databases_available, description;
      if (fn === 'db(' || fn === 'dbDrop(') {
        description = _.extend({}, this.descriptions[fn]());
        if (_.keys(this.databases_available).length === 0) {
          data = {
            no_database: true
          };
        } else {
          databases_available = _.keys(this.databases_available);
          data = {
            no_database: false,
            databases_available: databases_available
          };
        }
        description.description = this.databases_suggestions_template(data) + description.description;
        this.extra_suggestions = databases_available;
      } else if (fn === 'table(' || fn === 'tableDrop(') {
        database_used = this.extract_database_used();
        description = _.extend({}, this.descriptions[fn]());
        if (database_used.error === false) {
          data = {
            tables_available: this.databases_available[database_used.name],
            no_table: this.databases_available[database_used.name].length === 0
          };
          if (database_used.name != null) {
            data.database_name = database_used.name;
          }
        } else {
          data = {
            error: database_used.error
          };
        }
        description.description = this.tables_suggestions_template(data) + description.description;
        this.extra_suggestions = this.databases_available[database_used.name];
      } else {
        description = this.descriptions[fn](this.grouped_data);
        this.extra_suggestions = null;
      }
      return description;
    };

    Container.prototype.extract_database_used = function() {
      var arg, char, db_name, end_arg_position, found, last_db_position, query_before_cursor;
      query_before_cursor = this.codemirror.getRange({
        line: 0,
        ch: 0
      }, this.codemirror.getCursor());
      last_db_position = query_before_cursor.lastIndexOf('.db(');
      if (last_db_position === -1) {
        found = false;
        if (this.databases_available['test'] != null) {
          return {
            db_found: true,
            error: false,
            name: 'test'
          };
        } else {
          return {
            db_found: false,
            error: true
          };
        }
      } else {
        arg = query_before_cursor.slice(last_db_position + 5);
        char = query_before_cursor.slice(last_db_position + 4, last_db_position + 5);
        end_arg_position = arg.indexOf(char);
        if (end_arg_position === -1) {
          return {
            db_found: false,
            error: true
          };
        }
        db_name = arg.slice(0, end_arg_position);
        if (this.databases_available[db_name] != null) {
          return {
            db_found: true,
            error: false,
            name: db_name
          };
        } else {
          return {
            db_found: false,
            error: true
          };
        }
      }
    };

    Container.prototype.abort_query = function() {
      var _ref;
      this.disable_toggle_executing = false;
      this.toggle_executing(false);
      if ((_ref = this.state.query_result) != null) {
        _ref.force_end_gracefully();
      }
      return this.driver_handler.close_connection();
    };

    Container.prototype.execute_query = function() {
      var err, error, _ref;
      if (this.executing === true) {
        this.abort_query;
      }
      this.$('.profiler_enabled').slideUp('fast');
      this.state.cursor_timed_out = false;
      this.state.query_has_changed = false;
      this.raw_query = this.codemirror.getValue();
      this.query = this.clean_query(this.raw_query);
      try {
        if ((_ref = this.state.query_result) != null) {
          _ref.discard();
        }
        this.non_rethinkdb_query = '';
        this.index = 0;
        this.raw_queries = this.separate_queries(this.raw_query);
        this.queries = this.separate_queries(this.query);
        if (this.queries.length === 0) {
          error = this.query_error_template({
            no_query: true
          });
          return this.results_view_wrapper.render_error(null, error, true);
        } else {
          return this.execute_portion();
        }
      } catch (_error) {
        err = _error;
        this.results_view_wrapper.render_error(this.query, err, true);
        return this.save_query({
          query: this.raw_query,
          broken_query: true
        });
      }
    };

    Container.prototype.toggle_executing = function(executing) {
      var _ref;
      if (executing === this.executing) {
        if (executing && ((_ref = this.state.query_result) != null ? _ref.is_feed : void 0)) {
          this.$('.loading_query_img').hide();
        }
        return;
      }
      if (this.disable_toggle_executing) {
        return;
      }
      this.executing = executing;
      if (this.timeout_toggle_abort != null) {
        clearTimeout(this.timeout_toggle_abort);
      }
      if (executing) {
        return this.timeout_toggle_abort = setTimeout((function(_this) {
          return function() {
            var _ref1;
            _this.timeout_toggle_abort = null;
            if (!((_ref1 = _this.state.query_result) != null ? _ref1.is_feed : void 0)) {
              _this.$('.loading_query_img').show();
            }
            _this.$('.execute_query').hide();
            return _this.$('.abort_query').show();
          };
        })(this), this.delay_toggle_abort);
      } else {
        return this.timeout_toggle_abort = setTimeout((function(_this) {
          return function() {
            _this.timeout_toggle_abort = null;
            _this.$('.loading_query_img').hide();
            _this.$('.execute_query').show();
            return _this.$('.abort_query').hide();
          };
        })(this), this.delay_toggle_abort);
      }
    };

    Container.prototype.execute_portion = function() {
      var err, error, final_query, full_query, query_result, rdb_query;
      this.state.query_result = null;
      while (this.queries[this.index] != null) {
        full_query = this.non_rethinkdb_query;
        full_query += this.queries[this.index];
        try {
          rdb_query = this.evaluate(full_query);
        } catch (_error) {
          err = _error;
          if (this.queries.length > 1) {
            this.results_view_wrapper.render_error(this.raw_queries[this.index], err, true);
          } else {
            this.results_view_wrapper.render_error(null, err, true);
          }
          this.save_query({
            query: this.raw_query,
            broken_query: true
          });
          return false;
        }
        this.index++;
        if (rdb_query instanceof this.TermBaseConstructor) {
          final_query = this.index === this.queries.length;
          this.start_time = new Date();
          if (final_query) {
            query_result = new QueryResult({
              has_profile: this.state.options.profiler,
              current_query: this.raw_query,
              raw_query: this.raw_queries[this.index],
              driver_handler: this.driver_handler,
              events: {
                error: (function(_this) {
                  return function(query_result, err) {
                    return _this.results_view_wrapper.render_error(_this.query, err);
                  };
                })(this),
                ready: (function(_this) {
                  return function(query_result) {
                    var event, _i, _len, _ref, _results;
                    _this.state.pause_at = null;
                    if (query_result.is_feed) {
                      _this.toggle_executing(true);
                      _this.disable_toggle_executing = true;
                      _ref = ['end', 'discard', 'error'];
                      _results = [];
                      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
                        event = _ref[_i];
                        _results.push(query_result.on(event, function() {
                          _this.disable_toggle_executing = false;
                          return _this.toggle_executing(false);
                        }));
                      }
                      return _results;
                    }
                  };
                })(this)
              }
            });
            this.state.query_result = query_result;
            this.results_view_wrapper.set_query_result({
              query_result: this.state.query_result
            });
          }
          this.disable_toggle_executing = false;
          this.driver_handler.run_with_new_connection(rdb_query, {
            optargs: {
              binaryFormat: "raw",
              timeFormat: "raw",
              profile: this.state.options.profiler
            },
            connection_error: (function(_this) {
              return function(error) {
                _this.save_query({
                  query: _this.raw_query,
                  broken_query: true
                });
                return _this.error_on_connect(error);
              };
            })(this),
            callback: (function(_this) {
              return function(error, result) {
                if (final_query) {
                  _this.save_query({
                    query: _this.raw_query,
                    broken_query: false
                  });
                  return query_result.set(error, result);
                } else if (error) {
                  _this.save_query({
                    query: _this.raw_query,
                    broken_query: true
                  });
                  return _this.results_view_wrapper.render_error(_this.query, err);
                } else {
                  return _this.execute_portion();
                }
              };
            })(this)
          });
          return true;
        } else {
          this.non_rethinkdb_query += this.queries[this.index - 1];
          if (this.index === this.queries.length) {
            error = this.query_error_template({
              last_non_query: true
            });
            this.results_view_wrapper.render_error(this.raw_queries[this.index - 1], error, true);
            this.save_query({
              query: this.raw_query,
              broken_query: true
            });
          }
        }
      }
    };

    Container.prototype.evaluate = function(query) {
      "use strict";
      return eval(query);
    };

    Container.prototype.clean_query = function(query) {
      var char, i, is_parsing_string, result_inline_comment, result_multiple_line_comment, result_query, start, string_delimiter, to_skip, _i, _len;
      is_parsing_string = false;
      start = 0;
      result_query = '';
      for (i = _i = 0, _len = query.length; _i < _len; i = ++_i) {
        char = query[i];
        if (to_skip > 0) {
          to_skip--;
          continue;
        }
        if (is_parsing_string === true) {
          if (char === string_delimiter && (query[i - 1] != null) && query[i - 1] !== '\\') {
            result_query += query.slice(start, i + 1).replace(/\n/g, '\\n');
            start = i + 1;
            is_parsing_string = false;
            continue;
          }
        } else {
          if (char === '\'' || char === '"') {
            result_query += query.slice(start, i).replace(/\n/g, '');
            start = i;
            is_parsing_string = true;
            string_delimiter = char;
            continue;
          }
          result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
          if (result_inline_comment != null) {
            result_query += query.slice(start, i).replace(/\n/g, '');
            start = i;
            to_skip = result_inline_comment[0].length - 1;
            start += result_inline_comment[0].length;
            continue;
          }
          result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
          if (result_multiple_line_comment != null) {
            result_query += query.slice(start, i).replace(/\n/g, '');
            start = i;
            to_skip = result_multiple_line_comment[0].length - 1;
            start += result_multiple_line_comment[0].length;
            continue;
          }
        }
      }
      if (is_parsing_string) {
        result_query += query.slice(start, i).replace(/\n/g, '\\\\n');
      } else {
        result_query += query.slice(start, i).replace(/\n/g, '');
      }
      return result_query;
    };

    Container.prototype.separate_queries = function(query) {
      var char, i, is_parsing_string, last_query, position, queries, result_inline_comment, result_multiple_line_comment, stack, start, string_delimiter, to_skip, _i, _len;
      queries = [];
      is_parsing_string = false;
      stack = [];
      start = 0;
      position = {
        char: 0,
        line: 1
      };
      for (i = _i = 0, _len = query.length; _i < _len; i = ++_i) {
        char = query[i];
        if (char === '\n') {
          position.line++;
          position.char = 0;
        } else {
          position.char++;
        }
        if (to_skip > 0) {
          to_skip--;
          continue;
        }
        if (is_parsing_string === true) {
          if (char === string_delimiter && (query[i - 1] != null) && query[i - 1] !== '\\') {
            is_parsing_string = false;
            continue;
          }
        } else {
          if (char === '\'' || char === '"') {
            is_parsing_string = true;
            string_delimiter = char;
            continue;
          }
          result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
          if (result_inline_comment != null) {
            to_skip = result_inline_comment[0].length - 1;
            continue;
          }
          result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
          if (result_multiple_line_comment != null) {
            to_skip = result_multiple_line_comment[0].length - 1;
            continue;
          }
          if (char in this.stop_char.opening) {
            stack.push(char);
          } else if (char in this.stop_char.closing) {
            if (stack[stack.length - 1] !== this.stop_char.closing[char]) {
              throw this.query_error_template({
                syntax_error: true,
                bracket: char,
                line: position.line,
                position: position.char
              });
            } else {
              stack.pop();
            }
          } else if (char === ';' && stack.length === 0) {
            queries.push(query.slice(start, i + 1));
            start = i + 1;
          }
        }
      }
      if (start < query.length - 1) {
        last_query = query.slice(start);
        if (this.regex.white.test(last_query) === false) {
          queries.push(last_query);
        }
      }
      return queries;
    };

    Container.prototype.clear_query = function() {
      this.codemirror.setValue('');
      return this.codemirror.focus();
    };

    Container.prototype.error_on_connect = function(error) {
      if (/^(Unexpected token)/.test(error.message)) {
        this.results_view_wrapper.render_error(null, error);
        return this.save_query({
          query: this.raw_query,
          broken_query: true
        });
      } else {
        this.results_view_wrapper.cursor_timed_out();
        this.$('#user-alert-space').hide();
        this.$('#user-alert-space').html(this.alert_connection_fail_template({}));
        return this.$('#user-alert-space').slideDown('fast');
      }
    };

    Container.prototype.handle_gutter_click = function(editor, line) {
      var end, start;
      start = {
        line: line,
        ch: 0
      };
      end = {
        line: line,
        ch: this.codemirror.getLine(line).length
      };
      return this.codemirror.setSelection(start, end);
    };

    Container.prototype.toggle_size = function() {
      if (this.displaying_full_view === true) {
        this.display_normal();
        $(window).off('resize', this.display_full);
        this.displaying_full_view = false;
      } else {
        this.display_full();
        $(window).on('resize', this.display_full);
        this.displaying_full_view = true;
      }
      return this.results_view_wrapper.set_scrollbar();
    };

    Container.prototype.display_normal = function() {
      $('#cluster').addClass('container');
      $('#cluster').removeClass('cluster_with_margin');
      this.$('.wrapper_scrollbar').css('width', '888px');
      this.$('.option_icon').removeClass('fullscreen_exit');
      return this.$('.option_icon').addClass('fullscreen');
    };

    Container.prototype.display_full = function() {
      $('#cluster').removeClass('container');
      $('#cluster').addClass('cluster_with_margin');
      this.$('.wrapper_scrollbar').css('width', ($(window).width() - 92) + 'px');
      this.$('.option_icon').removeClass('fullscreen');
      return this.$('.option_icon').addClass('fullscreen_exit');
    };

    Container.prototype.remove = function() {
      this.results_view_wrapper.remove();
      this.history_view.remove();
      this.driver_handler.remove();
      this.display_normal();
      $(window).off('resize', this.display_full);
      $(document).unbind('mousemove', this.handle_mousemove);
      $(document).unbind('mouseup', this.handle_mouseup);
      clearTimeout(this.timeout_driver_connect);
      driver.stop_timer(this.timer);
      return Container.__super__.remove.call(this);
    };

    return Container;

  })(Backbone.View);
  ResultView = (function(_super) {
    __extends(ResultView, _super);

    function ResultView() {
      this.add_row = __bind(this.add_row, this);
      this.show_next_batch = __bind(this.show_next_batch, this);
      this.fetch_batch_rows = __bind(this.fetch_batch_rows, this);
      this.setStackSize = __bind(this.setStackSize, this);
      this.current_batch_size = __bind(this.current_batch_size, this);
      this.current_batch = __bind(this.current_batch, this);
      this.unpause_feed = __bind(this.unpause_feed, this);
      this.pause_feed = __bind(this.pause_feed, this);
      this.parent_pause_feed = __bind(this.parent_pause_feed, this);
      this.expand_tree_in_table = __bind(this.expand_tree_in_table, this);
      this.toggle_collapse = __bind(this.toggle_collapse, this);
      this.json_to_tree = __bind(this.json_to_tree, this);
      this.remove = __bind(this.remove, this);
      this.initialize = __bind(this.initialize, this);
      return ResultView.__super__.constructor.apply(this, arguments);
    }

    ResultView.prototype.tree_large_container_template = Handlebars.templates['dataexplorer_large_result_json_tree_container-template'];

    ResultView.prototype.tree_container_template = Handlebars.templates['dataexplorer_result_json_tree_container-template'];

    ResultView.prototype.events = function() {
      return {
        'click .jt_arrow': 'toggle_collapse',
        'click .jta_arrow_h': 'expand_tree_in_table',
        'mousedown': 'parent_pause_feed'
      };
    };

    ResultView.prototype.initialize = function(args) {
      this._patched_already = false;
      this.parent = args.parent;
      this.query_result = args.query_result;
      this.render();
      this.listenTo(this.query_result, 'end', (function(_this) {
        return function() {
          if (!_this.query_result.is_feed) {
            return _this.render();
          }
        };
      })(this));
      return this.fetch_batch_rows();
    };

    ResultView.prototype.remove = function() {
      this.removed_self = true;
      return ResultView.__super__.remove.call(this);
    };

    ResultView.prototype.max_datum_threshold = 1000;

    ResultView.prototype.has_too_many_datums = function(result) {
      if (this.has_too_many_datums_helper(result) > this.max_datum_threshold) {
        return true;
      }
      return false;
    };

    ResultView.prototype.json_to_tree = function(result) {
      if (this.has_too_many_datums(result)) {
        return this.tree_large_container_template({
          json_data: JSON.stringify(result, null, 4)
        });
      } else {
        return this.tree_container_template({
          tree: Utils.json_to_node(result)
        });
      }
    };

    ResultView.prototype.has_too_many_datums_helper = function(result) {
      var count, element, key, _i, _len;
      if (Object.prototype.toString.call(result) === '[object Object]') {
        count = 0;
        for (key in result) {
          count += this.has_too_many_datums_helper(result[key]);
          if (count > this.max_datum_threshold) {
            return count;
          }
        }
        return count;
      } else if (Array.isArray(result)) {
        count = 0;
        for (_i = 0, _len = result.length; _i < _len; _i++) {
          element = result[_i];
          count += this.has_too_many_datums_helper(element);
          if (count > this.max_datum_threshold) {
            return count;
          }
        }
        return count;
      }
      return 1;
    };

    ResultView.prototype.toggle_collapse = function(event) {
      this.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed');
      this.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed');
      this.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed');
      this.$(event.target).toggleClass('jt_arrow_hidden');
      return this.parent.set_scrollbar();
    };

    ResultView.prototype.expand_tree_in_table = function(event) {
      var classname_to_change, data, dom_element, result;
      dom_element = this.$(event.target).parent();
      this.$(event.target).remove();
      data = dom_element.data('json_data');
      result = this.json_to_tree(data);
      dom_element.html(result);
      classname_to_change = dom_element.parent().attr('class').split(' ')[0];
      $('.' + classname_to_change).css('max-width', 'none');
      classname_to_change = dom_element.parent().parent().attr('class');
      $('.' + classname_to_change).css('max-width', 'none');
      dom_element.css('max-width', 'none');
      return this.parent.set_scrollbar();
    };

    ResultView.prototype.parent_pause_feed = function(event) {
      return this.parent.pause_feed();
    };

    ResultView.prototype.pause_feed = function() {
      if (this.parent.container.state.pause_at == null) {
        return this.parent.container.state.pause_at = this.query_result.size();
      }
    };

    ResultView.prototype.unpause_feed = function() {
      if (this.parent.container.state.pause_at != null) {
        this.parent.container.state.pause_at = null;
        return this.render();
      }
    };

    ResultView.prototype.current_batch = function() {
      var latest, pause_at;
      switch (this.query_result.type) {
        case 'value':
          return this.query_result.value;
        case 'cursor':
          if (this.query_result.is_feed) {
            pause_at = this.parent.container.state.pause_at;
            if (pause_at != null) {
              latest = this.query_result.slice(Math.min(0, pause_at - this.parent.container.state.options.query_limit), pause_at - 1);
            } else {
              latest = this.query_result.slice(-this.parent.container.state.options.query_limit);
            }
            latest.reverse();
            return latest;
          } else {
            return this.query_result.slice(this.query_result.position, this.query_result.position + this.parent.container.state.options.query_limit);
          }
      }
    };

    ResultView.prototype.current_batch_size = function() {
      var _ref, _ref1;
      return (_ref = (_ref1 = this.current_batch()) != null ? _ref1.length : void 0) != null ? _ref : 0;
    };

    ResultView.prototype.setStackSize = function() {
      var iterableProto, _ref, _ref1, _ref2, _ref3;
      if (this._patched_already) {
        return;
      }
      iterableProto = (_ref = this.query_result.cursor) != null ? (_ref1 = _ref.__proto__) != null ? (_ref2 = _ref1.__proto__) != null ? (_ref3 = _ref2.constructor) != null ? _ref3.prototype : void 0 : void 0 : void 0 : void 0;
      if ((iterableProto != null ? iterableProto.stackSize : void 0) > 20) {
        iterableProto.stackSize = 20;
        return this._patched_already = true;
      }
    };

    ResultView.prototype.fetch_batch_rows = function() {
      if (this.query_result.type === !'cursor') {
        return;
      }
      this.setStackSize();
      if (this.query_result.is_feed || this.query_result.size() < this.query_result.position + this.parent.container.state.options.query_limit) {
        this.query_result.once('add', (function(_this) {
          return function(query_result, row) {
            if (_this.removed_self) {
              return;
            }
            if (_this.query_result.is_feed) {
              if (_this.parent.container.state.pause_at == null) {
                if (_this.paused_at == null) {
                  _this.query_result.drop_before(_this.query_result.size() - _this.parent.container.state.options.query_limit);
                }
                _this.add_row(row);
              }
              _this.parent.update_feed_metadata();
            }
            return _this.fetch_batch_rows();
          };
        })(this));
        return this.query_result.fetch_next();
      } else {
        this.parent.render();
        return this.render();
      }
    };

    ResultView.prototype.show_next_batch = function() {
      this.query_result.position += this.parent.container.state.options.query_limit;
      this.query_result.drop_before(this.parent.container.state.options.query_limit);
      this.render();
      this.parent.render();
      return this.fetch_batch_rows();
    };

    ResultView.prototype.add_row = function(row) {
      return this.render();
    };

    return ResultView;

  })(Backbone.View);
  TreeView = (function(_super) {
    __extends(TreeView, _super);

    function TreeView() {
      this.add_row = __bind(this.add_row, this);
      this.render = __bind(this.render, this);
      return TreeView.__super__.constructor.apply(this, arguments);
    }

    TreeView.prototype.className = 'results tree_view_container';

    TreeView.prototype.template = Handlebars.templates['dataexplorer_result_tree-template'];

    TreeView.prototype.render = function() {
      var row, tree_container, _i, _len, _ref;
      switch (this.query_result.type) {
        case 'value':
          this.$el.html(this.template({
            tree: this.json_to_tree(this.query_result.value)
          }));
          break;
        case 'cursor':
          this.$el.html(this.template({
            tree: []
          }));
          tree_container = this.$('.json_tree_container');
          _ref = this.current_batch();
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            row = _ref[_i];
            tree_container.append(this.json_to_tree(row));
          }
      }
      return this;
    };

    TreeView.prototype.add_row = function(row, noflash) {
      var children, node, tree_container;
      tree_container = this.$('.json_tree_container');
      node = $(this.json_to_tree(row)).prependTo(tree_container);
      if (!noflash) {
        node.addClass('flash');
      }
      children = tree_container.children();
      if (children.length > this.parent.container.state.options.query_limit) {
        return children.last().remove();
      }
    };

    return TreeView;

  })(ResultView);
  TableView = (function(_super) {
    __extends(TableView, _super);

    function TableView() {
      this.render = __bind(this.render, this);
      this.tag_record = __bind(this.tag_record, this);
      this.json_to_table = __bind(this.json_to_table, this);
      this.join_table = __bind(this.join_table, this);
      this.compute_data_for_type = __bind(this.compute_data_for_type, this);
      this.json_to_table_get_td_value = __bind(this.json_to_table_get_td_value, this);
      this.json_to_table_get_values = __bind(this.json_to_table_get_values, this);
      this.json_to_table_get_attr = __bind(this.json_to_table_get_attr, this);
      this.get_all_attr = __bind(this.get_all_attr, this);
      this.order_keys = __bind(this.order_keys, this);
      this.compute_occurrence = __bind(this.compute_occurrence, this);
      this.build_map_keys = __bind(this.build_map_keys, this);
      this.handle_mouseup = __bind(this.handle_mouseup, this);
      this.resize_column = __bind(this.resize_column, this);
      this.handle_mousemove = __bind(this.handle_mousemove, this);
      this.handle_mousedown = __bind(this.handle_mousedown, this);
      this.initialize = __bind(this.initialize, this);
      return TableView.__super__.constructor.apply(this, arguments);
    }

    TableView.prototype.className = 'results table_view_container';

    TableView.prototype.templates = {
      wrapper: Handlebars.templates['dataexplorer_result_table-template'],
      container: Handlebars.templates['dataexplorer_result_json_table_container-template'],
      tr_attr: Handlebars.templates['dataexplorer_result_json_table_tr_attr-template'],
      td_attr: Handlebars.templates['dataexplorer_result_json_table_td_attr-template'],
      tr_value: Handlebars.templates['dataexplorer_result_json_table_tr_value-template'],
      td_value: Handlebars.templates['dataexplorer_result_json_table_td_value-template'],
      td_value_content: Handlebars.templates['dataexplorer_result_json_table_td_value_content-template'],
      data_inline: Handlebars.templates['dataexplorer_result_json_table_data_inline-template'],
      no_result: Handlebars.templates['dataexplorer_result_empty-template']
    };

    TableView.prototype.default_size_column = 310;

    TableView.prototype.mouse_down = false;

    TableView.prototype.events = function() {
      return _.extend(TableView.__super__.events.call(this), {
        'mousedown .click_detector': 'handle_mousedown'
      });
    };

    TableView.prototype.initialize = function(args) {
      TableView.__super__.initialize.call(this, args);
      this.last_keys = this.parent.container.state.last_keys;
      this.last_columns_size = this.parent.container.state.last_columns_size;
      return this.listenTo(this.query_result, 'end', (function(_this) {
        return function() {
          if (_this.current_batch_size() === 0) {
            return _this.render();
          }
        };
      })(this));
    };

    TableView.prototype.handle_mousedown = function(event) {
      var _ref;
      if ((event != null ? (_ref = event.target) != null ? _ref.className : void 0 : void 0) === 'click_detector') {
        this.col_resizing = this.$(event.target).parent().data('col');
        this.start_width = this.$(event.target).parent().width();
        this.start_x = event.pageX;
        this.mouse_down = true;
        return $('body').toggleClass('resizing', true);
      }
    };

    TableView.prototype.handle_mousemove = function(event) {
      if (this.mouse_down) {
        this.parent.container.state.last_columns_size[this.col_resizing] = Math.max(5, this.start_width - this.start_x + event.pageX);
        return this.resize_column(this.col_resizing, this.parent.container.state.last_columns_size[this.col_resizing]);
      }
    };

    TableView.prototype.resize_column = function(col, size) {
      this.$('.col-' + col).css('max-width', size);
      this.$('.value-' + col).css('max-width', size - 20);
      this.$('.col-' + col).css('width', size);
      this.$('.value-' + col).css('width', size - 20);
      if (size < 20) {
        this.$('.value-' + col).css('padding-left', (size - 5) + 'px');
        return this.$('.value-' + col).css('visibility', 'hidden');
      } else {
        this.$('.value-' + col).css('padding-left', '15px');
        return this.$('.value-' + col).css('visibility', 'visible');
      }
    };

    TableView.prototype.handle_mouseup = function(event) {
      if (this.mouse_down === true) {
        this.mouse_down = false;
        $('body').toggleClass('resizing', false);
        return this.parent.set_scrollbar();
      }
    };


    /*
    keys =
        primitive_value_count: <int>
        object:
            key_1: <keys>
            key_2: <keys>
     */

    TableView.prototype.build_map_keys = function(args) {
      var key, keys_count, result, row, _results;
      keys_count = args.keys_count;
      result = args.result;
      if (jQuery.isPlainObject(result)) {
        if (result.$reql_type$ === 'TIME') {
          return keys_count.primitive_value_count++;
        } else if (result.$reql_type$ === 'BINARY') {
          return keys_count.primitive_value_count++;
        } else {
          _results = [];
          for (key in result) {
            row = result[key];
            if (keys_count['object'] == null) {
              keys_count['object'] = {};
            }
            if (keys_count['object'][key] == null) {
              keys_count['object'][key] = {
                primitive_value_count: 0
              };
            }
            _results.push(this.build_map_keys({
              keys_count: keys_count['object'][key],
              result: row
            }));
          }
          return _results;
        }
      } else {
        return keys_count.primitive_value_count++;
      }
    };

    TableView.prototype.compute_occurrence = function(keys_count) {
      var count_key, count_occurrence, key, row, _ref;
      if (keys_count['object'] == null) {
        return keys_count.occurrence = keys_count.primitive_value_count;
      } else {
        count_key = keys_count.primitive_value_count > 0 ? 1 : 0;
        count_occurrence = keys_count.primitive_value_count;
        _ref = keys_count['object'];
        for (key in _ref) {
          row = _ref[key];
          count_key++;
          this.compute_occurrence(row);
          count_occurrence += row.occurrence;
        }
        return keys_count.occurrence = count_occurrence / count_key;
      }
    };

    TableView.prototype.order_keys = function(keys) {
      var copy_keys, key, value, _ref;
      copy_keys = [];
      if (keys.object != null) {
        _ref = keys.object;
        for (key in _ref) {
          value = _ref[key];
          if (jQuery.isPlainObject(value)) {
            this.order_keys(value);
          }
          copy_keys.push({
            key: key,
            value: value.occurrence
          });
        }
        copy_keys.sort(function(a, b) {
          if (b.value - a.value) {
            return b.value - a.value;
          } else {
            if (a.key > b.key) {
              return 1;
            } else {
              return -1;
            }
          }
        });
      }
      keys.sorted_keys = _.map(copy_keys, function(d) {
        return d.key;
      });
      if (keys.primitive_value_count > 0) {
        return keys.sorted_keys.unshift(this.primitive_key);
      }
    };

    TableView.prototype.get_all_attr = function(args) {
      var attr, key, keys_count, new_prefix, new_prefix_str, prefix, prefix_str, _i, _len, _ref, _results;
      keys_count = args.keys_count;
      attr = args.attr;
      prefix = args.prefix;
      prefix_str = args.prefix_str;
      _ref = keys_count.sorted_keys;
      _results = [];
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        key = _ref[_i];
        if (key === this.primitive_key) {
          new_prefix_str = prefix_str;
          if (new_prefix_str.length > 0) {
            new_prefix_str = new_prefix_str.slice(0, -1);
          }
          _results.push(attr.push({
            prefix: prefix,
            prefix_str: new_prefix_str,
            is_primitive: true
          }));
        } else {
          if (keys_count['object'][key]['object'] != null) {
            new_prefix = DataUtils.deep_copy(prefix);
            new_prefix.push(key);
            _results.push(this.get_all_attr({
              keys_count: keys_count.object[key],
              attr: attr,
              prefix: new_prefix,
              prefix_str: (prefix_str != null ? prefix_str : '') + key + '.'
            }));
          } else {
            _results.push(attr.push({
              prefix: prefix,
              prefix_str: prefix_str,
              key: key
            }));
          }
        }
      }
      return _results;
    };

    TableView.prototype.json_to_table_get_attr = function(flatten_attr) {
      return this.templates.tr_attr({
        attr: flatten_attr
      });
    };

    TableView.prototype.json_to_table_get_values = function(args) {
      var attr_obj, col, document_list, flatten_attr, i, index, key, new_document, prefix, result, single_result, value, _i, _j, _k, _len, _len1, _len2, _ref;
      result = args.result;
      flatten_attr = args.flatten_attr;
      document_list = [];
      for (i = _i = 0, _len = result.length; _i < _len; i = ++_i) {
        single_result = result[i];
        new_document = {
          cells: []
        };
        for (col = _j = 0, _len1 = flatten_attr.length; _j < _len1; col = ++_j) {
          attr_obj = flatten_attr[col];
          key = attr_obj.key;
          value = single_result;
          _ref = attr_obj.prefix;
          for (_k = 0, _len2 = _ref.length; _k < _len2; _k++) {
            prefix = _ref[_k];
            value = value != null ? value[prefix] : void 0;
          }
          if (attr_obj.is_primitive !== true) {
            if (value != null) {
              value = value[key];
            } else {
              value = void 0;
            }
          }
          new_document.cells.push(this.json_to_table_get_td_value(value, col));
        }
        index = this.query_result.is_feed ? this.query_result.size() - i : i + 1;
        this.tag_record(new_document, index);
        document_list.push(new_document);
      }
      return this.templates.tr_value({
        document: document_list
      });
    };

    TableView.prototype.json_to_table_get_td_value = function(value, col) {
      var data;
      data = this.compute_data_for_type(value, col);
      return this.templates.td_value({
        col: col,
        cell_content: this.templates.td_value_content(data)
      });
    };

    TableView.prototype.compute_data_for_type = function(value, col) {
      var data, value_type;
      data = {
        value: value,
        class_value: 'value-' + col
      };
      value_type = typeof value;
      if (value === null) {
        data['value'] = 'null';
        data['classname'] = 'jta_null';
      } else if (value === void 0) {
        data['value'] = 'undefined';
        data['classname'] = 'jta_undefined';
      } else if ((value.constructor != null) && value.constructor === Array) {
        if (value.length === 0) {
          data['value'] = '[ ]';
          data['classname'] = 'empty array';
        } else {
          data['value'] = '[ ... ]';
          data['data_to_expand'] = JSON.stringify(value);
        }
      } else if (Object.prototype.toString.call(value) === '[object Object]' && value.$reql_type$ === 'TIME') {
        data['value'] = Utils.date_to_string(value);
        data['classname'] = 'jta_date';
      } else if (Object.prototype.toString.call(value) === '[object Object]' && value.$reql_type$ === 'BINARY') {
        data['value'] = Utils.binary_to_string(value);
        data['classname'] = 'jta_bin';
      } else if (Object.prototype.toString.call(value) === '[object Object]') {
        data['value'] = '{ ... }';
        data['is_object'] = true;
      } else if (value_type === 'number') {
        data['classname'] = 'jta_num';
      } else if (value_type === 'string') {
        if (/^(http|https):\/\/[^\s]+$/i.test(value)) {
          data['classname'] = 'jta_url';
        } else if (/^[a-z0-9]+@[a-z0-9]+.[a-z0-9]{2,4}/i.test(value)) {
          data['classname'] = 'jta_email';
        } else {
          data['classname'] = 'jta_string';
        }
      } else if (value_type === 'boolean') {
        data['classname'] = 'jta_bool';
        data.value = value === true ? 'true' : 'false';
      }
      return data;
    };

    TableView.prototype.join_table = function(data) {
      var data_cell, i, result, value, _i, _len;
      result = '';
      for (i = _i = 0, _len = data.length; _i < _len; i = ++_i) {
        value = data[i];
        data_cell = this.compute_data_for_type(value, 'float');
        data_cell['is_inline'] = true;
        if (i !== data.length - 1) {
          data_cell['need_comma'] = true;
        }
        result += this.templates.data_inline(data_cell);
      }
      return result;
    };

    TableView.prototype.json_to_table = function(result) {
      var flatten_attr, index, keys_count, result_entry, value, _i, _j, _len, _len1;
      if ((result.constructor == null) || result.constructor !== Array) {
        result = [result];
      }
      keys_count = {
        primitive_value_count: 0
      };
      for (_i = 0, _len = result.length; _i < _len; _i++) {
        result_entry = result[_i];
        this.build_map_keys({
          keys_count: keys_count,
          result: result_entry
        });
      }
      this.compute_occurrence(keys_count);
      this.order_keys(keys_count);
      flatten_attr = [];
      this.get_all_attr({
        keys_count: keys_count,
        attr: flatten_attr,
        prefix: [],
        prefix_str: ''
      });
      for (index = _j = 0, _len1 = flatten_attr.length; _j < _len1; index = ++_j) {
        value = flatten_attr[index];
        value.col = index;
      }
      this.last_keys = flatten_attr.map(function(attr, i) {
        if (attr.prefix_str !== '') {
          return attr.prefix_str + attr.key;
        }
        return attr.key;
      });
      this.parent.container.state.last_keys = this.last_keys;
      return this.templates.container({
        table_attr: this.json_to_table_get_attr(flatten_attr),
        table_data: this.json_to_table_get_values({
          result: result,
          flatten_attr: flatten_attr
        })
      });
    };

    TableView.prototype.tag_record = function(doc, i) {
      return doc.record = this.query_result.position + i;
    };

    TableView.prototype.render = function() {
      var col, column, current_size, expandable_columns, extra_size_table, first_row, index, keys, max_size, previous_keys, real_size, results, same_keys, value, _i, _j, _k, _len, _len1, _ref, _ref1, _ref2;
      previous_keys = this.parent.container.state.last_keys;
      results = this.current_batch();
      if (Object.prototype.toString.call(results) === '[object Array]') {
        if (results.length === 0) {
          this.$el.html(this.templates.wrapper({
            content: this.templates.no_result({
              ended: this.query_result.ended
            })
          }));
        } else {
          this.$el.html(this.templates.wrapper({
            content: this.json_to_table(results)
          }));
        }
      } else {
        if (results === void 0) {
          this.$el.html('');
        } else {
          this.$el.html(this.templates.wrapper({
            content: this.json_to_table([results])
          }));
        }
      }
      if (this.query_result.is_feed) {
        first_row = this.$('.jta_tr').eq(1).find('td:not(:first)');
        first_row.css({
          'background-color': '#eeeeff'
        });
        first_row.animate({
          'background-color': '#fbfbfb'
        });
      }
      if (this.parent.container.state.last_keys.length !== previous_keys.length) {
        same_keys = false;
      } else {
        same_keys = true;
        _ref = this.parent.container.state.last_keys;
        for (index = _i = 0, _len = _ref.length; _i < _len; index = ++_i) {
          keys = _ref[index];
          if (this.parent.container.state.last_keys[index] !== previous_keys[index]) {
            same_keys = false;
          }
        }
      }
      if (same_keys === true) {
        _ref1 = this.parent.container.state.last_columns_size;
        for (col in _ref1) {
          value = _ref1[col];
          this.resize_column(col, value);
        }
      } else {
        this.last_column_size = {};
      }
      extra_size_table = this.$('.json_table_container').width() - this.$('.json_table').width();
      if (extra_size_table > 0) {
        expandable_columns = [];
        for (index = _j = 0, _ref2 = this.last_keys.length - 1; 0 <= _ref2 ? _j <= _ref2 : _j >= _ref2; index = 0 <= _ref2 ? ++_j : --_j) {
          real_size = 0;
          this.$('.col-' + index).children().children().children().each(function(i, bloc) {
            var $bloc;
            $bloc = $(bloc);
            if (real_size < $bloc.width()) {
              return real_size = $bloc.width();
            }
          });
          if ((real_size != null) && real_size === real_size && real_size > this.default_size_column) {
            expandable_columns.push({
              col: index,
              size: real_size + 20
            });
          }
        }
        while (expandable_columns.length > 0) {
          expandable_columns.sort(function(a, b) {
            return a.size - b.size;
          });
          if (expandable_columns[0].size - this.$('.col-' + expandable_columns[0].col).width() < extra_size_table / expandable_columns.length) {
            extra_size_table = extra_size_table - (expandable_columns[0]['size'] - this.$('.col-' + expandable_columns[0].col).width());
            this.$('.col-' + expandable_columns[0]['col']).css('max-width', expandable_columns[0]['size']);
            this.$('.value-' + expandable_columns[0]['col']).css('max-width', expandable_columns[0]['size'] - 20);
            expandable_columns.shift();
          } else {
            max_size = extra_size_table / expandable_columns.length;
            for (_k = 0, _len1 = expandable_columns.length; _k < _len1; _k++) {
              column = expandable_columns[_k];
              current_size = this.$('.col-' + expandable_columns[0].col).width();
              this.$('.col-' + expandable_columns[0]['col']).css('max-width', current_size + max_size);
              this.$('.value-' + expandable_columns[0]['col']).css('max-width', current_size + max_size - 20);
            }
            expandable_columns = [];
          }
        }
      }
      return this;
    };

    return TableView;

  })(ResultView);
  RawView = (function(_super) {
    __extends(RawView, _super);

    function RawView() {
      this.render = __bind(this.render, this);
      this.adjust_height = __bind(this.adjust_height, this);
      this.init_after_dom_rendered = __bind(this.init_after_dom_rendered, this);
      return RawView.__super__.constructor.apply(this, arguments);
    }

    RawView.prototype.className = 'results raw_view_container';

    RawView.prototype.template = Handlebars.templates['dataexplorer_result_raw-template'];

    RawView.prototype.init_after_dom_rendered = function() {
      return this.adjust_height();
    };

    RawView.prototype.adjust_height = function() {
      var height;
      height = this.$('.raw_view_textarea')[0].scrollHeight;
      if (height > 0) {
        return this.$('.raw_view_textarea').height(height);
      }
    };

    RawView.prototype.render = function() {
      this.$el.html(this.template(JSON.stringify(this.current_batch())));
      this.adjust_height();
      return this;
    };

    return RawView;

  })(ResultView);
  ProfileView = (function(_super) {
    __extends(ProfileView, _super);

    function ProfileView() {
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ProfileView.__super__.constructor.apply(this, arguments);
    }

    ProfileView.prototype.className = 'results profile_view_container';

    ProfileView.prototype.template = Handlebars.templates['dataexplorer_result_profile-template'];

    ProfileView.prototype.initialize = function(args) {
      ZeroClipboard.setDefaults({
        moviePath: 'js/ZeroClipboard.swf',
        forceHandCursor: true
      });
      this.clip = new ZeroClipboard();
      return ProfileView.__super__.initialize.call(this, args);
    };

    ProfileView.prototype.compute_total_duration = function(profile) {
      var task, total_duration, _i, _len;
      total_duration = 0;
      for (_i = 0, _len = profile.length; _i < _len; _i++) {
        task = profile[_i];
        if (task['duration(ms)'] != null) {
          total_duration += task['duration(ms)'];
        } else if (task['mean_duration(ms)'] != null) {
          total_duration += task['mean_duration(ms)'];
        }
      }
      return total_duration;
    };

    ProfileView.prototype.compute_num_shard_accesses = function(profile) {
      var num_shard_accesses, task, _i, _len;
      num_shard_accesses = 0;
      for (_i = 0, _len = profile.length; _i < _len; _i++) {
        task = profile[_i];
        if (task['description'] === 'Perform read on shard.') {
          num_shard_accesses += 1;
        }
        if (Object.prototype.toString.call(task['sub_tasks']) === '[object Array]') {
          num_shard_accesses += this.compute_num_shard_accesses(task['sub_tasks']);
        }
        if (Object.prototype.toString.call(task['parallel_tasks']) === '[object Array]') {
          num_shard_accesses += this.compute_num_shard_accesses(task['parallel_tasks']);
        }
        if (Object.prototype.toString.call(task) === '[object Array]') {
          num_shard_accesses += this.compute_num_shard_accesses(task);
        }
      }
      return num_shard_accesses;
    };

    ProfileView.prototype.render = function() {
      var profile;
      if (this.query_result.profile == null) {
        this.$el.html(this.template({}));
      } else {
        profile = this.query_result.profile;
        this.$el.html(this.template({
          profile: {
            clipboard_text: JSON.stringify(profile, null, 2),
            tree: this.json_to_tree(profile),
            total_duration: Utils.prettify_duration(this.parent.container.driver_handler.total_duration),
            server_duration: Utils.prettify_duration(this.compute_total_duration(profile)),
            num_shard_accesses: this.compute_num_shard_accesses(profile)
          }
        }));
        this.clip.glue(this.$('button.copy_profile'));
        this.delegateEvents();
      }
      return this;
    };

    return ProfileView;

  })(ResultView);
  ResultViewWrapper = (function(_super) {
    __extends(ResultViewWrapper, _super);

    function ResultViewWrapper() {
      this.show_next_batch = __bind(this.show_next_batch, this);
      this.handle_mousemove = __bind(this.handle_mousemove, this);
      this.handle_mouseup = __bind(this.handle_mouseup, this);
      this.remove = __bind(this.remove, this);
      this.cursor_timed_out = __bind(this.cursor_timed_out, this);
      this.init_after_dom_rendered = __bind(this.init_after_dom_rendered, this);
      this.new_view = __bind(this.new_view, this);
      this.feed_info = __bind(this.feed_info, this);
      this.update_feed_metadata = __bind(this.update_feed_metadata, this);
      this.render = __bind(this.render, this);
      this.set_query_result = __bind(this.set_query_result, this);
      this.render_error = __bind(this.render_error, this);
      this.activate_profiler = __bind(this.activate_profiler, this);
      this.set_scrollbar = __bind(this.set_scrollbar, this);
      this.set_view = __bind(this.set_view, this);
      this.show_raw = __bind(this.show_raw, this);
      this.show_table = __bind(this.show_table, this);
      this.show_profile = __bind(this.show_profile, this);
      this.show_tree = __bind(this.show_tree, this);
      this.unpause_feed = __bind(this.unpause_feed, this);
      this.pause_feed = __bind(this.pause_feed, this);
      this.handle_scroll = __bind(this.handle_scroll, this);
      this.initialize = __bind(this.initialize, this);
      return ResultViewWrapper.__super__.constructor.apply(this, arguments);
    }

    ResultViewWrapper.prototype.className = 'result_view';

    ResultViewWrapper.prototype.template = Handlebars.templates['dataexplorer_result_container-template'];

    ResultViewWrapper.prototype.metadata_template = Handlebars.templates['dataexplorer-metadata-template'];

    ResultViewWrapper.prototype.option_template = Handlebars.templates['dataexplorer-option_page-template'];

    ResultViewWrapper.prototype.error_template = Handlebars.templates['dataexplorer-error-template'];

    ResultViewWrapper.prototype.cursor_timed_out_template = Handlebars.templates['dataexplorer-cursor_timed_out-template'];

    ResultViewWrapper.prototype.primitive_key = '_-primitive value-_--';

    ResultViewWrapper.prototype.views = {
      tree: TreeView,
      table: TableView,
      profile: ProfileView,
      raw: RawView
    };

    ResultViewWrapper.prototype.events = function() {
      return {
        'click .link_to_profile_view': 'show_profile',
        'click .link_to_tree_view': 'show_tree',
        'click .link_to_table_view': 'show_table',
        'click .link_to_raw_view': 'show_raw',
        'click .activate_profiler': 'activate_profiler',
        'click .more_results_link': 'show_next_batch',
        'click .pause_feed': 'pause_feed',
        'click .unpause_feed': 'unpause_feed'
      };
    };

    ResultViewWrapper.prototype.initialize = function(args) {
      this.container = args.container;
      this.view = args.view;
      this.view_object = null;
      this.scroll_handler = (function(_this) {
        return function() {
          return _this.handle_scroll();
        };
      })(this);
      this.floating_metadata = false;
      $(window).on('scroll', this.scroll_handler);
      return this.handle_scroll();
    };

    ResultViewWrapper.prototype.handle_scroll = function() {
      var pos, scroll, _ref;
      scroll = $(window).scrollTop();
      pos = ((_ref = this.$('.results_header').offset()) != null ? _ref.top : void 0) + 2;
      if (pos == null) {
        return;
      }
      if (this.floating_metadata && pos > scroll) {
        this.floating_metadata = false;
        this.$('.metadata').removeClass('floating_metadata');
        if (this.container.state.pause_at != null) {
          this.unpause_feed('automatic');
        }
      }
      if (!this.floating_metadata && pos < scroll) {
        this.floating_metadata = true;
        this.$('.metadata').addClass('floating_metadata');
        if (this.container.state.pause_at == null) {
          return this.pause_feed('automatic');
        }
      }
    };

    ResultViewWrapper.prototype.pause_feed = function(event) {
      var _ref;
      if (event === 'automatic') {
        this.auto_unpause = true;
      } else {
        this.auto_unpause = false;
        if (event != null) {
          event.preventDefault();
        }
      }
      if ((_ref = this.view_object) != null) {
        _ref.pause_feed();
      }
      return this.$('.metadata').addClass('feed_paused').removeClass('feed_unpaused');
    };

    ResultViewWrapper.prototype.unpause_feed = function(event) {
      var _ref;
      if (event === 'automatic') {
        if (!this.auto_unpause) {
          return;
        }
      } else {
        event.preventDefault();
      }
      if ((_ref = this.view_object) != null) {
        _ref.unpause_feed();
      }
      return this.$('.metadata').removeClass('feed_paused').addClass('feed_unpaused');
    };

    ResultViewWrapper.prototype.show_tree = function(event) {
      event.preventDefault();
      return this.set_view('tree');
    };

    ResultViewWrapper.prototype.show_profile = function(event) {
      event.preventDefault();
      return this.set_view('profile');
    };

    ResultViewWrapper.prototype.show_table = function(event) {
      event.preventDefault();
      return this.set_view('table');
    };

    ResultViewWrapper.prototype.show_raw = function(event) {
      event.preventDefault();
      return this.set_view('raw');
    };

    ResultViewWrapper.prototype.set_view = function(view) {
      var _ref;
      this.view = view;
      this.container.state.view = view;
      this.$(".link_to_" + this.view + "_view").parent().addClass('active');
      this.$(".link_to_" + this.view + "_view").parent().siblings().removeClass('active');
      if ((_ref = this.query_result) != null ? _ref.ready : void 0) {
        return this.new_view();
      }
    };

    ResultViewWrapper.prototype.set_scrollbar = function() {
      var content_container, content_name, position_scrollbar, that, width_value;
      if (this.view === 'table') {
        content_name = '.json_table';
        content_container = '.table_view_container';
      } else if (this.view === 'tree') {
        content_name = '.json_tree';
        content_container = '.tree_view_container';
      } else if (this.view === 'profile') {
        content_name = '.json_tree';
        content_container = '.profile_view_container';
      } else if (this.view === 'raw') {
        this.$('.wrapper_scrollbar').hide();
        return;
      }
      width_value = this.$(content_name).innerWidth();
      if (width_value < this.$(content_container).width()) {
        this.$('.wrapper_scrollbar').hide();
        if (this.set_scrollbar_scroll_handler != null) {
          return $(window).unbind('scroll', this.set_scrollbar_scroll_handler);
        }
      } else {
        this.$('.wrapper_scrollbar').show();
        this.$('.scrollbar_fake_content').width(width_value);
        $(".wrapper_scrollbar").scroll(function() {
          return $(content_container).scrollLeft($(".wrapper_scrollbar").scrollLeft());
        });
        $(content_container).scroll(function() {
          return $(".wrapper_scrollbar").scrollLeft($(content_container).scrollLeft());
        });
        position_scrollbar = function() {
          if ($(content_container).offset() != null) {
            if ($(window).scrollTop() + $(window).height() < $(content_container).offset().top + 20) {
              return that.$('.wrapper_scrollbar').hide();
            } else if ($(window).scrollTop() + $(window).height() < $(content_container).offset().top + $(content_container).height()) {
              that.$('.wrapper_scrollbar').show();
              that.$('.wrapper_scrollbar').css('overflow', 'auto');
              return that.$('.wrapper_scrollbar').css('margin-bottom', '0px');
            } else {
              return that.$('.wrapper_scrollbar').css('overflow', 'hidden');
            }
          }
        };
        that = this;
        position_scrollbar();
        this.set_scrollbar_scroll_handler = position_scrollbar;
        $(window).scroll(this.set_scrollbar_scroll_handler);
        return $(window).resize(function() {
          return position_scrollbar();
        });
      }
    };

    ResultViewWrapper.prototype.activate_profiler = function(event) {
      event.preventDefault();
      if (this.container.options_view.state === 'hidden') {
        return this.container.toggle_options({
          cb: (function(_this) {
            return function() {
              return setTimeout(function() {
                if (_this.container.state.options.profiler === false) {
                  _this.container.options_view.$('.option_description[data-option="profiler"]').click();
                }
                _this.container.options_view.$('.profiler_enabled').show();
                return _this.container.options_view.$('.profiler_enabled').css('visibility', 'visible');
              }, 100);
            };
          })(this)
        });
      } else {
        if (this.container.state.options.profiler === false) {
          this.container.options_view.$('.option_description[data-option="profiler"]').click();
        }
        this.container.options_view.$('.profiler_enabled').hide();
        this.container.options_view.$('.profiler_enabled').css('visibility', 'visible');
        return this.container.options_view.$('.profiler_enabled').slideDown('fast');
      }
    };

    ResultViewWrapper.prototype.render_error = function(query, err, js_error) {
      var _ref, _ref1;
      if ((_ref = this.view_object) != null) {
        _ref.remove();
      }
      this.view_object = null;
      if ((_ref1 = this.query_result) != null) {
        _ref1.discard();
      }
      this.$el.html(this.error_template({
        query: query,
        error: err.toString().replace(/^(\s*)/, ''),
        js_error: js_error === true
      }));
      return this;
    };

    ResultViewWrapper.prototype.set_query_result = function(_arg) {
      var query_result, _ref;
      query_result = _arg.query_result;
      if ((_ref = this.query_result) != null) {
        _ref.discard();
      }
      this.query_result = query_result;
      if (query_result.ready) {
        this.render();
        this.new_view();
      } else {
        this.query_result.on('ready', (function(_this) {
          return function() {
            _this.render();
            return _this.new_view();
          };
        })(this));
      }
      return this.query_result.on('end', (function(_this) {
        return function() {
          return _this.render();
        };
      })(this));
    };

    ResultViewWrapper.prototype.render = function(args) {
      var has_more_data, _ref, _ref1, _ref2, _ref3;
      if ((_ref = this.query_result) != null ? _ref.ready : void 0) {
        if ((_ref1 = this.view_object) != null) {
          _ref1.$el.detach();
        }
        has_more_data = !this.query_result.ended && this.query_result.position + this.container.state.options.query_limit <= this.query_result.size();
        this.$el.html(this.template({
          limit_value: (_ref2 = this.view_object) != null ? _ref2.current_batch_size() : void 0,
          skip_value: this.query_result.position,
          query_has_changed: args != null ? args.query_has_changed : void 0,
          show_more_data: has_more_data && !this.container.state.cursor_timed_out,
          cursor_timed_out_template: (!this.query_result.ended && this.container.state.cursor_timed_out ? this.cursor_timed_out_template() : void 0),
          execution_time_pretty: Utils.prettify_duration(this.container.driver_handler.total_duration),
          no_results: this.query_result.ended && this.query_result.size() === 0,
          num_results: this.query_result.size(),
          floating_metadata: this.floating_metadata,
          feed: this.feed_info()
        }));
        this.$('.execution_time').tooltip({
          for_dataexplorer: true,
          trigger: 'hover',
          placement: 'bottom'
        });
        this.$('.tab-content').html((_ref3 = this.view_object) != null ? _ref3.$el : void 0);
        this.$(".link_to_" + this.view + "_view").parent().addClass('active');
      }
      return this;
    };

    ResultViewWrapper.prototype.update_feed_metadata = function() {
      var info;
      info = this.feed_info();
      if (info == null) {
        return;
      }
      $('.feed_upcoming').text(info.upcoming);
      return $('.feed_overflow').parent().toggleClass('hidden', !info.overflow);
    };

    ResultViewWrapper.prototype.feed_info = function() {
      var total, _ref;
      if (this.query_result.is_feed) {
        total = (_ref = this.container.state.pause_at) != null ? _ref : this.query_result.size();
        return {
          ended: this.query_result.ended,
          overflow: this.container.state.options.query_limit < total,
          paused: this.container.state.pause_at != null,
          upcoming: this.query_result.size() - total
        };
      }
    };

    ResultViewWrapper.prototype.new_view = function() {
      var _ref;
      if ((_ref = this.view_object) != null) {
        _ref.remove();
      }
      this.view_object = new this.views[this.view]({
        parent: this,
        query_result: this.query_result
      });
      this.$('.tab-content').html(this.view_object.render().$el);
      this.init_after_dom_rendered();
      return this.set_scrollbar();
    };

    ResultViewWrapper.prototype.init_after_dom_rendered = function() {
      var _ref;
      return (_ref = this.view_object) != null ? typeof _ref.init_after_dom_rendered === "function" ? _ref.init_after_dom_rendered() : void 0 : void 0;
    };

    ResultViewWrapper.prototype.cursor_timed_out = function() {
      var _ref;
      this.container.state.cursor_timed_out = true;
      if (((_ref = this.container.state.query_result) != null ? _ref.ended : void 0) === true) {
        return this.$('.more_results_paragraph').html(this.cursor_timed_out_template());
      }
    };

    ResultViewWrapper.prototype.remove = function() {
      var _ref, _ref1;
      if ((_ref = this.view_object) != null) {
        _ref.remove();
      }
      if ((_ref1 = this.query_result) != null ? _ref1.is_feed : void 0) {
        this.query_result.force_end_gracefully();
      }
      if (this.set_scrollbar_scroll_handler != null) {
        $(window).unbind('scroll', this.set_scrollbar_scroll_handler);
      }
      $(window).unbind('resize');
      $(window).off('scroll', this.scroll_handler);
      return ResultViewWrapper.__super__.remove.call(this);
    };

    ResultViewWrapper.prototype.handle_mouseup = function(event) {
      var _ref;
      return (_ref = this.view_object) != null ? typeof _ref.handle_mouseup === "function" ? _ref.handle_mouseup(event) : void 0 : void 0;
    };

    ResultViewWrapper.prototype.handle_mousemove = function(event) {
      var _ref;
      return (_ref = this.view_object) != null ? typeof _ref.handle_mousedown === "function" ? _ref.handle_mousedown(event) : void 0 : void 0;
    };

    ResultViewWrapper.prototype.show_next_batch = function(event) {
      var _ref;
      event.preventDefault();
      $(window).scrollTop($('.results_container').offset().top);
      return (_ref = this.view_object) != null ? _ref.show_next_batch() : void 0;
    };

    return ResultViewWrapper;

  })(Backbone.View);
  OptionsView = (function(_super) {
    __extends(OptionsView, _super);

    function OptionsView() {
      this.render = __bind(this.render, this);
      this.change_query_limit = __bind(this.change_query_limit, this);
      this.toggle_option = __bind(this.toggle_option, this);
      this.initialize = __bind(this.initialize, this);
      return OptionsView.__super__.constructor.apply(this, arguments);
    }

    OptionsView.prototype.dataexplorer_options_template = Handlebars.templates['dataexplorer-options-template'];

    OptionsView.prototype.className = 'options_view';

    OptionsView.prototype.events = {
      'click li:not(.text-input)': 'toggle_option',
      'change #query_limit': 'change_query_limit'
    };

    OptionsView.prototype.initialize = function(args) {
      this.container = args.container;
      this.options = args.options;
      return this.state = 'hidden';
    };

    OptionsView.prototype.toggle_option = function(event) {
      var new_target, new_value;
      event.preventDefault();
      new_target = this.$(event.target).data('option');
      this.$('#' + new_target).prop('checked', !this.options[new_target]);
      if (event.target.nodeName !== 'INPUT') {
        new_value = this.$('#' + new_target).is(':checked');
        this.options[new_target] = new_value;
        if (window.localStorage != null) {
          window.localStorage.options = JSON.stringify(this.options);
        }
        if (new_target === 'profiler' && new_value === false) {
          return this.$('.profiler_enabled').slideUp('fast');
        }
      }
    };

    OptionsView.prototype.change_query_limit = function(event) {
      this.options['query_limit'] = parseInt(this.$('#query_limit').val(), 10) > 40 ? parseInt(this.$('#query_limit').val(), 10) : 40;
      if (window.localStorage != null) {
        window.localStorage.options = JSON.stringify(this.options);
      }
      return this.$('#query_limit').val(this.options['query_limit']);
    };

    OptionsView.prototype.render = function(displayed) {
      this.$el.html(this.dataexplorer_options_template(this.options));
      if (displayed === true) {
        this.$el.show();
      }
      this.delegateEvents();
      return this;
    };

    return OptionsView;

  })(Backbone.View);
  HistoryView = (function(_super) {
    __extends(HistoryView, _super);

    function HistoryView() {
      this.clear_history = __bind(this.clear_history, this);
      this.add_query = __bind(this.add_query, this);
      this.delete_query = __bind(this.delete_query, this);
      this.load_query = __bind(this.load_query, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      this.handle_mouseup = __bind(this.handle_mouseup, this);
      this.handle_mousemove = __bind(this.handle_mousemove, this);
      this.start_resize = __bind(this.start_resize, this);
      return HistoryView.__super__.constructor.apply(this, arguments);
    }

    HistoryView.prototype.dataexplorer_history_template = Handlebars.templates['dataexplorer-history-template'];

    HistoryView.prototype.dataexplorer_query_li_template = Handlebars.templates['dataexplorer-query_li-template'];

    HistoryView.prototype.className = 'history_container';

    HistoryView.prototype.size_history_displayed = 300;

    HistoryView.prototype.state = 'hidden';

    HistoryView.prototype.index_displayed = 0;

    HistoryView.prototype.events = {
      'click .load_query': 'load_query',
      'click .delete_query': 'delete_query'
    };

    HistoryView.prototype.start_resize = function(event) {
      this.start_y = event.pageY;
      this.start_height = this.container.$('.nano').height();
      this.mouse_down = true;
      return $('body').toggleClass('resizing', true);
    };

    HistoryView.prototype.handle_mousemove = function(event) {
      if (this.mouse_down === true) {
        this.height_history = Math.max(0, this.start_height - this.start_y + event.pageY);
        return this.container.$('.nano').height(this.height_history);
      }
    };

    HistoryView.prototype.handle_mouseup = function(event) {
      if (this.mouse_down === true) {
        this.mouse_down = false;
        $('.nano').nanoScroller({
          preventPageScrolling: true
        });
        return $('body').toggleClass('resizing', false);
      }
    };

    HistoryView.prototype.initialize = function(args) {
      this.container = args.container;
      this.history = args.history;
      return this.height_history = 204;
    };

    HistoryView.prototype.render = function(displayed) {
      var i, query, _i, _len, _ref;
      this.$el.html(this.dataexplorer_history_template());
      if (displayed === true) {
        this.$el.show();
      }
      if (this.history.length === 0) {
        this.$('.history_list').append(this.dataexplorer_query_li_template({
          no_query: true,
          displayed_class: 'displayed'
        }));
      } else {
        _ref = this.history;
        for (i = _i = 0, _len = _ref.length; _i < _len; i = ++_i) {
          query = _ref[i];
          this.$('.history_list').append(this.dataexplorer_query_li_template({
            query: query.query,
            broken_query: query.broken_query,
            id: i,
            num: i + 1
          }));
        }
      }
      this.delegateEvents();
      return this;
    };

    HistoryView.prototype.load_query = function(event) {
      var id;
      id = this.$(event.target).data().id;
      this.container.codemirror.setValue(this.history[parseInt(id)].query);
      this.container.state.current_query = this.history[parseInt(id)].query;
      return this.container.save_current_query();
    };

    HistoryView.prototype.delete_query = function(event) {
      var id, is_at_bottom, that;
      that = this;
      id = parseInt(this.$(event.target).data().id);
      this.history.splice(id, 1);
      window.localStorage.rethinkdb_history = JSON.stringify(this.history);
      is_at_bottom = this.$('.history_list').height() === $('.nano > .content').scrollTop() + $('.nano').height();
      return this.$('#query_history_' + id).slideUp('fast', (function(_this) {
        return function() {
          that.$(_this).remove();
          that.render();
          return that.container.adjust_collapsible_panel_height({
            is_at_bottom: is_at_bottom
          });
        };
      })(this));
    };

    HistoryView.prototype.add_query = function(args) {
      var broken_query, is_at_bottom, query, that;
      query = args.query;
      broken_query = args.broken_query;
      that = this;
      is_at_bottom = this.$('.history_list').height() === $('.nano > .content').scrollTop() + $('.nano').height();
      this.$('.history_list').append(this.dataexplorer_query_li_template({
        query: query,
        broken_query: broken_query,
        id: this.history.length - 1,
        num: this.history.length
      }));
      if (this.$('.no_history').length > 0) {
        return this.$('.no_history').slideUp('fast', function() {
          $(this).remove();
          if (that.state === 'visible') {
            return that.container.adjust_collapsible_panel_height({
              is_at_bottom: is_at_bottom
            });
          }
        });
      } else if (this.state === 'visible') {
        return this.container.adjust_collapsible_panel_height({
          delay_scroll: true,
          is_at_bottom: is_at_bottom
        });
      }
    };

    HistoryView.prototype.clear_history = function(event) {
      var that;
      that = this;
      event.preventDefault();
      this.container.clear_history();
      this.history = this.container.state.history;
      this.$('.query_history').slideUp('fast', function() {
        $(this).remove();
        if (that.$('.no_history').length === 0) {
          that.$('.history_list').append(that.dataexplorer_query_li_template({
            no_query: true,
            displayed_class: 'hidden'
          }));
          return that.$('.no_history').slideDown('fast');
        }
      });
      return that.container.adjust_collapsible_panel_height({
        size: 40,
        move_arrow: 'show',
        is_at_bottom: 'true'
      });
    };

    return HistoryView;

  })(Backbone.View);
  DriverHandler = (function() {
    function DriverHandler(options) {
      this.remove = __bind(this.remove, this);
      this.cursor_next = __bind(this.cursor_next, this);
      this.run_with_new_connection = __bind(this.run_with_new_connection, this);
      this.close_connection = __bind(this.close_connection, this);
      this._end = __bind(this._end, this);
      this._begin = __bind(this._begin, this);
      this.container = options.container;
      this.concurrent = 0;
      this.total_duration = 0;
    }

    DriverHandler.prototype._begin = function() {
      if (this.concurrent === 0) {
        this.container.toggle_executing(true);
        this.begin_time = new Date();
      }
      return this.concurrent++;
    };

    DriverHandler.prototype._end = function() {
      var now;
      if (this.concurrent > 0) {
        this.concurrent--;
        now = new Date();
        this.total_duration += now - this.begin_time;
        this.begin_time = now;
      }
      if (this.concurrent === 0) {
        return this.container.toggle_executing(false);
      }
    };

    DriverHandler.prototype.close_connection = function() {
      var _ref;
      if (((_ref = this.connection) != null ? _ref.open : void 0) === true) {
        driver.close(this.connection);
        this.connection = null;
        return this._end();
      }
    };

    DriverHandler.prototype.run_with_new_connection = function(query, _arg) {
      var callback, connection_error, optargs;
      callback = _arg.callback, connection_error = _arg.connection_error, optargs = _arg.optargs;
      this.close_connection();
      this.total_duration = 0;
      this.concurrent = 0;
      return driver.connect((function(_this) {
        return function(error, connection) {
          if (error != null) {
            connection_error(error);
          }
          connection.removeAllListeners('error');
          connection.on('error', function(error) {
            return connection_error(error);
          });
          _this.connection = connection;
          _this._begin();
          return query.private_run(connection, optargs, function(error, result) {
            _this._end();
            return callback(error, result);
          });
        };
      })(this));
    };

    DriverHandler.prototype.cursor_next = function(cursor, _arg) {
      var end, error, row;
      error = _arg.error, row = _arg.row, end = _arg.end;
      if (this.connection == null) {
        end();
      }
      this._begin();
      return cursor.next((function(_this) {
        return function(err, row_) {
          _this._end();
          if (err != null) {
            if (err.message === 'No more rows in the cursor.') {
              return end();
            } else {
              return error(err);
            }
          } else {
            return row(row_);
          }
        };
      })(this));
    };

    DriverHandler.prototype.remove = function() {
      return this.close_connection();
    };

    return DriverHandler;

  })();
  return Utils = {
    date_to_string: function(date) {
      var d, d_shifted, d_shifted_bis, hours, raw_date_str, sign, str_pieces, timezone, timezone_array, timezone_int;
      timezone = date.timezone;
      timezone_array = date.timezone.split(':');
      sign = timezone_array[0][0];
      timezone_array[0] = timezone_array[0].slice(1);
      timezone_int = (parseInt(timezone_array[0]) * 60 + parseInt(timezone_array[1])) * 60;
      if (sign === '-') {
        timezone_int = -1 * timezone_int;
      }
      d = new Date(date.epoch_time * 1000);
      timezone_int += d.getTimezoneOffset() * 60;
      d_shifted = new Date((date.epoch_time + timezone_int) * 1000);
      if (d.getTimezoneOffset() !== d_shifted.getTimezoneOffset()) {
        d_shifted_bis = new Date((date.epoch_time + timezone_int - (d.getTimezoneOffset() - d_shifted.getTimezoneOffset()) * 60) * 1000);
        if (d_shifted.getTimezoneOffset() !== d_shifted_bis.getTimezoneOffset()) {
          str_pieces = d_shifted_bis.toString().match(/([^ ]* )([^ ]* )([^ ]* )([^ ]* )(\d{2})(.*)/);
          hours = parseInt(str_pieces[5]);
          hours++;
          if (hours.toString().length === 1) {
            hours = "0" + hours.toString();
          } else {
            hours = hours.toString();
          }
          raw_date_str = str_pieces[1] + " " + str_pieces[2] + " " + str_pieces[3] + " " + str_pieces[4] + " " + hours + str_pieces[6];
        } else {
          raw_date_str = d_shifted_bis.toString();
        }
      } else {
        raw_date_str = d_shifted.toString();
      }
      return raw_date_str.slice(0, raw_date_str.indexOf('GMT') + 3) + timezone;
    },
    prettify_duration: function(duration) {
      var minutes;
      if (duration < 1) {
        return '<1ms';
      } else if (duration < 1000) {
        return duration.toFixed(0) + "ms";
      } else if (duration < 60 * 1000) {
        return (duration / 1000).toFixed(2) + "s";
      } else {
        minutes = Math.floor(duration / (60 * 1000));
        return minutes + "min " + ((duration - minutes * 60 * 1000) / 1000).toFixed(2) + "s";
      }
    },
    binary_to_string: function(bin) {
      var base64_digits, blocks_of_4, blocks_of_76, char, end, i, leftover, next, number_of_equals, size, sizeStr, snippet, str, _i, _len;
      blocks_of_76 = Math.floor(bin.data.length / 78);
      leftover = bin.data.length - blocks_of_76 * 78;
      base64_digits = 76 * blocks_of_76 + leftover;
      blocks_of_4 = Math.floor(base64_digits / 4);
      end = bin.data.slice(-2);
      if (end === '==') {
        number_of_equals = 2;
      } else if (end.slice(-1) === '=') {
        number_of_equals = 1;
      } else {
        number_of_equals = 0;
      }
      size = 3 * blocks_of_4 - number_of_equals;
      if (size >= 1073741824) {
        sizeStr = (size / 1073741824).toFixed(1) + 'GB';
      } else if (size >= 1048576) {
        sizeStr = (size / 1048576).toFixed(1) + 'MB';
      } else if (size >= 1024) {
        sizeStr = (size / 1024).toFixed(1) + 'KB';
      } else if (size === 1) {
        sizeStr = size + ' byte';
      } else {
        sizeStr = size + ' bytes';
      }
      if (size === 0) {
        return "<binary, " + sizeStr + ">";
      } else {
        str = atob(bin.data.slice(0, 8));
        snippet = '';
        for (i = _i = 0, _len = str.length; _i < _len; i = ++_i) {
          char = str[i];
          next = str.charCodeAt(i).toString(16);
          if (next.length === 1) {
            next = "0" + next;
          }
          snippet += next;
          if (i !== str.length - 1) {
            snippet += " ";
          }
        }
        if (size > str.length) {
          snippet += "...";
        }
        return "<binary, " + sizeStr + ", \"" + snippet + "\">";
      }
    },
    json_to_node: (function() {
      var json_to_node, template;
      template = {
        span: Handlebars.templates['dataexplorer_result_json_tree_span-template'],
        span_with_quotes: Handlebars.templates['dataexplorer_result_json_tree_span_with_quotes-template'],
        url: Handlebars.templates['dataexplorer_result_json_tree_url-template'],
        email: Handlebars.templates['dataexplorer_result_json_tree_email-template'],
        object: Handlebars.templates['dataexplorer_result_json_tree_object-template'],
        array: Handlebars.templates['dataexplorer_result_json_tree_array-template']
      };
      return json_to_node = function(value) {
        var data, element, key, last_key, output, sub_keys, sub_values, value_type, _i, _j, _len, _len1;
        value_type = typeof value;
        output = '';
        if (value === null) {
          return template.span({
            classname: 'jt_null',
            value: 'null'
          });
        } else if (Object.prototype.toString.call(value) === '[object Array]') {
          if (value.length === 0) {
            return '[ ]';
          } else {
            sub_values = [];
            for (_i = 0, _len = value.length; _i < _len; _i++) {
              element = value[_i];
              sub_values.push({
                value: json_to_node(element)
              });
              if (typeof element === 'string' && (/^(http|https):\/\/[^\s]+$/i.test(element) || /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(element))) {
                sub_values[sub_values.length - 1]['no_comma'] = true;
              }
            }
            sub_values[sub_values.length - 1]['no_comma'] = true;
            return template.array({
              values: sub_values
            });
          }
        } else if (Object.prototype.toString.call(value) === '[object Object]' && value.$reql_type$ === 'TIME') {
          return template.span({
            classname: 'jt_date',
            value: Utils.date_to_string(value)
          });
        } else if (Object.prototype.toString.call(value) === '[object Object]' && value.$reql_type$ === 'BINARY') {
          return template.span({
            classname: 'jt_bin',
            value: Utils.binary_to_string(value)
          });
        } else if (value_type === 'object') {
          sub_keys = [];
          for (key in value) {
            sub_keys.push(key);
          }
          sub_keys.sort();
          sub_values = [];
          for (_j = 0, _len1 = sub_keys.length; _j < _len1; _j++) {
            key = sub_keys[_j];
            last_key = key;
            sub_values.push({
              key: key,
              value: json_to_node(value[key])
            });
            if (typeof value[key] === 'string' && (/^(http|https):\/\/[^\s]+$/i.test(value[key]) || /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(value[key]))) {
              sub_values[sub_values.length - 1]['no_comma'] = true;
            }
          }
          if (sub_values.length !== 0) {
            sub_values[sub_values.length - 1]['no_comma'] = true;
          }
          data = {
            no_values: false,
            values: sub_values
          };
          if (sub_values.length === 0) {
            data.no_value = true;
          }
          return template.object(data);
        } else if (value_type === 'number') {
          return template.span({
            classname: 'jt_num',
            value: value
          });
        } else if (value_type === 'string') {
          if (/^(http|https):\/\/[^\s]+$/i.test(value)) {
            return template.url({
              url: value
            });
          } else if (/^[-0-9a-z.+_]+@[-0-9a-z.+_]+\.[a-z]{2,4}/i.test(value)) {
            return template.email({
              email: value
            });
          } else {
            return template.span_with_quotes({
              classname: 'jt_string',
              value: value
            });
          }
        } else if (value_type === 'boolean') {
          return template.span({
            classname: 'jt_bool',
            value: value ? 'true' : 'false'
          });
        }
      };
    })()
  };
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('TopBar', function() {
  this.Container = (function(_super) {
    __extends(Container, _super);

    function Container() {
      this.remove = __bind(this.remove, this);
      this.remove_parent_alert = __bind(this.remove_parent_alert, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return Container.__super__.constructor.apply(this, arguments);
    }

    Container.prototype.className = 'sidebar-container';

    Container.prototype.template = Handlebars.templates['sidebar-container-template'];

    Container.prototype.initialize = function(data) {
      this.model = data.model;
      this.issues = data.issues;
      this.client_panel = new TopBar.ClientConnectionStatus({
        model: this.model
      });
      this.servers_panel = new TopBar.ServersConnected({
        model: this.model
      });
      this.tables_panel = new TopBar.TablesAvailable({
        model: this.model
      });
      this.issues_panel = new TopBar.Issues({
        model: this.model
      });
      return this.issues_banner = new TopBar.IssuesBanner({
        model: this.model,
        collection: this.issues,
        container: this
      });
    };

    Container.prototype.render = function() {
      this.$el.html(this.template());
      this.$('.client-connection-status').html(this.client_panel.render().el);
      this.$('.servers-connected').html(this.servers_panel.render().el);
      this.$('.tables-available').html(this.tables_panel.render().el);
      this.$('.issues').html(this.issues_panel.render().el);
      this.$('.issues-banner').html(this.issues_banner.render().el);
      return this;
    };

    Container.prototype.remove_parent_alert = function(event) {
      var element;
      element = $(event.target).parent();
      return element.slideUp('fast', (function(_this) {
        return function() {
          element.remove();
          _this.issues_being_resolved();
          return _this.issues_banner.render();
        };
      })(this));
    };

    Container.prototype.remove = function() {
      this.client_panel.remove();
      this.servers_panel.remove();
      this.tables_panel.remove();
      this.issues_panel.remove();
      this.issues_banner.remove();
      return Container.__super__.remove.call(this);
    };

    return Container;

  })(Backbone.View);
  this.ClientConnectionStatus = (function(_super) {
    __extends(ClientConnectionStatus, _super);

    function ClientConnectionStatus() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ClientConnectionStatus.__super__.constructor.apply(this, arguments);
    }

    ClientConnectionStatus.prototype.className = 'client-connection-status';

    ClientConnectionStatus.prototype.template = Handlebars.templates['sidebar-client_connection_status-template'];

    ClientConnectionStatus.prototype.initialize = function() {
      return this.listenTo(this.model, 'change:me', this.render);
    };

    ClientConnectionStatus.prototype.render = function() {
      this.$el.html(this.template({
        disconnected: false,
        me: this.model.get('me')
      }));
      return this;
    };

    ClientConnectionStatus.prototype.remove = function() {
      this.stopListening();
      return ClientConnectionStatus.__super__.remove.call(this);
    };

    return ClientConnectionStatus;

  })(Backbone.View);
  this.ServersConnected = (function(_super) {
    __extends(ServersConnected, _super);

    function ServersConnected() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return ServersConnected.__super__.constructor.apply(this, arguments);
    }

    ServersConnected.prototype.template = Handlebars.templates['sidebar-servers_connected-template'];

    ServersConnected.prototype.initialize = function() {
      this.listenTo(this.model, 'change:num_servers', this.render);
      return this.listenTo(this.model, 'change:num_available_servers', this.render);
    };

    ServersConnected.prototype.render = function() {
      this.$el.html(this.template({
        num_available_servers: this.model.get('num_available_servers'),
        num_servers: this.model.get('num_servers'),
        error: this.model.get('num_available_servers') < this.model.get('num_servers')
      }));
      return this;
    };

    ServersConnected.prototype.remove = function() {
      this.stopListening();
      return ServersConnected.__super__.remove.call(this);
    };

    return ServersConnected;

  })(Backbone.View);
  this.TablesAvailable = (function(_super) {
    __extends(TablesAvailable, _super);

    function TablesAvailable() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return TablesAvailable.__super__.constructor.apply(this, arguments);
    }

    TablesAvailable.prototype.template = Handlebars.templates['sidebar-tables_available-template'];

    TablesAvailable.prototype.initialize = function() {
      this.listenTo(this.model, 'change:num_tables', this.render);
      return this.listenTo(this.model, 'change:num_available_tables', this.render);
    };

    TablesAvailable.prototype.render = function() {
      this.$el.html(this.template({
        num_tables: this.model.get('num_tables'),
        num_available_tables: this.model.get('num_available_tables'),
        error: this.model.get('num_available_tables') < this.model.get('num_tables')
      }));
      return this;
    };

    TablesAvailable.prototype.remove = function() {
      this.stopListening();
      return TablesAvailable.__super__.remove.call(this);
    };

    return TablesAvailable;

  })(Backbone.View);
  this.Issues = (function(_super) {
    __extends(Issues, _super);

    function Issues() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return Issues.__super__.constructor.apply(this, arguments);
    }

    Issues.prototype.className = 'issues';

    Issues.prototype.template = Handlebars.templates['sidebar-issues-template'];

    Issues.prototype.initialize = function() {
      return this.listenTo(this.model, 'change:num_issues', this.render);
    };

    Issues.prototype.render = function() {
      this.$el.html(this.template({
        num_issues: this.model.get('num_issues')
      }));
      return this;
    };

    Issues.prototype.remove = function() {
      this.stopListening();
      return Issues.__super__.remove.call(this);
    };

    return Issues;

  })(Backbone.View);
  return this.IssuesBanner = (function(_super) {
    __extends(IssuesBanner, _super);

    function IssuesBanner() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.toggle_display = __bind(this.toggle_display, this);
      this.initialize = __bind(this.initialize, this);
      return IssuesBanner.__super__.constructor.apply(this, arguments);
    }

    IssuesBanner.prototype.template = Handlebars.templates['sidebar-issues_banner-template'];

    IssuesBanner.prototype.resolve_issues_route = '#resolve_issues';

    IssuesBanner.prototype.events = {
      'click .btn-resolve-issues': 'toggle_display',
      'click .change-route': 'toggle_display'
    };

    IssuesBanner.prototype.initialize = function(data) {
      this.model = data.model;
      this.collection = data.collection;
      this.container = data.container;
      this.issues_view = [];
      this.show_resolve = true;
      this.listenTo(this.model, 'change:num_issues', this.render);
      this.collection.on('change:fixed', (function(_this) {
        return function() {
          var query;
          query = driver.queries.issues_with_ids();
          return driver.run_once(query, function(err, result) {
            return _this.collection.set(result);
          });
        };
      })(this));
      this.collection.each((function(_this) {
        return function(issue) {
          var view;
          view = new ResolveIssuesView.Issue({
            model: issue
          });
          _this.issues_view.push(view);
          return _this.$('.issues_list').append(view.render().$el);
        };
      })(this));
      this.collection.on('add', (function(_this) {
        return function(issue) {
          var view;
          view = new ResolveIssuesView.Issue({
            model: issue
          });
          _this.issues_view.push(view);
          _this.$('.issues_list').append(view.render().$el);
          return _this.render();
        };
      })(this));
      return this.collection.on('remove', (function(_this) {
        return function(issue) {
          var view, _i, _len, _ref, _results;
          _ref = _this.issues_view;
          _results = [];
          for (_i = 0, _len = _ref.length; _i < _len; _i++) {
            view = _ref[_i];
            if (view.model === issue) {
              issue.destroy();
              (function(view) {
                return view.$el.slideUp('fast', (function(_this) {
                  return function() {
                    return view.remove();
                  };
                })(this));
              })(view);
              break;
            } else {
              _results.push(void 0);
            }
          }
          return _results;
        };
      })(this));
    };

    IssuesBanner.prototype.toggle_display = function() {
      var btn;
      this.show_resolve = !this.show_resolve;
      if (this.show_resolve) {
        this.$('.all-issues').slideUp(300, "swing");
      } else {
        this.$('.all-issues').slideDown(300, "swing");
      }
      return btn = this.$('.btn-resolve-issues').toggleClass('show-issues').toggleClass('hide-issues').text(this.show_resolve ? 'Resolve issues' : 'Hide issues');
    };

    IssuesBanner.prototype.render = function() {
      this.$el.html(this.template({
        num_issues: this.model.get('num_issues'),
        no_issues: this.model.get('num_issues') === 0,
        show_banner: this.model.get('num_issues') > 0,
        show_resolve: this.show_resolve
      }));
      this.issues_view.forEach((function(_this) {
        return function(issue_view) {
          return _this.$('.issues_list').append(issue_view.render().$el);
        };
      })(this));
      return this;
    };

    IssuesBanner.prototype.remove = function() {
      this.issues_view.each(function(view) {
        return view.remove();
      });
      this.stopListening();
      return IssuesBanner.__super__.remove.call(this);
    };

    return IssuesBanner;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('ResolveIssuesView', function() {
  this.templates = {
    log_write_error: Handlebars.templates['issue-log_write_error'],
    outdated_index: Handlebars.templates['issue-outdated_index'],
    server_disconnected: Handlebars.templates['issue-server_disconnected'],
    server_ghost: Handlebars.templates['issue-server_ghost'],
    server_name_collision: Handlebars.templates['issue-name-collision'],
    db_name_collision: Handlebars.templates['issue-name-collision'],
    table_name_collision: Handlebars.templates['issue-name-collision'],
    table_needs_primary: Handlebars.templates['issue-table_needs_primary'],
    data_lost: Handlebars.templates['issue-data_lost'],
    write_acks: Handlebars.templates['issue-write-acks'],
    unknown: Handlebars.templates['resolve_issues-unknown-template']
  };
  return this.Issue = (function(_super) {
    __extends(Issue, _super);

    function Issue() {
      this.remove_server = __bind(this.remove_server, this);
      this.rename = __bind(this.rename, this);
      this.parseCollisionType = __bind(this.parseCollisionType, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return Issue.__super__.constructor.apply(this, arguments);
    }

    Issue.prototype.className = 'issue-container';

    Issue.prototype.initialize = function(data) {
      this.type = data.model.get('type');
      this.model.set('collisionType', this.parseCollisionType(this.model.get('type')));
      return this.listenTo(this.model, 'change', this.render);
    };

    Issue.prototype.events = {
      'click .remove-server': 'remove_server',
      'click .rename': 'rename'
    };

    Issue.prototype.render = function() {
      var templates;
      templates = ResolveIssuesView.templates;
      if (templates[this.type] != null) {
        this.$el.html(templates[this.type](this.model.toJSON()));
      } else {
        this.$el.html(templates.unknown(this.model.toJSON()));
      }
      return this;
    };

    Issue.prototype.parseCollisionType = function(fulltype) {
      var match;
      match = fulltype.match(/(db|server|table)_name_collision/);
      if (match != null) {
        if (match[1] === 'db') {
          return 'database';
        } else {
          return match[1];
        }
      } else {
        return match;
      }
    };

    Issue.prototype.rename = function(event) {
      var Type;
      Type = (function() {
        switch (this.model.get('collisionType')) {
          case 'database':
            return Database;
          case 'server':
            return Server;
          case 'table':
            return Table;
        }
      }).call(this);
      this.modal = new UIComponents.RenameItemModal({
        model: new Type({
          name: this.model.get('info').name,
          id: $(event.target).attr('id').match(/rename_(.*)/)[1]
        })
      });
      return this.modal.render();
    };

    Issue.prototype.remove_server = function(event) {
      var modalModel;
      modalModel = new Backbone.Model({
        name: this.model.get('info').disconnected_server,
        id: this.model.get('info').disconnected_server_id,
        parent: this.model
      });
      this.modal = new Modals.RemoveServerModal({
        model: modalModel
      });
      return this.modal.render();
    };

    return Issue;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('LogView', function() {
  this.LogsContainer = (function(_super) {
    __extends(LogsContainer, _super);

    function LogsContainer() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.fetch_data = __bind(this.fetch_data, this);
      this.increase_limit = __bind(this.increase_limit, this);
      this.initialize = __bind(this.initialize, this);
      return LogsContainer.__super__.constructor.apply(this, arguments);
    }

    LogsContainer.prototype.template = {
      main: Handlebars.templates['logs_container-template'],
      error: Handlebars.templates['error-query-template'],
      alert_message: Handlebars.templates['alert_message-template']
    };

    LogsContainer.prototype.events = {
      'click .next-log-entries': 'increase_limit'
    };

    LogsContainer.prototype.initialize = function(obj) {
      if (obj == null) {
        obj = {};
      }
      this.limit = obj.limit || 20;
      this.current_limit = this.limit;
      this.server_id = obj.server_id;
      this.container_rendered = false;
      this.query = obj.query || driver.queries.all_logs;
      this.logs = new Logs();
      this.logs_list = new LogView.LogsListView({
        collection: this.logs
      });
      return this.fetch_data();
    };

    LogsContainer.prototype.increase_limit = function(e) {
      e.preventDefault();
      this.current_limit += this.limit;
      driver.stop_timer(this.timer);
      return this.fetch_data();
    };

    LogsContainer.prototype.fetch_data = function() {
      return this.timer = driver.run(this.query(this.current_limit, this.server_id), 5000, (function(_this) {
        return function(error, result) {
          var index, log, _i, _len, _ref;
          if (error != null) {
            if (((_ref = _this.error) != null ? _ref.msg : void 0) !== error.msg) {
              _this.error = error;
              return _this.$el.html(_this.template.error({
                url: '#logs',
                error: error.message
              }));
            }
          } else {
            for (index = _i = 0, _len = result.length; _i < _len; index = ++_i) {
              log = result[index];
              _this.logs.add(new Log(log), {
                merge: true,
                index: index
              });
            }
            return _this.render();
          }
        };
      })(this));
    };

    LogsContainer.prototype.render = function() {
      if (!this.container_rendered) {
        this.$el.html(this.template.main);
        this.$('.log-entries-wrapper').html(this.logs_list.render().$el);
        this.container_rendered = true;
      } else {
        this.logs_list.render();
      }
      return this;
    };

    LogsContainer.prototype.remove = function() {
      driver.stop_timer(this.timer);
      return LogsContainer.__super__.remove.call(this);
    };

    return LogsContainer;

  })(Backbone.View);
  this.LogsListView = (function(_super) {
    __extends(LogsListView, _super);

    function LogsListView() {
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return LogsListView.__super__.constructor.apply(this, arguments);
    }

    LogsListView.prototype.tagName = 'ul';

    LogsListView.prototype.className = 'log-entries';

    LogsListView.prototype.initialize = function() {
      this.log_view_cache = {};
      if (this.collection.length === 0) {
        this.$('.no-logs').show();
      } else {
        this.$('.no-logs').hide();
      }
      this.listenTo(this.collection, 'add', (function(_this) {
        return function(log) {
          var position, view;
          view = _this.log_view_cache[log.get('id')];
          if (view == null) {
            view = new LogView.LogView({
              model: log
            });
            _this.log_view_cache[log.get('id')];
          }
          position = _this.collection.indexOf(log);
          if (_this.collection.length === 1) {
            _this.$el.html(view.render().$el);
          } else if (position === 0) {
            _this.$('.log-entry').prepend(view.render().$el);
          } else {
            _this.$('.log-entry').eq(position - 1).after(view.render().$el);
          }
          return _this.$('.no-logs').hide();
        };
      })(this));
      return this.listenTo(this.collection, 'remove', (function(_this) {
        return function(log) {
          var log_view;
          log_view = log_view_cache[log.get('id')];
          log_view.$el.slideUp('fast', function() {
            return log_view.remove();
          });
          delete log_view[log.get('id')];
          if (_this.collection.length === 0) {
            return _this.$('.no-logs').show();
          }
        };
      })(this));
    };

    LogsListView.prototype.render = function() {
      return this;
    };

    return LogsListView;

  })(Backbone.View);
  return this.LogView = (function(_super) {
    __extends(LogView, _super);

    function LogView() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return LogView.__super__.constructor.apply(this, arguments);
    }

    LogView.prototype.tagName = 'li';

    LogView.prototype.className = 'log-entry';

    LogView.prototype.template = Handlebars.templates['log-entry'];

    LogView.prototype.initialize = function() {
      return this.model.on('change', this.render);
    };

    LogView.prototype.render = function() {
      var template_model;
      template_model = this.model.toJSON();
      template_model['iso'] = template_model.timestamp.toISOString();
      this.$el.html(this.template(template_model));
      this.$('time.timeago').timeago();
      return this;
    };

    LogView.prototype.remove = function() {
      this.stopListening();
      return LogView.__super__.remove.call(this);
    };

    return LogView;

  })(Backbone.View);
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('Vis', function() {
  this.num_formatter = function(i) {
    var res;
    if (isNaN(i)) {
      return 'N/A';
    }
    if (i / 1000000000 >= 1) {
      res = '' + ((i / 1000000000).toFixed(1));
      if (res.slice(-2) === '.0') {
        res = res.slice(0, -2);
      }
      res += 'B';
    } else if (i / 1000000 >= 1) {
      res = '' + ((i / 1000000).toFixed(1));
      if (res.slice(-2) === '.0') {
        res = res.slice(0, -2);
      }
      res += 'M';
    } else if (i / 1000 >= 1) {
      res = '' + ((i / 1000).toFixed(1));
      if (res.slice(-2) === '.0') {
        res = res.slice(0, -2);
      }
      res += 'K';
    } else {
      res = '' + i.toFixed(0);
    }
    return res;
  };
  this.OpsPlotLegend = (function(_super) {
    __extends(OpsPlotLegend, _super);

    function OpsPlotLegend() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return OpsPlotLegend.__super__.constructor.apply(this, arguments);
    }

    OpsPlotLegend.prototype.className = 'ops-plot-legend';

    OpsPlotLegend.prototype.template = Handlebars.templates['ops_plot_legend-template'];

    OpsPlotLegend.prototype.initialize = function(_read_metric, _write_metric, _context) {
      this.context = _context;
      this.reads = null;
      this.writes = null;
      this.read_metric = _read_metric;
      return this.read_metric.on('change', (function(_this) {
        return function() {
          _this.reads = _read_metric.valueAt(_this.context.size() - 1);
          _this.writes = _write_metric.valueAt(_this.context.size() - 1);
          return _this.render();
        };
      })(this));
    };

    OpsPlotLegend.prototype.render = function() {
      this.$el.html(this.template({
        read_count: this.reads != null ? Vis.num_formatter(this.reads) : 'N/A',
        write_count: this.writes != null ? Vis.num_formatter(this.writes) : 'N/A'
      }));
      return this;
    };

    OpsPlotLegend.prototype.remove = function() {
      this.context.stop();
      return OpsPlotLegend.__super__.remove.call(this);
    };

    return OpsPlotLegend;

  })(Backbone.View);
  this.OpsPlot = (function(_super) {
    __extends(OpsPlot, _super);

    function OpsPlot() {
      this.remove = __bind(this.remove, this);
      this.render = __bind(this.render, this);
      this.make_metric = __bind(this.make_metric, this);
      return OpsPlot.__super__.constructor.apply(this, arguments);
    }

    OpsPlot.prototype.className = 'ops-plot';

    OpsPlot.prototype.template = Handlebars.templates['ops_plot-template'];

    OpsPlot.prototype.barebones_template = Handlebars.templates['ops_plot-template'];

    OpsPlot.prototype.type = 'cluster';

    OpsPlot.prototype.HEIGHT_IN_PIXELS = 200;

    OpsPlot.prototype.HEIGHT_IN_UNITS = 20500;

    OpsPlot.prototype.WIDTH_IN_PIXELS = 300;

    OpsPlot.prototype.WIDTH_IN_SECONDS = 60;

    OpsPlot.prototype.HAXIS_TICK_INTERVAL_IN_SECS = 15;

    OpsPlot.prototype.HAXIS_MINOR_SUBDIVISION_COUNT = 3;

    OpsPlot.prototype.VAXIS_TICK_SUBDIVISION_COUNT = 5;

    OpsPlot.prototype.VAXIS_MINOR_SUBDIVISION_COUNT = 2;

    OpsPlot.prototype.make_metric = function(name) {
      var cache, _metric;
      cache = new Vis.InterpolatingCache(this.WIDTH_IN_PIXELS, this.WIDTH_IN_PIXELS / this.WIDTH_IN_SECONDS, ((function(_this) {
        return function() {
          return _this.stats_fn()[name];
        };
      })(this)));
      _metric = (function(_this) {
        return function(start, stop, step, callback) {
          var num_desired;
          start = +start;
          stop = +stop;
          num_desired = (stop - start) / step;
          return callback(null, cache.step(num_desired));
        };
      })(this);
      this.context.on('focus', (function(_this) {
        return function(i) {
          return d3.selectAll('.value').style('right', i === null ? null : _this.context.size() - i + 'px');
        };
      })(this));
      return this.context.metric(_metric, name);
    };

    OpsPlot.prototype.initialize = function(_stats_fn, options) {
      if (options != null) {
        if (options.height != null) {
          this.HEIGHT_IN_PIXELS = options.height;
        }
        if (options.height_in_units != null) {
          this.HEIGHT_IN_UNITS = options.height_in_units;
        }
        if (options.width != null) {
          this.WIDTH_IN_PIXELS = options.width;
        }
        if (options.seconds != null) {
          this.WIDTH_IN_SECONDS = options.seconds;
        }
        if (options.haxis != null) {
          if (options.haxis.seconds_per_tick != null) {
            this.HAXIS_TICK_INTERVAL_IN_SECS = options.haxis.seconds_per_tick;
          }
          if (options.haxis.ticks_per_label != null) {
            this.HAXIS_MINOR_SUBDIVISION_COUNT = options.haxis.ticks_per_label;
          }
        }
        if (options.vaxis != null) {
          if (options.vaxis.num_ticks != null) {
            this.VAXIS_TICK_SUBDIVISION_COUNT = options.vaxis.num_ticks;
          }
          if (options.vaxis.ticks_per_label != null) {
            this.VAXIS_MINOR_SUBDIVISION_COUNT = options.vaxis.ticks_per_label;
          }
        }
        if (options.type != null) {
          this.type = options.type;
        }
      }
      OpsPlot.__super__.initialize.apply(this, arguments);
      this.stats_fn = _stats_fn;
      this.context = cubism.context().serverDelay(0).clientDelay(0).step(1000 / (this.WIDTH_IN_PIXELS / this.WIDTH_IN_SECONDS)).size(this.WIDTH_IN_PIXELS);
      this.read_stats = this.make_metric('keys_read');
      this.write_stats = this.make_metric('keys_set');
      return this.legend = new Vis.OpsPlotLegend(this.read_stats, this.write_stats, this.context);
    };

    OpsPlot.prototype.render = function() {
      this.$el.html(this.template({
        cluster: this.type === 'cluster',
        datacenter: this.type === 'datacenter',
        server: this.type === 'server',
        database: this.type === 'database',
        table: this.type === 'table'
      }));
      this.sensible_plot = this.context.sensible().height(this.HEIGHT_IN_PIXELS).colors(["#983434", "#729E51"]).extent([0, this.HEIGHT_IN_UNITS]);
      d3.select(this.$('.plot')[0]).call((function(_this) {
        return function(div) {
          div.data([[_this.read_stats, _this.write_stats]]);
          _this.selection = div.append('div').attr('class', 'chart');
          _this.selection.call(_this.sensible_plot);
          div.append('div').attr('class', 'haxis').call(_this.context.axis().orient('bottom').ticks(d3.time.seconds, _this.HAXIS_TICK_INTERVAL_IN_SECS).tickSubdivide(_this.HAXIS_MINOR_SUBDIVISION_COUNT - 1).tickSize(6, 3, 0).tickFormat(d3.time.format('%I:%M:%S')));
          div.append('div').attr('class', 'hgrid').call(_this.context.axis().orient('bottom').ticks(d3.time.seconds, _this.HAXIS_TICK_INTERVAL_IN_SECS).tickSize(_this.HEIGHT_IN_PIXELS + 4, 0, 0).tickFormat(""));
          div.append('div').attr('class', 'vaxis').call(_this.context.axis(_this.HEIGHT_IN_PIXELS, [_this.read_stats, _this.write_stats], _this.sensible_plot.scale()).orient('left').ticks(_this.VAXIS_TICK_SUBDIVISION_COUNT).tickSubdivide(_this.VAXIS_MINOR_SUBDIVISION_COUNT - 1).tickSize(6, 3, 0).tickFormat(Vis.num_formatter));
          div.append('div').attr('class', 'vgrid').call(_this.context.axis(_this.HEIGHT_IN_PIXELS, [_this.read_stats, _this.write_stats], _this.sensible_plot.scale()).orient('left').ticks(_this.VAXIS_TICK_SUBDIVISION_COUNT).tickSize(-(_this.WIDTH_IN_PIXELS + 35), 0, 0).tickFormat(""));
          return _this.$('.legend-container').html(_this.legend.render().el);
        };
      })(this));
      return this;
    };

    OpsPlot.prototype.remove = function() {
      this.sensible_plot.remove(this.selection);
      this.context.stop();
      this.legend.remove();
      return OpsPlot.__super__.remove.call(this);
    };

    return OpsPlot;

  })(Backbone.View);
  this.SizeBoundedCache = (function() {
    function SizeBoundedCache(num_data_points, _stat) {
      this.values = [];
      this.ndp = num_data_points;
      this.stat = _stat;
    }

    SizeBoundedCache.prototype.push = function(stats) {
      var i, value, _i, _ref, _ref1;
      if (typeof this.stat === 'function') {
        value = this.stat(stats);
      } else {
        if (stats != null) {
          value = stats[this.stat];
        }
      }
      if (isNaN(value)) {
        return;
      }
      this.values.push(value);
      if (this.values.length < this.ndp) {
        for (i = _i = _ref = this.values.length, _ref1 = this.ndp - 1; _ref <= _ref1 ? _i < _ref1 : _i > _ref1; i = _ref <= _ref1 ? ++_i : --_i) {
          this.values.push(value);
        }
      }
      if (this.values.length > this.ndp) {
        return this.values = this.values.slice(-this.ndp);
      }
    };

    SizeBoundedCache.prototype.get = function() {
      return this.values;
    };

    SizeBoundedCache.prototype.get_latest = function() {
      return this.values[this.values.length - 1];
    };

    return SizeBoundedCache;

  })();
  return this.InterpolatingCache = (function() {
    function InterpolatingCache(num_data_points, num_interpolation_points, get_data_fn) {
      var i, _i, _ref;
      this.ndp = num_data_points;
      this.nip = num_interpolation_points;
      this.get_data_fn = get_data_fn;
      this.values = [];
      for (i = _i = 0, _ref = this.ndp - 1; 0 <= _ref ? _i <= _ref : _i >= _ref; i = 0 <= _ref ? ++_i : --_i) {
        this.values.push(0);
      }
      this.next_value = null;
      this.last_date = null;
    }

    InterpolatingCache.prototype.step = function(num_points) {
      this.push_data();
      return this.values.slice(-num_points);
    };

    InterpolatingCache.prototype.push_data = function() {
      var current_value, elapsed_time, i, missing_steps, value_to_push, _i;
      if (this.last_date === null) {
        this.last_date = Date.now();
        this.values.push(this.get_data_fn());
        return true;
      }
      current_value = this.get_data_fn();
      if (this.next_value !== current_value) {
        this.start_value = this.values[this.values.length - 1];
        this.next_value = current_value;
        this.interpolation_step = 1;
      }
      elapsed_time = Date.now() - this.last_date;
      missing_steps = Math.max(1, Math.round(elapsed_time / 1000 * this.nip));
      for (i = _i = 1; 1 <= missing_steps ? _i <= missing_steps : _i >= missing_steps; i = 1 <= missing_steps ? ++_i : --_i) {
        if (this.values[this.values.length - 1] === this.next_value) {
          value_to_push = this.next_value;
        } else {
          value_to_push = this.start_value + ((this.next_value - this.start_value) / this.nip * this.interpolation_step);
          this.interpolation_step += 1;
          if (this.interpolation_step > this.nip) {
            value_to_push = this.next_value;
          }
        }
        this.values.push(value_to_push);
      }
      this.last_date = Date.now();
      if (this.values.length > this.ndp) {
        return this.values = this.values.slice(-this.ndp);
      }
    };

    return InterpolatingCache;

  })();
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; },
  __indexOf = [].indexOf || function(item) { for (var i = 0, l = this.length; i < l; i++) { if (i in this && this[i] === item) return i; } return -1; };

module('Modals', function() {
  this.AddDatabaseModal = (function(_super) {
    __extends(AddDatabaseModal, _super);

    function AddDatabaseModal() {
      this.on_success = __bind(this.on_success, this);
      this.on_submit = __bind(this.on_submit, this);
      this.render = __bind(this.render, this);
      this.initialize = __bind(this.initialize, this);
      return AddDatabaseModal.__super__.constructor.apply(this, arguments);
    }

    AddDatabaseModal.prototype.template = Handlebars.templates['add_database-modal-template'];

    AddDatabaseModal.prototype.templates = {
      error: Handlebars.templates['error_input-template']
    };

    AddDatabaseModal.prototype["class"] = 'add-database';

    AddDatabaseModal.prototype.initialize = function(databases) {
      AddDatabaseModal.__super__.initialize.apply(this, arguments);
      return this.databases = databases;
    };

    AddDatabaseModal.prototype.render = function() {
      AddDatabaseModal.__super__.render.call(this, {
        modal_title: "Add database",
        btn_primary_text: "Add"
      });
      return this.$('.focus_new_name').focus();
    };

    AddDatabaseModal.prototype.on_submit = function() {
      var database, error, _i, _len, _ref;
      AddDatabaseModal.__super__.on_submit.apply(this, arguments);
      this.formdata = form_data_as_object($('form', this.$modal));
      error = null;
      if (this.formdata.name === '') {
        error = true;
        this.$('.alert_modal').html(this.templates.error({
          database_is_empty: true
        }));
      } else if (/^[a-zA-Z0-9_]+$/.test(this.formdata.name) === false) {
        error = true;
        this.$('.alert_modal').html(this.templates.error({
          special_char_detected: true,
          type: 'database'
        }));
      } else {
        _ref = this.databases.models;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          database = _ref[_i];
          if (database.get('db') === this.formdata.name) {
            error = true;
            this.$('.alert_modal').html(this.templates.error({
              database_exists: true
            }));
            break;
          }
        }
      }
      if (error === null) {
        return driver.run_once(r.db(system_db).table("db_config").insert({
          name: this.formdata.name
        }), (function(_this) {
          return function(err, result) {
            if (err) {
              return _this.on_error(err);
            } else {
              if (result.inserted === 1) {
                _this.databases.add(new Database({
                  id: result.generated_keys[0],
                  name: _this.formdata.name,
                  tables: []
                }));
                return _this.on_success();
              } else {
                return _this.on_error(new Error(result.first_error || "Unknown error"));
              }
            }
          };
        })(this));
      } else {
        $('.alert_modal_content').slideDown('fast');
        return this.reset_buttons();
      }
    };

    AddDatabaseModal.prototype.on_success = function(response) {
      AddDatabaseModal.__super__.on_success.call(this);
      return window.app.current_view.render_message("The database " + this.formdata.name + " was successfully created.");
    };

    return AddDatabaseModal;

  })(UIComponents.AbstractModal);
  this.DeleteDatabaseModal = (function(_super) {
    __extends(DeleteDatabaseModal, _super);

    function DeleteDatabaseModal() {
      this.on_success = __bind(this.on_success, this);
      this.on_submit = __bind(this.on_submit, this);
      return DeleteDatabaseModal.__super__.constructor.apply(this, arguments);
    }

    DeleteDatabaseModal.prototype.template = Handlebars.templates['delete-database-modal'];

    DeleteDatabaseModal.prototype["class"] = 'delete_database-dialog';

    DeleteDatabaseModal.prototype.render = function(database_to_delete) {
      this.database_to_delete = database_to_delete;
      DeleteDatabaseModal.__super__.render.call(this, {
        modal_title: 'Delete database',
        btn_primary_text: 'Delete'
      });
      return this.$('.verification_name').focus();
    };

    DeleteDatabaseModal.prototype.on_submit = function() {
      if (this.$('.verification_name').val() !== this.database_to_delete.get('name')) {
        if (this.$('.mismatch_container').css('display') === 'none') {
          this.$('.mismatch_container').slideDown('fast');
        } else {
          this.$('.mismatch_container').hide();
          this.$('.mismatch_container').fadeIn();
        }
        this.reset_buttons();
        return true;
      }
      DeleteDatabaseModal.__super__.on_submit.apply(this, arguments);
      return driver.run_once(r.dbDrop(this.database_to_delete.get('name')), (function(_this) {
        return function(err, result) {
          if (err) {
            return _this.on_error(err);
          } else {
            if ((result != null ? result.dbs_dropped : void 0) === 1) {
              return _this.on_success();
            } else {
              return _this.on_error(new Error("The return result was not `{dbs_dropped: 1}`"));
            }
          }
        };
      })(this));
    };

    DeleteDatabaseModal.prototype.on_success = function(response) {
      DeleteDatabaseModal.__super__.on_success.call(this);
      if (Backbone.history.fragment === 'tables') {
        this.database_to_delete.destroy();
      } else {
        main_view.router.navigate('#tables', {
          trigger: true
        });
      }
      return window.app.current_view.render_message("The database " + (this.database_to_delete.get('name')) + " was successfully deleted.");
    };

    return DeleteDatabaseModal;

  })(UIComponents.AbstractModal);
  this.AddTableModal = (function(_super) {
    __extends(AddTableModal, _super);

    function AddTableModal() {
      this.remove = __bind(this.remove, this);
      this.on_success = __bind(this.on_success, this);
      this.on_submit = __bind(this.on_submit, this);
      this.render = __bind(this.render, this);
      this.hide_advanced_settings = __bind(this.hide_advanced_settings, this);
      this.show_advanced_settings = __bind(this.show_advanced_settings, this);
      this.initialize = __bind(this.initialize, this);
      return AddTableModal.__super__.constructor.apply(this, arguments);
    }

    AddTableModal.prototype.template = Handlebars.templates['add_table-modal-template'];

    AddTableModal.prototype.templates = {
      error: Handlebars.templates['error_input-template']
    };

    AddTableModal.prototype["class"] = 'add-table';

    AddTableModal.prototype.initialize = function(data) {
      AddTableModal.__super__.initialize.apply(this, arguments);
      this.db_id = data.db_id;
      this.db_name = data.db_name;
      this.tables = data.tables;
      this.container = data.container;
      return this.delegateEvents();
    };

    AddTableModal.prototype.show_advanced_settings = function(event) {
      event.preventDefault();
      this.$('.show_advanced_settings-link_container').fadeOut('fast', (function(_this) {
        return function() {
          return _this.$('.hide_advanced_settings-link_container').fadeIn('fast');
        };
      })(this));
      return this.$('.advanced_settings').slideDown('fast');
    };

    AddTableModal.prototype.hide_advanced_settings = function(event) {
      event.preventDefault();
      this.$('.hide_advanced_settings-link_container').fadeOut('fast', (function(_this) {
        return function() {
          return $('.show_advanced_settings-link_container').fadeIn('fast');
        };
      })(this));
      return this.$('.advanced_settings').slideUp('fast');
    };

    AddTableModal.prototype.render = function() {
      AddTableModal.__super__.render.call(this, {
        modal_title: "Add table to " + this.db_name,
        btn_primary_text: 'Create table'
      });
      this.$('.show_advanced_settings-link').click(this.show_advanced_settings);
      this.$('.hide_advanced_settings-link').click(this.hide_advanced_settings);
      return this.$('.focus-table-name').focus();
    };

    AddTableModal.prototype.on_submit = function() {
      var durability, input_error, primary_key, query, table, template_error, _i, _len, _ref;
      AddTableModal.__super__.on_submit.apply(this, arguments);
      this.formdata = form_data_as_object($('form', this.$modal));
      template_error = {};
      input_error = false;
      if (this.formdata.name === '') {
        input_error = true;
        template_error.table_name_empty = true;
      } else if (/^[a-zA-Z0-9_]+$/.test(this.formdata.name) === false) {
        input_error = true;
        template_error.special_char_detected = true;
        template_error.type = 'table';
      } else {
        _ref = this.tables;
        for (_i = 0, _len = _ref.length; _i < _len; _i++) {
          table = _ref[_i];
          if (table.name === this.formdata.name) {
            input_error = true;
            template_error.table_exists = true;
            break;
          }
        }
      }
      if (input_error === true) {
        $('.alert_modal').html(this.templates.error(template_error));
        $('.alert_modal_content').slideDown('fast');
        return this.reset_buttons();
      } else {
        if (this.formdata.write_disk === 'yes') {
          durability = 'soft';
        } else {
          durability = 'hard';
        }
        if (this.formdata.primary_key !== '') {
          primary_key = this.formdata.primary_key;
        } else {
          primary_key = 'id';
        }
        query = r.db(system_db).table("server_status").filter({
          status: "connected"
        }).coerceTo("ARRAY")["do"]((function(_this) {
          return function(servers) {
            return r.branch(servers.isEmpty(), r.error("No server is connected"), servers.sample(1).nth(0)("name")["do"](function(server) {
              return r.db(system_db).table("table_config").insert({
                db: _this.db_name,
                name: _this.formdata.name,
                primary_key: primary_key,
                shards: [
                  {
                    primary_replica: server,
                    replicas: [server]
                  }
                ]
              }, {
                returnChanges: true
              });
            }));
          };
        })(this));
        return driver.run_once(query, (function(_this) {
          return function(err, result) {
            if (err) {
              return _this.on_error(err);
            } else {
              if ((result != null ? result.errors : void 0) === 0) {
                return _this.on_success();
              } else {
                return _this.on_error(new Error("The returned result was not `{created: 1}`"));
              }
            }
          };
        })(this));
      }
    };

    AddTableModal.prototype.on_success = function(response) {
      AddTableModal.__super__.on_success.apply(this, arguments);
      return window.app.current_view.render_message("The table " + this.db_name + "." + this.formdata.name + " was successfully created.");
    };

    AddTableModal.prototype.remove = function() {
      this.stopListening();
      return AddTableModal.__super__.remove.call(this);
    };

    return AddTableModal;

  })(UIComponents.AbstractModal);
  this.RemoveTableModal = (function(_super) {
    __extends(RemoveTableModal, _super);

    function RemoveTableModal() {
      this.on_success = __bind(this.on_success, this);
      this.on_submit = __bind(this.on_submit, this);
      this.render = __bind(this.render, this);
      return RemoveTableModal.__super__.constructor.apply(this, arguments);
    }

    RemoveTableModal.prototype.template = Handlebars.templates['remove_table-modal-template'];

    RemoveTableModal.prototype["class"] = 'remove-namespace-dialog';

    RemoveTableModal.prototype.render = function(tables_to_delete) {
      this.tables_to_delete = tables_to_delete;
      RemoveTableModal.__super__.render.call(this, {
        modal_title: 'Delete tables',
        btn_primary_text: 'Delete',
        tables: tables_to_delete,
        single_delete: tables_to_delete.length === 1
      });
      return this.$('.btn-primary').focus();
    };

    RemoveTableModal.prototype.on_submit = function() {
      var query;
      RemoveTableModal.__super__.on_submit.apply(this, arguments);
      query = r.db(system_db).table('table_config').filter((function(_this) {
        return function(table) {
          return r.expr(_this.tables_to_delete).contains({
            database: table("db"),
            table: table("name")
          });
        };
      })(this))["delete"]({
        returnChanges: true
      });
      return driver.run_once(query, (function(_this) {
        return function(err, result) {
          var change, database, keep_going, position, table, _i, _j, _k, _len, _len1, _len2, _ref, _ref1, _ref2;
          if (err) {
            return _this.on_error(err);
          } else {
            if ((_this.collection != null) && ((result != null ? result.changes : void 0) != null)) {
              _ref = result.changes;
              for (_i = 0, _len = _ref.length; _i < _len; _i++) {
                change = _ref[_i];
                keep_going = true;
                _ref1 = _this.collection.models;
                for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
                  database = _ref1[_j];
                  if (keep_going === false) {
                    break;
                  }
                  if (database.get('name') === change.old_val.db) {
                    _ref2 = database.get('tables');
                    for (position = _k = 0, _len2 = _ref2.length; _k < _len2; position = ++_k) {
                      table = _ref2[position];
                      if (table.name === change.old_val.name) {
                        database.set({
                          tables: database.get('tables').slice(0, position).concat(database.get('tables').slice(position + 1))
                        });
                        keep_going = false;
                        break;
                      }
                    }
                  }
                }
              }
            }
            if ((result != null ? result.deleted : void 0) === _this.tables_to_delete.length) {
              return _this.on_success();
            } else {
              return _this.on_error(new Error("The value returned for `deleted` did not match the number of tables."));
            }
          }
        };
      })(this));
    };

    RemoveTableModal.prototype.on_success = function(response) {
      var index, message, table, _i, _len, _ref;
      RemoveTableModal.__super__.on_success.apply(this, arguments);
      message = "The tables ";
      _ref = this.tables_to_delete;
      for (index = _i = 0, _len = _ref.length; _i < _len; index = ++_i) {
        table = _ref[index];
        message += "" + table.database + "." + table.table;
        if (index < this.tables_to_delete.length - 1) {
          message += ", ";
        }
      }
      if (this.tables_to_delete.length === 1) {
        message += " was";
      } else {
        message += " were";
      }
      message += " successfully deleted.";
      if (Backbone.history.fragment !== 'tables') {
        main_view.router.navigate('#tables', {
          trigger: true
        });
      }
      window.app.current_view.render_message(message);
      return this.remove();
    };

    return RemoveTableModal;

  })(UIComponents.AbstractModal);
  this.RemoveServerModal = (function(_super) {
    __extends(RemoveServerModal, _super);

    function RemoveServerModal() {
      this.on_success_with_error = __bind(this.on_success_with_error, this);
      return RemoveServerModal.__super__.constructor.apply(this, arguments);
    }

    RemoveServerModal.prototype.template = Handlebars.templates['declare_server_dead-modal-template'];

    RemoveServerModal.prototype.alert_tmpl = Handlebars.templates['resolve_issues-resolved-template'];

    RemoveServerModal.prototype.template_issue_error = Handlebars.templates['fail_solve_issue-template'];

    RemoveServerModal.prototype["class"] = 'declare-server-dead';

    RemoveServerModal.prototype.initialize = function(data) {
      this.model = data.model;
      return RemoveServerModal.__super__.initialize.call(this, this.template);
    };

    RemoveServerModal.prototype.render = function() {
      RemoveServerModal.__super__.render.call(this, {
        server_name: this.model.get("name"),
        modal_title: "Permanently remove the server",
        btn_primary_text: 'Remove ' + this.model.get('name')
      });
      return this;
    };

    RemoveServerModal.prototype.on_submit = function() {
      var query;
      RemoveServerModal.__super__.on_submit.apply(this, arguments);
      if (this.$('.verification').val().toLowerCase() === 'remove') {
        query = r.db(system_db).table('server_config').get(this.model.get('id'))["delete"]();
        return driver.run_once(query, (function(_this) {
          return function(err, result) {
            if (err != null) {
              return _this.on_success_with_error();
            } else {
              return _this.on_success();
            }
          };
        })(this));
      } else {
        this.$('.error_verification').slideDown('fast');
        return this.reset_buttons();
      }
    };

    RemoveServerModal.prototype.on_success_with_error = function() {
      this.$('.error_answer').html(this.template_issue_error);
      if (this.$('.error_answer').css('display') === 'none') {
        this.$('.error_answer').slideDown('fast');
      } else {
        this.$('.error_answer').css('display', 'none');
        this.$('.error_answer').fadeIn();
      }
      return this.reset_buttons();
    };

    RemoveServerModal.prototype.on_success = function(response) {
      if (response) {
        this.on_success_with_error();
        return;
      }
      $('#issue-alerts').append(this.alert_tmpl({
        server_dead: {
          server_name: this.model.get("name")
        }
      }));
      this.remove();
      this.model.get('parent').set('fixed', true);
      return this;
    };

    return RemoveServerModal;

  })(UIComponents.AbstractModal);
  return this.ReconfigureModal = (function(_super) {
    __extends(ReconfigureModal, _super);

    function ReconfigureModal() {
      this.fixup_shards = __bind(this.fixup_shards, this);
      this.fetch_dryrun = __bind(this.fetch_dryrun, this);
      this.get_errors = __bind(this.get_errors, this);
      this.change_shards = __bind(this.change_shards, this);
      this.change_replicas = __bind(this.change_replicas, this);
      this.remove = __bind(this.remove, this);
      this.change_errors = __bind(this.change_errors, this);
      this.render = __bind(this.render, this);
      this.expanded_changed = __bind(this.expanded_changed, this);
      this.expand_link = __bind(this.expand_link, this);
      this.initialize = __bind(this.initialize, this);
      return ReconfigureModal.__super__.constructor.apply(this, arguments);
    }

    ReconfigureModal.prototype.template = Handlebars.templates['reconfigure-modal'];

    ReconfigureModal.prototype["class"] = 'reconfigure-modal';

    ReconfigureModal.prototype.custom_events = $.extend(UIComponents.AbstractModal.events, {
      'keyup .replicas.inline-edit': 'change_replicas',
      'keyup .shards.inline-edit': 'change_shards',
      'click .expand-tree': 'expand_link'
    });

    ReconfigureModal.prototype.initialize = function(obj) {
      this.events = $.extend(this.events, this.custom_events);
      this.model.set('expanded', false);
      this.fetch_dryrun();
      this.listenTo(this.model, 'change:num_replicas_per_shard', this.fetch_dryrun);
      this.listenTo(this.model, 'change:num_shards', this.fetch_dryrun);
      this.listenTo(this.model, 'change:errors', this.change_errors);
      this.listenTo(this.model, 'change:server_error', this.get_errors);
      this.listenTo(this.model, 'change:expanded', this.expanded_changed);
      this.error_on_empty = false;
      return ReconfigureModal.__super__.initialize.call(this, obj);
    };

    ReconfigureModal.prototype.expand_link = function(event) {
      event.preventDefault();
      event.target.blur();
      return this.model.set({
        expanded: !this.model.get('expanded')
      });
    };

    ReconfigureModal.prototype.expanded_changed = function() {
      this.$('.reconfigure-diff').toggleClass('expanded');
      this.$('.expand-tree').toggleClass('expanded');
      $('.reconfigure-modal').toggleClass('expanded');
      return $('.expandbox').toggleClass('expanded');
    };

    ReconfigureModal.prototype.render = function() {
      ReconfigureModal.__super__.render.call(this, $.extend(this.model.toJSON(), {
        modal_title: "Sharding and replication for " + ("" + (this.model.get('db')) + "." + (this.model.get('name'))),
        btn_primary_text: 'Apply configuration'
      }));
      this.diff_view = new TableView.ReconfigureDiffView({
        model: this.model,
        el: this.$('.reconfigure-diff')[0]
      });
      return this;
    };

    ReconfigureModal.prototype.change_errors = function() {
      var error, errors, message, _i, _len, _results;
      errors = this.model.get('errors');
      this.$('.alert.error p.error').removeClass('shown');
      this.$('.alert.error .server-msg').html('');
      if (errors.length > 0) {
        this.$('.btn.btn-primary').prop({
          disabled: true
        });
        this.$('.alert.error').addClass('shown');
        _results = [];
        for (_i = 0, _len = errors.length; _i < _len; _i++) {
          error = errors[_i];
          message = this.$(".alert.error p.error." + error).addClass('shown');
          if (error === 'server-error') {
            _results.push(this.$('.alert.error .server-msg').append(Handlebars.Utils.escapeExpression(this.model.get('server_error'))));
          } else {
            _results.push(void 0);
          }
        }
        return _results;
      } else {
        this.error_on_empty = false;
        this.$('.btn.btn-primary').removeAttr('disabled');
        return this.$('.alert.error').removeClass('shown');
      }
    };

    ReconfigureModal.prototype.remove = function() {
      if (typeof diff_view !== "undefined" && diff_view !== null) {
        diff_view.remove();
      }
      return ReconfigureModal.__super__.remove.call(this);
    };

    ReconfigureModal.prototype.change_replicas = function() {
      var replicas;
      replicas = parseInt(this.$('.replicas.inline-edit').val());
      if (!isNaN(replicas) || this.error_on_empty) {
        return this.model.set({
          num_replicas_per_shard: replicas
        });
      }
    };

    ReconfigureModal.prototype.change_shards = function() {
      var shards;
      shards = parseInt(this.$('.shards.inline-edit').val());
      if (!isNaN(shards) || this.error_on_empty) {
        return this.model.set({
          num_shards: shards
        });
      }
    };

    ReconfigureModal.prototype.on_submit = function() {
      var new_or_unchanged, new_shards, query;
      this.error_on_empty = true;
      this.change_replicas();
      this.change_shards();
      if (this.get_errors()) {
        return;
      } else {
        ReconfigureModal.__super__.on_submit.apply(this, arguments);
      }
      new_or_unchanged = function(doc) {
        return doc('change').eq('added').or(doc('change').not());
      };
      new_shards = r.expr(this.model.get('shards')).filter(new_or_unchanged).map(function(shard) {
        return {
          primary_replica: shard('replicas').filter({
            role: 'primary'
          })(0)('id'),
          replicas: shard('replicas').filter(new_or_unchanged)('id')
        };
      });
      query = r.db(system_db).table('table_config', {
        identifierFormat: 'uuid'
      }).get(this.model.get('id')).update({
        shards: new_shards
      }, {
        returnChanges: true
      });
      return driver.run_once(query, (function(_this) {
        return function(error, result) {
          var parent;
          if (error != null) {
            return _this.model.set({
              server_error: error.msg
            });
          } else {
            _this.reset_buttons();
            _this.remove();
            parent = _this.model.get('parent');
            parent.model.set({
              num_replicas_per_shard: _this.model.get('num_replicas_per_shard'),
              num_shards: _this.model.get('num_shards')
            });
            parent.progress_bar.skip_to_processing();
            return _this.model.get('parent').fetch_progress();
          }
        };
      })(this));
    };

    ReconfigureModal.prototype.get_errors = function() {
      var errors, num_default_servers, num_replicas, num_servers, num_shards;
      errors = [];
      num_shards = this.model.get('num_shards');
      num_replicas = this.model.get('num_replicas_per_shard');
      num_servers = this.model.get('num_servers');
      num_default_servers = this.model.get('num_default_servers');
      if (num_shards === 0) {
        errors.push('zero-shards');
      } else if (isNaN(num_shards)) {
        errors.push('no-shards');
      } else if (num_shards > num_default_servers) {
        if (num_shards > num_servers) {
          errors.push('too-many-shards');
        } else {
          errors.push('not-enough-defaults-shard');
        }
      }
      if (num_replicas === 0) {
        errors.push('zero-replicas');
      } else if (isNaN(num_replicas)) {
        errors.push('no-replicas');
      } else if (num_replicas > num_default_servers) {
        if (num_replicas > num_servers) {
          errors.push('too-many-replicas');
        } else {
          errors.push('not-enough-defaults-replica');
        }
      }
      if (this.model.get('server_error') != null) {
        errors.push('server-error');
      }
      this.model.set({
        errors: errors
      });
      if (errors.length > 0) {
        this.model.set({
          shards: []
        });
      }
      return errors.length > 0;
    };

    ReconfigureModal.prototype.fetch_dryrun = function() {
      var id, query;
      if (this.get_errors()) {
        return;
      }
      id = function(x) {
        return x;
      };
      query = r.db(this.model.get('db')).table(this.model.get('name')).reconfigure({
        shards: this.model.get('num_shards'),
        replicas: this.model.get('num_replicas_per_shard'),
        dryRun: true
      })('config_changes')(0)["do"](function(diff) {
        return r.object(r.args(diff.keys().map(function(key) {
          return [key, diff(key)('shards')];
        }).concatMap(id)));
      })["do"](function(doc) {
        return doc.merge({
          lookups: r.object(r.args(doc('new_val')('replicas').concatMap(id).setUnion(doc('old_val')('replicas').concatMap(id)).concatMap(function(server) {
            return [
              server, r.db(system_db).table('server_status').filter({
                name: server
              })(0)('id')
            ];
          })))
        });
      });
      return driver.run_once(query, (function(_this) {
        return function(error, result) {
          if (error != null) {
            return _this.model.set({
              server_error: error.msg,
              shards: []
            });
          } else {
            _this.model.set({
              server_error: null
            });
            return _this.model.set({
              shards: _this.fixup_shards(result.old_val, result.new_val, result.lookups)
            });
          }
        };
      })(this));
    };

    ReconfigureModal.prototype.shard_sort = function(a, b) {
      if (a.role === 'primary') {
        return -1;
      } else if ('primary' === b.role || 'primary' === b.old_role) {
        return +1;
      } else if (a.role === b.role) {
        if (a.name === b.name) {
          return 0;
        } else if (a.name < b.name) {
          return -1;
        } else {
          return +1;
        }
      }
    };

    ReconfigureModal.prototype.role = function(isPrimary) {
      if (isPrimary) {
        return 'primary';
      } else {
        return 'secondary';
      }
    };

    ReconfigureModal.prototype.fixup_shards = function(old_shards, new_shards, lookups) {
      var i, new_role, new_shard, old_role, old_shard, replica, replica_deleted, shard_deleted, shard_diff, shard_diffs, _i, _j, _k, _l, _len, _len1, _len2, _len3, _len4, _m, _ref, _ref1, _ref2, _ref3;
      shard_diffs = [];
      for (i = _i = 0, _len = old_shards.length; _i < _len; i = ++_i) {
        old_shard = old_shards[i];
        if (i >= new_shards.length) {
          new_shard = {
            primary_replica: null,
            replicas: []
          };
          shard_deleted = true;
        } else {
          new_shard = new_shards[i];
          shard_deleted = false;
        }
        shard_diff = {
          replicas: [],
          change: shard_deleted ? 'deleted' : null
        };
        _ref = old_shard.replicas;
        for (_j = 0, _len1 = _ref.length; _j < _len1; _j++) {
          replica = _ref[_j];
          replica_deleted = __indexOf.call(new_shard.replicas, replica) < 0;
          if (replica_deleted) {
            new_role = null;
          } else {
            new_role = this.role(replica === new_shard.primary_replica);
          }
          old_role = this.role(replica === old_shard.primary_replica);
          shard_diff.replicas.push({
            name: replica,
            id: lookups[replica],
            change: replica_deleted ? 'deleted' : null,
            role: new_role,
            old_role: old_role,
            role_change: !replica_deleted && new_role !== old_role
          });
        }
        _ref1 = new_shard.replicas;
        for (_k = 0, _len2 = _ref1.length; _k < _len2; _k++) {
          replica = _ref1[_k];
          if (__indexOf.call(old_shard.replicas, replica) >= 0) {
            continue;
          }
          shard_diff.replicas.push({
            name: replica,
            id: lookups[replica],
            change: 'added',
            role: this.role(replica === new_shard.primary_replica),
            old_role: null,
            role_change: false
          });
        }
        shard_diff.replicas.sort(this.shard_sort);
        shard_diffs.push(shard_diff);
      }
      _ref2 = new_shards.slice(old_shards.length);
      for (_l = 0, _len3 = _ref2.length; _l < _len3; _l++) {
        new_shard = _ref2[_l];
        shard_diff = {
          replicas: [],
          change: 'added'
        };
        _ref3 = new_shard.replicas;
        for (_m = 0, _len4 = _ref3.length; _m < _len4; _m++) {
          replica = _ref3[_m];
          shard_diff.replicas.push({
            name: replica,
            id: lookups[replica],
            change: 'added',
            role: this.role(replica === new_shard.primary_replica),
            old_role: null,
            role_change: false
          });
        }
        shard_diff.replicas.sort(this.shard_sort);
        shard_diffs.push(shard_diff);
      }
      return shard_diffs;
    };

    return ReconfigureModal;

  })(UIComponents.AbstractModal);
});
var Dashboard, Database, Databases, Distribution, Index, Indexes, Issue, Issues, Log, Logs, Reconfigure, Responsibilities, Responsibility, Server, Servers, Shard, ShardAssignment, ShardAssignments, Stats, Table, Tables,
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; },
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };

Servers = (function(_super) {
  __extends(Servers, _super);

  function Servers() {
    return Servers.__super__.constructor.apply(this, arguments);
  }

  Servers.prototype.model = Server;

  Servers.prototype.name = 'Servers';

  return Servers;

})(Backbone.Collection);

Server = (function(_super) {
  __extends(Server, _super);

  function Server() {
    return Server.__super__.constructor.apply(this, arguments);
  }

  return Server;

})(Backbone.Model);

Tables = (function(_super) {
  __extends(Tables, _super);

  function Tables() {
    return Tables.__super__.constructor.apply(this, arguments);
  }

  Tables.prototype.model = Table;

  Tables.prototype.name = 'Tables';

  Tables.prototype.comparator = 'name';

  return Tables;

})(Backbone.Collection);

Table = (function(_super) {
  __extends(Table, _super);

  function Table() {
    return Table.__super__.constructor.apply(this, arguments);
  }

  return Table;

})(Backbone.Model);

Reconfigure = (function(_super) {
  __extends(Reconfigure, _super);

  function Reconfigure() {
    return Reconfigure.__super__.constructor.apply(this, arguments);
  }

  return Reconfigure;

})(Backbone.Model);

Databases = (function(_super) {
  __extends(Databases, _super);

  function Databases() {
    return Databases.__super__.constructor.apply(this, arguments);
  }

  Databases.prototype.model = Database;

  Databases.prototype.name = 'Databases';

  Databases.prototype.comparator = 'name';

  return Databases;

})(Backbone.Collection);

Database = (function(_super) {
  __extends(Database, _super);

  function Database() {
    return Database.__super__.constructor.apply(this, arguments);
  }

  return Database;

})(Backbone.Model);

Indexes = (function(_super) {
  __extends(Indexes, _super);

  function Indexes() {
    return Indexes.__super__.constructor.apply(this, arguments);
  }

  Indexes.prototype.model = Index;

  Indexes.prototype.name = 'Indexes';

  Indexes.prototype.comparator = 'index';

  return Indexes;

})(Backbone.Collection);

Index = (function(_super) {
  __extends(Index, _super);

  function Index() {
    return Index.__super__.constructor.apply(this, arguments);
  }

  return Index;

})(Backbone.Model);

Distribution = (function(_super) {
  __extends(Distribution, _super);

  function Distribution() {
    return Distribution.__super__.constructor.apply(this, arguments);
  }

  Distribution.prototype.model = Shard;

  Distribution.prototype.name = 'Shards';

  return Distribution;

})(Backbone.Collection);

Shard = (function(_super) {
  __extends(Shard, _super);

  function Shard() {
    return Shard.__super__.constructor.apply(this, arguments);
  }

  return Shard;

})(Backbone.Model);

ShardAssignments = (function(_super) {
  __extends(ShardAssignments, _super);

  function ShardAssignments() {
    return ShardAssignments.__super__.constructor.apply(this, arguments);
  }

  ShardAssignments.prototype.model = ShardAssignment;

  ShardAssignments.prototype.name = 'ShardAssignment';

  ShardAssignments.prototype.comparator = function(a, b) {
    if (a.get('shard_id') < b.get('shard_id')) {
      return -1;
    } else if (a.get('shard_id') > b.get('shard_id')) {
      return 1;
    } else {
      if (a.get('start_shard') === true) {
        return -1;
      } else if (b.get('start_shard') === true) {
        return 1;
      } else if (a.get('end_shard') === true) {
        return 1;
      } else if (b.get('end_shard') === true) {
        return -1;
      } else if (a.get('primary') === true && b.get('replica') === true) {
        return -1;
      } else if (a.get('replica') === true && b.get('primary') === true) {
        return 1;
      } else if (a.get('replica') === true && b.get('replica') === true) {
        if (a.get('replica_position') < b.get('replica_position')) {
          return -1;
        } else if (a.get('replica_position') > b.get('replica_position')) {
          return 1;
        }
        return 0;
      }
    }
  };

  return ShardAssignments;

})(Backbone.Collection);

ShardAssignment = (function(_super) {
  __extends(ShardAssignment, _super);

  function ShardAssignment() {
    return ShardAssignment.__super__.constructor.apply(this, arguments);
  }

  return ShardAssignment;

})(Backbone.Model);

Responsibilities = (function(_super) {
  __extends(Responsibilities, _super);

  function Responsibilities() {
    return Responsibilities.__super__.constructor.apply(this, arguments);
  }

  Responsibilities.prototype.model = Responsibility;

  Responsibilities.prototype.name = 'Responsibility';

  Responsibilities.prototype.comparator = function(a, b) {
    if (a.get('db') < b.get('db')) {
      return -1;
    } else if (a.get('db') > b.get('db')) {
      return 1;
    } else {
      if (a.get('table') < b.get('table')) {
        return -1;
      } else if (a.get('table') > b.get('table')) {
        return 1;
      } else {
        if (a.get('is_table') === true) {
          return -1;
        } else if (b.get('is_table') === true) {
          return 1;
        } else if (a.get('index') < b.get('index')) {
          return -1;
        } else if (a.get('index') > b.get('index')) {
          return 1;
        } else {
          return 0;
        }
      }
    }
  };

  return Responsibilities;

})(Backbone.Collection);

Responsibility = (function(_super) {
  __extends(Responsibility, _super);

  function Responsibility() {
    return Responsibility.__super__.constructor.apply(this, arguments);
  }

  return Responsibility;

})(Backbone.Model);

Dashboard = (function(_super) {
  __extends(Dashboard, _super);

  function Dashboard() {
    return Dashboard.__super__.constructor.apply(this, arguments);
  }

  return Dashboard;

})(Backbone.Model);

Issue = (function(_super) {
  __extends(Issue, _super);

  function Issue() {
    return Issue.__super__.constructor.apply(this, arguments);
  }

  return Issue;

})(Backbone.Model);

Issues = (function(_super) {
  __extends(Issues, _super);

  function Issues() {
    return Issues.__super__.constructor.apply(this, arguments);
  }

  Issues.prototype.model = Issue;

  Issues.prototype.name = 'Issues';

  return Issues;

})(Backbone.Collection);

Logs = (function(_super) {
  __extends(Logs, _super);

  function Logs() {
    return Logs.__super__.constructor.apply(this, arguments);
  }

  Logs.prototype.model = Log;

  Logs.prototype.name = 'Logs';

  return Logs;

})(Backbone.Collection);

Log = (function(_super) {
  __extends(Log, _super);

  function Log() {
    return Log.__super__.constructor.apply(this, arguments);
  }

  return Log;

})(Backbone.Model);

Stats = (function(_super) {
  __extends(Stats, _super);

  function Stats() {
    this.get_stats = __bind(this.get_stats, this);
    this.on_result = __bind(this.on_result, this);
    this.initialize = __bind(this.initialize, this);
    return Stats.__super__.constructor.apply(this, arguments);
  }

  Stats.prototype.initialize = function(query) {
    return this.set({
      keys_read: 0,
      keys_set: 0
    });
  };

  Stats.prototype.on_result = function(err, result) {
    if (err != null) {
      return console.log(err);
    } else {
      return this.set(result);
    }
  };

  Stats.prototype.get_stats = function() {
    return this.toJSON();
  };

  return Stats;

})(Backbone.Model);

module('DataUtils', function() {
  this.stripslashes = function(str) {
    str = str.replace(/\\'/g, '\'');
    str = str.replace(/\\"/g, '"');
    str = str.replace(/\\0/g, "\x00");
    str = str.replace(/\\\\/g, '\\');
    return str;
  };
  this.is_integer = function(data) {
    return data.search(/^\d+$/) !== -1;
  };
  return this.deep_copy = function(data) {
    var key, result, value, _i, _len;
    if (typeof data === 'boolean' || typeof data === 'number' || typeof data === 'string' || typeof data === 'number' || data === null || data === void 0) {
      return data;
    } else if (typeof data === 'object' && Object.prototype.toString.call(data) === '[object Array]') {
      result = [];
      for (_i = 0, _len = data.length; _i < _len; _i++) {
        value = data[_i];
        result.push(this.deep_copy(value));
      }
      return result;
    } else if (typeof data === 'object') {
      result = {};
      for (key in data) {
        value = data[key];
        result[key] = this.deep_copy(value);
      }
      return result;
    }
  };
});
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

module('TopBar', function() {
  return this.NavBarView = (function(_super) {
    __extends(NavBarView, _super);

    function NavBarView() {
      this.update_cog_icon = __bind(this.update_cog_icon, this);
      this.set_active_tab = __bind(this.set_active_tab, this);
      this.render = __bind(this.render, this);
      this.init_typeahead = __bind(this.init_typeahead, this);
      this.initialize = __bind(this.initialize, this);
      return NavBarView.__super__.constructor.apply(this, arguments);
    }

    NavBarView.prototype.id = 'navbar';

    NavBarView.prototype.className = 'container';

    NavBarView.prototype.template = Handlebars.templates['navbar_view-template'];

    NavBarView.prototype.events = {
      'click .options_link': 'update_cog_icon'
    };

    NavBarView.prototype.initialize = function(data) {
      this.databases = data.databases;
      this.tables = data.tables;
      this.servers = data.servers;
      this.container = data.container;
      return this.options_state = 'hidden';
    };

    NavBarView.prototype.init_typeahead = function() {
      return this.$('input.search-box').typeahead({
        source: (function(_this) {
          return function(typeahead, query) {
            var databases, servers, tables;
            servers = _.map(_this.servers.models, function(server) {
              return {
                id: server.get('id'),
                name: server.get('name') + ' (server)',
                type: 'servers'
              };
            });
            tables = _.map(_this.tables.models, function(table) {
              return {
                id: table.get('id'),
                name: table.get('name') + ' (table)',
                type: 'tables'
              };
            });
            databases = _.map(_this.databases.models, function(database) {
              return {
                id: database.get('id'),
                name: database.get('name') + ' (database)',
                type: 'databases'
              };
            });
            return servers.concat(tables).concat(databases);
          };
        })(this),
        property: 'name',
        onselect: function(obj) {
          return window.app.navigate('#' + obj.type + '/' + obj.id, {
            trigger: true
          });
        }
      });
    };

    NavBarView.prototype.render = function(route) {
      this.$el.html(this.template());
      this.set_active_tab(route);
      return this;
    };

    NavBarView.prototype.set_active_tab = function(route) {
      if (route != null) {
        switch (route) {
          case 'dashboard':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-dashboard').addClass('active');
          case 'index_tables':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-tables').addClass('active');
          case 'table':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-tables').addClass('active');
          case 'database':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-tables').addClass('active');
          case 'index_servers':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-servers').addClass('active');
          case 'server':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-servers').addClass('active');
          case 'datacenter':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-servers').addClass('active');
          case 'dataexplorer':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-dataexplorer').addClass('active');
          case 'logs':
            this.$('ul.nav-left li').removeClass('active');
            return this.$('li#nav-logs').addClass('active');
        }
      }
    };

    NavBarView.prototype.update_cog_icon = function(event) {
      this.$('.cog_icon').toggleClass('active');
      return this.container.toggle_options(event);
    };

    return NavBarView;

  })(Backbone.View);
});
var BackboneCluster,
  __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __hasProp = {}.hasOwnProperty,
  __extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

BackboneCluster = (function(_super) {
  __extends(BackboneCluster, _super);

  function BackboneCluster() {
    this.update_active_tab = __bind(this.update_active_tab, this);
    return BackboneCluster.__super__.constructor.apply(this, arguments);
  }

  BackboneCluster.prototype.routes = {
    '': 'dashboard',
    'tables': 'index_tables',
    'tables/': 'index_tables',
    'tables/:id': 'table',
    'tables/:id/': 'table',
    'servers': 'index_servers',
    'servers/': 'index_servers',
    'servers/:id': 'server',
    'servers/:id/': 'server',
    'dashboard': 'dashboard',
    'dashboard/': 'dashboard',
    'logs': 'logs',
    'logs/': 'logs',
    'dataexplorer': 'dataexplorer',
    'dataexplorer/': 'dataexplorer'
  };

  BackboneCluster.prototype.initialize = function(data) {
    BackboneCluster.__super__.initialize.apply(this, arguments);
    this.navbar = data.navbar;
    window.app = this;
    this.container = $('#cluster');
    this.current_view = new Backbone.View;
    return this.bind('route', this.update_active_tab);
  };

  BackboneCluster.prototype.update_active_tab = function(route) {
    return this.navbar.set_active_tab(route);
  };

  BackboneCluster.prototype.index_tables = function() {
    this.current_view.remove();
    this.current_view = new TablesView.DatabasesContainer;
    return this.container.html(this.current_view.render().$el);
  };

  BackboneCluster.prototype.index_servers = function(data) {
    this.current_view.remove();
    this.current_view = new ServersView.ServersContainer;
    return this.container.html(this.current_view.render().$el);
  };

  BackboneCluster.prototype.dashboard = function() {
    this.current_view.remove();
    this.current_view = new DashboardView.DashboardContainer;
    return this.container.html(this.current_view.render().$el);
  };

  BackboneCluster.prototype.logs = function() {
    this.current_view.remove();
    this.current_view = new LogView.LogsContainer;
    return this.container.html(this.current_view.render().$el);
  };

  BackboneCluster.prototype.dataexplorer = function() {
    this.current_view.remove();
    this.current_view = new DataExplorerView.Container({
      state: DataExplorerView.state
    });
    this.container.html(this.current_view.render().$el);
    this.current_view.init_after_dom_rendered();
    return this.current_view.results_view_wrapper.set_scrollbar();
  };

  BackboneCluster.prototype.table = function(id) {
    this.current_view.remove();
    this.current_view = new TableView.TableContainer(id);
    return this.container.html(this.current_view.render().$el);
  };

  BackboneCluster.prototype.server = function(id) {
    this.current_view.remove();
    this.current_view = new ServerView.ServerContainer(id);
    return this.container.html(this.current_view.render().$el);
  };

  return BackboneCluster;

})(Backbone.Router);
var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; },
  __slice = [].slice;

window.system_db = 'rethinkdb';

$(function() {
  window.r = require('rethinkdb');
  window.driver = new Driver;
  window.view_data_backup = {};
  window.main_view = new MainView.MainContainer();
  $('body').html(main_view.render().$el);
  main_view.start_router();
  Backbone.sync = function(method, model, success, error) {
    return 0;
  };
  return DataExplorerView.Container.prototype.set_docs(reql_docs);
});

this.Driver = (function() {
  function Driver(args) {
    this.stop_timer = __bind(this.stop_timer, this);
    this.run = __bind(this.run, this);
    this.run_once = __bind(this.run_once, this);
    this.hack_driver = __bind(this.hack_driver, this);
    var port;
    if (window.location.port === '') {
      if (window.location.protocol === 'https:') {
        port = 443;
      } else {
        port = 80;
      }
    } else {
      port = parseInt(window.location.port);
    }
    this.server = {
      host: window.location.hostname,
      port: port,
      protocol: window.location.protocol === 'https:' ? 'https' : 'http',
      pathname: window.location.pathname
    };
    this.hack_driver();
    this.state = 'ok';
    this.timers = {};
    this.index = 0;
  }

  Driver.prototype.hack_driver = function() {
    var TermBase, that;
    TermBase = r.expr(1).constructor.__super__.constructor.__super__;
    if (TermBase.private_run == null) {
      that = this;
      TermBase.private_run = TermBase.run;
      return TermBase.run = function() {
        throw new Error("You should remove .run() from your queries when using the Data Explorer.\nThe query will be built and sent by the Data Explorer itself.");
      };
    }
  };

  Driver.prototype.connect = function(callback) {
    return r.connect(this.server, callback);
  };

  Driver.prototype.close = function(conn) {
    return conn.close({
      noreplyWait: false
    });
  };

  Driver.prototype.run_once = function(query, callback) {
    return this.connect((function(_this) {
      return function(error, connection) {
        if (error != null) {
          if (window.is_disconnected != null) {
            if (_this.state === 'ok') {
              window.is_disconnected.display_fail();
            }
          } else {
            window.is_disconnected = new IsDisconnected;
          }
          return _this.state = 'fail';
        } else {
          if (_this.state === 'fail') {
            return window.location.reload(true);
          } else {
            _this.state = 'ok';
            return query.private_run(connection, function(err, result) {
              if (typeof (result != null ? result.toArray : void 0) === 'function') {
                return result.toArray(function(err, result) {
                  return callback(err, result);
                });
              } else {
                return callback(err, result);
              }
            });
          }
        }
      };
    })(this));
  };

  Driver.prototype.run = function(query, delay, callback, index) {
    if (index == null) {
      this.index++;
    }
    index = this.index;
    this.timers[index] = {};
    ((function(_this) {
      return function(index) {
        return _this.connect(function(error, connection) {
          var fn;
          if (error != null) {
            if (window.is_disconnected != null) {
              if (_this.state === 'ok') {
                window.is_disconnected.display_fail();
              }
            } else {
              window.is_disconnected = new IsDisconnected;
            }
            return _this.state = 'fail';
          } else {
            if (_this.state === 'fail') {
              return window.location.reload(true);
            } else {
              _this.state = 'ok';
              if (_this.timers[index] != null) {
                _this.timers[index].connection = connection;
                return (fn = function() {
                  var err;
                  try {
                    return query.private_run(connection, function(err, result) {
                      if (typeof (result != null ? result.toArray : void 0) === 'function') {
                        return result.toArray(function(err, result) {
                          if ((err != null ? err.msg : void 0) === "This HTTP connection is not open." || (err != null ? err.message : void 0) === "This HTTP connection is not open") {
                            console.log("Connection lost. Retrying.");
                            return _this.run(query, delay, callback, index);
                          }
                          callback(err, result);
                          if (_this.timers[index] != null) {
                            return _this.timers[index].timeout = setTimeout(fn, delay);
                          }
                        });
                      } else {
                        if ((err != null ? err.msg : void 0) === "This HTTP connection is not open." || (err != null ? err.message : void 0) === "This HTTP connection is not open") {
                          console.log("Connection lost. Retrying.");
                          return _this.run(query, delay, callback, index);
                        }
                        callback(err, result);
                        if (_this.timers[index] != null) {
                          return _this.timers[index].timeout = setTimeout(fn, delay);
                        }
                      }
                    });
                  } catch (_error) {
                    err = _error;
                    console.log(err);
                    return _this.run(query, delay, callback, index);
                  }
                })();
              }
            }
          }
        });
      };
    })(this))(index);
    return index;
  };

  Driver.prototype.stop_timer = function(timer) {
    var _ref, _ref1, _ref2;
    clearTimeout((_ref = this.timers[timer]) != null ? _ref.timeout : void 0);
    if ((_ref1 = this.timers[timer]) != null) {
      if ((_ref2 = _ref1.connection) != null) {
        _ref2.close({
          noreplyWait: false
        });
      }
    }
    return delete this.timers[timer];
  };

  Driver.prototype.admin = function() {
    return {
      cluster_config: r.db(system_db).table('cluster_config'),
      cluster_config_id: r.db(system_db).table('cluster_config', {
        identifierFormat: 'uuid'
      }),
      current_issues: r.db(system_db).table('current_issues'),
      current_issues_id: r.db(system_db).table('current_issues', {
        identifierFormat: 'uuid'
      }),
      db_config: r.db(system_db).table('db_config'),
      db_config_id: r.db(system_db).table('db_config', {
        identifierFormat: 'uuid'
      }),
      jobs: r.db(system_db).table('jobs'),
      jobs_id: r.db(system_db).table('jobs', {
        identifierFormat: 'uuid'
      }),
      logs: r.db(system_db).table('logs'),
      logs_id: r.db(system_db).table('logs', {
        identifierFormat: 'uuid'
      }),
      server_config: r.db(system_db).table('server_config'),
      server_config_id: r.db(system_db).table('server_config', {
        identifierFormat: 'uuid'
      }),
      server_status: r.db(system_db).table('server_status'),
      server_status_id: r.db(system_db).table('server_status', {
        identifierFormat: 'uuid'
      }),
      stats: r.db(system_db).table('stats'),
      stats_id: r.db(system_db).table('stats', {
        identifierFormat: 'uuid'
      }),
      table_config: r.db(system_db).table('table_config'),
      table_config_id: r.db(system_db).table('table_config', {
        identifierFormat: 'uuid'
      }),
      table_status: r.db(system_db).table('table_status'),
      table_status_id: r.db(system_db).table('table_status', {
        identifierFormat: 'uuid'
      })
    };
  };

  Driver.prototype.helpers = {
    match: function() {
      var action, previous, specs, val, variable, _i, _len, _ref, _ref1;
      variable = arguments[0], specs = 2 <= arguments.length ? __slice.call(arguments, 1) : [];
      previous = r.error("nothing matched " + variable);
      _ref = specs.reverse();
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        _ref1 = _ref[_i], val = _ref1[0], action = _ref1[1];
        previous = r.branch(r.expr(variable).eq(val), action, previous);
      }
      return previous;
    },
    identity: function(x) {
      return x;
    }
  };

  Driver.prototype.queries = {
    all_logs: function(limit) {
      var server_conf;
      server_conf = driver.admin().server_config;
      return r.db(system_db).table('logs', {
        identifierFormat: 'uuid'
      }).orderBy({
        index: r.desc('id')
      }).limit(limit).map(function(log) {
        return log.merge({
          server: server_conf.get(log('server'))('name'),
          server_id: log('server')
        });
      });
    },
    server_logs: function(limit, server_id) {
      var server_conf;
      server_conf = driver.admin().server_config;
      return r.db(system_db).table('logs', {
        identifierFormat: 'uuid'
      }).orderBy({
        index: r.desc('id')
      }).filter({
        server: server_id
      }).limit(limit).map(function(log) {
        return log.merge({
          server: server_conf.get(log('server'))('name'),
          server_id: log('server')
        });
      });
    },
    issues_with_ids: function(current_issues) {
      var current_issues_id;
      if (current_issues == null) {
        current_issues = driver.admin().current_issues;
      }
      current_issues_id = driver.admin().current_issues_id;
      return current_issues.merge(function(issue) {
        var invalid_config, issue_id, log_write_error, outdated_index, server_disconnected;
        issue_id = current_issues_id.get(issue('id'));
        server_disconnected = {
          disconnected_server_id: issue_id('info')('disconnected_server'),
          reporting_servers: issue('info')('reporting_servers').map(issue_id('info')('reporting_servers'), function(server, server_id) {
            return {
              server: server,
              server_id: server_id
            };
          })
        };
        log_write_error = {
          servers: issue('info')('servers').map(issue_id('info')('servers'), function(server, server_id) {
            return {
              server: server,
              server_id: server_id
            };
          })
        };
        outdated_index = {
          tables: issue('info')('tables').map(issue_id('info')('tables'), function(table, table_id) {
            return {
              db: table('db'),
              db_id: table_id('db'),
              table_id: table_id('table'),
              table: table('table'),
              indexes: table('indexes')
            };
          })
        };
        invalid_config = {
          table_id: issue_id('info')('table'),
          db_id: issue_id('info')('db')
        };
        return {
          info: driver.helpers.match(issue('type'), ['server_disconnected', server_disconnected], ['log_write_error', log_write_error], ['outdated_index', outdated_index], ['table_needs_primary', invalid_config], ['data_lost', invalid_config], ['write_acks', invalid_config], [issue('type'), issue('info')])
        };
      }).coerceTo('array');
    },
    tables_with_primaries_not_ready: function(table_config_id, table_status) {
      if (table_config_id == null) {
        table_config_id = driver.admin().table_config_id;
      }
      if (table_status == null) {
        table_status = driver.admin().table_status;
      }
      return r["do"](driver.admin().server_config.map(function(x) {
        return [x('id'), x('name')];
      }).coerceTo('ARRAY').coerceTo('OBJECT'), function(server_names) {
        return table_status.map(table_config_id, function(status, config) {
          return {
            id: status('id'),
            name: status('name'),
            db: status('db'),
            shards: status('shards').map(r.range(), config('shards'), function(shard, pos, conf_shard) {
              var primary_id, primary_name;
              primary_id = conf_shard('primary_replica');
              primary_name = server_names(primary_id);
              return {
                position: pos.add(1),
                num_shards: status('shards').count(),
                primary_id: primary_id,
                primary_name: primary_name["default"]('-'),
                primary_state: shard('replicas').filter({
                  server: primary_name
                }, {
                  "default": r.error()
                })('state')(0)["default"]('missing')
              };
            }).filter(function(shard) {
              return r.expr(['ready', 'looking_for_primary_replica']).contains(shard('primary_state')).not();
            }).coerceTo('array')
          };
        }).filter(function(table) {
          return table('shards').isEmpty().not();
        }).coerceTo('array');
      });
    },
    tables_with_replicas_not_ready: function(table_config_id, table_status) {
      if (table_config_id == null) {
        table_config_id = driver.admin().table_config_id;
      }
      if (table_status == null) {
        table_status = driver.admin().table_status;
      }
      return table_status.map(table_config_id, function(status, config) {
        return {
          id: status('id'),
          name: status('name'),
          db: status('db'),
          shards: status('shards').map(r.range(), config('shards'), function(shard, pos, conf_shard) {
            return {
              position: pos.add(1),
              num_shards: status('shards').count(),
              replicas: shard('replicas').filter(function(replica) {
                return r.expr(['ready', 'looking_for_primary_replica', 'offloading_data']).contains(replica('state')).not();
              }).map(conf_shard('replicas'), function(replica, replica_id) {
                return {
                  replica_id: replica_id,
                  replica_name: replica('server')
                };
              }).coerceTo('array')
            };
          }).coerceTo('array')
        };
      }).filter(function(table) {
        return table('shards')(0)('replicas').isEmpty().not();
      }).coerceTo('array');
    },
    num_primaries: function(table_config_id) {
      if (table_config_id == null) {
        table_config_id = driver.admin().table_config_id;
      }
      return table_config_id('shards').map(function(x) {
        return x.count();
      }).sum();
    },
    num_connected_primaries: function(table_status) {
      if (table_status == null) {
        table_status = driver.admin().table_status;
      }
      return table_status.map(function(table) {
        return table('shards')('primary_replica').count(function(primary) {
          return primary.ne(null);
        });
      }).sum();
    },
    num_replicas: function(table_config_id) {
      if (table_config_id == null) {
        table_config_id = driver.admin().table_config_id;
      }
      return table_config_id('shards').map(function(shards) {
        return shards.map(function(shard) {
          return shard("replicas").count();
        }).sum();
      }).sum();
    },
    num_connected_replicas: function(table_status) {
      if (table_status == null) {
        table_status = driver.admin().table_status;
      }
      return table_status('shards').map(function(shards) {
        return shards('replicas').map(function(replica) {
          return replica('state').count(function(replica) {
            return r.expr(['ready', 'looking_for_primary_replica']).contains(replica);
          });
        }).sum();
      }).sum();
    },
    disconnected_servers: function(server_status) {
      if (server_status == null) {
        server_status = driver.admin().server_status;
      }
      return server_status.filter(function(server) {
        return server("status").ne("connected");
      }).map(function(server) {
        return {
          time_disconnected: server('connection')('time_disconnected'),
          name: server('name')
        };
      }).coerceTo('array');
    },
    num_disconnected_tables: function(table_status) {
      if (table_status == null) {
        table_status = driver.admin().table_status;
      }
      return table_status.count(function(table) {
        var shard_is_down;
        shard_is_down = function(shard) {
          return shard('primary_replica').eq(null);
        };
        return table('shards').map(shard_is_down).contains(true);
      });
    },
    num_tables_w_missing_replicas: function(table_status) {
      if (table_status == null) {
        table_status = driver.admin().table_status;
      }
      return table_status.count(function(table) {
        return table('status')('all_replicas_ready').not();
      });
    },
    num_connected_servers: function(server_status) {
      if (server_status == null) {
        server_status = driver.admin().server_status;
      }
      return server_status.count(function(server) {
        return server('status').eq("connected");
      });
    },
    num_sindex_issues: function(current_issues) {
      if (current_issues == null) {
        current_issues = driver.admin().current_issues;
      }
      return current_issues.count(function(issue) {
        return issue('type').eq('outdated_index');
      });
    },
    num_sindexes_constructing: function(jobs) {
      if (jobs == null) {
        jobs = driver.admin().jobs;
      }
      return jobs.count(function(job) {
        return job('type').eq('index_construction');
      });
    }
  };

  return Driver;

})();

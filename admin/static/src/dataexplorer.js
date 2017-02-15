// Copyright 2010-2015 RethinkDB, all rights reserved.

const app = require('./app');
const { system_db } = app;
const { driver } = app;
const util = require('./util');

const r = require('rethinkdb');

const DEFAULTS = {
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

Object.freeze(DEFAULTS);
Object.freeze(DEFAULTS.options);
// state is initially a mutable version of defaults. It's modified later
const dataexplorer_state = _.extend({}, DEFAULTS);

// This class represents the results of a query.
//
// If there is a profile, `profile` is set. After a 'ready' event,
// one of `error`, `value` or `cursor` is always set, `type`
// indicates which. `ended` indicates whether there are any more
// results to read.
//
// It triggers the following events:
//  * ready: The first response has been received
//  * add: Another row has been received from a cursor
//  * error: An error has occurred
//  * end: There are no more documents to fetch
//  * discard: The results have been discarded
class QueryResult {
    static initClass() {
        _.extend(this.prototype, Backbone.Events);
    }

    constructor(options) {
        this.set = this.set.bind(this);
        this.discard = this.discard.bind(this);
        this.fetch_next = this.fetch_next.bind(this);
        this.set_error = this.set_error.bind(this);
        this.size = this.size.bind(this);
        this.force_end_gracefully = this.force_end_gracefully.bind(this);
        this.drop_before = this.drop_before.bind(this);
        this.slice = this.slice.bind(this);
        this.at_beginning = this.at_beginning.bind(this);
        this.has_profile = options.has_profile;
        this.current_query = options.current_query;
        this.raw_query = options.raw_query;
        this.driver_handler = options.driver_handler;
        this.ready = false;
        this.position = 0;
        this.server_duration = null;
        if (options.events != null) {
            for (let event of Object.keys(options.events || {})) {
                const handler = options.events[event];
                this.on(event, handler);
            }
        }
    }

    // Can be used as a callback to run
    set(error, result) {
        if (error != null) {
            return this.set_error(error);
        } else if (!this.discard_results) {
            let value;
            if (this.has_profile) {
                this.profile = result.profile;
                this.server_duration = result.profile.reduce(((total, prof) => total + (prof['duration(ms)'] || prof['mean_duration(ms)'])), 0);
                ({ value } = result);
            } else {
                this.profile = null;
                this.server_duration = __guard__(result, x => x.profile[0]['duration(ms)']);
                ({ value } = result);
            }
            if ((value != null) && (typeof value._next === 'function') && !(value instanceof Array)) { // if it's a cursor
                this.type = 'cursor';
                this.results = [];
                this.results_offset = 0;
                this.cursor = value;
                this.is_feed = this.cursor.toString().match(/\[object .*Feed\]/);
                this.missing = 0;
                this.ended = false;
                this.server_duration = null;  // ignore server time if batched response
            } else {
                this.type = 'value';
                this.value = value;
                this.ended = true;
            }
            this.ready = true;
            return this.trigger('ready', this);
        }
    }

    // Discard the results
    discard() {
        this.trigger('discard', this);
        this.off();
        this.type = 'discarded';
        this.discard_results = true;
        delete this.profile;
        delete this.value;
        delete this.results;
        delete this.results_offset;
        __guardMethod__(__guard__(this.cursor, x => x.close()), 'catch', o => o.catch(() => null));
        return delete this.cursor;
    }

    // Gets the next result from the cursor
    fetch_next() {
        if (!this.ended) {
            try {
                return this.driver_handler.cursor_next(this.cursor, {
                    end: () => {
                        this.ended = true;
                        return this.trigger('end', this);
                    },
                    error: error => {
                        if (!this.ended) {
                            return this.set_error(error);
                        }
                    },
                    row: row => {
                        if (this.discard_results) {
                            return;
                        }
                        this.results.push(row);
                        return this.trigger('add', this, row);
                    }
                }
                );
            } catch (error) {
                return this.set_error(error);
            }
        }
    }

    set_error(error) {
        this.type = 'error';
        this.error = error;
        this.trigger('error', this, error);
        this.discard_results = true;
        return this.ended = true;
    }

    size() {
        switch (this.type) {
            case 'value':
                if (this.value instanceof Array) {
                    return this.value.length;
                } else {
                    return 1;
                }
            case 'cursor':
                return this.results.length + this.results_offset;
        }
    }

    force_end_gracefully() {
        if (this.is_feed) {
            this.ended = true;
            __guard__(this.cursor, x => x.close().catch(() => null));
            return this.trigger('end', this);
        }
    }

    drop_before(n) {
        if (n > this.results_offset) {
            this.results = this.results.slice(n - this.results_offset );
            return this.results_offset = n;
        }
    }

    slice(from, to) {
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
            return this.results.slice(from ,  to + 1 || undefined);
        } else {
            return this.results.slice(from );
        }
    }

    at_beginning() {
        if (this.results_offset != null) {
            return this.results_offset === 0;
        } else {
            return true;
        }
    }
}
QueryResult.initClass();

class Container extends Backbone.View {
    constructor(...args) {
        this.mouse_down_description = this.mouse_down_description.bind(this);
        this.stop_propagation = this.stop_propagation.bind(this);
        this.mouse_up_description = this.mouse_up_description.bind(this);
        this.start_resize_history = this.start_resize_history.bind(this);
        this.clear_history_view = this.clear_history_view.bind(this);
        this.toggle_pressed_buttons = this.toggle_pressed_buttons.bind(this);
        this.toggle_history = this.toggle_history.bind(this);
        this.toggle_options = this.toggle_options.bind(this);
        this.hide_collapsible_panel = this.hide_collapsible_panel.bind(this);
        this.move_arrow = this.move_arrow.bind(this);
        this.adjust_collapsible_panel_height = this.adjust_collapsible_panel_height.bind(this);
        this.deactivate_overflow = this.deactivate_overflow.bind(this);
        this.activate_overflow = this.activate_overflow.bind(this);
        this.convert_type = this.convert_type.bind(this);
        this.expand_types = this.expand_types.bind(this);
        this.set_doc_description = this.set_doc_description.bind(this);
        this.set_docs = this.set_docs.bind(this);
        this.save_query = this.save_query.bind(this);
        this.clear_history = this.clear_history.bind(this);
        this.save_current_query = this.save_current_query.bind(this);
        this.initialize = this.initialize.bind(this);
        this.fetch_data = this.fetch_data.bind(this);
        this.handle_mousemove = this.handle_mousemove.bind(this);
        this.handle_mouseup = this.handle_mouseup.bind(this);
        this.handle_mousedown = this.handle_mousedown.bind(this);
        this.render = this.render.bind(this);
        this.init_after_dom_rendered = this.init_after_dom_rendered.bind(this);
        this.on_blur = this.on_blur.bind(this);
        this.handle_click = this.handle_click.bind(this);
        this.pair_char = this.pair_char.bind(this);
        this.get_next_char = this.get_next_char.bind(this);
        this.get_previous_char = this.get_previous_char.bind(this);
        this.insert_next = this.insert_next.bind(this);
        this.remove_next = this.remove_next.bind(this);
        this.move_cursor = this.move_cursor.bind(this);
        this.count_char = this.count_char.bind(this);
        this.count_not_closed_brackets = this.count_not_closed_brackets.bind(this);
        this.handle_keypress = this.handle_keypress.bind(this);
        this.last_element_type_if_incomplete = this.last_element_type_if_incomplete.bind(this);
        this.get_last_key = this.get_last_key.bind(this);
        this.extract_data_from_query = this.extract_data_from_query.bind(this);
        this.count_group_level = this.count_group_level.bind(this);
        this.create_suggestion = this.create_suggestion.bind(this);
        this.create_safe_regex = this.create_safe_regex.bind(this);
        this.show_suggestion = this.show_suggestion.bind(this);
        this.show_suggestion_without_moving = this.show_suggestion_without_moving.bind(this);
        this.show_description = this.show_description.bind(this);
        this.hide_suggestion = this.hide_suggestion.bind(this);
        this.hide_description = this.hide_description.bind(this);
        this.hide_suggestion_and_description = this.hide_suggestion_and_description.bind(this);
        this.show_or_hide_arrow = this.show_or_hide_arrow.bind(this);
        this.move_suggestion = this.move_suggestion.bind(this);
        this.highlight_suggestion = this.highlight_suggestion.bind(this);
        this.remove_highlight_suggestion = this.remove_highlight_suggestion.bind(this);
        this.write_suggestion = this.write_suggestion.bind(this);
        this.select_suggestion = this.select_suggestion.bind(this);
        this.mouseover_suggestion = this.mouseover_suggestion.bind(this);
        this.mouseout_suggestion = this.mouseout_suggestion.bind(this);
        this.extend_description = this.extend_description.bind(this);
        this.extract_database_used = this.extract_database_used.bind(this);
        this.abort_query = this.abort_query.bind(this);
        this.execute_query = this.execute_query.bind(this);
        this.toggle_executing = this.toggle_executing.bind(this);
        this.execute_portion = this.execute_portion.bind(this);
        this.evaluate = this.evaluate.bind(this);
        this.separate_queries = this.separate_queries.bind(this);
        this.clear_query = this.clear_query.bind(this);
        this.error_on_connect = this.error_on_connect.bind(this);
        this.handle_gutter_click = this.handle_gutter_click.bind(this);
        this.toggle_size = this.toggle_size.bind(this);
        this.display_normal = this.display_normal.bind(this);
        this.display_full = this.display_full.bind(this);
        this.remove = this.remove.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.id = 'dataexplorer';
        this.prototype.template = require('../handlebars/dataexplorer_view.hbs');
        this.prototype.input_query_template = require('../handlebars/dataexplorer_input_query.hbs');
        this.prototype.description_template = require('../handlebars/dataexplorer-description.hbs');
        this.prototype.template_suggestion_name = require('../handlebars/dataexplorer_suggestion_name_li.hbs');
        this.prototype.description_with_example_template = require('../handlebars/dataexplorer-description_with_example.hbs');
        this.prototype.alert_connection_fail_template = require('../handlebars/alert-connection_fail.hbs');
        this.prototype.databases_suggestions_template = require('../handlebars/dataexplorer-databases_suggestions.hbs');
        this.prototype.tables_suggestions_template = require('../handlebars/dataexplorer-tables_suggestions.hbs');
        this.prototype.reason_dataexplorer_broken_template = require('../handlebars/dataexplorer-reason_broken.hbs');
        this.prototype.query_error_template = require('../handlebars/dataexplorer-query_error.hbs');

        // Constants
        this.prototype.line_height = 13; // Define the height of a line (used for a line is too long)
        this.prototype.size_history = 50;

        this.prototype.max_size_stack = 100; // If the stack of the query (including function, string, object etc. is greater than @max_size_stack, we stop parsing the query
        this.prototype.max_size_query = 1000; // If the query is more than 1000 char, we don't show suggestion (codemirror doesn't highlight/parse if the query is more than 1000 characdd_ters too

        this.prototype.delay_toggle_abort = 70; // If a query didn't return during this period (ms) we let people abort the query

        this.prototype.events = {
            'mouseup .CodeMirror': 'handle_click',
            'mousedown .suggestion_name_li': 'select_suggestion', // Keep mousedown to compete with blur on .input_query
            'mouseover .suggestion_name_li' : 'mouseover_suggestion',
            'mouseout .suggestion_name_li' : 'mouseout_suggestion',
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
            // Let people click on the description and select text
            'mousedown .suggestion_description': 'mouse_down_description',
            'click .suggestion_description': 'stop_propagation',
            'mouseup .suggestion_description': 'mouse_up_description',
            'mousedown .suggestion_full_container': 'mouse_down_description',
            'click .suggestion_full_container': 'stop_propagation',
            'mousedown .CodeMirror': 'mouse_down_description',
            'click .CodeMirror': 'stop_propagation'
        };


        this.prototype.displaying_full_view = false;

        // Build the suggestions
        this.prototype.map_state = // Map function -> state
            {'': ''};
        this.prototype.descriptions = {};
        this.prototype.suggestions = {}; // Suggestions[state] = function for this state

        this.prototype.types = {
            value: ['number', 'bool', 'string', 'array', 'object', 'time', 'binary', 'line', 'point', 'polygon'],
            any: ['number', 'bool', 'string', 'array', 'object', 'stream', 'selection', 'table', 'db', 'r', 'error', 'binary', 'line', 'point', 'polygon'],
            geometry: ['line', 'point', 'polygon'],
            sequence: ['table', 'selection', 'stream', 'array'],
            stream: ['table', 'selection'],
            grouped_stream: ['stream', 'array'] // We use full_tag because we need to differentiate between r. and r(
        };


        // All the commands we are going to ignore
        this.prototype.ignored_commands = {
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

        // We have to keep track of a lot of things because web-kit browsers handle the events keydown, keyup, blur etc... in a strange way.
        this.prototype.current_suggestions = [];
        this.prototype.current_highlighted_suggestion = -1;
        this.prototype.current_conpleted_query = '';
        this.prototype.query_first_part = '';
        this.prototype.query_last_part = '';
        this.prototype.mouse_type_event = {
            click: true,
            dblclick: true,
            mousedown: true,
            mouseup: true,
            mouseover: true,
            mouseout: true,
            mousemove: true
        };
        this.prototype.char_breakers = {
            '.': true,
            '}': true,
            ')': true,
            ',': true,
            ';': true,
            ']': true
        };

        this.prototype.matching_opening_bracket = {
            '(': ')',
            '{': '}',
            '[': ']'
        };
        this.prototype.matching_closing_bracket = {
            ')': '(',
            '}': '{',
            ']': '['
        };

        // Extract information from the current query
        // Regex used
        this.prototype.regex = {
            anonymous:/^(\s)*function\s*\(([a-zA-Z0-9,\s]*)\)(\s)*{/,
            loop:/^(\s)*(for|while)(\s)*\(([^\)]*)\)(\s)*{/,
            method: /^(\s)*([a-zA-Z0-9]*)\(/, // forEach( merge( filter(
            row: /^(\s)*row\(/,
            method_var: /^(\s)*(\d*[a-zA-Z][a-zA-Z0-9]*)\./, // r. r.row. (r.count will be caught later)
            return : /^(\s)*return(\s)*/,
            object: /^(\s)*{(\s)*/,
            array: /^(\s)*\[(\s)*/,
            white: /^(\s)+$/,
            white_or_empty: /^(\s)*$/,
            white_replace: /\s/g,
            white_start: /^(\s)+/,
            comma: /^(\s)*,(\s)*/,
            semicolon: /^(\s)*;(\s)*/,
            number: /^[0-9]+\.?[0-9]*/,
            inline_comment: /^(\s)*\/\/.*(\n|$)/,
            multiple_line_comment: /^(\s)*\/\*[^(\*\/)]*\*\//,
            get_first_non_white_char: /\s*(\S+)/,
            last_char_is_white: /.*(\s+)$/
        };
        this.prototype.stop_char = { // Just for performance (we look for a stop_char in constant time - which is better than having 3 and conditions) and cleaner code
            opening: {
                '(': ')',
                '{': '}',
                '[': ']'
            },
            closing: {
                ')': '(', // Match the opening character
                '}': '{',
                ']': '['
            }
        };
    }

    mouse_down_description(event) {
        this.keep_suggestions_on_blur = true;
        return this.stop_propagation(event);
    }

    stop_propagation(event) {
        return event.stopPropagation();
    }

    mouse_up_description(event) {
        this.keep_suggestions_on_blur = false;
        return this.stop_propagation(event);
    }

    start_resize_history(event) {
        return this.history_view.start_resize(event);
    }

    clear_history_view(event) {
        const that = this;
        this.clear_history(); // Delete from localstorage

        return this.history_view.clear_history(event);
    }

    // Method that make sure that just one button (history or option) is active
    // We give this button an "active" class that make it looks like it's pressed.
    toggle_pressed_buttons() {
        if (this.history_view.state === 'visible') {
            dataexplorer_state.history_state = 'visible';
            this.$('.clear_queries_link').fadeIn('fast');
            this.$('.close_queries_link').addClass('active');
        } else {
            dataexplorer_state.history_state = 'hidden';
            this.$('.clear_queries_link').fadeOut('fast');
            this.$('.close_queries_link').removeClass('active');
        }

        if (this.options_view.state === 'visible') {
            dataexplorer_state.options_state = 'visible';
            return this.$('.toggle_options_link').addClass('active');
        } else {
            dataexplorer_state.options_state = 'hidden';
            return this.$('.toggle_options_link').removeClass('active');
        }
    }

    // Show/hide the history view
    toggle_history(args) {
        const that = this;

        this.deactivate_overflow();
        if (args.no_animation === true) {
            // We just show the history
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
                    is_at_bottom: true});
                return that.toggle_pressed_buttons();
            }); // Re-execute toggle_pressed_buttons because we delay the fadeIn
        } else if (this.history_view.state === 'hidden') {
            this.history_view.state = 'visible';
            this.$('.content').html(this.history_view.render(true).$el);
            this.history_view.delegateEvents();
            this.move_arrow({
                type: 'history',
                move_arrow: 'show'
            });
            this.adjust_collapsible_panel_height({
                is_at_bottom: true});
        } else if (this.history_view.state === 'visible') {
            this.history_view.state = 'hidden';
            this.hide_collapsible_panel('history');
        }

        return this.toggle_pressed_buttons();
    }

    // Show/hide the options view
    toggle_options(args) {
        const that = this;

        this.deactivate_overflow();
        if (__guard__(args, x => x.no_animation) === true) {
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
                cb: __guard__(args, x1 => x1.cb)});
        } else if (this.options_view.state === 'visible') {
            this.options_view.state = 'hidden';
            this.hide_collapsible_panel('options');
        }

        return this.toggle_pressed_buttons();
    }

    // Hide the collapsible_panel whether it contains the option or history view
    hide_collapsible_panel(type) {
        const that = this;
        this.deactivate_overflow();
        this.$('.nano').animate(
            {height: 0}
            , 200
            , function() {
                that.activate_overflow();
                // We don't want to hide the view if the user changed the state of the view while it was being animated
                if (((type === 'history') && (that.history_view.state === 'hidden')) || ((type === 'options') && (that.options_view.state === 'hidden'))) {
                    that.$('.nano_border').hide(); // In case the user trigger hide/show really fast
                    that.$('.arrow_dataexplorer').hide(); // In case the user trigger hide/show really fast
                    return that.$(this).css('visibility', 'hidden');
                }
        });
        this.$('.nano_border').slideUp('fast');
        return this.$('.arrow_dataexplorer').slideUp('fast');
    }

    // Move the arrow that points to the active button (on top of the collapsible panel). In case the user switch from options to history (or the opposite), we need to animate it
    move_arrow(args) {
        // args =
        //   type: 'options'/'history'
        //   move_arrow: 'show'/'animate'
        let margin_right;
        if (args.type === 'options') {
            margin_right = 74;
        } else if (args.type === 'history') {
            margin_right = 154;
        }

        if (args.move_arrow === 'show') {
            this.$('.arrow_dataexplorer').css('margin-right', margin_right);
            this.$('.arrow_dataexplorer').show();
        } else if (args.move_arrow === 'animate') {
            this.$('.arrow_dataexplorer').animate(
                {'margin-right': margin_right}
                , 200);
        }
        return this.$('.nano_border').show();
    }

    // Adjust the height of the container of the history/option view
    // Arguments:
    //   size: size of the collapsible panel we want // If not specified, we are going to try to figure it out ourselves
    //   no_animation: boolean (do we need to animate things or just to show it)
    //   is_at_bottom: boolean (if we were at the bottom, we want to scroll down once we have added elements in)
    //   delay_scroll: boolean, true if we just added a query - It speficied if we adjust the height then scroll or do both at the same time
    adjust_collapsible_panel_height(args) {
        let size;
        const that = this;
        if (__guard__(args, x => x.size) != null) {
            ({ size } = args);
        } else {
            if (__guard__(args, x1 => x1.extra) != null) {
                size = Math.min(this.$('.content > div').height()+args.extra, this.history_view.height_history);
            } else {
                size = Math.min(this.$('.content > div').height(), this.history_view.height_history);
            }
        }

        this.deactivate_overflow();

        let duration = Math.max(150, size);
        duration = Math.min(duration, 250);
        //@$('.nano').stop(true, true).animate
        this.$('.nano').css('visibility', 'visible'); // In case the user trigger hide/show really fast
        if (__guard__(args, x2 => x2.no_animation) === true) {
            this.$('.nano').height(size);
            this.$('.nano > .content').scrollTop(this.$('.nano > .content > div').height());
            this.$('.nano').css('visibility', 'visible'); // In case the user trigger hide/show really fast
            this.$('.arrow_dataexplorer').show(); // In case the user trigger hide/show really fast
            this.$('.nano_border').show(); // In case the user trigger hide/show really fast
            if (__guard__(args, x3 => x3.no_animation) === true) {
                this.$('.nano').nanoScroller({preventPageScrolling: true});
            }
            return this.activate_overflow();

        } else {
            this.$('.nano').animate(
                {height: size}
                , duration
                , function() {
                    that.$(this).css('visibility', 'visible'); // In case the user trigger hide/show really fast
                    that.$('.arrow_dataexplorer').show(); // In case the user trigger hide/show really fast
                    that.$('.nano_border').show(); // In case the user trigger hide/show really fast
                    that.$(this).nanoScroller({preventPageScrolling: true});
                    that.activate_overflow();
                    if ((args != null) && (args.delay_scroll === true) && (args.is_at_bottom === true)) {
                        that.$('.nano > .content').animate(
                            {scrollTop: that.$('.nano > .content > div').height()}
                            , 200);
                    }
                    if (__guard__(args, x4 => x4.cb) != null) {
                        return args.cb();
                    }
            });

            if ((args != null) && (args.delay_scroll !== true) && (args.is_at_bottom === true)) {
                return that.$('.nano > .content').animate(
                    {scrollTop: that.$('.nano > .content > div').height()}
                    , 200);
            }
        }
    }



    // We deactivate the scrollbar (if there isn't) while animating to have a smoother experience. We´ll put back the scrollbar once the animation is done.
    deactivate_overflow() {
        if ($(window).height() >= $(document).height()) {
            return $('body').css('overflow', 'hidden');
        }
    }

    activate_overflow() {
        return $('body').css('overflow', 'auto'); // Boolean for the full view (true if full view)
    }

    // Method to close an alert/warning/arror
    close_alert(event) {
        event.preventDefault();
        return $(event.currentTarget).parent().slideUp('fast', function() { return $(this).remove(); });
    }

    // Convert meta types (value, any or sequence) to an array of types or return an array composed of just the type
    convert_type(type) {
        if (this.types[type] != null) {
            return this.types[type];
        } else {
            return [type];
        }
    }

    // Flatten an array
    expand_types(ar) {
        let element;
        const result = [];
        if (_.isArray(ar)) {
            for (element of Array.from(ar)) {
                result.concat(this.convert_type(element));
            }
        } else {
            result.concat(this.convert_type(element));
        }
        return result;
    }

    // Once we are done moving the doc, we could generate a .js in the makefile file with the data so we don't have to do an ajax request+all this stuff
    set_doc_description(command, tag, suggestions) {
        let full_tag, parent_value;
        if (command['body'] != null) {
            // The body of `bracket` uses `()` and not `bracket()`
            // so we manually set the variables dont_need_parenthesis and full_tag
            let dont_need_parenthesis;
            if (tag === 'bracket') {
                dont_need_parenthesis = false;
                full_tag = tag+'(';
            } else {
                dont_need_parenthesis = !(new RegExp(tag+'\\(')).test(command['body']);
                if (dont_need_parenthesis) {
                    full_tag = tag; // Here full_tag is just the name of the tag
                } else {
                    full_tag = tag+'('; // full tag is the name plus a parenthesis (we will match the parenthesis too)
                }
            }

            this.descriptions[full_tag] = grouped_data => {
                return {
                    name: tag,
                    args: __guard__(/.*(\(.*\))/.exec(command['body']), x => x[1]),
                    description:
                        this.description_with_example_template({
                            description: command['description'],
                            example: command['example'],
                            grouped_data: (grouped_data === true) && (full_tag !== 'group(') && (full_tag !== 'ungroup(')
                        })
                };
            };
        }

        const parents = {};
        let returns = [];
        for (let pair of Array.from(command.io != null ? command.io : [])) {
            let parent_values = (pair[0] === null) ? '' : pair[0];
            let return_values = pair[1];

            parent_values = this.convert_type(parent_values);
            return_values = this.convert_type(return_values);
            returns = returns.concat(return_values);
            for (parent_value of Array.from(parent_values)) {
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
    }

    // Method called on the content of reql_docs.json
    // Load the suggestions in @suggestions, @map_state, @descriptions
    set_docs(data) {
        for (let key in data) {
            const command = data[key];
            let tag = command['name'];
            if (tag in this.ignored_commands) {
                continue;
            }
            if (tag === '() (bracket)') { // The parentheses will be added later
                // Add `(attr)`
                tag = '';
                this.set_doc_description(command, tag, this.suggestions);

                // Add `bracket(sttr)`
                tag = 'bracket';
            } else if (tag === 'toJsonString, toJSON') {
                // Add the `toJsonString()` alias
                tag = 'toJsonString';
                this.set_doc_description(command, tag, this.suggestions);
                // Also add `toJSON()`
                tag = 'toJSON';
            }
            this.set_doc_description(command, tag, this.suggestions);
        }

        const relations = data['types'];

        for (let state in this.suggestions) {
            this.suggestions[state].sort();
        }

        if (Container.prototype.focus_on_codemirror === true) {
            // "@" refers to prototype -_-
            // In case we give focus to codemirror then load the docs, we show the suggestion
            return app.main.router.current_view.handle_keypress();
        }
    }

    // Save the query in the history
    // The size of the history is infinite per session. But we will just save @size_history queries in localStorage
    save_query(args) {
        let { query } = args;
        const { broken_query } = args;
        // Remove empty lines
        query = query.replace(/^\s*$[\n\r]{1,}/gm, '');
        query = query.replace(/\s*$/, ''); // Remove the white spaces at the end of the query (like newline/space/tab)
        if (window.localStorage != null) {
            if ((dataexplorer_state.history.length === 0) || ((dataexplorer_state.history[dataexplorer_state.history.length-1].query !== query) && (this.regex.white.test(query) === false))) {
                dataexplorer_state.history.push({
                    query,
                    broken_query
                });
                if (dataexplorer_state.history.length>this.size_history) {
                    window.localStorage.rethinkdb_history = JSON.stringify(dataexplorer_state.history.slice(dataexplorer_state.history.length-this.size_history));
                } else {
                    window.localStorage.rethinkdb_history = JSON.stringify(dataexplorer_state.history);
                }
                return this.history_view.add_query({
                    query,
                    broken_query
                });
            }
        }
    }

    clear_history() {
        dataexplorer_state.history.length = 0;
        return window.localStorage.rethinkdb_history = JSON.stringify(dataexplorer_state.history);
    }

    // Save the current query (the one in codemirror) in local storage
    save_current_query() {
        if (window.localStorage != null) {
            return window.localStorage.current_query = JSON.stringify(this.codemirror.getValue());
        }
    }

    initialize(args) {
        this.TermBaseConstructor = r.expr(1).constructor.__super__.constructor.__super__.constructor;

        this.state = args.state;
        this.executing = false;

        // Load options from local storage
        if (window.localStorage != null) {
            try {
                this.state.options = JSON.parse(window.localStorage.options);
                // Ensure no keys are without a default value in the
                // options object
                _.defaults(this.state.options, DEFAULTS.options);
            } catch (err) {
                window.localStorage.removeItem('options');
            }
            // Whatever the case with default values, we need to sync
            // up with the current application's idea of the options
            window.localStorage.options = JSON.stringify(this.state.options);
        }

        // Load the query that was written in code mirror (that may not have been executed before)
        if (typeof __guard__(window.localStorage, x => x.current_query) === 'string') {
            try {
                dataexplorer_state.current_query = JSON.parse(window.localStorage.current_query);
            } catch (err) {
                window.localStorage.removeItem('current_query');
            }
        }


        if (__guard__(window.localStorage, x1 => x1.rethinkdb_history) != null) {
            try {
                dataexplorer_state.history = JSON.parse(window.localStorage.rethinkdb_history);
            } catch (err) {
                window.localStorage.removeItem('rethinkdb_history');
            }
        }

        this.query_has_changed = __guard__(dataexplorer_state.query_result, x2 => x2.current_query) !== dataexplorer_state.current_query;

        // Index used to navigate through history with the keyboard
        this.history_displayed_id = 0; // 0 means we are showing the draft, n>0 means we are showing the nth query in the history

        // We escape the last function because we are building a regex on top of it.
        // Structure: [ [ pattern, replacement], [pattern, replacement], ... ]
        this.unsafe_to_safe_regexstr = [
            [/\\/g, '\\\\'], // This one has to be first
            [/\(/g, '\\('],
            [/\)/g, '\\)'],
            [/\^/g, '\\^'],
            [/\$/g, '\\$'],
            [/\*/g, '\\*'],
            [/\+/g, '\\+'],
            [/\?/g, '\\?'],
            [/\./g, '\\.'],
            [/\|/g, '\\|'],
            [/\{/g, '\\{'],
            [/\}/g, '\\}'],
            [/\[/g, '\\[']
        ];

        this.results_view_wrapper = new ResultViewWrapper({
            container: this,
            view: dataexplorer_state.view
        });

        this.options_view = new OptionsView({
            container: this,
            options: dataexplorer_state.options
        });

        this.history_view = new HistoryView({
            container: this,
            history: dataexplorer_state.history
        });

        this.driver_handler = new DriverHandler({
            container: this});

        // These events were caught here to avoid being bound and unbound every time
        // The results changed. It should ideally be caught in the individual result views
        // that actually need it.
        $(window).mousemove(this.handle_mousemove);
        $(window).mouseup(this.handle_mouseup);
        $(window).mousedown(this.handle_mousedown);
        this.keep_suggestions_on_blur = false;

        this.databases_available = {};
        return this.fetch_data();
    }

    fetch_data() {
        // We fetch all "proper" tables from `table_config`. In addition, we need
        // to fetch the list of system tables separately.
        const query = r.db(system_db).table('table_config')
            .pluck('db', 'name')
            .group('db')
            .ungroup()
            .map(group => [group("group"), group("reduction")("name").orderBy( x => x)])
            .coerceTo("OBJECT")
            .merge(r.object(system_db, r.db(system_db).tableList().coerceTo("ARRAY")));
        return this.timer = driver.run(query, 5000, (error, result) => {
            if (error != null) {
                // Nothing bad, we'll try again, let's just log the error
                console.log("Error: Could not fetch databases and tables");
                return console.log(error);
            } else {
                return this.databases_available = result;
            }
        }
        );
    }

    handle_mousemove(event) {
        this.results_view_wrapper.handle_mousemove(event);
        return this.history_view.handle_mousemove(event);
    }

    handle_mouseup(event) {
        this.results_view_wrapper.handle_mouseup(event);
        return this.history_view.handle_mouseup(event);
    }

    handle_mousedown(event) {
        // $(window) caught a mousedown event, so it wasn't caught by $('.suggestion_description')
        // Let's hide the suggestion/description
        this.keep_suggestions_on_blur = false;
        return this.hide_suggestion_and_description();
    }

    render() {
        this.$el.html(this.template());
        this.$('.input_query_full_container').html(this.input_query_template());

        // Check if the browser supports the JavaScript driver
        // We do not support internet explorer (even IE 10) and old browsers.
        if (__guard__(navigator, x => x.appName) === 'Microsoft Internet Explorer') {
            this.$('.reason_dataexplorer_broken').html(this.reason_dataexplorer_broken_template({
                is_internet_explorer: true})
            );
            this.$('.reason_dataexplorer_broken').slideDown('fast');
            this.$('.button_query').prop('disabled', true);
        } else if (((typeof DataView === 'undefined' || DataView === null)) || ((typeof Uint8Array === 'undefined' || Uint8Array === null))) { // The main two components that the javascript driver requires.
            this.$('.reason_dataexplorer_broken').html(this.reason_dataexplorer_broken_template);
            this.$('.reason_dataexplorer_broken').slideDown('fast');
            this.$('.button_query').prop('disabled', true);
        } else if (r == null) { // In case the javascript driver is not found (if build from source for example)
            this.$('.reason_dataexplorer_broken').html(this.reason_dataexplorer_broken_template({
                no_driver: true})
            );
            this.$('.reason_dataexplorer_broken').slideDown('fast');
            this.$('.button_query').prop('disabled', true);
        }

        // Let's bring back the data explorer to its old state (if there was)
        if (__guard__(dataexplorer_state, x1 => x1.query_result) != null) {
            this.results_view_wrapper.set_query_result({
                query_result: dataexplorer_state.query_result});
        }

        this.$('.results_container').html(this.results_view_wrapper.render({query_has_changed: this.query_has_changed}).$el);

        // The query in code mirror is set in init_after_dom_rendered (because we can't set it now)

        return this;
    }

    // This method has to be called AFTER the el element has been inserted in the DOM tree, mostly for codemirror
    init_after_dom_rendered() {
        this.codemirror = CodeMirror.fromTextArea(document.getElementById('input_query'), {
            mode: {
                name: 'javascript'
            },
            onKeyEvent: this.handle_keypress,
            lineNumbers: true,
            lineWrapping: true,
            matchBrackets: true,
            tabSize: 2
        }
        );

        this.codemirror.on('blur', this.on_blur);
        this.codemirror.on('gutterClick', this.handle_gutter_click);

        this.codemirror.setSize('100%', 'auto');
        if (dataexplorer_state.current_query != null) {
            this.codemirror.setValue(dataexplorer_state.current_query);
        }
        this.codemirror.focus(); // Give focus

        // Track if the focus is on codemirror
        // We use it to refresh the docs once the reql_docs.json is loaded
        dataexplorer_state.focus_on_codemirror = true;

        this.codemirror.setCursor(this.codemirror.lineCount(), 0);
        if (this.codemirror.getValue() === '') { // We show suggestion for an empty query only
            this.handle_keypress();
        }
        this.results_view_wrapper.init_after_dom_rendered();

        this.draft = this.codemirror.getValue();

        if (dataexplorer_state.history_state === 'visible') { // If the history was visible, we show it
            this.toggle_history({
                no_animation: true});
        }

        if (dataexplorer_state.options_state === 'visible') { // If the history was visible, we show it
            return this.toggle_options({
                no_animation: true});
        }
    }

    on_blur() {
        dataexplorer_state.focus_on_codemirror = false;
        // We hide the description only if the user isn't selecting text from a description.
        if (this.keep_suggestions_on_blur === false) {
            return this.hide_suggestion_and_description();
        }
    }

    handle_click(event) {
        return this.handle_keypress(null, event);
    }

    // Pair ', ", {, [, (
    // Return true if we want code mirror to ignore the key event
    pair_char(event, stack) {
        if (__guard__(event, x => x.which) != null) {
            // If there is a selection and the user hit a quote, we wrap the seleciton in quotes
            let char_to_insert, next_char, num_not_closed_bracket, num_quote;
            if ((this.codemirror.getSelection() !== '') && (event.type === 'keypress')) { // This is madness. If we look for keydown, shift+right arrow match a single quote...
                char_to_insert = String.fromCharCode(event.which);
                if (((char_to_insert != null) && (char_to_insert === '"')) || (char_to_insert === "'")) {
                    this.codemirror.replaceSelection(char_to_insert+this.codemirror.getSelection()+char_to_insert);
                    event.preventDefault();
                    return true;
                }
            }

            if (event.which === 8) { // Backspace
                if (event.type !== 'keydown') {
                    return true;
                }
                const previous_char = this.get_previous_char();
                if (previous_char === null) {
                    return true;
                }
                // If the user remove the opening bracket and the next char is the closing bracket, we delete both
                if (previous_char in this.matching_opening_bracket) {
                    next_char = this.get_next_char();
                    if (next_char === this.matching_opening_bracket[previous_char]) {
                        num_not_closed_bracket = this.count_not_closed_brackets(previous_char);
                        if (num_not_closed_bracket <= 0) {
                            this.remove_next();
                            return true;
                        }
                    }
                // If the user remove the first quote of an empty string, we remove both quotes
                } else if ((previous_char === '"') || (previous_char === "'")) {
                    next_char = this.get_next_char();
                    if ((next_char === previous_char) && (this.get_previous_char(2) !== '\\')) {
                        num_quote = this.count_char(char_to_insert);
                        if ((num_quote%2) === 0) {
                            this.remove_next();
                            return true;
                        }
                    }
                }
                return true;
            }

            if (event.type !== 'keypress') { // We catch keypress because single and double quotes have not the same keyCode on keydown/keypres #thisIsMadness
                return true;
            }

            char_to_insert = String.fromCharCode(event.which);
            if (char_to_insert != null) { // and event.which isnt 91 # 91 map to [ on OS X
                if (this.codemirror.getSelection() !== '') {
                    if (char_to_insert in this.matching_opening_bracket || char_to_insert in this.matching_closing_bracket) {
                        this.codemirror.replaceSelection('');
                    } else {
                        return true;
                    }
                }
                const last_element_incomplete_type = this.last_element_type_if_incomplete(stack);
                if ((char_to_insert === '"') || (char_to_insert === "'")) {
                    num_quote = this.count_char(char_to_insert);
                    next_char = this.get_next_char();
                    if (next_char === char_to_insert) { // Next char is a single quote
                        if ((num_quote%2) === 0) {
                            if ((last_element_incomplete_type === 'string') || (last_element_incomplete_type === 'object_key')) { // We are at the end of a string and the user just wrote a quote
                                this.move_cursor(1);
                                event.preventDefault();
                                return true;
                            } else {
                                // We are at the begining of a string, so let's just add one quote
                                return true;
                            }
                        } else {
                            // Let's add the closing/opening quote missing
                            return true;
                        }
                    } else {
                        if ((num_quote%2) === 0) { // Next char is not a single quote and the user has an even number of quotes.
                            // Let's keep a number of quote even, so we add one extra quote
                            const last_key = this.get_last_key(stack);
                            if (last_element_incomplete_type === 'string') {
                                return true;
                            } else if ((last_element_incomplete_type === 'object_key') && ((last_key !== '') && (this.create_safe_regex(char_to_insert).test(last_key) === true))) { // A key in an object can be seen as a string
                                return true;
                            } else {
                                this.insert_next(char_to_insert);
                            }
                        } else { // Else we'll just insert one quote
                            return true;
                        }
                    }
                } else if (last_element_incomplete_type !== 'string') {
                    next_char = this.get_next_char();

                    if (char_to_insert in this.matching_opening_bracket) {
                        num_not_closed_bracket = this.count_not_closed_brackets(char_to_insert);
                        if (num_not_closed_bracket >= 0) { // We insert a closing bracket only if it help having a balanced number of opened/closed brackets
                            this.insert_next(this.matching_opening_bracket[char_to_insert]);
                            return true;
                        }
                        return true;
                    } else if (char_to_insert in this.matching_closing_bracket) {
                        const opening_char = this.matching_closing_bracket[char_to_insert];
                        num_not_closed_bracket = this.count_not_closed_brackets(opening_char);
                        if (next_char === char_to_insert) {
                            if (num_not_closed_bracket <= 0) { // g(f(...|) In this case we add a closing parenthesis. Same behavior as in Ace
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
    }

    get_next_char() {
        const cursor_end = this.codemirror.getCursor();
        cursor_end.ch++;
        return this.codemirror.getRange(this.codemirror.getCursor(), cursor_end);
    }

    get_previous_char(less_value) {
        const cursor_start = this.codemirror.getCursor();
        const cursor_end = this.codemirror.getCursor();
        if (less_value != null) {
            cursor_start.ch -= less_value;
            cursor_end.ch -= (less_value-1);
        } else {
            cursor_start.ch--;
        }
        if (cursor_start.ch < 0) {
            return null;
        }
        return this.codemirror.getRange(cursor_start, cursor_end);
    }


    // Insert str after the cursor in codemirror
    insert_next(str) {
        this.codemirror.replaceRange(str, this.codemirror.getCursor());
        return this.move_cursor(-1);
    }

    remove_next() {
        const end_cursor = this.codemirror.getCursor();
        end_cursor.ch++;
        return this.codemirror.replaceRange('', this.codemirror.getCursor(), end_cursor);
    }

    // Move cursor of move_value
    // A negative value move the cursor to the left
    move_cursor(move_value) {
        const cursor = this.codemirror.getCursor();
        cursor.ch += move_value;
        if (cursor.ch < 0) {
            cursor.ch = 0;
        }
        return this.codemirror.setCursor(cursor);
    }


    // Count how many time char_to_count appeared ignoring strings and comments
    count_char(char_to_count) {
        const query = this.codemirror.getValue();

        let is_parsing_string = false;
        let to_skip = 0;
        let result = 0;

        for (let i = 0; i < query.length; i++) {
            const char = query[i];
            if (to_skip > 0) { // Because we cannot mess with the iterator in coffee-script
                to_skip--;
                continue;
            }

            if (is_parsing_string === true) {
                if ((char === string_delimiter) && (query[i-1] != null) && (query[i-1] !== '\\')) { // We were in a string. If we see string_delimiter and that the previous character isn't a backslash, we just reached the end of the string.
                    is_parsing_string = false; // Else we just keep parsing the string
                    if (char === char_to_count) {
                        result++;
                    }
                }
            } else { // if element.is_parsing_string is false
                if (char === char_to_count) {
                    result++;
                }

                if ((char === '\'') || (char === '"')) { // So we get a string here
                    is_parsing_string = true;
                    var string_delimiter = char;
                    continue;
                }

                const result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
                if (result_inline_comment != null) {
                    to_skip = result_inline_comment[0].length-1;
                    continue;
                }
                const result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
                if (result_multiple_line_comment != null) {
                    to_skip = result_multiple_line_comment[0].length-1;
                    continue;
                }
            }
        }

        return result;
    }


    // opening_char has to be in @matching_bracket
    // Count how many time opening_char has been opened but not closed
    // A result < 0 means that the closing char has been found more often than the opening one
    count_not_closed_brackets(opening_char) {
        const query = this.codemirror.getValue();

        let is_parsing_string = false;
        let to_skip = 0;
        let result = 0;

        for (let i = 0; i < query.length; i++) {
            const char = query[i];
            if (to_skip > 0) { // Because we cannot mess with the iterator in coffee-script
                to_skip--;
                continue;
            }

            if (is_parsing_string === true) {
                if ((char === string_delimiter) && (query[i-1] != null) && (query[i-1] !== '\\')) { // We were in a string. If we see string_delimiter and that the previous character isn't a backslash, we just reached the end of the string.
                    is_parsing_string = false; // Else we just keep parsing the string
                }
            } else { // if element.is_parsing_string is false
                if (char === opening_char) {
                    result++;
                } else if (char === this.matching_opening_bracket[opening_char]) {
                    result--;
                }

                if ((char === '\'') || (char === '"')) { // So we get a string here
                    is_parsing_string = true;
                    var string_delimiter = char;
                    continue;
                }

                const result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
                if (result_inline_comment != null) {
                    to_skip = result_inline_comment[0].length-1;
                    continue;
                }
                const result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
                if (result_multiple_line_comment != null) {
                    to_skip = result_multiple_line_comment[0].length-1;
                    continue;
                }
            }
        }

        return result;
    }


    // Handle events on codemirror
    // Return true if we want code mirror to ignore the event
    handle_keypress(editor, event) {
        let i, query, query_after_cursor, query_before_cursor, regex, result, stack, suggestion;
        if (this.ignored_next_keyup === true) {
            if ((__guard__(event, x => x.type) === 'keyup') && (__guard__(event, x1 => x1.which) !== 9)) {
                this.ignored_next_keyup = false;
            }
            return true;
        }

        dataexplorer_state.focus_on_codemirror = true;

        // Let's hide the tooltip if the user just clicked on the textarea. We'll only display later the suggestions if there are (no description)
        if (__guard__(event, x2 => x2.type) === 'mouseup') {
            this.hide_suggestion_and_description();
        }

        // Save the last query (even incomplete)
        dataexplorer_state.current_query = this.codemirror.getValue();
        this.save_current_query();

        // Look for special commands
        if (__guard__(event, x3 => x3.which) != null) {
            if ((event.type !== 'keydown') && (((event.ctrlKey === true) || (event.metaKey === true)) && (event.which === 32))) {
                // Because on event.type == 'keydown' we are going to change the state (hidden or displayed) of @$('.suggestion_description') and @$('.suggestion_name_list'), we don't want to fire this event a second time
                return true;
            }

            if ((event.which === 27) || ((event.type === 'keydown') && (((event.ctrlKey === true) || (event.metaKey === true)) && (event.which === 32)) && ((this.$('.suggestion_description').css('display') !== 'none') || (this.$('.suggestion_name_list').css('display') !== 'none')))) {
                // We caugh ESC or (Ctrl/Cmd+space with suggestion/description being displayed)
                event.preventDefault(); // Keep focus on code mirror
                this.hide_suggestion_and_description();
                query_before_cursor = this.codemirror.getRange({line: 0, ch: 0}, this.codemirror.getCursor());
                query_after_cursor = this.codemirror.getRange(this.codemirror.getCursor(), {line:this.codemirror.lineCount()+1, ch: 0});

                // Compute the structure of the query written by the user.
                // We compute it earlier than before because @pair_char also listen on keydown and needs stack
                stack = this.extract_data_from_query({
                    size_stack: 0,
                    query: query_before_cursor,
                    position: 0
                });

                if (stack === null) { // Stack is null if the query was too big for us to parse
                    this.ignore_tab_keyup = false;
                    this.hide_suggestion_and_description();
                    return false;
                }

                this.current_highlighted_suggestion = -1;
                this.current_highlighted_extra_suggestion = -1;
                this.$('.suggestion_name_list').empty();

                // Valid step, let's save the data
                this.query_last_part = query_after_cursor;

                this.current_suggestions = [];
                this.current_element = '';
                this.current_extra_suggestion = '';
                this.written_suggestion = null;
                this.cursor_for_auto_completion = this.codemirror.getCursor();
                this.description = null;

                result =
                    {status: null};
                    // create_suggestion is going to fill to_complete and to_describe
                    //to_complete: undefined
                    //to_describe: undefined

                // Create the suggestion/description
                this.create_suggestion({
                    stack,
                    query: query_before_cursor,
                    result
                });
                result.suggestions = this.uniq(result.suggestions);

                this.grouped_data = this.count_group_level(stack).count_group > 0;

                if (__guard__(result.suggestions, x4 => x4.length) > 0) {
                    for (i = 0; i < result.suggestions.length; i++) {
                        suggestion = result.suggestions[i];
                        if ((suggestion !== 'ungroup(') || (this.grouped_data === true)) { // We add the suggestion for `ungroup` only if we are in a group_stream/data (using the flag @grouped_data)
                            result.suggestions.sort(); // We could eventually sort things earlier with a merge sort but for now that should be enough
                            this.current_suggestions.push(suggestion);
                            this.$('.suggestion_name_list').append(this.template_suggestion_name({
                                id: i,
                                suggestion
                            })
                            );
                        }
                    }
                } else if (result.description != null) {
                    this.description = result.description;
                }

                return true;
            } else if ((event.which === 13) && ((event.shiftKey === false) && (event.ctrlKey === false) && (event.metaKey === false))) {
                if (event.type === 'keydown') {
                    if (this.current_highlighted_suggestion > -1) {
                        event.preventDefault();
                        this.handle_keypress();
                        return true;
                    }

                    const previous_char = this.get_previous_char();
                    if (previous_char in this.matching_opening_bracket) {
                        const next_char = this.get_next_char();
                        if (this.matching_opening_bracket[previous_char] === next_char) {
                            const cursor = this.codemirror.getCursor();
                            this.insert_next('\n');
                            this.codemirror.indentLine(cursor.line+1, 'smart');
                            this.codemirror.setCursor(cursor);
                            return false;
                        }
                    }
                }
            } else if (((event.which === 9) && (event.ctrlKey === false))  || ((event.type === 'keydown') && (((event.ctrlKey === true) || (event.metaKey === true)) && (event.which === 32)) && ((this.$('.suggestion_description').css('display') === 'none') && (this.$('.suggestion_name_list').css('display') === 'none')))) {
                // If the user just hit tab, we are going to show the suggestions if they are hidden
                // or if they suggestions are already shown, we are going to cycle through them.
                //
                // If the user just hit Ctrl/Cmd+space with suggestion/description being hidden we show the suggestions
                // Note that the user cannot cycle through suggestions because we make sure in the IF condition that suggestion/description are hidden
                // If the suggestions/description are visible, the event will be caught earlier with ESC
                event.preventDefault();
                if (event.type !== 'keydown') {
                    return false;
                } else {
                    if (__guard__(this.current_suggestions, x5 => x5.length) > 0) {
                        if (this.$('.suggestion_name_list').css('display') === 'none') {
                            this.show_suggestion();
                            return true;
                        } else {
                            // We can retrieve the content of codemirror only on keyup events. The users may write "r." then hit "d" then "tab" If the events are triggered this way
                            // keydown d - keydown tab - keyup d - keyup tab
                            // We want to only show the suggestions for r.d
                            let cached_query;
                            if (this.written_suggestion === null) {
                                cached_query = this.query_first_part+this.current_element+this.query_last_part;
                            } else {
                                cached_query = this.query_first_part+this.written_suggestion+this.query_last_part;
                            }
                            if (cached_query !== this.codemirror.getValue()) { // We fired a keydown tab before a keyup, so our suggestions are not up to date
                                this.current_element = this.codemirror.getValue().slice(this.query_first_part.length, this.codemirror.getValue().length-this.query_last_part.length);
                                regex = this.create_safe_regex(this.current_element);
                                const new_suggestions = [];
                                let new_highlighted_suggestion = -1;
                                for (let index = 0; index < this.current_suggestions.length; index++) {
                                    suggestion = this.current_suggestions[index];
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
                                    for (i = 0; i < this.current_suggestions.length; i++) {
                                        suggestion = this.current_suggestions[i];
                                        this.$('.suggestion_name_list').append(this.template_suggestion_name({
                                            id: i,
                                            suggestion
                                        })
                                        );
                                    }
                                    this.ignored_next_keyup = true;
                                } else {
                                    this.hide_suggestion_and_description();
                                }
                            }


                            // Switch throught the suggestions
                            if (event.shiftKey) {
                                this.current_highlighted_suggestion--;
                                if (this.current_highlighted_suggestion < -1) {
                                    this.current_highlighted_suggestion = this.current_suggestions.length-1;
                                } else if (this.current_highlighted_suggestion < 0) {
                                    this.show_suggestion_without_moving();
                                    this.remove_highlight_suggestion();
                                    this.write_suggestion({
                                        suggestion_to_write: this.current_element});
                                    this.ignore_tab_keyup = true; // If we are switching suggestion, we don't want to do anything else related to tab
                                    return true;
                                }
                            } else {
                                this.current_highlighted_suggestion++;
                                if (this.current_highlighted_suggestion >= this.current_suggestions.length) {
                                    this.show_suggestion_without_moving();
                                    this.remove_highlight_suggestion();
                                    this.write_suggestion({
                                        suggestion_to_write: this.current_element});
                                    this.ignore_tab_keyup = true; // If we are switching suggestion, we don't want to do anything else related to tab
                                    this.current_highlighted_suggestion = -1;
                                    return true;
                                }
                            }
                            if (this.current_suggestions[this.current_highlighted_suggestion] != null) {
                                this.show_suggestion_without_moving();
                                this.highlight_suggestion(this.current_highlighted_suggestion); // Highlight the current suggestion
                                this.write_suggestion({
                                    suggestion_to_write: this.current_suggestions[this.current_highlighted_suggestion]}); // Auto complete with the highlighted suggestion
                                this.ignore_tab_keyup = true; // If we are switching suggestion, we don't want to do anything else related to tab
                                return true;
                            }
                        }
                    } else if (this.description != null) {
                        if (this.$('.suggestion_description').css('display') === 'none') {
                            // We show it once only because we don't want to move the cursor around
                            this.show_description();
                            return true;
                        }

                        if ((this.extra_suggestions != null) && (this.extra_suggestions.length > 0) && (this.extra_suggestion.start_body === this.extra_suggestion.start_body)) {
                            // Trim suggestion
                            if (__guard__(__guard__(__guard__(this.extra_suggestion, x8 => x8.body), x7 => x7[0]), x6 => x6.type) === 'string') {
                                if (this.extra_suggestion.body[0].complete === true) {
                                    this.extra_suggestions = [];
                                } else {
                                    // Remove quotes around the table/db name
                                    const current_name = this.extra_suggestion.body[0].name.replace(/^\s*('|")/, '').replace(/('|")\s*$/, '');
                                    regex = this.create_safe_regex(current_name);
                                    const new_extra_suggestions = [];
                                    for (suggestion of Array.from(this.extra_suggestions)) {
                                        if (regex.test(suggestion) === true) {
                                            new_extra_suggestions.push(suggestion);
                                        }
                                    }
                                    this.extra_suggestions = new_extra_suggestions;
                                }
                            }

                            if (this.extra_suggestions.length > 0) { // If there are still some valid suggestions
                                let move_outside;
                                query = this.codemirror.getValue();

                                // We did not parse what is after the cursor, so let's take a look
                                let start_search = this.extra_suggestion.start_body;
                                if (__guard__(__guard__(this.extra_suggestion.body, x10 => x10[0]), x9 => x9.name.length) != null) {
                                    start_search += this.extra_suggestion.body[0].name.length;
                                }

                                // Define @query_first_part and @query_last_part
                                // Note that ) is not a valid character for a db/table name
                                const end_body = query.indexOf(')', start_search);
                                this.query_last_part = '';
                                if (end_body !== -1) {
                                    this.query_last_part = query.slice(end_body);
                                }
                                this.query_first_part = query.slice(0, this.extra_suggestion.start_body);
                                const lines = this.query_first_part.split('\n');

                                if (event.shiftKey === true) {
                                    this.current_highlighted_extra_suggestion--;
                                } else {
                                    this.current_highlighted_extra_suggestion++;
                                }

                                if (this.current_highlighted_extra_suggestion >= this.extra_suggestions.length) {
                                    this.current_highlighted_extra_suggestion = -1;
                                } else if (this.current_highlighted_extra_suggestion < -1) {
                                    this.current_highlighted_extra_suggestion = this.extra_suggestions.length-1;
                                }

                                // Create the next suggestion
                                suggestion = '';
                                if (this.current_highlighted_extra_suggestion === -1) {
                                    if (this.current_extra_suggestion != null) {
                                        if (/^\s*'/.test(this.current_extra_suggestion) === true) {
                                            suggestion = this.current_extra_suggestion+"'";
                                        } else if (/^\s*"/.test(this.current_extra_suggestion) === true) {
                                            suggestion = this.current_extra_suggestion+'"';
                                        }
                                    }
                                } else {
                                    let string_delimiter;
                                    if (dataexplorer_state.options.electric_punctuation === false) {
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
                                    suggestion = string_delimiter+this.extra_suggestions[this.current_highlighted_extra_suggestion]+string_delimiter;
                                }

                                this.write_suggestion({
                                    move_outside,
                                    suggestion_to_write: suggestion
                                });
                                this.ignore_tab_keyup = true; // If we are switching suggestion, we don't want to do anything else related to tab
                            }
                        }
                    }
                }
            }

            // If the user hit enter and (Ctrl or Shift)
            if ((event.which === 13) && (event.shiftKey || event.ctrlKey || event.metaKey)) {
                this.hide_suggestion_and_description();
                event.preventDefault();
                if (event.type !== 'keydown') {
                    return true;
                }
                this.execute_query();
                return true;
            // Ctrl/Cmd + V
            } else if ((event.ctrlKey || event.metaKey) && (event.which === 86) && (event.type === 'keydown')) {
                this.last_action_is_paste = true;
                this.num_released_keys = 0; // We want to know when the user release Ctrl AND V
                if (event.metaKey) {
                    this.num_released_keys++; // Because on OS X, the keyup event is not fired when the metaKey is pressed (true for Firefox, Chrome, Safari at least...)
                }
                this.hide_suggestion_and_description();
                return true;
            // When the user release Ctrl/Cmd after a Ctrl/Cmd + V
            } else if ((event.type === 'keyup') && (this.last_action_is_paste === true) && ((event.which === 17) || (event.which === 91))) {
                this.num_released_keys++;
                if (this.num_released_keys === 2) {
                    this.last_action_is_paste = false;
                }
                this.hide_suggestion_and_description();
                return true;
            // When the user release V after a Ctrl/Cmd + V
            } else if ((event.type === 'keyup') && (this.last_action_is_paste === true) && (event.which === 86)) {
                this.num_released_keys++;
                if (this.num_released_keys === 2) {
                    this.last_action_is_paste = false;
                }
                this.hide_suggestion_and_description();
                return true;
            // Catching history navigation
            } else if ((event.type === 'keyup') && event.altKey && (event.which === 38)) { // Key up
                if (this.history_displayed_id < dataexplorer_state.history.length) {
                    this.history_displayed_id++;
                    this.codemirror.setValue(dataexplorer_state.history[dataexplorer_state.history.length-this.history_displayed_id].query);
                    event.preventDefault();
                    return true;
                }
            } else if ((event.type === 'keyup') && event.altKey && (event.which === 40)) { // Key down
                if (this.history_displayed_id > 1) {
                    this.history_displayed_id--;
                    this.codemirror.setValue(dataexplorer_state.history[dataexplorer_state.history.length-this.history_displayed_id].query);
                    event.preventDefault();
                    return true;
                } else if (this.history_displayed_id === 1) {
                    this.history_displayed_id--;
                    this.codemirror.setValue(this.draft);
                    this.codemirror.setCursor(this.codemirror.lineCount(), 0); // We hit the draft and put the cursor at the end
                }
            } else if ((event.type === 'keyup') && event.altKey && (event.which === 33)) { // Page up
                this.history_displayed_id = dataexplorer_state.history.length;
                this.codemirror.setValue(dataexplorer_state.history[dataexplorer_state.history.length-this.history_displayed_id].query);
                event.preventDefault();
                return true;
            } else if ((event.type === 'keyup') && event.altKey && (event.which === 34)) { // Page down
                this.history_displayed_id = this.history.length;
                this.codemirror.setValue(dataexplorer_state.history[dataexplorer_state.history.length-this.history_displayed_id].query);
                this.codemirror.setCursor(this.codemirror.lineCount(), 0); // We hit the draft and put the cursor at the end
                event.preventDefault();
                return true;
            }
        }
        // If there is a hilighted suggestion, we want to catch enter
        if (this.$('.suggestion_name_li_hl').length > 0) {
            if (__guard__(event, x11 => x11.which) === 13) {
                event.preventDefault();
                this.handle_keypress();
                return true;
            }
        }

        // We are scrolling in history
        if ((this.history_displayed_id !== 0) && (event != null)) {
            // We catch ctrl, shift, alt and command
            if (event.ctrlKey || event.shiftKey || event.altKey || (event.which === 16) || (event.which === 17) || (event.which === 18) || (event.which === 20) || ((event.which === 91) && (event.type !== 'keypress')) || (event.which === 92) || event.type in this.mouse_type_event) {
                return false;
            }
        }

        // We catch ctrl, shift, alt and command but don't look for active key (active key here refer to ctrl, shift, alt being pressed and hold)
        if ((event != null) && ((event.which === 16) || (event.which === 17) || (event.which === 18) || (event.which === 20) || ((event.which === 91) && (event.type !== 'keypress')) || (event.which === 92))) {
            return false;
        }


        // Avoid arrows+home+end+page down+pageup
        // if event? and (event.which is 24 or event.which is ..)
        // 0 is for firefox...
        if ((event == null) || ((event.which !== 37) && (event.which !== 38) && (event.which !== 39) && (event.which !== 40) && (event.which !== 33) && (event.which !== 34) && (event.which !== 35) && (event.which !== 36) && (event.which !== 0))) {
            this.history_displayed_id = 0;
            this.draft = this.codemirror.getValue();
        }

        // The expensive operations are coming. If the query is too long, we just don't parse the query
        if (this.codemirror.getValue().length > this.max_size_query) {
            // Return true or false will break the event propagation
            return undefined;
        }

        query_before_cursor = this.codemirror.getRange({line: 0, ch: 0}, this.codemirror.getCursor());
        query_after_cursor = this.codemirror.getRange(this.codemirror.getCursor(), {line:this.codemirror.lineCount()+1, ch: 0});

        // Compute the structure of the query written by the user.
        // We compute it earlier than before because @pair_char also listen on keydown and needs stack
        stack = this.extract_data_from_query({
            size_stack: 0,
            query: query_before_cursor,
            position: 0
        });


        if (stack === null) { // Stack is null if the query was too big for us to parse
            this.ignore_tab_keyup = false;
            this.hide_suggestion_and_description();
            return false;
        }

        if (dataexplorer_state.options.electric_punctuation === true) {
            this.pair_char(event, stack); // Pair brackets/quotes
        }

        // We just look at key up so we don't fire the call 3 times
        if ((__guard__(event, x12 => x12.type) != null) && (event.type !== 'keyup') && (event.which !== 9) && (event.type !== 'mouseup')) {
            return false;
        }
        if (__guard__(event, x13 => x13.which) === 16) { // We don't do anything with Shift.
            return false;
        }

        // Tab is an exception, we let it pass (tab bring back suggestions) - What we want is to catch keydown
        if ((this.ignore_tab_keyup === true) && (__guard__(event, x14 => x14.which) === 9)) {
            if (event.type === 'keyup') {
                this.ignore_tab_keyup = false;
            }
            return true;
        }

        this.current_highlighted_suggestion = -1;
        this.current_highlighted_extra_suggestion = -1;
        this.$('.suggestion_name_list').empty();

        // Valid step, let's save the data
        this.query_last_part = query_after_cursor;

        // If a selection is active, we just catch shift+enter
        if (this.codemirror.getSelection() !== '') {
            this.hide_suggestion_and_description();
            if ((event != null) && (event.which === 13) && (event.shiftKey || event.ctrlKey || event.metaKey)) { // If the user hit enter and (Ctrl or Shift or Cmd)
                this.hide_suggestion_and_description();
                if (event.type !== 'keydown') {
                    return true;
                }
                this.execute_query();
                return true; // We do not replace the selection with a new line
            }
            // If the user select something and end somehwere with suggestion
            if (__guard__(event, x15 => x15.type) !== 'mouseup') {
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

        result =
            {status: null};
            // create_suggestion is going to fill to_complete and to_describe
            //to_complete: undefined
            //to_describe: undefined

        // If we are in the middle of a function (text after the cursor - that is not an element in @char_breakers or a comment), we just show a description, not a suggestion
        const result_non_white_char_after_cursor = this.regex.get_first_non_white_char.exec(query_after_cursor);

        if ((result_non_white_char_after_cursor !== null) && !(__guard__(result_non_white_char_after_cursor[1], x16 => x16[0]) in this.char_breakers || (__guard__(result_non_white_char_after_cursor[1], x17 => x17.match(/^((\/\/)|(\/\*))/)) !== null))) {
            result.status = 'break_and_look_for_description';
            this.hide_suggestion();
        } else {
            const result_last_char_is_white = this.regex.last_char_is_white.exec(query_before_cursor[query_before_cursor.length-1]);
            if (result_last_char_is_white !== null) {
                result.status = 'break_and_look_for_description';
                this.hide_suggestion();
            }
        }

        // Create the suggestion/description
        this.create_suggestion({
            stack,
            query: query_before_cursor,
            result
        });
        result.suggestions = this.uniq(result.suggestions);

        this.grouped_data = this.count_group_level(stack).count_group > 0;

        if (__guard__(result.suggestions, x18 => x18.length) > 0) {
            let show_suggestion = false;
            for (i = 0; i < result.suggestions.length; i++) {
                suggestion = result.suggestions[i];
                if ((suggestion !== 'ungroup(') || (this.grouped_data === true)) { // We add the suggestion for `ungroup` only if we are in a group_stream/data (using the flag @grouped_data)
                    show_suggestion = true;
                    this.current_suggestions.push(suggestion);
                    this.$('.suggestion_name_list').append(this.template_suggestion_name({
                        id: i,
                        suggestion
                    })
                    );
                }
            }
            if ((dataexplorer_state.options.suggestions === true) && (show_suggestion === true)) {
                this.show_suggestion();
            } else {
                this.hide_suggestion();
            }
            this.hide_description();
        } else if (result.description != null) {
            this.hide_suggestion();
            this.description = result.description;
            if ((dataexplorer_state.options.suggestions === true) && (__guard__(event, x19 => x19.type) !== 'mouseup')) {
                this.show_description();
            } else {
                this.hide_description();
            }
        } else {
            this.hide_suggestion_and_description();
        }
        if (__guard__(event, x20 => x20.which) === 9) { // Catch tab
            // If you're in a string, you add a TAB. If you're at the beginning of a newline with preceding whitespace, you add a TAB. If it's any other case do nothing.
            if ((this.last_element_type_if_incomplete(stack) !== 'string') && (this.regex.white_or_empty.test(this.codemirror.getLine(this.codemirror.getCursor().line).slice(0, this.codemirror.getCursor().ch)) !== true)) {
                return true;
            } else {
                return false;
            }
        }
        return true;
    }

    // Similar to underscore's uniq but faster with a hashmap
    uniq(ar) {
        if ((ar == null) || (ar.length === 0)) {
            return ar;
        }
        const result = [];
        const hash = {};
        for (let element of Array.from(ar)) {
            hash[element] = true;
        }
        for (let key in hash) {
            result.push(key);
        }
        result.sort();
        return result;
    }

    // Return the type of the last incomplete object or an empty string
    last_element_type_if_incomplete(stack) {
        if (((stack == null)) || (stack.length === 0)) {
            return '';
        }

        const element = stack[stack.length-1];
        if (element.body != null) {
            return this.last_element_type_if_incomplete(element.body);
        } else {
            if (element.complete === false) {
                return element.type;
            } else {
                return '';
            }
        }
    }

     // Get the last key if the last element is a key of an object
    get_last_key(stack) {
        if (((stack == null)) || (stack.length === 0)) {
            return '';
        }

        const element = stack[stack.length-1];
        if (element.body != null) {
            return this.get_last_key(element.body);
        } else {
            if ((element.complete === false) && (element.key != null)) {
                return element.key;
            } else {
                return '';
            }
        }
    }

   // We build a stack of the query.
    // Chained functions are in the same array, arguments/inner queries are in a nested array
    // element.type in ['string', 'function', 'var', 'separator', 'anonymous_function', 'object', 'array_entry', 'object_key' 'array']
    extract_data_from_query(args) {
        let body, body_start, entry_start, i, new_element;
        let { size_stack } = args;
        const { query } = args;
        const context = (args.context != null) ? util.deep_copy(args.context) : {};
        const { position } = args;

        const stack = [];
        let element = {
            type: null,
            context,
            complete: false
        };
        let start = 0;

        let is_parsing_string = false;
        let to_skip = 0;

        for (i = 0; i < query.length; i++) {
            const char = query[i];
            if (to_skip > 0) { // Because we cannot mess with the iterator in coffee-script
                to_skip--;
                continue;
            }

            if (is_parsing_string === true) {
                if ((char === string_delimiter) && (query[i-1] != null) && (query[i-1] !== '\\')) { // We were in a string. If we see string_delimiter and that the previous character isn't a backslash, we just reached the end of the string.
                    is_parsing_string = false; // Else we just keep parsing the string
                    if (element.type === 'string') {
                        element.name = query.slice(start, i+1);
                        element.complete = true;
                        stack.push(element);
                        size_stack++;
                        if (size_stack > this.max_size_stack) {
                            return null;
                        }
                        element = {
                            type: null,
                            context,
                            complete: false
                        };
                        start = i+1;
                    }
                }
            } else { // if element.is_parsing_string is false
                var result_regex, stack_stop_char;
                if ((char === '\'') || (char === '"')) { // So we get a string here
                    is_parsing_string = true;
                    var string_delimiter = char;
                    if (element.type === null) {
                        element.type = 'string';
                        start = i;
                    }
                    continue;
                }


                if (element.type === null) {
                    const result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
                    if (result_inline_comment != null) {
                        to_skip = result_inline_comment[0].length-1;
                        start += result_inline_comment[0].length;
                        continue;
                    }
                    const result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
                    if (result_multiple_line_comment != null) {
                        to_skip = result_multiple_line_comment[0].length-1;
                        start += result_multiple_line_comment[0].length;
                        continue;
                    }

                    if (start === i) {
                        var new_start;
                        const result_white = this.regex.white_start.exec(query.slice(i));
                        if (result_white != null) {
                            to_skip = result_white[0].length-1;
                            start += result_white[0].length;
                            continue;
                        }

                        // Check for anonymous function
                        result_regex = this.regex.anonymous.exec(query.slice(i));
                        if (result_regex !== null) {
                            element.type = 'anonymous_function';
                            const list_args = __guard__(result_regex[2], x => x.split(','));
                            element.args = [];
                            const new_context = util.deep_copy(context);
                            for (let arg of Array.from(list_args)) {
                                arg = arg.replace(/(^\s*)|(\s*$)/gi,""); // Removing leading/trailing spaces
                                new_context[arg] = true;
                                element.args.push(arg);
                            }
                            element.context = new_context;
                            to_skip = result_regex[0].length;
                            body_start = i+result_regex[0].length;
                            stack_stop_char = ['{'];
                            continue;
                        }

                        // Check for a for loop
                        result_regex = this.regex.loop.exec(query.slice(i));
                        if (result_regex !== null) {
                            element.type = 'loop';
                            element.context = context;
                            to_skip = result_regex[0].length;
                            body_start = i+result_regex[0].length;
                            stack_stop_char = ['{'];
                            continue;
                        }

                        // Check for return
                        result_regex = this.regex.return.exec(query.slice(i));
                        if (result_regex !== null) {
                            // I'm not sure we need to keep track of return, but let's keep it for now
                            element.type = 'return';
                            element.complete = true;
                            to_skip = result_regex[0].length-1;
                            stack.push(element);
                            size_stack++;
                            if (size_stack > this.max_size_stack) {
                                return null;
                            }
                            element = {
                                type: null,
                                context,
                                complete: false
                            };

                            start = i+result_regex[0].length;
                            continue;
                        }

                        // Check for object
                        result_regex = this.regex.object.exec(query.slice(i));
                        if (result_regex !== null) {
                            element.type = 'object';
                            element.next_key = null;
                            element.body = []; // We need to keep tracker of the order of pairs
                            element.current_key_start = i+result_regex[0].length;
                            to_skip = result_regex[0].length-1;
                            stack_stop_char = ['{'];
                            continue;
                        }

                        // Check for array
                        result_regex = this.regex.array.exec(query.slice(i));
                        if (result_regex !== null) {
                            element.type = 'array';
                            element.next_key = null;
                            element.body = [];
                            entry_start = i+result_regex[0].length;
                            to_skip = result_regex[0].length-1;
                            stack_stop_char = ['['];
                            continue;
                        }

                        if (char === '.') {
                            new_start = i+1;
                        } else {
                            new_start = i;
                        }

                        // Check for a standard method
                        result_regex = this.regex.method.exec(query.slice(new_start));
                        if (result_regex !== null) {
                            var position_opening_parenthesis;
                            const result_regex_row = this.regex.row.exec(query.slice(new_start));
                            if (result_regex_row !== null) {
                                position_opening_parenthesis = result_regex_row[0].indexOf('(');
                                element.type = 'function'; // TODO replace with function
                                element.name = 'row';
                                stack.push(element);
                                size_stack++;
                                if (size_stack > this.max_size_stack) {
                                    return null;
                                }
                                element = {
                                    type: 'function',
                                    name: '(',
                                    position: position+3+1,
                                    context,
                                    complete: 'false'
                                };
                                stack_stop_char = ['('];
                                start += position_opening_parenthesis;
                                to_skip = ((result_regex[0].length-1)+new_start)-i;
                                continue;

                            } else {
                                if ((__guard__(stack[stack.length-1], x1 => x1.type) === 'function') || (__guard__(stack[stack.length-1], x2 => x2.type) === 'var')) { // We want the query to start with r. or arg.
                                    element.type = 'function';
                                    element.name = result_regex[0];
                                    element.position = position+new_start;
                                    start += new_start-i;
                                    to_skip = ((result_regex[0].length-1)+new_start)-i;
                                    stack_stop_char = ['('];
                                    continue;
                                } else {
                                    position_opening_parenthesis = result_regex[0].indexOf('(');
                                    if ((position_opening_parenthesis !== -1) && result_regex[0].slice(0, position_opening_parenthesis) in context) {
                                        // Save the var
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
                                            position: position+position_opening_parenthesis+1,
                                            context,
                                            complete: 'false'
                                        };
                                        stack_stop_char = ['('];
                                        start = position_opening_parenthesis;
                                        to_skip = result_regex[0].length-1;
                                        continue;
                                    }
                                }
                            }
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


                        // Check for method without parenthesis r., r.row., doc.
                        result_regex = this.regex.method_var.exec(query.slice(new_start));
                        if (result_regex !== null) {
                            if (result_regex[0].slice(0, result_regex[0].length-1) in context) {
                                element.type = 'var';
                                element.real_type = this.types.value;
                            } else {
                                element.type = 'function';
                            }
                            element.position = position+new_start;
                            element.name = result_regex[0].slice(0, result_regex[0].length-1).replace(/\s/, '');
                            element.complete = true;
                            to_skip = ((element.name.length-1)+new_start)-i;
                            stack.push(element);
                            size_stack++;
                            if (size_stack > this.max_size_stack) {
                                return null;
                            }
                            element = {
                                type: null,
                                context,
                                complete: false
                            };
                            start = new_start+to_skip+1;
                            start -= new_start-i;
                            continue;
                        }

                        // Look for a comma
                        result_regex = this.regex.comma.exec(query.slice(i));
                        if (result_regex !== null) {
                            // element should have been pushed in stack. If not, the query is malformed
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
                                context,
                                complete: false
                            };
                            start = ((i+result_regex[0].length)-1)+1;
                            to_skip = result_regex[0].length-1;
                            continue;
                        }

                        // Look for a semi colon
                        result_regex = this.regex.semicolon.exec(query.slice(i));
                        if (result_regex !== null) {
                            // element should have been pushed in stack. If not, the query is malformed
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
                                context,
                                complete: false
                            };
                            start = ((i+result_regex[0].length)-1)+1;
                            to_skip = result_regex[0].length-1;
                            continue;
                        }
                    //else # if element.start isnt i
                    // We caught the white spaces, so there is nothing to do here
                    } else {
                        if (char === ';') {
                            // We just encountered a semi colon. We have an unknown element
                            // So We just got a random javascript statement, let's just ignore it
                            start = i+1;
                        }
                    }
                } else { // element.type isnt null
                    // Catch separator like for groupedMapReduce
                    result_regex = this.regex.comma.exec(query.slice(i));
                    if ((result_regex !== null) && (stack_stop_char.length < 1)) {
                        // element should have been pushed in stack. If not, the query is malformed
                        stack.push({
                            type: 'separator',
                            complete: true,
                            name: query.slice(i, result_regex[0].length),
                            position: position+i
                        });
                        size_stack++;
                        if (size_stack > this.max_size_stack) {
                            return null;
                        }
                        element = {
                            type: null,
                            context,
                            complete: false
                        };
                        start = (i+result_regex[0].length)-1;
                        to_skip = result_regex[0].length-1;
                        continue;

                    // Catch for anonymous function
                    } else if (element.type === 'anonymous_function') {
                        if (char in this.stop_char.opening) {
                            stack_stop_char.push(char);
                        } else if (char in this.stop_char.closing) {
                            if (stack_stop_char[stack_stop_char.length-1] === this.stop_char.closing[char]) {
                                stack_stop_char.pop();
                                if (stack_stop_char.length === 0) {
                                    element.body = this.extract_data_from_query({
                                        size_stack,
                                        query: query.slice(body_start, i),
                                        context: element.context,
                                        position: position+body_start
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
                                        context
                                    };
                                    start = i+1;
                                }
                            }
                        }
                            //else the written query is broken here. The user forgot to close something?
                            //TODO Default behavior? Wait for Brackets/Ace to see how we handle errors
                    } else if (element.type === 'loop') {
                        if (char in this.stop_char.opening) {
                            stack_stop_char.push(char);
                        } else if (char in this.stop_char.closing) {
                            if (stack_stop_char[stack_stop_char.length-1] === this.stop_char.closing[char]) {
                                stack_stop_char.pop();
                                if (stack_stop_char.length === 0) {
                                    element.body = this.extract_data_from_query({
                                        size_stack,
                                        query: query.slice(body_start, i),
                                        context: element.context,
                                        position: position+body_start
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
                                        context
                                    };
                                    start = i+1;
                                }
                            }
                        }
                    // Catch for function
                    } else if (element.type === 'function') {
                        if (char in this.stop_char.opening) {
                            stack_stop_char.push(char);
                        } else if (char in this.stop_char.closing) {
                            if (stack_stop_char[stack_stop_char.length-1] === this.stop_char.closing[char]) {
                                stack_stop_char.pop();
                                if (stack_stop_char.length === 0) {
                                    element.body = this.extract_data_from_query({
                                        size_stack,
                                        query: query.slice(start+element.name.length, i),
                                        context: element.context,
                                        position: position+start+element.name.length
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
                                        context
                                    };
                                    start = i+1;
                                }
                            }
                        }

                    // Catch for object
                    } else if (element.type === 'object') {
                        // Since we are sure that we are not in a string, we can just look for colon and comma
                        // Still, we need to check the stack_stop_char since we can have { key: { inner: 'test, 'other_inner'}, other_key: 'other_value'}
                        const keys_values = [];
                        if (char in this.stop_char.opening) {
                            stack_stop_char.push(char);
                        } else if (char in this.stop_char.closing) {
                            if (stack_stop_char[stack_stop_char.length-1] === this.stop_char.closing[char]) {
                                stack_stop_char.pop();
                                if (stack_stop_char.length === 0) {
                                    // We just reach a }, it's the end of the object
                                    if (element.next_key != null) {
                                        body = this.extract_data_from_query({
                                            size_stack,
                                            query: query.slice(element.current_value_start, i),
                                            context: element.context,
                                            position: position+element.current_value_start
                                        });
                                        if (body === null) {
                                            return null;
                                        }
                                        new_element = {
                                            type: 'object_key',
                                            key: element.next_key,
                                            key_complete: true,
                                            complete: false,
                                            body
                                        };
                                        element.body[element.body.length-1] = new_element;
                                    }
                                    element.next_key = null; // No more next_key
                                    element.complete = true;
                                    // if not element.next_key?
                                    // The next key is not defined, this is a broken query.
                                    // TODO show error once brackets/ace will be used

                                    stack.push(element);
                                    size_stack++;
                                    if (size_stack > this.max_size_stack) {
                                        return null;
                                    }
                                    element = {
                                        type: null,
                                        context
                                    };
                                    start = i+1;
                                    continue;
                                }
                            }
                        }

                        if (element.next_key == null) {
                            if ((stack_stop_char.length === 1) && (char === ':')) {
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
                                    element.body[element.body.length-1] = new_element;
                                }
                                element.next_key = query.slice(element.current_key_start, i);
                                element.current_value_start = i+1;
                            }
                        } else {
                            result_regex = this.regex.comma.exec(query.slice(i));
                            if ((stack_stop_char.length === 1) && (result_regex !== null)) { //We reached the end of a value
                                body = this.extract_data_from_query({
                                    size_stack,
                                    query: query.slice(element.current_value_start, i),
                                    context: element.context,
                                    position: element.current_value_start
                                });
                                if (body === null) {
                                    return null;
                                }
                                new_element = {
                                    type: 'object_key',
                                    key:  element.next_key,
                                    key_complete: true,
                                    body
                                };
                                element.body[element.body.length-1] = new_element;
                                to_skip = result_regex[0].length-1;
                                element.next_key = null;
                                element.current_key_start = i+result_regex[0].length;
                            }
                        }
                    // Catch for array
                    } else if (element.type === 'array') {
                        if (char in this.stop_char.opening) {
                            stack_stop_char.push(char);
                        } else if (char in this.stop_char.closing) {
                            if (stack_stop_char[stack_stop_char.length-1] === this.stop_char.closing[char]) {
                                stack_stop_char.pop();
                                if (stack_stop_char.length === 0) {
                                    // We just reach a ], it's the end of the object
                                    body = this.extract_data_from_query({
                                        size_stack,
                                        query: query.slice(entry_start, i),
                                        context: element.context,
                                        position: position+entry_start
                                    });
                                    if (body === null) {
                                        return null;
                                    }
                                    new_element = {
                                        type: 'array_entry',
                                        complete: true,
                                        body
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

                        if ((stack_stop_char.length === 1) && (char === ',')) {
                            body = this.extract_data_from_query({
                                size_stack,
                                query: query.slice(entry_start, i),
                                context: element.context,
                                position: position+entry_start
                            });
                            if (body === null) {
                                return null;
                            }
                            new_element = {
                                type: 'array_entry',
                                complete: true,
                                body
                            };
                            if (new_element.body.length > 0) {
                                element.body.push(new_element);
                                size_stack++;
                                if (size_stack > this.max_size_stack) {
                                    return null;
                                }
                            }
                            entry_start = i+1;
                        }
                    }
                }
            }
        }

        // We just reached the end, let's try to find the type of the incomplete element
        if (element.type !== null) {
            element.complete = false;
            if (element.type === 'function') {
                element.body = this.extract_data_from_query({
                    size_stack,
                    query: query.slice(start+element.name.length),
                    context: element.context,
                    position: position+start+element.name.length
                });
                if (element.body === null) {
                    return null;
                }
            } else if (element.type === 'anonymous_function') {
                element.body = this.extract_data_from_query({
                    size_stack,
                    query: query.slice(body_start),
                    context: element.context,
                    position: position+body_start
                });
                if (element.body === null) {
                    return null;
                }
            } else if (element.type === 'loop') {
                element.body = this.extract_data_from_query({
                    size_stack,
                    query: query.slice(body_start),
                    context: element.context,
                    position: position+body_start
                });
                if (element.body === null) {
                    return null;
                }
            } else if (element.type === 'string') {
                element.name = query.slice(start);
            } else if (element.type === 'object') {
                if (element.next_key == null) { // Key not defined yet
                    new_element = {
                        type: 'object_key',
                        key: query.slice(element.current_key_start),
                        key_complete: false,
                        complete: false
                    };
                    element.body.push(new_element); // They key was not defined, so we add a new element
                    size_stack++;
                    if (size_stack > this.max_size_stack) {
                        return null;
                    }
                    element.next_key = query.slice(element.current_key_start);
                } else {
                    body = this.extract_data_from_query({
                        size_stack,
                        query: query.slice(element.current_value_start),
                        context: element.context,
                        position: position+element.current_value_start
                    });
                    if (body === null) {
                        return null;
                    }
                    new_element = {
                        type: 'object_key',
                        key: element.next_key,
                        key_complete: true,
                        complete: false,
                        body
                    };
                    element.body[element.body.length-1] = new_element;
                    element.next_key = null; // No more next_key
                }
            } else if (element.type === 'array') {
                body = this.extract_data_from_query({
                    size_stack,
                    query: query.slice(entry_start),
                    context: element.context,
                    position: position+entry_start
                });
                if (body === null) {
                    return null;
                }
                new_element = {
                    type: 'array_entry',
                    complete: false,
                    body
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
                element.position = position+start;
                element.name = query.slice(start+1);
                element.complete = false;
            } else {
                element.name = query.slice(start);
                element.position = position+start;
                element.complete = false;
            }
            stack.push(element);
            size_stack++;
            if (size_stack > this.max_size_stack) {
                return null;
            }
        }
        return stack;
    }

    // Count the number of `group` commands minus `ungroup` commands in the current level
    // We count per level because we don't want to report a positive number of group for nested queries, e.g:
    // r.table("foo").group("bar").map(function(doc) { doc.merge(
    //
    // We return an object with two fields
    //   - count_group: number of `group` commands minus the number of `ungroup` commands
    //   - parse_level: should we keep parsing the same level
    count_group_level(stack) {
        let parse_level;
        let count_group = 0;
        if (stack.length > 0) {
             // Flag for whether or not we should keep looking for group/ungroup
             // we want the warning to appear only at the same level
            parse_level = true;

            const element = stack[stack.length-1];
            if ((element.body != null) && (element.body.length > 0) && (element.complete === false)) {
                const parse_body = this.count_group_level(element.body);
                count_group += parse_body.count_group;
                ({ parse_level } = parse_body);

                if (element.body[0].type === 'return') {
                    parse_level = false;
                }
                if (element.body[element.body.length-1].type === 'function') {
                    parse_level = false;
                }
            }

            if (parse_level === true) {
                for (let i = stack.length-1; i >= 0; i--) {
                    if ((stack[i].type === 'function') && (stack[i].name === 'ungroup(')) {
                        count_group -= 1;
                    } else if ((stack[i].type === 'function') && (stack[i].name === 'group(')) {
                        count_group += 1;
                    }
                }
            }
        }

        return {
            count_group,
            parse_level
        };
    }

    // Decide if we have to show a suggestion or a description
    // Mainly use the stack created by extract_data_from_query
    create_suggestion(args) {
        let suggestion;
        const { stack } = args;
        const { query } = args;
        const { result } = args;

        // No stack, ie an empty query
        if ((result.status === null) && (stack.length === 0)) {
            result.suggestions = [];
            result.status = 'done';
            this.query_first_part = '';
            if (this.suggestions[''] != null) { // The docs may not have loaded
                for (suggestion of Array.from(this.suggestions[''])) {
                    result.suggestions.push(suggestion);
                }
            }
        }

        return (() => {
            const result1 = [];
            for (let i = stack.length-1; i >= 0; i--) {
                let item;
                const element = stack[i];
                if ((element.body != null) && (element.body.length > 0) && (element.complete === false)) {
                    this.create_suggestion({
                        stack: element.body,
                        query: __guard__(args, x => x.query),
                        result: args.result
                    });
                }

                if (result.status === 'done') {
                    continue;
                }


                if (result.status === null) {
                    // Top of the stack
                    if (element.complete === true) {
                        if (element.type === 'function') {
                            if ((element.complete === true) || (element.name === '')) {
                                result.suggestions = null;
                                result.status = 'look_for_description';
                                break;
                            } else {
                                result.suggestions = null;
                                result.description = element.name;
                                //Define the current argument we have. It's the suggestion whose index is -1
                                this.extra_suggestion = {
                                    start_body: element.position + element.name.length,
                                    body: element.body
                                };
                                if (__guard__(__guard__(__guard__(element.body, x3 => x3[0]), x2 => x2.name), x1 => x1.length) != null) {
                                    this.cursor_for_auto_completion.ch -= element.body[0].name.length;
                                    this.current_extra_suggestion = element.body[0].name;
                                }
                                result.status = 'done';
                            }
                        } else if ((element.type === 'anonymous_function') || (element.type === 'separator') || (element.type === 'object') || (element.type === 'object_key') || (element.type === 'return') || ('element.type' === 'array')) {
                            // element.type === 'object' is impossible I think with the current implementation of extract_data_from_query
                            result.suggestions = null;
                            result.status = 'look_for_description';
                            break; // We want to look in the upper levels
                        }
                        //else type cannot be null (because not complete)
                    } else { // if element.complete is false
                        if (element.type === 'function') {
                            if (element.body != null) { // It means that element.body.length === 0
                                // We just opened a new function, so let's just show the description
                                result.suggestions = null;
                                result.description = element.name; // That means we are going to describe the function named element.name
                                this.extra_suggestion = {
                                    start_body: element.position + element.name.length,
                                    body: element.body
                                };
                                if (__guard__(__guard__(__guard__(element.body, x6 => x6[0]), x5 => x5.name), x4 => x4.length) != null) {
                                    this.cursor_for_auto_completion.ch -= element.body[0].name.length;
                                    this.current_extra_suggestion = element.body[0].name;
                                }
                                result.status = 'done';
                                break;
                            } else {
                                // function not complete, need suggestion
                                result.suggestions = [];
                                result.suggestions_regex = this.create_safe_regex(element.name); // That means we are going to give all the suggestions that match element.name and that are in the good group (not yet defined)
                                result.description = null;
                                this.query_first_part = query.slice(0, element.position+1);
                                this.current_element = element.name;
                                this.cursor_for_auto_completion.ch -= element.name.length;
                                this.current_query;
                                if (i !== 0) {
                                    result.status = 'look_for_state';
                                } else {
                                    result.state = '';
                                }
                            }
                        } else if ((element.type === 'anonymous_function') || (element.type === 'object_key') || (element.type === 'string') || (element.type === 'separator') || (element.type === 'array')) {
                            result.suggestions = null;
                            result.status = 'look_for_description';
                            break;
                        //else if element.type is 'object' # Not possible
                        //else if element.type is 'var' # Not possible because we require a . or ( to asssess that it's a var
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
                        if (__guard__(__guard__(__guard__(element.body, x9 => x9[0]), x8 => x8.name), x7 => x7.length) != null) {
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
                    if ((element.type === 'function') && (element.complete === false) && (element.name.indexOf('(') !== -1)) {
                        result.description = element.name;
                        this.extra_suggestion = {
                            start_body: element.position + element.name.length,
                            body: element.body
                        };
                        if (__guard__(__guard__(__guard__(element.body, x12 => x12[0]), x11 => x11.name), x10 => x10.length) != null) {
                            this.cursor_for_auto_completion.ch -= element.body[0].name.length;
                            this.current_extra_suggestion = element.body[0].name;
                        }
                        result.suggestions = null;
                        item = result.status = 'done';
                    } else {
                        if (element.type !== 'function') {
                            break;
                        } else {
                            result.status = 'look_for_description';
                            break;
                        }
                    }
                } else if (result.status === 'look_for_state') {
                    var state;
                    if ((element.type === 'function') && (element.complete === true)) {
                        result.state = element.name;
                        if (this.map_state[element.name] != null) {
                            for (state of Array.from(this.map_state[element.name])) {
                                if (this.suggestions[state] != null) {
                                    for (suggestion of Array.from(this.suggestions[state])) {
                                        if (result.suggestions_regex.test(suggestion) === true) {
                                            result.suggestions.push(suggestion);
                                        }
                                    }
                                }
                            }
                        }
                        //else # This is a non valid ReQL function.
                        // It may be a personalized function defined in the data explorer...
                        item = result.status = 'done';
                    } else if ((element.type === 'var') && (element.complete === true)) {
                        result.state = element.real_type;
                        for (let type of Array.from(result.state)) {
                            if (this.suggestions[type] != null) {
                                for (suggestion of Array.from(this.suggestions[type])) {
                                    if (result.suggestions_regex.test(suggestion) === true) {
                                        result.suggestions.push(suggestion);
                                    }
                                }
                            }
                        }
                        item = result.status = 'done';
                    }
                }
                result1.push(item);
            }
            return result1;
        })();
    }
                //else # Is that possible? A function can only be preceded by a function (except for r)

    // Create regex based on the user input. We make it safe
    create_safe_regex(str) {
        for (let char of Array.from(this.unsafe_to_safe_regexstr)) {
            str = str.replace(char[0], char[1]);
        }
        return new RegExp(`^(${str})`, 'i');
    }

    // Show suggestion and determine where to put the box
    show_suggestion() {
        this.move_suggestion();
        const margin = (parseInt(this.$('.CodeMirror-cursor').css('top').replace('px', ''))+this.line_height)+'px';
        this.$('.suggestion_full_container').css('margin-top', margin);
        this.$('.arrow').css('margin-top', margin);

        this.$('.suggestion_name_list').show();
        return this.$('.arrow').show();
    }

    // If want to show suggestion without moving the arrow
    show_suggestion_without_moving() {
        this.$('.arrow').show();
        return this.$('.suggestion_name_list').show();
    }

    // Show description and determine where to put it
    show_description() {
        if (this.descriptions[this.description] != null) { // Just for safety
            const margin = (parseInt(this.$('.CodeMirror-cursor').css('top').replace('px', ''))+this.line_height)+'px';

            this.$('.suggestion_full_container').css('margin-top', margin);
            this.$('.arrow').css('margin-top', margin);

            this.$('.suggestion_description').html(this.description_template(this.extend_description(this.description)));

            this.$('.suggestion_description').show();
            this.move_suggestion();
            return this.show_or_hide_arrow();
        } else {
            return this.hide_description();
        }
    }

    hide_suggestion() {
        this.$('.suggestion_name_list').hide();
        return this.show_or_hide_arrow();
    }
    hide_description() {
        this.$('.suggestion_description').hide();
        return this.show_or_hide_arrow();
    }
    hide_suggestion_and_description() {
        this.hide_suggestion();
        return this.hide_description();
    }

    // Show the arrow if suggestion or/and description is being displayed
    show_or_hide_arrow() {
        if ((this.$('.suggestion_name_list').css('display') === 'none') && (this.$('.suggestion_description').css('display') === 'none')) {
            return this.$('.arrow').hide();
        } else {
            return this.$('.arrow').show();
        }
    }

    // Move the suggestion. We have steps of 200 pixels and try not to overlaps button if we can. If we cannot, we just hide them all since their total width is less than 200 pixels
    move_suggestion() {
        const margin_left = parseInt(this.$('.CodeMirror-cursor').css('left').replace('px', ''))+23;
        this.$('.arrow').css('margin-left', margin_left);
        if (margin_left < 200) {
            return this.$('.suggestion_full_container').css('left', '0px');
        } else {
            const max_margin = this.$('.CodeMirror-scroll').width()-418;

            const margin_left_bloc = Math.min(max_margin, Math.floor(margin_left/200)*200);
            if (margin_left > ((max_margin+418)-150-23)) { // We are really at the end
                return this.$('.suggestion_full_container').css('left', (max_margin-34)+'px');
            } else if (margin_left_bloc > (max_margin-150-23)) {
                return this.$('.suggestion_full_container').css('left', (max_margin-34-150)+'px');
            } else {
                return this.$('.suggestion_full_container').css('left', (margin_left_bloc-100)+'px');
            }
        }
    }

    //Highlight suggestion. Method called when the user hit tab or mouseover
    highlight_suggestion(id) {
        this.remove_highlight_suggestion();
        this.$('.suggestion_name_li').eq(id).addClass('suggestion_name_li_hl');
        this.$('.suggestion_description').html(this.description_template(this.extend_description(this.current_suggestions[id])));

        return this.$('.suggestion_description').show();
    }

    remove_highlight_suggestion() {
        return this.$('.suggestion_name_li').removeClass('suggestion_name_li_hl');
    }

    // Write the suggestion in the code mirror
    write_suggestion(args) {
        const { suggestion_to_write } = args;
        const move_outside = args.move_outside === true; // So default value is false

        let ch = this.cursor_for_auto_completion.ch+suggestion_to_write.length;

        if (dataexplorer_state.options.electric_punctuation === true) {
            if ((suggestion_to_write[suggestion_to_write.length-1] === '(') && (this.count_not_closed_brackets('(') >= 0)) {
                this.codemirror.setValue(this.query_first_part+suggestion_to_write+')'+this.query_last_part);
                this.written_suggestion = suggestion_to_write+')';
            } else {
                this.codemirror.setValue(this.query_first_part+suggestion_to_write+this.query_last_part);
                this.written_suggestion = suggestion_to_write;
                if ((move_outside === false) && ((suggestion_to_write[suggestion_to_write.length-1] === '"') || (suggestion_to_write[suggestion_to_write.length-1] === "'"))) {
                    ch--;
                }
            }
            this.codemirror.focus(); // Useful if the user used the mouse to select a suggestion
            return this.codemirror.setCursor({
                line: this.cursor_for_auto_completion.line,
                ch
            });
        } else {
            this.codemirror.setValue(this.query_first_part+suggestion_to_write+this.query_last_part);
            this.written_suggestion = suggestion_to_write;
            if ((move_outside === false) && ((suggestion_to_write[suggestion_to_write.length-1] === '"') || (suggestion_to_write[suggestion_to_write.length-1] === "'"))) {
                ch--;
            }
            this.codemirror.focus(); // Useful if the user used the mouse to select a suggestion
            return this.codemirror.setCursor({
                line: this.cursor_for_auto_completion.line,
                ch
            });
        }
    }


    // Select the suggestion. Called by mousdown .suggestion_name_li
    select_suggestion(event) {
        const suggestion_to_write = this.$(event.target).html();
        this.write_suggestion({
            suggestion_to_write});

        // Give back focus to code mirror
        this.hide_suggestion();

        // Put back in the stack
        return setTimeout(() => {
            this.handle_keypress(); // That's going to describe the function the user just selected
            return this.codemirror.focus();
        } // Useful if the user used the mouse to select a suggestion
        , 0); // Useful if the user used the mouse to select a suggestion
    }

    // Highlight a suggestion in case of a mouseover
    mouseover_suggestion(event) {
        return this.highlight_suggestion(event.target.dataset.id);
    }

    // Hide suggestion in case of a mouse out
    mouseout_suggestion(event) {
        return this.hide_description();
    }

    // Extend description for .db() and .table() with dbs/tables names
    extend_description(fn) {
        let data, databases_available, description;
        if ((fn === 'db(') || (fn === 'dbDrop(')) {
            description = _.extend({}, this.descriptions[fn]());
            if (_.keys(this.databases_available).length === 0) {
                data =
                    {no_database: true};
            } else {
                databases_available = _.keys(this.databases_available);
                data = {
                    no_database: false,
                    databases_available
                };
            }
            description.description = this.databases_suggestions_template(data)+description.description;
            this.extra_suggestions= databases_available; // @extra_suggestions store the suggestions for arguments. So far they are just for db(), dbDrop(), table(), tableDrop()
        } else if ((fn === 'table(') || (fn === 'tableDrop(')) {
            // Look for the argument of the previous db()
            const database_used = this.extract_database_used();
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
                data =
                    {error: database_used.error};
            }

            description.description = this.tables_suggestions_template(data) + description.description;

            this.extra_suggestions = this.databases_available[database_used.name];
        } else {
            description = this.descriptions[fn](this.grouped_data);
            this.extra_suggestions= null;
        }
        return description;
    }

    // We could create a new stack with @extract_data_from_query, but that would be a more expensive for not that much
    // We can not use the previous stack too since autocompletion doesn't validate the query until you hit enter (or another key than tab)
    extract_database_used() {
        const query_before_cursor = this.codemirror.getRange({line: 0, ch: 0}, this.codemirror.getCursor());
        // We cannot have ".db(" in a db name
        const last_db_position = query_before_cursor.lastIndexOf('.db(');
        if (last_db_position === -1) {
            const found = false;
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
            const arg = query_before_cursor.slice(last_db_position+5); // +4 for .db(
            const char = query_before_cursor.slice(last_db_position+4, last_db_position+5); // ' or " used for the argument of db()
            const end_arg_position = arg.indexOf(char); // Check for the next quote or apostrophe
            if (end_arg_position === -1) {
                return {
                    db_found: false,
                    error: true
                };
            }
            const db_name = arg.slice(0, end_arg_position);
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
    }

    abort_query() {
        this.disable_toggle_executing = false;
        this.toggle_executing(false);
        __guard__(dataexplorer_state.query_result, x => x.force_end_gracefully());
        return this.driver_handler.close_connection();
    }

    // Function that execute the queries in a synchronous way.
    execute_query() {
        // We don't let people execute more than one query at a time on the same connection
        // While we remove the button run, `execute_query` could still be called with Shift+Enter
        if (this.executing === true) {
            this.abort_query;
        }

        // Hide the option, if already hidden, nothing happens.
        this.$('.profiler_enabled').slideUp('fast');

        // The user just executed a query, so we reset cursor_timed_out to false
        dataexplorer_state.cursor_timed_out = false;
        dataexplorer_state.query_has_changed = false;

        this.raw_query = this.codemirror.getSelection() || this.codemirror.getValue();

        this.query = this.clean_query(this.raw_query); // Save it because we'll use it in @callback_multilples_queries

        // Execute the query
        try {
            __guard__(dataexplorer_state.query_result, x => x.discard());
            // Separate queries
            this.non_rethinkdb_query = ''; // Store the statements that don't return a rethinkdb query (like "var a = 1;")
            this.index = 0; // index of the query currently being executed

            this.raw_queries = this.separate_queries(this.raw_query); // We first split raw_queries
            this.queries = this.separate_queries(this.query);

            if (this.queries.length === 0) {
                const error = this.query_error_template({
                    no_query: true});
                return this.results_view_wrapper.render_error(null, error, true);
            } else {
                return this.execute_portion();
            }

        } catch (err) {
            // Missing brackets, so we display everything (we don't know if we properly splitted the query)
            this.results_view_wrapper.render_error(this.query, err, true);
            return this.save_query({
                query: this.raw_query,
                broken_query: true
            });
        }
    }

    toggle_executing(executing) {
        if (executing === this.executing) {
            if (executing && __guard__(dataexplorer_state.query_result, x => x.is_feed)) {
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
            return this.timeout_toggle_abort = setTimeout(() => {
                this.timeout_toggle_abort = null;
                if (!__guard__(dataexplorer_state.query_result, x1 => x1.is_feed)) {
                    this.$('.loading_query_img').show();
                }
                this.$('.execute_query').hide();
                return this.$('.abort_query').show();
            }
            , this.delay_toggle_abort);
        } else {
            return this.timeout_toggle_abort = setTimeout(() => {
                this.timeout_toggle_abort = null;
                this.$('.loading_query_img').hide();
                this.$('.execute_query').show();
                return this.$('.abort_query').hide();
            }
            , this.delay_toggle_abort);
        }
    }

    // A portion is one query of the whole input.
    execute_portion() {
        dataexplorer_state.query_result = null;
        while (this.queries[this.index] != null) {
            var rdb_query;
            let full_query = this.non_rethinkdb_query;
            full_query += this.queries[this.index];

            try {
                rdb_query = this.evaluate(full_query);
            } catch (err) {
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
                var query_result;
                var final_query = this.index === this.queries.length;
                this.start_time = new Date();

                if (final_query) {
                    query_result = new QueryResult({
                        has_profile: dataexplorer_state.options.profiler,
                        current_query: this.raw_query,
                        raw_query: this.raw_queries[this.index],
                        driver_handler: this.driver_handler,
                        events: {
                            error: (query_result, err) => {
                                return this.results_view_wrapper.render_error(this.query, err);
                            },
                            ready: query_result => {
                                dataexplorer_state.pause_at = null;
                                if (query_result.is_feed) {
                                    this.toggle_executing(true);
                                    this.disable_toggle_executing = true;
                                    return ['end', 'discard', 'error'].map((event) =>
                                        query_result.on(event, () => {
                                            this.disable_toggle_executing = false;
                                            return this.toggle_executing(false);
                                        }
                                        ));
                                }
                            }
                        }
                    });

                    dataexplorer_state.query_result = query_result;

                    this.results_view_wrapper.set_query_result({
                        query_result: dataexplorer_state.query_result});
                }

                this.disable_toggle_executing = false;
                this.driver_handler.run_with_new_connection(rdb_query, {
                    optargs: {
                        binaryFormat: "raw",
                        timeFormat: "raw",
                        profile: dataexplorer_state.options.profiler
                    },
                    connection_error: error => {
                        this.save_query({
                            query: this.raw_query,
                            broken_query: true
                        });
                        return this.error_on_connect(error);
                    },
                    callback: (error, result) => {
                        if (final_query) {
                            this.save_query({
                                query: this.raw_query,
                                broken_query: false
                            });
                            return query_result.set(error, result);
                        } else if (error) {
                            this.save_query({
                                query: this.raw_query,
                                broken_query: true
                            });
                            return this.results_view_wrapper.render_error(this.query, err);
                        } else {
                            return this.execute_portion();
                        }
                    }
                }
                );

                return true;
            } else {
                this.non_rethinkdb_query += this.queries[this.index-1];
                if (this.index === this.queries.length) {
                    const error = this.query_error_template({
                        last_non_query: true});
                    this.results_view_wrapper.render_error(this.raw_queries[this.index-1], error, true);

                    this.save_query({
                        query: this.raw_query,
                        broken_query: true
                    });
                }
            }
        }
    }

    // Evaluate the query
    // We cannot force eval to a local scope, but "use strict" will declare variables in the scope at least
    evaluate(query) {
        "use strict";
        return eval(query);
    }

    // In a string \n becomes \\n, outside a string we just remove \n, so
    //   r
    //   .expr('hello
    //   world')
    // becomes
    //   r.expr('hello\nworld')
    //  We also remove comments from the query
    clean_query(query) {
        let i;
        let is_parsing_string = false;
        let start = 0;

        let result_query = '';
        for (i = 0; i < query.length; i++) {
            const char = query[i];
            if (to_skip > 0) {
                to_skip--;
                continue;
            }

            if (is_parsing_string === true) {
                if ((char === string_delimiter) && (query[i-1] != null) && (query[i-1] !== '\\')) {
                    result_query += query.slice(start, i+1).replace(/\n/g, '\\n');
                    start = i+1;
                    is_parsing_string = false;
                    continue;
                }
            } else { // if element.is_parsing_string is false
                var to_skip;
                if ((char === '\'') || (char === '"')) {
                    result_query += query.slice(start, i).replace(/\n/g, '');
                    start = i;
                    is_parsing_string = true;
                    var string_delimiter = char;
                    continue;
                }

                const result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
                if (result_inline_comment != null) {
                    result_query += query.slice(start, i).replace(/\n/g, '');
                    start = i;
                    to_skip = result_inline_comment[0].length-1;
                    start += result_inline_comment[0].length;
                    continue;
                }
                const result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
                if (result_multiple_line_comment != null) {
                    result_query += query.slice(start, i).replace(/\n/g, '');
                    start = i;
                    to_skip = result_multiple_line_comment[0].length-1;
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
    }

    // Split input in queries. We use semi colon, pay attention to string, brackets and comments
    separate_queries(query) {
        const queries = [];
        let is_parsing_string = false;
        const stack = [];
        let start = 0;

        const position = {
            char: 0,
            line: 1
        };

        for (let i = 0; i < query.length; i++) {
            const char = query[i];
            if (char === '\n') {
                position.line++;
                position.char = 0;
            } else {
                position.char++;
            }

            if (to_skip > 0) { // Because we cannot mess with the iterator in coffee-script
                to_skip--;
                continue;
            }

            if (is_parsing_string === true) {
                if ((char === string_delimiter) && (query[i-1] != null) && (query[i-1] !== '\\')) {
                    is_parsing_string = false;
                    continue;
                }
            } else { // if element.is_parsing_string is false
                var to_skip;
                if ((char === '\'') || (char === '"')) {
                    is_parsing_string = true;
                    var string_delimiter = char;
                    continue;
                }

                const result_inline_comment = this.regex.inline_comment.exec(query.slice(i));
                if (result_inline_comment != null) {
                    to_skip = result_inline_comment[0].length-1;
                    continue;
                }
                const result_multiple_line_comment = this.regex.multiple_line_comment.exec(query.slice(i));
                if (result_multiple_line_comment != null) {
                    to_skip = result_multiple_line_comment[0].length-1;
                    continue;
                }


                if (char in this.stop_char.opening) {
                    stack.push(char);
                } else if (char in this.stop_char.closing) {
                    if (stack[stack.length-1] !== this.stop_char.closing[char]) {
                        throw this.query_error_template({
                            syntax_error: true,
                            bracket: char,
                            line: position.line,
                            position: position.char
                        });
                    } else {
                        stack.pop();
                    }
                } else if ((char === ';') && (stack.length === 0)) {
                    queries.push(query.slice(start, i+1));
                    start = i+1;
                }
            }
        }

        if (start < (query.length-1)) {
            const last_query = query.slice(start);
            if (this.regex.white.test(last_query) === false) {
                queries.push(last_query);
            }
        }

        return queries;
    }

    // Clear the input
    clear_query() {
        this.codemirror.setValue('');
        return this.codemirror.focus();
    }

    // Called if there is any on the connection
    error_on_connect(error) {
        if (/^(Unexpected token)/.test(error.message)) {
            // Unexpected token, the server couldn't parse the message
            // The truth is we don't know which query failed (unexpected token), but it seems safe to suppose in 99% that the last one failed.
            this.results_view_wrapper.render_error(null, error);

            // We save the query since the callback will never be called.
            return this.save_query({
                query: this.raw_query,
                broken_query: true
            });

        } else {
            this.results_view_wrapper.cursor_timed_out();
            // We fail to connect, so we display a message except if we were already disconnected and we are not trying to manually reconnect
            // So if the user fails to reconnect after a failure, the alert will still flash
            this.$('#user-alert-space').hide();
            this.$('#user-alert-space').html(this.alert_connection_fail_template({}));
            return this.$('#user-alert-space').slideDown('fast');
        }
    }

    handle_gutter_click(editor, line) {
        const start = {
            line,
            ch: 0
        };
        const end = {
            line,
            ch: this.codemirror.getLine(line).length
        };
        return this.codemirror.setSelection(start, end);
    }

    // Switch between full view and normal view
    toggle_size() {
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
    }

    display_normal() {
        $('#cluster').addClass('container');
        $('#cluster').removeClass('cluster_with_margin');
        this.$('.wrapper_scrollbar').css('width', '888px');
        this.$('.option_icon').removeClass('fullscreen_exit');
        return this.$('.option_icon').addClass('fullscreen');
    }

    display_full() {
        $('#cluster').removeClass('container');
        $('#cluster').addClass('cluster_with_margin');
        this.$('.wrapper_scrollbar').css('width', ($(window).width()-92)+'px');
        this.$('.option_icon').removeClass('fullscreen');
        return this.$('.option_icon').addClass('fullscreen_exit');
    }

    remove() {
        this.results_view_wrapper.remove();
        this.history_view.remove();
        this.driver_handler.remove();

        this.display_normal();
        $(window).off('resize', this.display_full);
        $(document).unbind('mousemove', this.handle_mousemove);
        $(document).unbind('mouseup', this.handle_mouseup);

        clearTimeout(this.timeout_driver_connect);
        driver.stop_timer(this.timer);
        // We do not destroy the cursor, because the user might come back and use it.
        return super.remove();
    }
}
Container.initClass();


// An abstract base class
class ResultView extends Backbone.View {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.remove = this.remove.bind(this);
        this.json_to_tree = this.json_to_tree.bind(this);
        this.toggle_collapse = this.toggle_collapse.bind(this);
        this.expand_tree_in_table = this.expand_tree_in_table.bind(this);
        this.parent_pause_feed = this.parent_pause_feed.bind(this);
        this.pause_feed = this.pause_feed.bind(this);
        this.unpause_feed = this.unpause_feed.bind(this);
        this.current_batch = this.current_batch.bind(this);
        this.current_batch_size = this.current_batch_size.bind(this);
        this.setStackSize = this.setStackSize.bind(this);
        this.fetch_batch_rows = this.fetch_batch_rows.bind(this);
        this.show_next_batch = this.show_next_batch.bind(this);
        this.add_row = this.add_row.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.tree_large_container_template = require('../handlebars/dataexplorer_large_result_json_tree_container.hbs');
        this.prototype.tree_container_template = require('../handlebars/dataexplorer_result_json_tree_container.hbs');

        this.prototype.max_datum_threshold = 1000;
    }

    events() {
        return {
            'click .jt_arrow': 'toggle_collapse',
            'click .jta_arrow_h': 'expand_tree_in_table',
            'mousedown': 'parent_pause_feed'
        };
    }

    initialize(args) {
        this._patched_already = false;
        this.parent = args.parent;
        this.query_result = args.query_result;
        this.render();
        this.listenTo(this.query_result, 'end', () => {
            if (!this.query_result.is_feed) {
                return this.render();
            }
        }
        );
        return this.fetch_batch_rows();
    }

    remove() {
        this.removed_self = true;
        return super.remove();
    }

    // Return whether there are too many datums
    // If there are too many, we will disable syntax highlighting to avoid freezing the page
    has_too_many_datums(result) {
        if (this.has_too_many_datums_helper(result) > this.max_datum_threshold) {
            return true;
        }
        return false;
    }

    json_to_tree(result) {
        // If the results are too large, we just display the raw indented JSON to avoid freezing the interface
        if (this.has_too_many_datums(result)) {
            return this.tree_large_container_template({
                json_data: JSON.stringify(result, null, 4)});
        } else {
            return this.tree_container_template({
                tree: util.json_to_node(result)});
        }
    }

    // Return the number of datums if there are less than @max_datum_threshold
    // Or return a number greater than @max_datum_threshold
    has_too_many_datums_helper(result) {
        let count;
        if (Object.prototype.toString.call(result) === '[object Object]') {
            count = 0;
            for (let key in result) {
                count += this.has_too_many_datums_helper(result[key]);
                if (count > this.max_datum_threshold) {
                    return count;
                }
            }
            return count;
        } else if (Array.isArray(result)) {
            count = 0;
            for (let element of Array.from(result)) {
                count += this.has_too_many_datums_helper(element);
                if (count > this.max_datum_threshold) {
                    return count;
                }
            }
            return count;
        }

        return 1;
    }

    toggle_collapse(event) {
        this.$(event.target).nextAll('.jt_collapsible').toggleClass('jt_collapsed');
        this.$(event.target).nextAll('.jt_points').toggleClass('jt_points_collapsed');
        this.$(event.target).nextAll('.jt_b').toggleClass('jt_b_collapsed');
        this.$(event.target).toggleClass('jt_arrow_hidden');
        return this.parent.set_scrollbar();
    }

    expand_tree_in_table(event) {
        const dom_element = this.$(event.target).parent();
        this.$(event.target).remove();
        const data = dom_element.data('json_data');
        const result = this.json_to_tree(data);
        dom_element.html(result);
        let classname_to_change = dom_element.parent().attr('class').split(' ')[0];
        $(`.${classname_to_change}`).css('max-width', 'none');
        classname_to_change = dom_element.parent().parent().attr('class');
        $(`.${classname_to_change}`).css('max-width', 'none');
        dom_element.css('max-width', 'none');
        return this.parent.set_scrollbar();
    }

    parent_pause_feed(event) {
        return this.parent.pause_feed();
    }

    pause_feed() {
        if (this.parent.container.state.pause_at == null) {
            return this.parent.container.state.pause_at = this.query_result.size();
        }
    }

    unpause_feed() {
        if (this.parent.container.state.pause_at != null) {
            this.parent.container.state.pause_at = null;
            return this.render();
        }
    }

    current_batch() {
        switch (this.query_result.type) {
            case 'value':
                return this.query_result.value;
            case 'cursor':
                if (this.query_result.is_feed) {
                    let latest;
                    const { pause_at } = this.parent.container.state;
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
    }

    current_batch_size() {
        let left;
        return (left = __guard__(this.current_batch(), x => x.length)) != null ? left : 0;
    }

    setStackSize() {
        // In some versions of firefox, the effective recursion
        // limit gets hit sometimes by the driver. Here we patch
        // the driver's built in stackSize to 12 (normally it's
        // 100). The driver will invoke callbacks with setImmediate
        // vs directly invoking themif the stackSize limit is
        // exceeded, which keeps the stack size manageable (but
        // results in worse tracebacks).
        if (this._patched_already) {
            return;
        }
        const iterableProto = __guard__(__guard__(__guard__(__guard__(this.query_result.cursor, x3 => x3.__proto__), x2 => x2.__proto__), x1 => x1.constructor), x => x.prototype);
        if (__guard__(iterableProto, x4 => x4.stackSize) > 12) {
            console.log("Patching stack limit on cursors to 12");
            iterableProto.stackSize = 12;
            return this._patched_already = true;
        }
    }


    // TODO: rate limit events to avoid freezing the browser when there are too many
    fetch_batch_rows() {
        if (this.query_result.type === !'cursor') {
            return;
        }
        this.setStackSize();
        if (this.query_result.is_feed || (this.query_result.size() < (this.query_result.position + this.parent.container.state.options.query_limit))) {
            this.query_result.once('add', (query_result, row) => {
                if (this.removed_self) {
                    return;
                }
                if (this.query_result.is_feed) {
                    if (this.parent.container.state.pause_at == null) {
                        if (this.paused_at == null) {
                            this.query_result.drop_before(this.query_result.size() - this.parent.container.state.options.query_limit);
                        }
                        this.add_row(row);
                    }
                    this.parent.update_feed_metadata();
                }
                return this.fetch_batch_rows();
            }
            );
            return this.query_result.fetch_next();
        } else {
            this.parent.render();
            return this.render();
        }
    }

    show_next_batch() {
        this.query_result.position += this.parent.container.state.options.query_limit;
        this.query_result.drop_before(this.parent.container.state.options.query_limit);
        this.render();
        this.parent.render();
        return this.fetch_batch_rows();
    }

    add_row(row) {
        // TODO: Don't render the whole view on every change
        return this.render();
    }
}
ResultView.initClass();


class TreeView extends ResultView {
    constructor(...args) {
        this.render = this.render.bind(this);
        this.add_row = this.add_row.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.className = 'results tree_view_container';
        this.prototype.templates = {
            wrapper: require('../handlebars/dataexplorer_result_tree.hbs'),
            no_result: require('../handlebars/dataexplorer_result_empty.hbs')
        };
    }

    render() {
        if (__guard__(this.query_result.results, x => x.length) === 0) {
            this.$el.html(this.templates.wrapper({tree: this.templates.no_result({
                ended: this.query_result.ended,
                at_beginning: this.query_result.at_beginning()
            })
            })
            );
            return this;
        }
        switch (this.query_result.type) {
            case 'value':
                this.$el.html(this.templates.wrapper({tree: this.json_to_tree(this.query_result.value)}));
                break;
            case 'cursor':
                this.$el.html(this.templates.wrapper({tree: []}));
                const tree_container = this.$('.json_tree_container');
                for (let row of Array.from(this.current_batch())) {
                    tree_container.append(this.json_to_tree(row));
                }
                break;
        }
        return this;
    }

    add_row(row, noflash) {
        const tree_container = this.$('.json_tree_container');
        const node = $(this.json_to_tree(row)).prependTo(tree_container);
        if (!noflash) {
            node.addClass('flash');
        }
        const children = tree_container.children();
        if (children.length > this.parent.container.state.options.query_limit) {
            return children.last().remove();
        }
    }
}
TreeView.initClass();


class TableView extends ResultView {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.handle_mousedown = this.handle_mousedown.bind(this);
        this.handle_mousemove = this.handle_mousemove.bind(this);
        this.resize_column = this.resize_column.bind(this);
        this.handle_mouseup = this.handle_mouseup.bind(this);
        this.build_map_keys = this.build_map_keys.bind(this);
        this.compute_occurrence = this.compute_occurrence.bind(this);
        this.order_keys = this.order_keys.bind(this);
        this.get_all_attr = this.get_all_attr.bind(this);
        this.json_to_table_get_attr = this.json_to_table_get_attr.bind(this);
        this.json_to_table_get_values = this.json_to_table_get_values.bind(this);
        this.json_to_table_get_td_value = this.json_to_table_get_td_value.bind(this);
        this.compute_data_for_type = this.compute_data_for_type.bind(this);
        this.join_table = this.join_table.bind(this);
        this.json_to_table = this.json_to_table.bind(this);
        this.tag_record = this.tag_record.bind(this);
        this.render = this.render.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.className = 'results table_view_container';

        this.prototype.templates = {
            wrapper: require('../handlebars/dataexplorer_result_table.hbs'),
            container: require('../handlebars/dataexplorer_result_json_table_container.hbs'),
            tr_attr: require('../handlebars/dataexplorer_result_json_table_tr_attr.hbs'),
            td_attr: require('../handlebars/dataexplorer_result_json_table_td_attr.hbs'),
            tr_value: require('../handlebars/dataexplorer_result_json_table_tr_value.hbs'),
            td_value: require('../handlebars/dataexplorer_result_json_table_td_value.hbs'),
            td_value_content: require('../handlebars/dataexplorer_result_json_table_td_value_content.hbs'),
            data_inline: require('../handlebars/dataexplorer_result_json_table_data_inline.hbs'),
            no_result: require('../handlebars/dataexplorer_result_empty.hbs')
        };

        this.prototype.default_size_column = 310; // max-width value of a cell of a table (as defined in the css file)
        this.prototype.mouse_down = false;
    }

    events() { return _.extend(super.events(),
        {'mousedown .click_detector': 'handle_mousedown'}); }

    initialize(args) {
        super.initialize(args);
        this.last_keys = this.parent.container.state.last_keys; // Arrays of the last keys displayed
        this.last_columns_size = this.parent.container.state.last_columns_size; // Size of the columns displayed. Undefined if a column has the default size
        return this.listenTo(this.query_result, 'end', () => {
            if (this.current_batch_size() === 0) {
                return this.render();
            }
        }
        );
    }

    handle_mousedown(event) {
        if (__guard__(__guard__(event, x1 => x1.target), x => x.className) === 'click_detector') {
            this.col_resizing = this.$(event.target).parent().data('col');
            this.start_width = this.$(event.target).parent().width();
            this.start_x = event.pageX;
            this.mouse_down = true;
            return $('body').toggleClass('resizing', true);
        }
    }

    handle_mousemove(event) {
        if (this.mouse_down) {
            this.parent.container.state.last_columns_size[this.col_resizing] = Math.max(5, (this.start_width-this.start_x)+event.pageX); // Save the personalized size
            return this.resize_column(this.col_resizing, this.parent.container.state.last_columns_size[this.col_resizing]); // Resize
        }
    }

    resize_column(col, size) {
        this.$(`.col-${col}`).css('max-width', size);
        this.$(`.value-${col}`).css('max-width', size-20);
        this.$(`.col-${col}`).css('width', size);
        this.$(`.value-${col}`).css('width', size-20);
        if (size < 20) {
            this.$(`.value-${col}`).css('padding-left', (size-5)+'px');
            return this.$(`.value-${col}`).css('visibility', 'hidden');
        } else {
            this.$(`.value-${col}`).css('padding-left', '15px');
            return this.$(`.value-${col}`).css('visibility', 'visible');
        }
    }

    handle_mouseup(event) {
        if (this.mouse_down === true) {
            this.mouse_down = false;
            $('body').toggleClass('resizing', false);
            return this.parent.set_scrollbar();
        }
    }

    /*
    keys =
        primitive_value_count: <int>
        object:
            key_1: <keys>
            key_2: <keys>
    */
    build_map_keys(args) {
        const { keys_count } = args;
        const { result } = args;

        if (jQuery.isPlainObject(result)) {
            if (result.$reql_type$ === 'TIME') {
                return keys_count.primitive_value_count++;
            } else if (result.$reql_type$ === 'BINARY') {
                return keys_count.primitive_value_count++;
            } else {
                return (() => {
                    const result1 = [];
                    for (let key in result) {
                        const row = result[key];
                        if (keys_count['object'] == null) {
                            keys_count['object'] = {}; // That's define only if there are keys!
                        }
                        if (keys_count['object'][key] == null) {
                            keys_count['object'][key] =
                                {primitive_value_count: 0};
                        }
                        result1.push(this.build_map_keys({
                            keys_count: keys_count['object'][key],
                            result: row
                        }));
                    }
                    return result1;
                })();
            }
        } else {
            return keys_count.primitive_value_count++;
        }
    }

    // Compute occurrence of each key. The occurence can be a float since we compute the average occurence of all keys for an object
    compute_occurrence(keys_count) {
        if (keys_count['object'] == null) { // That means we are accessing only a primitive value
            return keys_count.occurrence = keys_count.primitive_value_count;
        } else {
            let count_key = keys_count.primitive_value_count > 0 ? 1 : 0;
            let count_occurrence = keys_count.primitive_value_count;
            for (let key in keys_count['object']) {
                const row = keys_count['object'][key];
                count_key++;
                this.compute_occurrence(row);
                count_occurrence += row.occurrence;
            }
            return keys_count.occurrence = count_occurrence/count_key; // count_key cannot be 0
        }
    }

    // Sort the keys per level
    order_keys(keys) {
        let key;
        const copy_keys = [];
        if (keys.object != null) {
            let value;
            for (key in keys.object) {
                value = keys.object[key];
                if (jQuery.isPlainObject(value)) {
                    this.order_keys(value);
                }

                copy_keys.push({
                    key,
                    value: value.occurrence
                });
            }
            // If we could know if a key is a primary key, that would be awesome
            copy_keys.sort(function(a, b) {
                if (b.value-a.value) {
                    return b.value-a.value;
                } else {
                    if (a.key > b.key) {
                        return 1;
                    } else { // We cannot have two times the same key
                        return -1;
                    }
                }
            });
        }
        keys.sorted_keys = _.map(copy_keys, d => d.key);
        if (keys.primitive_value_count > 0) {
            return keys.sorted_keys.unshift(this.primitive_key);
        }
    }

    // Flatten the object returns by build_map_keys().
    // We get back an array of keys
    get_all_attr(args) {
        const { keys_count } = args;
        const { attr } = args;
        const { prefix } = args;
        const { prefix_str } = args;
        return (() => {
            const result = [];
            for (let key of Array.from(keys_count.sorted_keys)) {
                if (key === this.primitive_key) {
                    let new_prefix_str = prefix_str; // prefix_str without the last dot
                    if (new_prefix_str.length > 0) {
                        new_prefix_str = new_prefix_str.slice(0, -1);
                    }
                    result.push(attr.push({
                        prefix,
                        prefix_str: new_prefix_str,
                        is_primitive: true
                    }));
                } else {
                    if (keys_count['object'][key]['object'] != null) {
                        const new_prefix = util.deep_copy(prefix);
                        new_prefix.push(key);
                        result.push(this.get_all_attr({
                            keys_count: keys_count.object[key],
                            attr,
                            prefix: new_prefix,
                            prefix_str: ((prefix_str != null) ? prefix_str : '')+key+'.'
                        }));
                    } else {
                        result.push(attr.push({
                            prefix,
                            prefix_str,
                            key
                        }));
                    }
                }
            }
            return result;
        })();
    }

    json_to_table_get_attr(flatten_attr) {
        return this.templates.tr_attr({
            attr: flatten_attr});
    }

    json_to_table_get_values(args) {
        const { result } = args;
        const { flatten_attr } = args;

        const document_list = [];
        for (let i = 0; i < result.length; i++) {
            const single_result = result[i];
            const new_document =
                {cells: []};
            for (let col = 0; col < flatten_attr.length; col++) {
                const attr_obj = flatten_attr[col];
                const { key } = attr_obj;
                let value = single_result;
                for (var prefix of Array.from(attr_obj.prefix)) {
                    value = __guard__(value, x => x[prefix]);
                }
                if (attr_obj.is_primitive !== true) {
                    if (value != null) {
                        value = value[key];
                    } else {
                        value = undefined;
                    }
                }
                new_document.cells.push(this.json_to_table_get_td_value(value, col));
            }
            const index = this.query_result.is_feed ? this.query_result.size() - i : i + 1;
            this.tag_record(new_document, index);
            document_list.push(new_document);
        }
        return this.templates.tr_value({
            document: document_list});
    }

    json_to_table_get_td_value(value, col) {
        const data = this.compute_data_for_type(value, col);

        return this.templates.td_value({
            col,
            cell_content: this.templates.td_value_content(data)
        });
    }

    compute_data_for_type(value,  col) {
        const data = {
            value,
            class_value: `value-${col}`
        };

        const value_type = typeof value;
        if (value === null) {
            data['value'] = 'null';
            data['classname'] = 'jta_null';
        } else if (value === undefined) {
            data['value'] = 'undefined';
            data['classname'] = 'jta_undefined';
        } else if ((value.constructor != null) && (value.constructor === Array)) {
            if (value.length === 0) {
                data['value'] = '[ ]';
                data['classname'] = 'empty array';
            } else {
                data['value'] = '[ ... ]';
                data['data_to_expand'] = JSON.stringify(value);
            }
        } else if ((Object.prototype.toString.call(value) === '[object Object]') && (value.$reql_type$ === 'TIME')) {
            data['value'] = util.date_to_string(value);
            data['classname'] = 'jta_date';
        } else if ((Object.prototype.toString.call(value) === '[object Object]') && (value.$reql_type$ === 'BINARY')) {
            data['value'] = util.binary_to_string(value);
            data['classname'] = 'jta_bin';
        } else if (Object.prototype.toString.call(value) === '[object Object]') {
            data['value'] = '{ ... }';
            data['is_object'] = true;
        } else if (value_type === 'number') {
            data['classname'] = 'jta_num';
        } else if (value_type === 'string') {
            if (/^(http|https):\/\/[^\s]+$/i.test(value)) {
                data['classname'] = 'jta_url';
            } else if (/^[a-z0-9]+@[a-z0-9]+.[a-z0-9]{2,4}/i.test(value)) { // We don't handle .museum extension and special characters
                data['classname'] = 'jta_email';
            } else {
                data['classname'] = 'jta_string';
            }
        } else if (value_type === 'boolean') {
            data['classname'] = 'jta_bool';
            data.value = value === true ? 'true' : 'false';
        }

        return data;
    }

    // Helper for expanding a table when showing an object (creating new columns)
    join_table(data) {
        let result = '';
        for (let i = 0; i < data.length; i++) {
            const value = data[i];
            const data_cell = this.compute_data_for_type(value, 'float');
            data_cell['is_inline'] = true;
            if (i !== (data.length-1)) {
                data_cell['need_comma'] = true;
            }

            result += this.templates.data_inline(data_cell);
        }

        return result;
    }

    // Build the table
    // We order by the most frequent keys then by alphabetic order
    json_to_table(result) {
        // While an Array type is never returned by the driver, we still build an Array in the data explorer
        // when a cursor is returned (since we just print @limit results)
        if ((result.constructor == null) || (result.constructor !== Array)) {
            result = [result];
        }

        const keys_count =
            {primitive_value_count: 0};

        for (let result_entry of Array.from(result)) {
            this.build_map_keys({
                keys_count,
                result: result_entry
            });
        }
        this.compute_occurrence(keys_count);

        this.order_keys(keys_count);

        const flatten_attr = [];

        this.get_all_attr({ // fill attr[]
            keys_count,
            attr: flatten_attr,
            prefix: [],
            prefix_str: ''
        });
        for (let index = 0; index < flatten_attr.length; index++) {
            const value = flatten_attr[index];
            value.col = index;
        }

        this.last_keys = flatten_attr.map(function(attr, i) {
            if (attr.prefix_str !== '') {
                return attr.prefix_str+attr.key;
            }
            return attr.key;
        });
        this.parent.container.state.last_keys = this.last_keys;


        return this.templates.container({
            table_attr: this.json_to_table_get_attr(flatten_attr),
            table_data: this.json_to_table_get_values({
                result,
                flatten_attr
            })
        });
    }

    tag_record(doc, i) {
        return doc.record = this.query_result.position + i;
    }

    render() {
        let col, index, same_keys;
        const previous_keys = this.parent.container.state.last_keys; // Save previous keys. @last_keys will be updated in @json_to_table
        const results = this.current_batch();
        if (Object.prototype.toString.call(results) === '[object Array]') {
            if (results.length === 0) {
                this.$el.html(this.templates.wrapper({content: this.templates.no_result({
                    ended: this.query_result.ended,
                    at_beginning: this.query_result.at_beginning()
                })
                })
                );
            } else {
                this.$el.html(this.templates.wrapper({content: this.json_to_table(results)}));
            }
        } else {
            if (results === undefined) {
                this.$el.html('');
            } else {
                this.$el.html(this.templates.wrapper({content: this.json_to_table([results])}));
            }
        }

        if (this.query_result.is_feed) {
            // TODO: highlight all new rows, not just the latest one
            const first_row = this.$('.jta_tr').eq(1).find('td:not(:first)');
            first_row.css({'background-color': '#eeeeff'});
            first_row.animate({'background-color': '#fbfbfb'});
        }

        // Check if the keys are the same
        if (this.parent.container.state.last_keys.length !== previous_keys.length) {
            same_keys = false;
        } else {
            same_keys = true;
            for (index = 0; index < this.parent.container.state.last_keys.length; index++) {
                const keys = this.parent.container.state.last_keys[index];
                if (this.parent.container.state.last_keys[index] !== previous_keys[index]) {
                    same_keys = false;
                }
            }
        }

        // TODO we should just check if previous_keys is included in last_keys
        // If the keys are the same, we are going to resize the columns as they were before
        if (same_keys === true) {
            for (col in this.parent.container.state.last_columns_size) {
                const value = this.parent.container.state.last_columns_size[col];
                this.resize_column(col, value);
            }
        } else {
            // Reinitialize @last_columns_size
            this.last_column_size = {};
        }

        // Let's try to expand as much as we can
        let extra_size_table = this.$('.json_table_container').width()-this.$('.json_table').width();
        if (extra_size_table > 0) { // The table doesn't take the full width
            let expandable_columns = [];
            for (index = 0, end = this.last_keys.length-1, asc = 0 <= end; asc ? index <= end : index >= end; asc ? index++ : index--) { // We skip the column record
                var asc, end;
                var real_size = 0;
                this.$(`.col-${index}`).children().children().children().each(function(i, bloc) {
                    const $bloc = $(bloc);
                    if (real_size<$bloc.width()) {
                        return real_size = $bloc.width();
                    }
                });
                if ((real_size != null) && (real_size === real_size) && (real_size > this.default_size_column)) {
                    expandable_columns.push({
                        col: index,
                        size: real_size+20
                    }); // 20 for padding
                }
            }
            while (expandable_columns.length > 0) {
                expandable_columns.sort((a, b) => a.size-b.size);
                if ((expandable_columns[0].size-this.$(`.col-${expandable_columns[0].col}`).width()) < (extra_size_table/expandable_columns.length)) {
                    extra_size_table = extra_size_table-(expandable_columns[0]['size']-this.$(`.col-${expandable_columns[0].col}`).width());

                    this.$(`.col-${expandable_columns[0]['col']}`).css('max-width', expandable_columns[0]['size']);
                    this.$(`.value-${expandable_columns[0]['col']}`).css('max-width', expandable_columns[0]['size']-20);
                    expandable_columns.shift();
                } else {
                    const max_size = extra_size_table/expandable_columns.length;
                    for (let column of Array.from(expandable_columns)) {
                        const current_size = this.$(`.col-${expandable_columns[0].col}`).width();
                        this.$(`.col-${expandable_columns[0]['col']}`).css('max-width', current_size+max_size);
                        this.$(`.value-${expandable_columns[0]['col']}`).css('max-width', (current_size+max_size)-20);
                    }
                    expandable_columns = [];
                }
            }
        }

        return this;
    }
}
TableView.initClass();


class RawView extends ResultView {
    constructor(...args) {
        this.init_after_dom_rendered = this.init_after_dom_rendered.bind(this);
        this.adjust_height = this.adjust_height.bind(this);
        this.render = this.render.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.className = 'results raw_view_container';

        this.prototype.template = require('../handlebars/dataexplorer_result_raw.hbs');
    }

    init_after_dom_rendered() {
        return this.adjust_height();
    }

    adjust_height() {
        const height = this.$('.raw_view_textarea')[0].scrollHeight;
        if (height > 0) {
            return this.$('.raw_view_textarea').height(height);
        }
    }

    render() {
        this.$el.html(this.template(JSON.stringify(this.current_batch())));
        this.adjust_height();
        return this;
    }
}
RawView.initClass();


class ProfileView extends ResultView {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.render = this.render.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.className = 'results profile_view_container';

        this.prototype.template =
            require('../handlebars/dataexplorer_result_profile.hbs');
    }


    initialize(args) {
        ZeroClipboard.setDefaults({
            moviePath: 'js/ZeroClipboard.swf',
            forceHandCursor: true
        }); //TODO Find a fix for chromium(/linux?)
        this.clip = new ZeroClipboard();
        return super.initialize(args);
    }

    compute_total_duration(profile) {
        return profile.reduce(((total, task) => total + (task['duration(ms)'] || task['mean_duration(ms)'])) ,0);
    }

    compute_num_shard_accesses(profile) {
        let num_shard_accesses = 0;
        for (let task of Array.from(profile)) {
            if (task['description'] === 'Perform read on shard.') {
                num_shard_accesses += 1;
            }
            if (Object.prototype.toString.call(task['sub_tasks']) === '[object Array]') {
                num_shard_accesses += this.compute_num_shard_accesses(task['sub_tasks']);
            }
            if (Object.prototype.toString.call(task['parallel_tasks']) === '[object Array]') {
                num_shard_accesses += this.compute_num_shard_accesses(task['parallel_tasks']);
            }

            // In parallel tasks, we get arrays of tasks instead of a super task
            if (Object.prototype.toString.call(task) === '[object Array]') {
                num_shard_accesses += this.compute_num_shard_accesses(task);
            }
        }

        return num_shard_accesses;
    }

    render() {
        if (this.query_result.profile == null) {
            this.$el.html(this.template({}));
        } else {
            const { profile } = this.query_result;
            this.$el.html(this.template({
                profile: {
                    clipboard_text: JSON.stringify(profile, null, 2),
                    tree: this.json_to_tree(profile),
                    total_duration: util.prettify_duration(this.parent.container.driver_handler.total_duration),
                    server_duration: util.prettify_duration(this.compute_total_duration(profile)),
                    num_shard_accesses: this.compute_num_shard_accesses(profile)
                }
            })
            );

            this.clip.glue(this.$('button.copy_profile'));
            this.delegateEvents();
        }
        return this;
    }
}
ProfileView.initClass();

class ResultViewWrapper extends Backbone.View {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.handle_scroll = this.handle_scroll.bind(this);
        this.pause_feed = this.pause_feed.bind(this);
        this.unpause_feed = this.unpause_feed.bind(this);
        this.show_tree = this.show_tree.bind(this);
        this.show_profile = this.show_profile.bind(this);
        this.show_table = this.show_table.bind(this);
        this.show_raw = this.show_raw.bind(this);
        this.set_view = this.set_view.bind(this);
        this.set_scrollbar = this.set_scrollbar.bind(this);
        this.activate_profiler = this.activate_profiler.bind(this);
        this.render_error = this.render_error.bind(this);
        this.set_query_result = this.set_query_result.bind(this);
        this.render = this.render.bind(this);
        this.update_feed_metadata = this.update_feed_metadata.bind(this);
        this.feed_info = this.feed_info.bind(this);
        this.new_view = this.new_view.bind(this);
        this.init_after_dom_rendered = this.init_after_dom_rendered.bind(this);
        this.cursor_timed_out = this.cursor_timed_out.bind(this);
        this.remove = this.remove.bind(this);
        this.handle_mouseup = this.handle_mouseup.bind(this);
        this.handle_mousemove = this.handle_mousemove.bind(this);
        this.show_next_batch = this.show_next_batch.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.className = 'result_view';
        this.prototype.template = require('../handlebars/dataexplorer_result_container.hbs');
        this.prototype.option_template = require('../handlebars/dataexplorer-option_page.hbs');
        this.prototype.error_template = require('../handlebars/dataexplorer-error.hbs');
        this.prototype.cursor_timed_out_template = require('../handlebars/dataexplorer-cursor_timed_out.hbs');
        this.prototype.primitive_key = '_-primitive value-_--'; // We suppose that there is no key with such value in the database.

        this.prototype.views = {
            tree: TreeView,
            table: TableView,
            profile: ProfileView,
            raw: RawView
        };
    }

    events() {
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
    }

    initialize(args) {
        this.container = args.container;
        this.view = args.view;
        this.view_object = null;
        this.scroll_handler = () => this.handle_scroll();
        this.floating_metadata = false;
        $(window).on('scroll', this.scroll_handler);
        return this.handle_scroll();
    }

    handle_scroll() {
        const scroll = $(window).scrollTop();
        const pos = __guard__(this.$('.results_header').offset(), x => x.top) + 2;
        if (pos == null) {
            return;
        }
        if (this.floating_metadata && (pos > scroll)) {
            this.floating_metadata = false;
            this.$('.metadata').removeClass('floating_metadata');
            if (this.container.state.pause_at != null) {
                this.unpause_feed('automatic');
            }
        }
        if (!this.floating_metadata && (pos < scroll)) {
            this.floating_metadata = true;
            this.$('.metadata').addClass('floating_metadata');
            if (this.container.state.pause_at == null) {
                return this.pause_feed('automatic');
            }
        }
    }

    pause_feed(event) {
       if (event === 'automatic') {
            this.auto_unpause = true;
        } else {
            this.auto_unpause = false;
            __guard__(event, x => x.preventDefault());
        }
       __guard__(this.view_object, x1 => x1.pause_feed());
       return this.$('.metadata').addClass('feed_paused').removeClass('feed_unpaused');
   }

    unpause_feed(event) {
        if (event === 'automatic') {
            if (!this.auto_unpause) {
                return;
            }
        } else {
            event.preventDefault();
        }
        __guard__(this.view_object, x => x.unpause_feed());
        return this.$('.metadata').removeClass('feed_paused').addClass('feed_unpaused');
    }

    show_tree(event) {
        event.preventDefault();
        return this.set_view('tree');
    }
    show_profile(event) {
        event.preventDefault();
        return this.set_view('profile');
    }
    show_table(event) {
        event.preventDefault();
        return this.set_view('table');
    }
    show_raw(event) {
        event.preventDefault();
        return this.set_view('raw');
    }

    set_view(view) {
        this.view = view;
        this.container.state.view = view;
        this.$(`.link_to_${this.view}_view`).parent().addClass('active');
        this.$(`.link_to_${this.view}_view`).parent().siblings().removeClass('active');
        if (__guard__(this.query_result, x => x.ready)) {
            return this.new_view();
        }
    }

    // TODO: The scrollbar sometime shows up when it is not needed
    set_scrollbar() {
        let content_container, content_name;
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
            // There is no scrolbar with the raw view
            return;
        }

        // Set the floating scrollbar
        const width_value = this.$(content_name).innerWidth(); // Include padding
        if (width_value < this.$(content_container).width()) {
            // If there is no need for scrollbar, we hide the one on the top
            this.$('.wrapper_scrollbar').hide();
            if (this.set_scrollbar_scroll_handler != null) {
                return $(window).unbind('scroll', this.set_scrollbar_scroll_handler);
            }
        } else {
            // Else we set the fake_content to the same width as the table that contains data and links the two scrollbars
            this.$('.wrapper_scrollbar').show();
            this.$('.scrollbar_fake_content').width(width_value);

            $(".wrapper_scrollbar").scroll(() => $(content_container).scrollLeft($(".wrapper_scrollbar").scrollLeft()));
            $(content_container).scroll(() => $(".wrapper_scrollbar").scrollLeft($(content_container).scrollLeft()));

            const position_scrollbar = function() {
                if ($(content_container).offset() != null) {
                    // Sometimes we don't have to display the scrollbar (when the results are not shown because the query is too big)
                    if (($(window).scrollTop()+$(window).height()) < ($(content_container).offset().top+20)) { // bottom of the window < beginning of $('.json_table_container') // 20 pixels is the approximate height of the scrollbar (so we don't show JUST the scrollbar)
                        return that.$('.wrapper_scrollbar').hide();
                    // We show the scrollbar and stick it to the bottom of the window because there ismore content below
                    } else if (($(window).scrollTop()+$(window).height()) < ($(content_container).offset().top+$(content_container).height())) { // bottom of the window < end of $('.json_table_container')
                        that.$('.wrapper_scrollbar').show();
                        that.$('.wrapper_scrollbar').css('overflow', 'auto');
                        return that.$('.wrapper_scrollbar').css('margin-bottom', '0px');
                    // And sometimes we "hide" it
                    } else {
                        // We can not hide .wrapper_scrollbar because it would break the binding between wrapper_scrollbar and content_container
                        return that.$('.wrapper_scrollbar').css('overflow', 'hidden');
                    }
                }
            };

            var that = this;
            position_scrollbar();
            this.set_scrollbar_scroll_handler = position_scrollbar;
            $(window).scroll(this.set_scrollbar_scroll_handler);
            return $(window).resize(() => position_scrollbar());
        }
    }

    activate_profiler(event) {
        event.preventDefault();
        if (this.container.options_view.state === 'hidden') {
            return this.container.toggle_options({
                cb: () => {
                    return setTimeout( () => {
                        if (this.container.state.options.profiler === false) {
                            this.container.options_view.$('.option_description[data-option="profiler"]').click();
                        }
                        this.container.options_view.$('.profiler_enabled').show();
                        return this.container.options_view.$('.profiler_enabled').css('visibility', 'visible');
                    }
                    , 100);
                }
            });
        } else {
            if (this.container.state.options.profiler === false) {
                this.container.options_view.$('.option_description[data-option="profiler"]').click();
            }
            this.container.options_view.$('.profiler_enabled').hide();
            this.container.options_view.$('.profiler_enabled').css('visibility', 'visible');
            return this.container.options_view.$('.profiler_enabled').slideDown('fast');
        }
    }

    render_error(query, err, js_error) {
        __guard__(this.view_object, x => x.remove());
        this.view_object = null;
        __guard__(this.query_result, x1 => x1.discard());
        this.$el.html(this.error_template({
            query,
            error: err.toString().replace(/^(\s*)/, ''),
            js_error: js_error === true
        })
        );
        return this;
    }

    set_query_result({query_result}) {
        __guard__(this.query_result, x => x.discard());
        this.query_result = query_result;
        if (query_result.ready) {
            this.render();
            this.new_view();
        } else {
            this.query_result.on('ready', () => {
                this.render();
                return this.new_view();
            }
            );
        }
        return this.query_result.on('end', () => {
            return this.render();
        }
        );
    }

    render(args) {
        if (__guard__(this.query_result, x => x.ready)) {
            __guard__(this.view_object, x1 => x1.$el.detach());
            const has_more_data = !this.query_result.ended && ((this.query_result.position + this.container.state.options.query_limit) <= this.query_result.size());
            const batch_size = __guard__(this.view_object, x2 => x2.current_batch_size());
            this.$el.html(this.template({
                range_begin: this.query_result.position + 1,
                range_end: batch_size && (this.query_result.position + batch_size),
                query_has_changed: __guard__(args, x3 => x3.query_has_changed),
                show_more_data: has_more_data && !this.container.state.cursor_timed_out,
                cursor_timed_out_template: (
                    !this.query_result.ended && this.container.state.cursor_timed_out ? this.cursor_timed_out_template() : undefined),
                execution_time_pretty: util.prettify_duration(this.query_result.server_duration),
                no_results: this.query_result.ended && (this.query_result.size() === 0),
                num_results: this.query_result.size(),
                floating_metadata: this.floating_metadata,
                feed: this.feed_info()
            })
            );
            this.$('.execution_time').tooltip({
                for_dataexplorer: true,
                trigger: 'hover',

                placement: 'bottom'
            });
            this.$('.tab-content').html(__guard__(this.view_object, x4 => x4.$el));
            this.$(`.link_to_${this.view}_view`).parent().addClass('active');
        }
        return this;
    }

    update_feed_metadata() {
        const info = this.feed_info();
        if (info == null) {
            return;
        }
        $('.feed_upcoming').text(info.upcoming);
        return $('.feed_overflow').parent().toggleClass('hidden', !info.overflow);
    }

    feed_info() {
        if (this.query_result.is_feed) {
            const total = this.container.state.pause_at != null ? this.container.state.pause_at : this.query_result.size();
            return {
                ended: this.query_result.ended,
                overflow: this.container.state.options.query_limit < total,
                paused: (this.container.state.pause_at != null),
                upcoming: this.query_result.size() - total
            };
        }
    }

    new_view() {
        __guard__(this.view_object, x => x.remove());
        this.view_object = new this.views[this.view]({
            parent: this,
            query_result: this.query_result
        });
        this.$('.tab-content').html(this.view_object.render().$el);
        this.init_after_dom_rendered();
        return this.set_scrollbar();
    }

    init_after_dom_rendered() {
        return __guardMethod__(this.view_object, 'init_after_dom_rendered', o => o.init_after_dom_rendered());
    }

    // Check if the cursor timed out. If yes, make sure that the user cannot fetch more results
    cursor_timed_out() {
        this.container.state.cursor_timed_out = true;
        if (__guard__(this.container.state.query_result, x => x.ended) === true) {
            return this.$('.more_results_paragraph').html(this.cursor_timed_out_template());
        }
    }

    remove() {
        __guard__(this.view_object, x => x.remove());
        if (__guard__(this.query_result, x1 => x1.is_feed)) {
            this.query_result.force_end_gracefully();
        }
        if (this.set_scrollbar_scroll_handler != null) {
            $(window).unbind('scroll', this.set_scrollbar_scroll_handler);
        }
        $(window).unbind('resize');
        $(window).off('scroll', this.scroll_handler);
        return super.remove();
    }

    handle_mouseup(event) {
        return __guardMethod__(this.view_object, 'handle_mouseup', o => o.handle_mouseup(event));
    }

    handle_mousemove(event) {
        return __guardMethod__(this.view_object, 'handle_mousedown', o => o.handle_mousedown(event));
    }

    show_next_batch(event) {
        event.preventDefault();
        $(window).scrollTop($('.results_container').offset().top);
        return __guard__(this.view_object, x => x.show_next_batch());
    }
}
ResultViewWrapper.initClass();

class OptionsView extends Backbone.View {
    constructor(...args) {
        this.initialize = this.initialize.bind(this);
        this.toggle_option = this.toggle_option.bind(this);
        this.change_query_limit = this.change_query_limit.bind(this);
        this.render = this.render.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.dataexplorer_options_template = require('../handlebars/dataexplorer-options.hbs');
        this.prototype.className = 'options_view';

        this.prototype.events = {
            'click li:not(.text-input)': 'toggle_option',
            'change #query_limit': 'change_query_limit'
        };
    }

    initialize(args) {
        this.container = args.container;
        this.options = args.options;
        return this.state = 'hidden';
    }

    toggle_option(event) {
        event.preventDefault();
        const new_target = this.$(event.target).data('option');
        this.$(`#${new_target}`).prop('checked', !this.options[new_target]);
        if (event.target.nodeName !== 'INPUT') { // Label we catch if for us
            const new_value = this.$(`#${new_target}`).is(':checked');
            this.options[new_target] = new_value;
            if (window.localStorage != null) {
                window.localStorage.options = JSON.stringify(this.options);
            }
            if ((new_target === 'profiler') && (new_value === false)) {
                return this.$('.profiler_enabled').slideUp('fast');
            }
        }
    }

    change_query_limit(event) {
        const query_limit = parseInt(this.$("#query_limit").val(), 10) || DEFAULTS.options.query_limit;
        this.options['query_limit'] = Math.max(query_limit, 1);
        if (window.localStorage != null) {
            window.localStorage.options = JSON.stringify(this.options);
        }
        return this.$('#query_limit').val(this.options['query_limit']); // In case the input is reset to 40
    }

    render(displayed) {
        this.$el.html(this.dataexplorer_options_template(this.options));
        if (displayed === true) {
            this.$el.show();
        }
        this.delegateEvents();
        return this;
    }
}
OptionsView.initClass();

class HistoryView extends Backbone.View {
    constructor(...args) {
        this.start_resize = this.start_resize.bind(this);
        this.handle_mousemove = this.handle_mousemove.bind(this);
        this.handle_mouseup = this.handle_mouseup.bind(this);
        this.initialize = this.initialize.bind(this);
        this.render = this.render.bind(this);
        this.load_query = this.load_query.bind(this);
        this.delete_query = this.delete_query.bind(this);
        this.add_query = this.add_query.bind(this);
        this.clear_history = this.clear_history.bind(this);
        super(...args);
    }

    static initClass() {
        this.prototype.dataexplorer_history_template = require('../handlebars/dataexplorer-history.hbs');
        this.prototype.dataexplorer_query_li_template = require('../handlebars/dataexplorer-query_li.hbs');
        this.prototype.className = 'history_container';

        this.prototype.size_history_displayed = 300;
        this.prototype.state = 'hidden'; // hidden, visible
        this.prototype.index_displayed = 0;

        this.prototype.events = {
            'click .load_query': 'load_query',
            'click .delete_query': 'delete_query'
        };
    }

    start_resize(event) {
        this.start_y = event.pageY;
        this.start_height = this.container.$('.nano').height();
        this.mouse_down = true;
        return $('body').toggleClass('resizing', true);
    }

    handle_mousemove(event) {
        if (this.mouse_down === true) {
            this.height_history = Math.max(0, (this.start_height-this.start_y)+event.pageY);
            return this.container.$('.nano').height(this.height_history);
        }
    }

    handle_mouseup(event) {
        if (this.mouse_down === true) {
            this.mouse_down = false;
            $('.nano').nanoScroller({preventPageScrolling: true});
            return $('body').toggleClass('resizing', false);
        }
    }

    initialize(args) {
        this.container = args.container;
        this.history = args.history;
        return this.height_history = 204;
    }

    render(displayed) {
        this.$el.html(this.dataexplorer_history_template());
        if (displayed === true) {
            this.$el.show();
        }
        if (this.history.length === 0) {
            this.$('.history_list').append(this.dataexplorer_query_li_template({
                no_query: true,
                displayed_class: 'displayed'
            })
            );
        } else {
            for (let i = 0; i < this.history.length; i++) {
                const query = this.history[i];
                this.$('.history_list').append(this.dataexplorer_query_li_template({
                    query: query.query,
                    broken_query: query.broken_query,
                    id: i,
                    num: i+1
                })
                );
            }
        }
        this.delegateEvents();
        return this;
    }

    load_query(event) {
        const { id } = this.$(event.target).data();
        // Set + save codemirror
        this.container.codemirror.setValue(this.history[parseInt(id)].query);
        this.container.state.current_query = this.history[parseInt(id)].query;
        return this.container.save_current_query();
    }

    delete_query(event) {
        const that = this;

        // Remove the query and overwrite localStorage.rethinkdb_history
        const id = parseInt(this.$(event.target).data().id);
        this.history.splice(id, 1);
        window.localStorage.rethinkdb_history = JSON.stringify(this.history);

        // Animate the deletion
        const is_at_bottom = this.$('.history_list').height() === ($('.nano > .content').scrollTop()+$('.nano').height());
        return this.$(`#query_history_${id}`).slideUp('fast', () => {
            that.$(this).remove();
            that.render();
            return that.container.adjust_collapsible_panel_height({
                is_at_bottom});
        }
        );
    }


    add_query(args) {
        const { query } = args;
        const { broken_query } = args;

        const that = this;
        const is_at_bottom = this.$('.history_list').height() === ($('.nano > .content').scrollTop()+$('.nano').height());

        this.$('.history_list').append(this.dataexplorer_query_li_template({
            query,
            broken_query,
            id: this.history.length-1,
            num: this.history.length
        })
        );

        if (this.$('.no_history').length > 0) {
            return this.$('.no_history').slideUp('fast', function() {
                $(this).remove();
                if (that.state === 'visible') {
                    return that.container.adjust_collapsible_panel_height({
                        is_at_bottom});
                }
            });
        } else if (that.state === 'visible') {
            return this.container.adjust_collapsible_panel_height({
                delay_scroll: true,
                is_at_bottom
            });
        }
    }

    clear_history(event) {
        const that = this;
        event.preventDefault();
        this.container.clear_history();
        this.history = this.container.state.history;

        this.$('.query_history').slideUp('fast', function() {
            $(this).remove();
            if (that.$('.no_history').length === 0) {
                that.$('.history_list').append(that.dataexplorer_query_li_template({
                    no_query: true,
                    displayed_class: 'hidden'
                })
                );
                return that.$('.no_history').slideDown('fast');
            }
        });
        return that.container.adjust_collapsible_panel_height({
            size: 40,
            move_arrow: 'show',
            is_at_bottom: 'true'
        });
    }
}
HistoryView.initClass();

class DriverHandler {
    constructor(options) {
        this._begin = this._begin.bind(this);
        this._end = this._end.bind(this);
        this.close_connection = this.close_connection.bind(this);
        this.run_with_new_connection = this.run_with_new_connection.bind(this);
        this.cursor_next = this.cursor_next.bind(this);
        this.remove = this.remove.bind(this);
        this.container = options.container;
        this.concurrent = 0;
        this.total_duration = 0;
    }

    _begin() {
        if (this.concurrent === 0) {
            this.container.toggle_executing(true);
            this.begin_time = new Date();
        }
        return this.concurrent++;
    }

    _end() {
        if (this.concurrent > 0) {
            this.concurrent--;
            const now = new Date();
            this.total_duration += now - this.begin_time;
            this.begin_time = now;
        }
        if (this.concurrent === 0) {
            return this.container.toggle_executing(false);
        }
    }

    close_connection() {
        if (__guard__(this.connection, x => x.open) === true) {
            driver.close(this.connection);
            this.connection = null;
            return this._end();
        }
    }

    run_with_new_connection(query, {callback, connection_error, optargs}) {
        this.close_connection();
        this.total_duration = 0;
        this.concurrent = 0;

        return driver.connect((error, connection) => {
            if (error != null) {
                connection_error(error);
            }
            connection.removeAllListeners('error'); // See issue 1347
            connection.on('error', error => {
                return connection_error(error);
            }
            );
            this.connection = connection;
            this._begin();
            return query.private_run(connection, optargs, (error, result) => {
                this._end();
                return callback(error, result);
            }
            );
        }
        );
    }

    cursor_next(cursor, {error, row, end}) {
        if (this.connection == null) {
            end();
        }
        this._begin();
        return cursor.next((err, row_) => {
            this._end();
            if (err != null) {
                if (err.message === 'No more rows in the cursor.') {
                    return end();
                } else {
                    return error(err);
                }
            } else {
                return row(row_);
            }
        }
        );
    }

    remove() {
        return this.close_connection();
    }
}



exports.QueryResult = QueryResult;
exports.Container = Container;
exports.ResultView = ResultView;
exports.TreeView = TreeView;
exports.TableView = TableView;
exports.RawView = RawView;
exports.ProfileView = ProfileView;
exports.ResultViewWrapper = ResultViewWrapper;
exports.OptionsView = OptionsView;
exports.HistoryView = HistoryView;
exports.DriverHandler = DriverHandler;
exports.dataexplorer_state = dataexplorer_state;

function __guard__(value, transform) {
  return (typeof value !== 'undefined' && value !== null) ? transform(value) : undefined;
}
function __guardMethod__(obj, methodName, transform) {
  if (typeof obj !== 'undefined' && obj !== null && typeof obj[methodName] === 'function') {
    return transform(obj, methodName);
  } else {
    return undefined;
  }
}

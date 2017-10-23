// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "pprint/js_pprint.hpp"

#include <vector>

#include "containers/object_buffer.hpp"
#include "rdb_protocol/base64.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/term_storage.hpp"

namespace pprint {


class js_pretty_printer_t {
    unsigned int depth;
    bool prepend_ok;
    bool in_r_expr;

    pprint_streamer stream;

public:
    std::vector<stream_elem> elems() RVALUE_THIS { return std::move(stream).elems(); }

    js_pretty_printer_t() : depth(0), prepend_ok(true), in_r_expr(false) {}


    void walk(const ql::raw_term_t &t) {
        with_dynamic<decltype(depth)> wd_depth(&depth, depth + 1);
        if (depth > MAX_DEPTH) {
            // Crude attempt to avoid blowing stack
            stream.add(dotdotdot);
            return;
        }
        switch (static_cast<int>(t.type())) {
        case Term::DATUM: {
            object_buffer_t<prepend_r_expr_t> prep;
            if (!in_r_expr) {
                prep.create(this);
            }
            to_js_datum(t.datum());
        } break;
        case Term::FUNCALL: {
            // Hack here: JS users expect .do in suffix position if
            // there's only one argument.
            if (t.num_args() == 2) {
                string_dots_together(t);
            } else {
                toplevel_funcall(t);
            }
        } break;
        case Term::MAKE_ARRAY: {
            with_dynamic<bool> wd(&in_r_expr, true);
            object_buffer_t<prepend_r_expr_t> prep;
            if (!wd.old()) {
                prep.create(this);
            }
            to_js_array(t);
        } break;
        case Term::MAKE_OBJ: {
            with_dynamic<bool> wd(&in_r_expr, true);
            if (wd.old()) {
                to_js_natural_object(t);
            } else {
                to_js_wrapped_object(t);
            }
        } break;
        case Term::FUNC:
            to_js_func(t);
            break;
        case Term::VAR: {
            r_sanity_check(t.num_args() == 1);
            ql::raw_term_t arg0 = t.arg(0);
            r_sanity_check(arg0.type() == Term::DATUM);
            var_name(arg0.datum());
        } break;
        case Term::IMPLICIT_VAR: {
            prepend_r_dot prep(this);
            stream.add(row);
        } break;
        default:
            if (should_continue_string(t)) {
                string_dots_together(t);
            } else if (should_use_parens(t)) {
                standard_funcall(t);
            } else {
                standard_literal(t);
            }
            break;
        }
    }

private:
    std::string to_js_name(const ql::raw_term_t &t) {
        return to_js_name(Term::TermType_Name(t.type()));
    }

    std::string to_js_name(const std::string &s) {
        std::string result;
        for (auto it = s.begin(); it != s.end(); ++it) {
            if (*it == '_') {
                ++it;
                if (it == s.end()) {
                    result += '_';
                    break;
                }
                result += toupper(*it);
            } else {
                result += tolower(*it);
            }
        }
        return result;
    }

    void to_js_array(const ql::raw_term_t &t) {
        r_sanity_check(t.num_optargs() == 0);

        wrap<brackets> brack(this);
        nested nest(&stream);

        with_dynamic<bool> wd(&in_r_expr, true);

        for (size_t i = 0; i < t.num_args(); ++i) {
            if (i != 0) {
                comma_linebreak();
            }
            walk(t.arg(i));
        }
    }

    void to_js_natural_object(const ql::raw_term_t &t) {
        wrap<braces> brace(this);
        nested nest(&stream);

        bool first = true;
        t.each_optarg([&](const ql::raw_term_t &item, const std::string &name) {
                if (!first) {
                    comma_linebreak();
                }
                first = false;
                nested nest2(&stream);
                push_text(strprintf("\"%s\":", name.c_str()));
                add_cond_linebreak();
                walk(item);
            });
    }

    void to_js_wrapped_object(const ql::raw_term_t &t) {
        prepend_r_dot prep(this);
        stream.add(object);
        nested nest(&stream);

        bool first = true;
        t.each_optarg([&](const ql::raw_term_t &item, const std::string &name) {
                if (!first) {
                    comma_linebreak();
                }
                first = false;
                push_text(strprintf("\"%s\"", name.c_str()));
                comma_linebreak();
                walk(item);
            });
    }

    void to_js_datum(const ql::datum_t &d) {
        switch (d.get_type()) {
        case ql::datum_t::type_t::MINVAL: {
            prepend_r_dot prep(this);
            stream.add(minval);
            return;
        }
        case ql::datum_t::type_t::MAXVAL: {
            prepend_r_dot prep(this);
            stream.add(maxval);
        } return;
        case ql::datum_t::type_t::R_NULL:
            stream.add(nil);
            return;
        case ql::datum_t::type_t::R_BOOL:
            stream.add(d.as_bool() ? true_v : false_v);
            return;
        case ql::datum_t::type_t::R_NUM:
            push_text(trunc(d.as_num()) == d.as_num()
                             ? std::to_string(lrint(d.as_num()))
                             : std::to_string(d.as_num()));
            return;
        case ql::datum_t::type_t::R_BINARY: {
            stream.add(node_buffer);
            wrap<parens> paren(this);
            nested nest(&stream);
            push_text(encode_base64(d.as_binary().data(),
                                           d.as_binary().size()));
            comma_linebreak();
            stream.add(base64_str);
            return;
        }
        case ql::datum_t::type_t::R_STR:
            push_text(strprintf("\"%s\"", d.as_str().to_std().c_str()));
            return;
        case ql::datum_t::type_t::R_ARRAY: {
            wrap<brackets> brack(this);
            nested nest(&stream);

            for (size_t i = 0; i < d.arr_size(); ++i) {
                if (i != 0) {
                    comma_linebreak();
                }
                to_js_datum(d.get(i));
            }
            return;
        }
        case ql::datum_t::type_t::R_OBJECT: {
            nested nest(&stream);
            wrap<braces> brace(this);

            for (size_t i = 0; i < d.obj_size(); ++i) {
                if (i != 0) {
                    comma_linebreak();
                }
                auto pair = d.get_pair(i);
                {
                    nested nest2(&stream);
                    push_text(strprintf("\"%s\":", pair.first.to_std().c_str()));
                    add_cond_linebreak();
                    to_js_datum(pair.second);
                }
            }

            return;
        }
        case ql::datum_t::type_t::UNINITIALIZED: // fallthrough intentional
        default:
            unreachable();
        }
    }

    void render_optargs(const ql::raw_term_t &t) {
        wrap<braces> brace(this);
        nested nest(&stream);

        bool first = true;
        t.each_optarg([&](const ql::raw_term_t &item, const std::string &name) {
                if (!first) {
                    comma_linebreak();
                }
                first = false;

                nested nest2(&stream);
                push_text(strprintf("\"%s\":", to_js_name(name).c_str()));
                add_cond_linebreak();
                walk(item);
            });
    }

    optional<ql::raw_term_t>
        visit_stringing(const ql::raw_term_t &var,
                        std::vector<std::function<void()>> *stack,
                        bool *last_is_dot,
                        bool *last_should_r_wrap) {
        switch (static_cast<int>(var.type())) {
        case Term::BRACKET: {
            r_sanity_check(var.num_args() == 2);
            stack->push_back([this, var]() {
                wrap<parens> paren(this);

                with_dynamic<bool> wd(&in_r_expr, true);
                walk(var.arg(1));
                if (var.num_optargs() > 0) {
                    stream.add(comma);
                    add_cond_linebreak();
                    render_optargs(var);
                }
            });

            *last_is_dot = false;
            *last_should_r_wrap = false;
            return make_optional(var.arg(0));
        }
        case Term::FUNCALL: {
            r_sanity_check(var.num_args() == 2);
            r_sanity_check(var.num_optargs() == 0);

            stack->push_back([this, var]() {
                stream.add(do_st);
                wrap<parens> paren(this);
                with_dynamic<bool> wd(&in_r_expr, true);

                nested nest(&stream);
                walk(var.arg(0));
            });

            *last_is_dot = true;
            *last_should_r_wrap = true;
            return make_optional(var.arg(1));
        }
        case Term::DATUM: {
            stack->push_back([this, var]() {
                with_dynamic<bool> wd(&in_r_expr, true);

                prepend_r_expr_t prep(this);
                to_js_datum(var.datum());
            });

            *last_is_dot = false;
            *last_should_r_wrap = false;
            return optional<ql::raw_term_t>();
        }
        case Term::MAKE_OBJ: {
            stack->push_back([this, var]() {
                with_dynamic<bool> wd(&in_r_expr, true);
                to_js_wrapped_object(var);
            });
            *last_is_dot = false;
            *last_should_r_wrap = false;
            return optional<ql::raw_term_t>();
        }
        case Term::VAR: {
            r_sanity_check(var.num_args() == 1);
            ql::raw_term_t arg = var.arg(0);
            r_sanity_check(arg.type() == Term::DATUM);

            stack->push_back([this, arg]() {
                var_name(arg.datum());
            });

            *last_is_dot = false;
            *last_should_r_wrap = false;
            return optional<ql::raw_term_t>();
        }
        case Term::IMPLICIT_VAR: {
            stack->push_back([this]() {
                stream.add(row);
            });
            *last_is_dot = false;
            *last_should_r_wrap = true;
            return optional<ql::raw_term_t>();
        }
        default: {
            stack->push_back([this, var]() {
                push_text(to_js_name(var));
                wrap<parens> paren(this);

                with_dynamic<bool> wd(&in_r_expr, true);

                bool first = true;
                switch (var.num_args()) {
                case 0:
                    break;
                case 1:
                    break;
                default: {
                    nested nest(&stream);

                    for (size_t i = 1, e = var.num_args(); i < e; ++i) {
                        if (!first) {
                            comma_linebreak();
                        }
                        first = false;
                        walk(var.arg(i));
                    }
                } break;
                }

                if (var.num_optargs() > 0) {
                    if (!first) {
                        comma_linebreak();
                    }
                    first = false;
                    render_optargs(var);
                }
            });
            *last_should_r_wrap = should_use_rdot(var);
            switch (var.num_args()) {
            case 0: {
                *last_is_dot = false;
                return optional<ql::raw_term_t>();
            }
            default: {
                *last_is_dot = true;
                return make_optional(var.arg(0));
            }
            }
        }
        }
    }

    void string_dots_together(const ql::raw_term_t &t) {
        std::vector<std::function<void()>> stack;
        bool last_is_dot = false;
        bool last_should_r_wrap = false;
        optional<ql::raw_term_t> var(t);
        while (var && should_continue_string(*var)) {
            if (last_is_dot) {
                stack.push_back([this]() {
                    add_dot_linebreak();
                });
            }
            var = visit_stringing(*var, &stack, &last_is_dot, &last_should_r_wrap);
        }

        if (!var) {
            if (last_should_r_wrap) {
                prepend_r_dot prep(this);
                if (last_is_dot) {
                    add_dot_linebreak();
                }
                reverse(std::move(stack));
            } else {
                if (last_is_dot) {
                    add_dot_linebreak();
                }
                reverse(std::move(stack));
            }
        } else if (should_use_rdot(*var)) {
            prepend_r_dot prep(this);

            {
                with_dynamic<bool> wd(&prepend_ok, false);
                walk(*var);
            }

            if (last_is_dot) {
                add_dot_linebreak();
            }
            reverse(std::move(stack));
        } else {
            nested nest(&stream);
            walk(*var);
            if (last_is_dot) {
                stream.add(justdot);
            }
            reverse(std::move(stack));
        }
    }

    void reverse(std::vector<std::function<void()>> &&stack) {
        for (auto it = stack.end(), end = stack.begin(); it != end; ) {
            --it;
            (*it)();
        }
    }

    void toplevel_funcall(const ql::raw_term_t &t) {
        r_sanity_check(t.num_args() >= 1);
        r_sanity_check(t.num_optargs() == 0);
        prepend_r_dot prep(this);

        stream.add(do_st);
        stream.add(lparen);

        walk(t.arg(0));

        {
            with_dynamic<bool> wd(&in_r_expr, true);
            nested nest(&stream);

            bool first = true;
            for (size_t i = 1; i < t.num_args(); ++i) {
                if (!first) {
                    comma_linebreak();
                }
                first = false;
                walk(t.arg(i));
            }
            if (!first) {
                comma_linebreak();
            }
            first = false;
        }

        stream.add(rparen);
    }

    void standard_funcall(const ql::raw_term_t &t) {
        // One or the other.
        object_buffer_t<prepend_r_dot> prep;
        object_buffer_t<nested> nest;

        if (should_use_rdot(t)) {
            prep.create(this);
        } else {
            nest.create(&stream);
        }

        term_name(t);

        with_dynamic<bool> wd(&in_r_expr, true);
        wrap<parens> parens(this);

        bool firstarg = true;
        for (size_t i = 0; i < t.num_args(); ++i) {
            if (!firstarg) {
                comma_linebreak();
            }
            firstarg = false;
            walk(t.arg(i));
        }
        if (t.num_optargs() > 0) {
            if (!firstarg) {
                comma_linebreak();
            }
            firstarg = false;
            render_optargs(t);
        }
    }

    void standard_literal(const ql::raw_term_t &t) {
        r_sanity_check(t.num_args() == 0);
        prepend_r_dot prep(this);
        term_name(t);
    }

    void push_text(std::string &&payload) {
        stream.add(text_elem{std::move(payload)});
    }

    // Set and unset a "dynamically scoped" (sort of) variable.
    template <class T>
    class with_dynamic {
        T *var_;
        T old_;
        DISABLE_COPYING(with_dynamic);
    public:
        explicit with_dynamic(T *var, T value) : var_(var), old_(*var) {
            *var = value;
        }
        ~with_dynamic() {
            *var_ = old_;
        }
        T old() const {
            return old_;
        }
    };

    template <class T>
    class wrap {
        js_pretty_printer_t *pp_;
        DISABLE_COPYING(wrap);
    public:
        explicit wrap(js_pretty_printer_t *pp) : pp_(pp) {
            T::enter(pp);
        }
        ~wrap() {
            T::exit(pp_);
        }
    };

    class prepend_r_dot {
        object_buffer_t<nested> nested_;
        DISABLE_COPYING(prepend_r_dot);
    public:
        explicit prepend_r_dot(js_pretty_printer_t *pp) {
            if (pp->prepend_ok) {
                pp->stream.add(r_st);
                nested_.create(&pp->stream);
                pp->stream.add(justdot);
            }
        }
    };

    struct parens {
        static void enter(js_pretty_printer_t *pp) { pp->stream.add(lparen); }
        static void exit(js_pretty_printer_t *pp) { pp->stream.add(rparen); }
    };

    struct brackets {
        static void enter(js_pretty_printer_t *pp) { pp->stream.add(lbracket); }
        static void exit(js_pretty_printer_t *pp) { pp->stream.add(rbracket); }
    };

    struct braces {
        static void enter(js_pretty_printer_t *pp) { pp->stream.add(lbrace); }
        static void exit(js_pretty_printer_t *pp) { pp->stream.add(rbrace); }
    };

    class prepend_r_expr_t {
        prepend_r_dot r_dot_;
        object_buffer_t<wrap<parens>> parens_;
    public:
        explicit prepend_r_expr_t(js_pretty_printer_t *pp)
            : r_dot_(pp) {
            pp->stream.add(expr);
            parens_.create(pp);
        }
    };

    void term_name(const ql::raw_term_t &t) {
        switch (static_cast<int>(t.type())) {
        case Term::JAVASCRIPT:
            stream.add(js);
            break;
        default:
            push_text(to_js_name(t));
            break;
        }
    }

    bool should_use_rdot(const ql::raw_term_t &t) {
        switch (static_cast<int>(t.type())) {
        case Term::VAR:
        case Term::DATUM:
            return false;
        default:
            return true;
        }
    }

    bool should_continue_string(const ql::raw_term_t &t) {
        switch (static_cast<int>(t.type())) {
        case Term::ERROR:
        case Term::UUID:
        case Term::HTTP:
        case Term::DB:
        case Term::EQ:
        case Term::NE:
        case Term::LT:
        case Term::LE:
        case Term::GT:
        case Term::GE:
        case Term::NOT:
        case Term::ADD:
        case Term::SUB:
        case Term::MUL:
        case Term::DIV:
        case Term::MOD:
        case Term::BIT_AND:
        case Term::BIT_OR:
        case Term::BIT_XOR:
        case Term::BIT_NOT:
        case Term::BIT_SAL:
        case Term::BIT_SAR:
        case Term::OBJECT:
        case Term::RANGE:
        case Term::DB_CREATE:
        case Term::DB_DROP:
        case Term::DB_LIST:
        case Term::BRANCH:
        case Term::OR:
        case Term::AND:
        case Term::JSON:
        case Term::ISO8601:
        case Term::EPOCH_TIME:
        case Term::NOW:
        case Term::TIME:
        case Term::MONDAY:
        case Term::TUESDAY:
        case Term::WEDNESDAY:
        case Term::THURSDAY:
        case Term::FRIDAY:
        case Term::SATURDAY:
        case Term::SUNDAY:
        case Term::JANUARY:
        case Term::FEBRUARY:
        case Term::MARCH:
        case Term::APRIL:
        case Term::MAY:
        case Term::JUNE:
        case Term::JULY:
        case Term::AUGUST:
        case Term::SEPTEMBER:
        case Term::OCTOBER:
        case Term::NOVEMBER:
        case Term::DECEMBER:
        case Term::LITERAL:
        case Term::RANDOM:
        case Term::GEOJSON:
        case Term::POINT:
        case Term::LINE:
        case Term::POLYGON:
        case Term::CIRCLE:
        case Term::DATUM:
        case Term::VAR:
        case Term::MAKE_ARRAY:
        case Term::MAKE_OBJ:
        case Term::ARGS:
        case Term::JAVASCRIPT:
        case Term::ASC:
        case Term::DESC:
        case Term::MINVAL:
        case Term::MAXVAL:
            return false;
        case Term::TABLE:
        case Term::FUNCALL:
            return t.num_args() == 2;
        default:
            return true;
        }
    }

    bool should_use_parens(const ql::raw_term_t &t) {
        // handle malformed protobufs
        if (t.num_args() > 0) return true;
        switch (static_cast<int>(t.type())) {
        case Term::MINVAL:
        case Term::MAXVAL:
        case Term::IMPLICIT_VAR:
        case Term::MONDAY:
        case Term::TUESDAY:
        case Term::WEDNESDAY:
        case Term::THURSDAY:
        case Term::FRIDAY:
        case Term::SATURDAY:
        case Term::SUNDAY:
        case Term::JANUARY:
        case Term::FEBRUARY:
        case Term::MARCH:
        case Term::APRIL:
        case Term::MAY:
        case Term::JUNE:
        case Term::JULY:
        case Term::AUGUST:
        case Term::SEPTEMBER:
        case Term::OCTOBER:
        case Term::NOVEMBER:
        case Term::DECEMBER:
            return false;
        default:
            return true;
        }
    }

    void var_name(const ql::datum_t &d) {
        push_text(print_var(lrint(d.as_num())));
    }

    void to_js_func(const ql::raw_term_t &t) {
        r_sanity_check(t.type() == Term::FUNC);
        r_sanity_check(t.num_args() == 2);

        nested nest(&stream);
        stream.add(lambda_1);
        {
            nested nest2(&stream);
            stream.add(lambda_2);
            stream.add(sp);

            ql::raw_term_t arg0 = t.arg(0);
            if (arg0.type() == Term::MAKE_ARRAY) {
                wrap<parens> paren(this);
                nested nest_args(&stream);
                for (size_t i = 0; i < arg0.num_args(); ++i) {
                    ql::raw_term_t item = arg0.arg(i);
                    if (i != 0) {
                        comma_linebreak();
                    }
                    r_sanity_check(item.type() == Term::DATUM);
                    ql::datum_t d = item.datum();
                    r_sanity_check(d.get_type() == ql::datum_t::type_t::R_NUM);
                    var_name(d);
                }

            } else if (arg0.type() == Term::DATUM &&
                       arg0.datum().get_type() == ql::datum_t::type_t::R_ARRAY) {
                wrap<parens> paren(this);
                nested nest_args(&stream);
                ql::datum_t d = arg0.datum();
                for (size_t i = 0; i < d.arr_size(); ++i) {
                    if (i != 0) {
                        comma_linebreak();
                    }
                    var_name(d.get(i));
                }

            } else {
                walk(arg0);
            }
            stream.add(sp);
            stream.add(lbrace);
            stream.add_crlf();

            with_dynamic<bool> wd(&in_r_expr, true);

            stream.add(return_st);
            stream.add(sp);
            {
                nested nest_body(&stream);
                walk(t.arg(1));
            }
            stream.add(semicolon);
        }
        stream.add_crlf();
        stream.add(rbrace);
    }

    void comma_linebreak() {
        stream.add(comma);
        add_cond_linebreak();
    }

    void add_cond_linebreak() {
        stream.add(cond_elem_spec{" ", "", ""});
    }

    void add_dot_linebreak() {
        stream.add(cond_elem_spec{".", ".", ""});
    }



    static text_elem lparen, rparen, lbracket, rbracket, lbrace, rbrace;
    static text_elem colon, quote, sp, justdot, dotdotdot, comma;
    static text_elem nil, minval, maxval, true_v, false_v, r_st, json;
    static text_elem row, do_st, return_st, lambda_1, lambda_2, expr;
    static text_elem object, js, node_buffer, base64_str, semicolon;

    static const unsigned int MAX_DEPTH = 15;
};

text_elem js_pretty_printer_t::lparen{"("};
text_elem js_pretty_printer_t::rparen{")"};
text_elem js_pretty_printer_t::lbracket{"["};
text_elem js_pretty_printer_t::rbracket{"]"};
text_elem js_pretty_printer_t::lbrace{"{"};
text_elem js_pretty_printer_t::rbrace{"}"};
text_elem js_pretty_printer_t::colon{":"};
text_elem js_pretty_printer_t::semicolon{";"};
text_elem js_pretty_printer_t::comma{","};
text_elem js_pretty_printer_t::justdot{"."};
text_elem js_pretty_printer_t::dotdotdot{"..."};
text_elem js_pretty_printer_t::nil{"nil"};
text_elem js_pretty_printer_t::minval{"minval"};
text_elem js_pretty_printer_t::maxval{"maxval"};
text_elem js_pretty_printer_t::true_v{"true"};
text_elem js_pretty_printer_t::false_v{"false"};
text_elem js_pretty_printer_t::sp{" "};
text_elem js_pretty_printer_t::quote{"\""};
text_elem js_pretty_printer_t::r_st{"r"};
text_elem js_pretty_printer_t::json{"json"};
text_elem js_pretty_printer_t::row{"row"};
text_elem js_pretty_printer_t::return_st{"return"};
text_elem js_pretty_printer_t::do_st{"do"};
text_elem js_pretty_printer_t::lambda_1{"func"};
text_elem js_pretty_printer_t::lambda_2{"tion"};
text_elem js_pretty_printer_t::expr{"expr"};
text_elem js_pretty_printer_t::object{"object"};
text_elem js_pretty_printer_t::js{"js"};
text_elem js_pretty_printer_t::node_buffer{"Buffer"};
text_elem js_pretty_printer_t::base64_str{"'base64'"};

std::string pretty_print_as_js(size_t width, const ql::raw_term_t &t) {
    js_pretty_printer_t pp;
    pp.walk(t);
    return pretty_print(width, std::move(pp).elems());
}

#ifndef NDEBUG

// This function is not called by anything nor should it ever be
// exported from this file.  Its sole reason for existence is to
// remind people who add new Term types and new datum types to update
// the pretty printer.
//
// Pretty printer elements that need to be updated:
// - `should_use_parens` should add the new Term to the list if it
//   represents a constant, non functional value.  Think `r.minval`.
// - `should_continue_string` should add the new Term to the list if
//   it should be used in a string-of-dotted-expressions context.  So
//   like `r.foo(1).bar(2).baz(4)` instead of `r.eq(1, 2)`.
//   ^^ What does this mean??
// - `should_use_rdot` should add the new Term to the list if it
//   represents some sort of specialized language feature--for example
//   literals like 4, or strings, or variable names.  These are very
//   rare.
// - `term_name` should add the new Term to the list if its name in
//   JavaScript is different from its name in the protocol.  Currently
//   the only example is `Term::JAVASCRIPT` which is called `r.js` in
//   JavaScript.
// - `visit_stringing` should have the new Term added if it requires
//   exotic handling when in a dotted list.  This should be extremely
//   rare; things like `Term::BRACKET` which turn into `x(4)` are
//   where it usually shows up.
// - `walk` should have the new Term added if it requires
//   exotic handling when not in dotted list context.  These should
//   also be rare, and largely similar to the cases where
//   `visit_stringing` has to have a new Term added.
//
// Finally if a new datum type is added, `to_js_datum` would need to
// be updated.
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
static void pprint_update_reminder() {
    Term::TermType type = Term::UPDATE;
    switch (type) {
    case Term::UPDATE:
    case Term::DELETE:
    case Term::INSERT:
    case Term::REPLACE:
    case Term::DB_CREATE:
    case Term::DB_DROP:
    case Term::TABLE_CREATE:
    case Term::TABLE_DROP:
    case Term::WAIT:
    case Term::RECONFIGURE:
    case Term::REBALANCE:
    case Term::SYNC:
    case Term::GRANT:
    case Term::SET_WRITE_HOOK:
    case Term::GET_WRITE_HOOK:
    case Term::INDEX_CREATE:
    case Term::INDEX_DROP:
    case Term::INDEX_WAIT:
    case Term::INDEX_RENAME:
    case Term::DATUM:
    case Term::MAKE_ARRAY:
    case Term::MAKE_OBJ:
    case Term::BINARY:
    case Term::VAR:
    case Term::JAVASCRIPT:
    case Term::HTTP:
    case Term::ERROR:
    case Term::IMPLICIT_VAR:
    case Term::RANDOM:
    case Term::DB:
    case Term::TABLE:
    case Term::GET:
    case Term::GET_ALL:
    case Term::EQ:
    case Term::NE:
    case Term::LT:
    case Term::LE:
    case Term::GT:
    case Term::GE:
    case Term::NOT:
    case Term::ADD:
    case Term::SUB:
    case Term::MUL:
    case Term::DIV:
    case Term::MOD:
    case Term::BIT_AND:
    case Term::BIT_OR:
    case Term::BIT_XOR:
    case Term::BIT_NOT:
    case Term::BIT_SAL:
    case Term::BIT_SAR:
    case Term::APPEND:
    case Term::PREPEND:
    case Term::DIFFERENCE:
    case Term::SET_INSERT:
    case Term::SET_INTERSECTION:
    case Term::SET_UNION:
    case Term::SET_DIFFERENCE:
    case Term::SLICE:
    case Term::OFFSETS_OF:
    case Term::GET_FIELD:
    case Term::HAS_FIELDS:
    case Term::PLUCK:
    case Term::WITHOUT:
    case Term::MERGE:
    case Term::LITERAL:
    case Term::BETWEEN:
    case Term::CHANGES:
    case Term::REDUCE:
    case Term::MAP:
    case Term::FOLD:
    case Term::FILTER:
    case Term::CONCAT_MAP:
    case Term::GROUP:
    case Term::ORDER_BY:
    case Term::DISTINCT:
    case Term::COUNT:
    case Term::SUM:
    case Term::AVG:
    case Term::MIN:
    case Term::MAX:
    case Term::UNION:
    case Term::NTH:
    case Term::BRACKET:
    case Term::ARGS:
    case Term::LIMIT:
    case Term::SKIP:
    case Term::INNER_JOIN:
    case Term::OUTER_JOIN:
    case Term::EQ_JOIN:
    case Term::ZIP:
    case Term::RANGE:
    case Term::INSERT_AT:
    case Term::DELETE_AT:
    case Term::CHANGE_AT:
    case Term::SPLICE_AT:
    case Term::COERCE_TO:
    case Term::UNGROUP:
    case Term::TYPE_OF:
    case Term::FUNCALL:
    case Term::BRANCH:
    case Term::OR:
    case Term::AND:
    case Term::FOR_EACH:
    case Term::FUNC:
    case Term::ASC:
    case Term::DESC:
    case Term::INFO:
    case Term::MATCH:
    case Term::SPLIT:
    case Term::UPCASE:
    case Term::DOWNCASE:
    case Term::SAMPLE:
    case Term::IS_EMPTY:
    case Term::DEFAULT:
    case Term::CONTAINS:
    case Term::KEYS:
    case Term::VALUES:
    case Term::OBJECT:
    case Term::WITH_FIELDS:
    case Term::JSON:
    case Term::TO_JSON_STRING:
    case Term::ISO8601:
    case Term::TO_ISO8601:
    case Term::EPOCH_TIME:
    case Term::TO_EPOCH_TIME:
    case Term::NOW:
    case Term::IN_TIMEZONE:
    case Term::DURING:
    case Term::DATE:
    case Term::TIME_OF_DAY:
    case Term::TIMEZONE:
    case Term::TIME:
    case Term::YEAR:
    case Term::MONTH:
    case Term::DAY:
    case Term::DAY_OF_WEEK:
    case Term::DAY_OF_YEAR:
    case Term::HOURS:
    case Term::MINUTES:
    case Term::SECONDS:
    case Term::MONDAY:
    case Term::TUESDAY:
    case Term::WEDNESDAY:
    case Term::THURSDAY:
    case Term::FRIDAY:
    case Term::SATURDAY:
    case Term::SUNDAY:
    case Term::JANUARY:
    case Term::FEBRUARY:
    case Term::MARCH:
    case Term::APRIL:
    case Term::MAY:
    case Term::JUNE:
    case Term::JULY:
    case Term::AUGUST:
    case Term::SEPTEMBER:
    case Term::OCTOBER:
    case Term::NOVEMBER:
    case Term::DECEMBER:
    case Term::DB_LIST:
    case Term::TABLE_LIST:
    case Term::CONFIG:
    case Term::STATUS:
    case Term::INDEX_LIST:
    case Term::INDEX_STATUS:
    case Term::GEOJSON:
    case Term::TO_GEOJSON:
    case Term::POINT:
    case Term::LINE:
    case Term::POLYGON:
    case Term::DISTANCE:
    case Term::INTERSECTS:
    case Term::INCLUDES:
    case Term::CIRCLE:
    case Term::GET_INTERSECTING:
    case Term::FILL:
    case Term::GET_NEAREST:
    case Term::UUID:
    case Term::POLYGON_SUB:
    case Term::BETWEEN_DEPRECATED:
    case Term::MINVAL:
    case Term::MAXVAL:
    case Term::FLOOR:
    case Term::CEIL:
    case Term::ROUND:
        break;
    }
    ql::datum_t::type_t d = ql::datum_t::type_t::R_NULL;
    switch(d) {
    case ql::datum_t::type_t::R_NULL:
    case ql::datum_t::type_t::MINVAL:
    case ql::datum_t::type_t::MAXVAL:
    case ql::datum_t::type_t::R_BOOL:
    case ql::datum_t::type_t::R_NUM:
    case ql::datum_t::type_t::R_STR:
    case ql::datum_t::type_t::R_BINARY:
    case ql::datum_t::type_t::R_ARRAY:
    case ql::datum_t::type_t::R_OBJECT:
    case ql::datum_t::type_t::UNINITIALIZED:
        break;
    }
}

#endif  // NDEBUG

} // namespace pprint


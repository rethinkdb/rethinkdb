// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "pprint/js_pprint.hpp"

#include <math.h>

#include <vector>
#include <memory>

#include "errors.hpp"

#include "pprint/generic_term_walker.hpp"
#include "rdb_protocol/base64.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/term_storage.hpp"

namespace pprint {

class js_pretty_printer_t
    : public generic_term_walker_t<counted_t<const document_t> > {
    unsigned int depth;
    bool prepend_ok, in_r_expr;
    typedef std::vector<counted_t<const document_t> > v;
public:
    js_pretty_printer_t() : depth(0), prepend_ok(true), in_r_expr(false) {}
protected:
    counted_t<const document_t> visit_generic(const ql::raw_term_t &t) override {
        bool old_r_expr = in_r_expr;
        ++depth;
        if (depth > MAX_DEPTH) return dotdotdot; // Crude attempt to avoid blowing stack
        counted_t<const document_t> doc;
        switch (static_cast<int>(t.type())) {
        case Term::DATUM:
            doc = to_js_datum(t.datum());
            if (!in_r_expr) {
                doc = prepend_r_expr(doc);
            }
            break;
        case Term::FUNCALL:
            // Hack here: JS users expect .do in suffix position if
            // there's only one argument.
            if (t.num_args() == 2) {
                doc = string_dots_together(t);
            } else {
                doc = toplevel_funcall(t);
            }
            break;
        case Term::MAKE_ARRAY:
            in_r_expr = true;
            doc = to_js_array(t);
            if (!old_r_expr) {
                doc = prepend_r_expr(doc);
            }
            break;
        case Term::MAKE_OBJ:
            in_r_expr = true;
            if (old_r_expr) {
                doc = to_js_natural_object(t);
            } else {
                doc = to_js_wrapped_object(t);
            }
            break;
        case Term::FUNC:
            doc = to_js_func(t);
            break;
        case Term::VAR: {
            r_sanity_check(t.num_args() == 1);
            ql::raw_term_t arg0 = t.arg(0);
            r_sanity_check(arg0.type() == Term::DATUM);
            doc = var_name(arg0.datum());
        } break;
        case Term::IMPLICIT_VAR:
            doc = prepend_r_dot(row);
            break;
        default:
            if (should_continue_string(t)) {
                doc = string_dots_together(t);
            } else if (should_use_parens(t)) {
                doc = standard_funcall(t);
            } else {
                doc = standard_literal(t);
            }
            break;
        }
        in_r_expr = old_r_expr;
        --depth;
        return doc;
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

    counted_t<const document_t> to_js_array(const ql::raw_term_t &t) {
        r_sanity_check(t.num_optargs() == 0);
        bool old_r_expr = in_r_expr;
        in_r_expr = true;
        std::vector<counted_t<const document_t> > term;
        for (size_t i = 0; i < t.num_args(); ++i) {
            if (!term.empty()) { term.push_back(comma_linebreak); }
            term.push_back(visit_generic(t.arg(i)));
        }
        in_r_expr = old_r_expr;
        return make_c(lbrack, make_nest(make_concat(std::move(term))), rbrack);
    }

    counted_t<const document_t> to_js_object(const ql::raw_term_t &t) {
        r_sanity_check(t.num_args() == 0);
        return in_r_expr ? to_js_natural_object(t) : to_js_wrapped_object(t);
    }

    counted_t<const document_t> to_js_natural_object(const ql::raw_term_t &t) {
        std::vector<counted_t<const document_t> > term;
        t.each_optarg([&](const ql::raw_term_t &item, const std::string &name) {
                if (!term.empty()) { term.push_back(comma_linebreak); }
                term.push_back(make_nc(
                    make_text(strprintf("\"%s\":", name.c_str())),
                    cond_linebreak,
                    visit_generic(item)));
            });
        return make_c(lbrace, make_nest(make_concat(std::move(term))), rbrace);
    }

    counted_t<const document_t> to_js_wrapped_object(const ql::raw_term_t &t) {
        std::vector<counted_t<const document_t> > term;
        t.each_optarg([&](const ql::raw_term_t &item, const std::string &name) {
                if (!term.empty()) { term.push_back(comma_linebreak); }
                term.push_back(make_text(
                    strprintf("\"%s\"", name.c_str())));
                term.push_back(comma_linebreak);
                term.push_back(visit_generic(item));
            });
        return prepend_r_obj(make_nest(make_concat(std::move(term))));
    }

    counted_t<const document_t> to_js_datum(const ql::datum_t &d) {
        switch (d.get_type()) {
        case ql::datum_t::type_t::MINVAL:
            return prepend_r_dot(minval);
        case ql::datum_t::type_t::MAXVAL:
            return prepend_r_dot(maxval);
        case ql::datum_t::type_t::R_NULL:
            return nil;
        case ql::datum_t::type_t::R_BOOL:
            return d.as_bool() ? true_v : false_v;
        case ql::datum_t::type_t::R_NUM:
            return make_text(trunc(d.as_num()) == d.as_num()
                             ? std::to_string(lrint(d.as_num()))
                             : std::to_string(d.as_num()));
        case ql::datum_t::type_t::R_BINARY: {
            std::vector<counted_t<const document_t> > args;
            args.push_back(make_text(encode_base64(d.as_binary().data(),
                                                   d.as_binary().size())));
            args.push_back(comma_linebreak);
            args.push_back(base64_str);
            return make_c(node_buffer, lparen, make_nc(std::move(args)), rparen);
        }
        case ql::datum_t::type_t::R_STR:
            return make_text(strprintf("\"%s\"", d.as_str().to_std().c_str()));
        case ql::datum_t::type_t::R_ARRAY:
        {
            std::vector<counted_t<const document_t> > term;
            for (size_t i = 0; i < d.arr_size(); ++i) {
                if (!term.empty()) { term.push_back(comma_linebreak); }
                term.push_back(to_js_datum(d.get(i)));
            }
            return make_c(lbrack, make_nc(std::move(term)), rbrack);
        }
        case ql::datum_t::type_t::R_OBJECT:
        {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lbrace);
            for (size_t i = 0; i < d.obj_size(); ++i) {
                if (i != 0) { term.push_back(comma_linebreak); }
                auto pair = d.get_pair(i);
                term.push_back(make_nc(
                    make_text(strprintf("\"%s\":", pair.first.to_std().c_str())),
                    cond_linebreak,
                    to_js_datum(pair.second)));
            }
            term.push_back(rbrace);
            return make_nest(make_concat(std::move(term)));
        }
        case ql::datum_t::type_t::UNINITIALIZED: // fallthrough intentional
        default:
            unreachable();
        }
    }

    counted_t<const document_t> render_optargs(const ql::raw_term_t &t) {
        std::vector<counted_t<const document_t> > optargs;
        t.each_optarg([&](const ql::raw_term_t &item, const std::string &name) {
                if (!optargs.empty()) { optargs.push_back(comma_linebreak); }
                counted_t<const document_t> inner = make_c(make_text(
                        strprintf("\"%s\":", to_js_name(name).c_str())),
                    cond_linebreak,
                    visit_generic(item));
                optargs.push_back(make_nest(std::move(inner)));
            });
        return make_c(lbrace, make_nest(make_concat(std::move(optargs))), rbrace);
    }

    boost::optional<ql::raw_term_t>
        visit_stringing(const ql::raw_term_t &var,
                        std::vector<counted_t<const document_t> > *stack,
                        bool *last_is_dot,
                        bool *last_should_r_wrap) {
        bool insert_trailing_comma = false;
        bool old_r_expr = in_r_expr;
        switch (static_cast<int>(var.type())) {
        case Term::BRACKET: {
            r_sanity_check(var.num_args() == 2);
            stack->push_back(rparen);
            in_r_expr = true;
            if (var.num_optargs() > 0) {
                stack->push_back(render_optargs(var));
                stack->push_back(cond_linebreak);
                stack->push_back(comma);
            }
            stack->push_back(visit_generic(var.arg(1)));
            in_r_expr = old_r_expr;
            stack->push_back(lparen);
            *last_is_dot = false;
            *last_should_r_wrap = false;
            return boost::make_optional(var.arg(0));
        }
        case Term::FUNCALL: {
            r_sanity_check(var.num_args() == 2);
            r_sanity_check(var.num_optargs() == 0);
            stack->push_back(rparen);
            in_r_expr = true;
            stack->push_back(make_nest(visit_generic(var.arg(0))));
            in_r_expr = old_r_expr;
            stack->push_back(lparen);
            stack->push_back(do_st);
            stack->push_back(dot_linebreak);
            *last_is_dot = true;
            *last_should_r_wrap = true;
            return boost::make_optional(var.arg(1));
        }
        case Term::DATUM:
            in_r_expr = true;
            stack->push_back(prepend_r_expr(to_js_datum(var.datum())));
            in_r_expr = old_r_expr;
            *last_is_dot = false;
            *last_should_r_wrap = false;
            return boost::optional<ql::raw_term_t>();
        case Term::MAKE_OBJ:
            in_r_expr = true;
            stack->push_back(to_js_wrapped_object(var));
            in_r_expr = old_r_expr;
            *last_is_dot = false;
            *last_should_r_wrap = false;
            return boost::optional<ql::raw_term_t>();
        case Term::VAR: {
            r_sanity_check(var.num_args() == 1);
            ql::raw_term_t arg = var.arg(0);
            r_sanity_check(arg.type() == Term::DATUM);
            stack->push_back(var_name(arg.datum()));
            *last_is_dot = false;
            *last_should_r_wrap = false;
            return boost::optional<ql::raw_term_t>();
        }
        case Term::IMPLICIT_VAR:
            stack->push_back(row);
            *last_is_dot = false;
            *last_should_r_wrap = true;
            return boost::optional<ql::raw_term_t>();
        default:
            stack->push_back(rparen);
            in_r_expr = true;
            if (var.num_optargs() > 0) {
                stack->push_back(render_optargs(var));
                insert_trailing_comma = true;
            }
            switch (var.num_args()) {
            case 0:
                in_r_expr = old_r_expr;
                stack->push_back(lparen);
                stack->push_back(make_text(to_js_name(var)));
                *last_should_r_wrap = should_use_rdot(var);
                *last_is_dot = false;
                return boost::optional<ql::raw_term_t>();
            case 1:
                in_r_expr = old_r_expr;
                stack->push_back(lparen);
                stack->push_back(make_text(to_js_name(var)));
                stack->push_back(dot_linebreak);
                *last_is_dot = true;
                *last_should_r_wrap = should_use_rdot(var);
                return boost::make_optional(var.arg(0));
            default:
                std::vector<counted_t<const document_t> > args;
                for (size_t i = 1; i < var.num_args(); ++i) {
                    if (!args.empty()) { args.push_back(comma_linebreak); }
                    args.push_back(visit_generic(var.arg(i)));
                }
                if (insert_trailing_comma) { stack->push_back(comma_linebreak); }
                stack->push_back(make_nest(reverse(std::move(args), false)));

                in_r_expr = old_r_expr;
                stack->push_back(lparen);
                stack->push_back(make_text(to_js_name(var)));
                stack->push_back(dot_linebreak);
                *last_is_dot = true;
                *last_should_r_wrap = should_use_rdot(var);
                return boost::make_optional(var.arg(0));
            }
        }
    }

    counted_t<const document_t> string_dots_together(const ql::raw_term_t &t) {
        std::vector<counted_t<const document_t> > stack;
        bool last_is_dot = false;
        bool last_should_r_wrap = false;
        boost::optional<ql::raw_term_t> var(t);
        while (var && should_continue_string(*var)) {
            var = visit_stringing(*var, &stack, &last_is_dot, &last_should_r_wrap);
        }

        if (!var) {
            if (last_should_r_wrap) {
                return prepend_r_dot(reverse(std::move(stack), false));
            } else {
                return reverse(std::move(stack), false);
            }
        } else if (should_use_rdot(*var)) {
            bool old = prepend_ok;
            prepend_ok = false;
            counted_t<const document_t> subdoc = visit_generic(*var);
            prepend_ok = old;
            return prepend_r_dot(make_c(subdoc, reverse(std::move(stack), false)));
        } else {
            return make_nc(visit_generic(*var), reverse(std::move(stack), last_is_dot));
        }
    }

    counted_t<const document_t>
    reverse(std::vector<counted_t<const document_t> > stack, bool last_is_dot) {
        // this song & dance ensures the nest starts after the punctuation.
        if (last_is_dot) {
            stack.pop_back();
            stack.push_back(justdot);
        }
        return make_concat(stack.rbegin(), stack.rend());
    }

    counted_t<const document_t> toplevel_funcall(const ql::raw_term_t &t) {
        r_sanity_check(t.num_args() >= 1);
        r_sanity_check(t.num_optargs() == 0);
        std::vector<counted_t<const document_t> > term;
        term.push_back(do_st);
        term.push_back(lparen);
        bool old_r_expr = in_r_expr;
        in_r_expr = true;
        ql::raw_term_t arg0 = t.arg(0);
        std::vector<counted_t<const document_t> > args;
        for (size_t i = 1; i < t.num_args(); ++i) {
            if (!args.empty()) { args.push_back(comma_linebreak); }
            args.push_back(visit_generic(t.arg(i)));
        }
        if (!args.empty()) { args.push_back(comma_linebreak); }
        in_r_expr = old_r_expr;
        term.push_back(visit_generic(arg0));
        term.push_back(make_nest(make_concat(std::move(args))));
        term.push_back(rparen);
        return prepend_r_dot(make_concat(std::move(term)));
    }

    counted_t<const document_t> standard_funcall(const ql::raw_term_t &t) {
        std::vector<counted_t<const document_t> > term;
        term.push_back(term_name(t));
        bool old_r_expr = in_r_expr;
        in_r_expr = true;
        std::vector<counted_t<const document_t> > args;
        for (size_t i = 0; i < t.num_args(); ++i) {
            if (!args.empty()) { args.push_back(comma_linebreak); }
            args.push_back(visit_generic(t.arg(i)));
        }
        if (t.num_optargs() > 0) {
            if (!args.empty()) { args.push_back(comma_linebreak); }
            args.push_back(render_optargs(t));
        }
        term.push_back(wrap_parens(make_concat(std::move(args))));
        in_r_expr = old_r_expr;
        if (should_use_rdot(t)) {
            return prepend_r_dot(make_concat(std::move(term)));
        } else {
            return make_nest(make_concat(std::move(term)));
        }
    }

    counted_t<const document_t> standard_literal(const ql::raw_term_t &t) {
        r_sanity_check(t.num_args() == 0);
        return prepend_r_dot(term_name(t));
    }

    counted_t<const document_t> prepend_r_dot(counted_t<const document_t> doc) {
        if (!prepend_ok) return doc;
        return make_c(r_st, make_nc(justdot, doc));
    }

    counted_t<const document_t> wrap_with(counted_t<const document_t> left,
                                          counted_t<const document_t> doc,
                                          counted_t<const document_t> right) {
        return make_c(left, make_nest(doc), right);
    }

    counted_t<const document_t> wrap_parens(counted_t<const document_t> doc) {
        return wrap_with(lparen, doc, rparen);
    }

    counted_t<const document_t> prepend_r_expr(counted_t<const document_t> doc) {
        return prepend_r_dot(make_c(expr, wrap_parens(doc)));
    }

    counted_t<const document_t> prepend_r_obj(counted_t<const document_t> doc) {
        return prepend_r_dot(make_c(object, wrap_parens(doc)));
    }

    template <typename... Ts>
    counted_t<const document_t> make_nc(Ts &&... docs) {
        return make_nest(make_concat({std::forward<Ts>(docs)...}));
    }

    template <typename... Ts>
    counted_t<const document_t> make_c(Ts &&... docs) {
        return make_concat({std::forward<Ts>(docs)...});
    }

    counted_t<const document_t> term_name(const ql::raw_term_t &t) {
        switch (static_cast<int>(t.type())) {
        case Term::JAVASCRIPT:
            return js;
        default:
            return make_text(to_js_name(t));
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

    counted_t<const document_t> var_name(const ql::datum_t &d) {
        return make_text(print_var(lrint(d.as_num())));
    }

    counted_t<const document_t> to_js_func(const ql::raw_term_t &t) {
        r_sanity_check(t.type() == Term::FUNC);
        r_sanity_check(t.num_args() == 2);
        counted_t<const document_t> arglist;
        ql::raw_term_t arg0 = t.arg(0);
        if (arg0.type() == Term::MAKE_ARRAY) {
            std::vector<counted_t<const document_t> > args;
            for (size_t i = 0; i < arg0.num_args(); ++i) {
                ql::raw_term_t item = arg0.arg(i);
                if (!args.empty()) { args.push_back(comma_linebreak); }
                r_sanity_check(item.type() == Term::DATUM);
                ql::datum_t d = item.datum();
                r_sanity_check(d.get_type() == ql::datum_t::type_t::R_NUM);
                args.push_back(var_name(d));
            }
            arglist = make_c(lparen, make_nest(make_concat(std::move(args))), rparen);
        } else if (arg0.type() == Term::DATUM &&
                   arg0.datum().get_type() == ql::datum_t::type_t::R_ARRAY) {
            ql::datum_t d = arg0.datum();
            std::vector<counted_t<const document_t> > args;
            for (size_t i = 0; i < d.arr_size(); ++i) {
                if (!args.empty()) { args.push_back(comma_linebreak); }
                args.push_back(var_name(d.get(i)));
            }
            arglist = make_c(lparen, make_nest(make_concat(std::move(args))), rparen);
        } else {
            arglist = visit_generic(arg0);
        }
        bool old_r_expr = in_r_expr;
        in_r_expr = false;
        counted_t<const document_t> body =
            make_c(return_st,
                   sp,
                   make_nest(visit_generic(t.arg(1))),
                   semicolon);
        in_r_expr = old_r_expr;
        return make_nc(lambda_1,
                       make_nc(lambda_2,
                               sp,
                               std::move(arglist),
                               sp,
                               lbrace,
                               uncond_linebreak,
                               std::move(body)),
                       uncond_linebreak,
                       rbrace);
    }

    static counted_t<const document_t> lparen, rparen, lbrack, rbrack, lbrace, rbrace;
    static counted_t<const document_t> colon, quote, sp, justdot, dotdotdot, comma;
    static counted_t<const document_t> nil, minval, maxval, true_v, false_v, r_st, json;
    static counted_t<const document_t> row, do_st, return_st, lambda_1, lambda_2, expr;
    static counted_t<const document_t> object, js, node_buffer, base64_str, semicolon;
    static counted_t<const document_t> comma_linebreak;

    static const unsigned int MAX_DEPTH = 15;
};

counted_t<const document_t> js_pretty_printer_t::lparen = make_text("(");
counted_t<const document_t> js_pretty_printer_t::rparen = make_text(")");
counted_t<const document_t> js_pretty_printer_t::lbrack = make_text("[");
counted_t<const document_t> js_pretty_printer_t::rbrack = make_text("]");
counted_t<const document_t> js_pretty_printer_t::lbrace = make_text("{");
counted_t<const document_t> js_pretty_printer_t::rbrace = make_text("}");
counted_t<const document_t> js_pretty_printer_t::colon = make_text(":");
counted_t<const document_t> js_pretty_printer_t::semicolon = make_text(";");
counted_t<const document_t> js_pretty_printer_t::comma = make_text(",");
counted_t<const document_t> js_pretty_printer_t::justdot = make_text(".");
counted_t<const document_t> js_pretty_printer_t::dotdotdot = make_text("...");
counted_t<const document_t> js_pretty_printer_t::nil = make_text("null");
counted_t<const document_t> js_pretty_printer_t::minval = make_text("minval");
counted_t<const document_t> js_pretty_printer_t::maxval = make_text("maxval");
counted_t<const document_t> js_pretty_printer_t::true_v = make_text("true");
counted_t<const document_t> js_pretty_printer_t::false_v = make_text("false");
counted_t<const document_t> js_pretty_printer_t::sp = make_text(" ");
counted_t<const document_t> js_pretty_printer_t::quote = make_text("\"");
counted_t<const document_t> js_pretty_printer_t::r_st = make_text("r");
counted_t<const document_t> js_pretty_printer_t::json = make_text("json");
counted_t<const document_t> js_pretty_printer_t::row = make_text("row");
counted_t<const document_t> js_pretty_printer_t::return_st = make_text("return");
counted_t<const document_t> js_pretty_printer_t::do_st = make_text("do");
counted_t<const document_t> js_pretty_printer_t::lambda_1 = make_text("func");
counted_t<const document_t> js_pretty_printer_t::lambda_2 = make_text("tion");
counted_t<const document_t> js_pretty_printer_t::expr = make_text("expr");
counted_t<const document_t> js_pretty_printer_t::object = make_text("object");
counted_t<const document_t> js_pretty_printer_t::js = make_text("js");
counted_t<const document_t> js_pretty_printer_t::node_buffer = make_text("Buffer");
counted_t<const document_t> js_pretty_printer_t::base64_str = make_text("'base64'");
counted_t<const document_t> js_pretty_printer_t::comma_linebreak =
    make_concat({js_pretty_printer_t::comma, make_cond(" ", "", "")});

counted_t<const document_t> render_as_javascript(const ql::raw_term_t &t) {
    return js_pretty_printer_t().walk(t);
}

} // namespace pprint


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
// - `visit_generic` should have the new Term added if it requires
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

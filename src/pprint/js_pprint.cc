#include "pprint/pprint.hpp"

#include <math.h>

#include <vector>
#include <memory>

#include "rdb_protocol/ql2_extensions.pb.h"
#include "debug.hpp"

namespace pprint {

template <typename Accumulator>
class generic_term_walker_t {
public:
    virtual ~generic_term_walker_t() {}

    virtual Accumulator walk(Term *t) {
        return std::move(visit_generic(t));
    }
protected:
    virtual Accumulator visit_generic(Term *t) = 0;
};

// we know what we're doing here, and I don't think 169 random
// Term types is going to clarify anything.
#pragma GCC diagnostic ignored "-Wswitch-enum"

class js_pretty_printer_t
    : public generic_term_walker_t<counted_t<const document_t> > {
    unsigned int depth = 0;
    typedef std::vector<counted_t<const document_t> > v;
protected:
    virtual counted_t<const document_t> visit_generic(Term *t) {
        ++depth;
        if (depth > MAX_DEPTH) return dotdotdot; // Crude attempt to avoid blowing stack
        counted_t<const document_t> doc;
        switch (t->type()) {
        case Term::DATUM:
            doc = to_js_datum(t->mutable_datum());
            break;
/*        case Term::BRANCH:
            doc = string_branches_together(t);
            break;*/
        case Term::FUNCALL:
            // Hack here: JS users expect .do in suffix position if
            // there's only one argument.
            if (t->args_size() == 2) {
                doc = string_dots_together(t);
            } else {
                doc = toplevel_funcall(t);
            }
            break;
        case Term::MAKE_ARRAY:
            doc = to_js_array(t);
            break;
        case Term::MAKE_OBJ:
            doc = to_js_object(t);
            break;
        case Term::FUNC:
            doc = to_js_func(t);
            break;
        case Term::VAR:
            guarantee(t->args_size() == 1);
            guarantee(t->mutable_args(0)->type() == Term::DATUM);
            doc = var_name(t->mutable_args(0)->mutable_datum());
            break;
        case Term::IMPLICIT_VAR:
            doc = make_concat(r_st, justdot, row);
            break;
        default:
            if (should_continue_string(t)) {
                doc = string_dots_together(t);
            } else {
                doc = standard_funcall(t);
            }
            break;
        }
        --depth;
        return std::move(doc);
    }
private:
    std::string to_js_name(Term *t) {
        return to_js_name(Term_TermType_Name(t->type()));
    }
    std::string to_js_name(std::string s) {
        std::string result = "";
        for (auto it = s.begin(); it != s.end(); ++it) {
            if (*it == '_') {
                ++it;
                result += toupper(*it);
            } else {
                result += tolower(*it);
            }
        }
        return std::move(result);
    }
    counted_t<const document_t> to_js_array(Term *t) {
        guarantee(t->optargs_size() == 0);
        std::vector<counted_t<const document_t> > term;
        for (int i = 0; i < t->args_size(); ++i) {
            if (i != 0) {
                term.push_back(comma);
                term.push_back(br);
            }
            term.push_back(visit_generic(t->mutable_args(i)));
        }
        return make_concat(lbrack, make_nest(make_concat(std::move(term))), rbrack);
    }
    counted_t<const document_t> to_js_object(Term *t) {
        guarantee(t->args_size() == 0);
        std::vector<counted_t<const document_t> > term;
        for (int i = 0; i < t->optargs_size(); ++i) {
            if (i != 0) {
                term.push_back(comma);
                term.push_back(br);
            }
            Term_AssocPair *ap = t->mutable_optargs(i);
            term.push_back(make_nest(make_concat(make_text("\"" + ap->key() + "\":"),
                                                 br,
                                                 visit_generic(ap->mutable_val()))));
        }
        return make_concat(lbrace, make_nest(make_concat(std::move(term))), rbrace);
    }
    counted_t<const document_t> to_js_datum(Datum *d) {
        switch (d->type()) {
        case Datum::R_NULL:
            return nil;
        case Datum::R_BOOL:
            return d->r_bool() ? true_v : false_v;
        case Datum::R_NUM:
        {
            double num = d->r_num();
            return make_text((fabs(num - trunc(num)) < 1e-10)
                             ? std::to_string(lrint(num))
                             : std::to_string(num));
        }
        case Datum::R_STR:
            return make_text("\"" + d->r_str() + "\"");
        case Datum::R_ARRAY:
        {
            std::vector<counted_t<const document_t> > term;
            for (int i = 0; i < d->r_array_size(); ++i) {
                if (i != 0) {
                    term.push_back(comma);
                    term.push_back(br);
                }
                term.push_back(to_js_datum(d->mutable_r_array(i)));
            }
            return make_concat(lbrack, make_nest(make_concat(std::move(term))), rbrack);
        }
        case Datum::R_OBJECT:
        {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lbrace);
            for (int i = 0; i < d->r_object_size(); ++i) {
                if (i != 0) {
                    term.push_back(comma);
                    term.push_back(br);
                }
                Datum_AssocPair *ap = d->mutable_r_object(i);
                term.push_back(make_text("\"" + ap->key() + "\":"));
                term.push_back(br);
                term.push_back(to_js_datum(ap->mutable_val()));
            }
            term.push_back(rbrace);
            return make_nest(make_concat(std::move(term)));
        }
        case Datum::R_JSON:
            return make_concat(r_st, justdot, json, lparen, quote, make_text(d->r_str()),
                               quote, rparen);
        default:
            unreachable();
        }
    }
    counted_t<const document_t> render_optargs(Term *t) {
        std::vector<counted_t<const document_t> > optargs;
        for (int i = 0; i < t->optargs_size(); ++i) {
            // don't insert redundant space
            if (i != 0) {
                optargs.push_back(comma);
                optargs.push_back(br);
            }
            Term_AssocPair *ap = t->mutable_optargs(i);
            optargs.push_back(make_nest(make_concat(make_text("\"" +
                                                              to_js_name(ap->key()) +
                                                              "\":"),
                                                    br,
                                                    visit_generic(ap->mutable_val()))));
        }
        return make_concat(lbrace, make_nest(make_concat(std::move(optargs))), rbrace);
    }
    std::pair<bool, Term *>
    visit_stringing(Term *var, std::vector<counted_t<const document_t> > *stack) {
        bool first = true;
        switch (var->type()) {
        case Term::BRACKET:
            stack->push_back(rparen);
            if (var->optargs_size() > 0) {
                stack->push_back(render_optargs(var));
                first = false;
            }
            if (var->args_size() > 1) { // arg 0 is the base
                for (int i = var->args_size() - 1; i > 0; --i) {
                    if (first) {
                        first = false;
                    } else {
                        stack->push_back(br);
                        stack->push_back(comma);
                    }
                    stack->push_back(visit_generic(var->mutable_args(i)));
                }
            }
            stack->push_back(lparen);
            return std::make_pair(false, var->mutable_args(0));
        case Term::FUNCALL:
            guarantee(var->args_size() == 2);
            guarantee(var->optargs_size() == 0);
            stack->push_back(rparen);
            stack->push_back(make_nest(visit_generic(var->mutable_args(0))));
            stack->push_back(lparen);
            stack->push_back(do_st);
            stack->push_back(dot);
            return std::make_pair(true, var->mutable_args(1));
        case Term::DATUM:
            stack->push_back(to_js_datum(var->mutable_datum()));
            return std::make_pair(false, nullptr);
        case Term::VAR:
            guarantee(var->args_size() == 1);
            guarantee(var->mutable_args(0)->type() == Term::DATUM);
            stack->push_back(var_name(var->mutable_args(0)->mutable_datum()));
            return std::make_pair(false, nullptr);
        default:
            stack->push_back(rparen);
            if (var->optargs_size() > 0) {
                stack->push_back(render_optargs(var));
                first = false;
            }
            switch (var->args_size()) {
            case 0:
                stack->push_back(lparen);
                stack->push_back(make_text(to_js_name(var)));
                return std::make_pair(false, nullptr);
            case 1:
                stack->push_back(lparen);
                stack->push_back(make_text(to_js_name(var)));
                stack->push_back(dot);
                return std::make_pair(true, var->mutable_args(0));
            default:
                for (int i = var->args_size() - 1; i > 0; --i) {
                    if (first) {
                        first = false;
                    } else {
                        stack->push_back(br);
                        stack->push_back(comma);
                    }
                    stack->push_back(visit_generic(var->mutable_args(i)));
                }

                stack->push_back(lparen);
                stack->push_back(make_text(to_js_name(var)));
                stack->push_back(dot);
                return std::make_pair(true, var->mutable_args(0));
            }
        }
    }
    counted_t<const document_t> string_dots_together(Term *t) {
        std::vector<counted_t<const document_t> > stack;
        Term *var = t;
        bool last_is_dot = false;
        while (var != nullptr && should_continue_string(var)) {
            auto pair = visit_stringing(var, &stack);
            last_is_dot = pair.first;
            var = pair.second;
        }
        guarantee(var != t);
        if (var == nullptr) {
            return reverse(std::move(stack), last_is_dot);
        } else if (should_use_rdot(var)) {
            return make_concat(r_st,
                               make_nest(make_concat(justdot, visit_generic(var),
                                                     reverse(std::move(stack), false))));
        } else {
            return make_nest(make_concat(visit_generic(var),
                                         reverse(std::move(stack),
                                                last_is_dot)));
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
    counted_t<const document_t> toplevel_funcall(Term *t) {
        guarantee(t->args_size() >= 1);
        guarantee(t->optargs_size() == 0);
        std::vector<counted_t<const document_t> > term;
        term.push_back(r_st);
        term.push_back(justdot);
        term.push_back(do_st);
        term.push_back(lparen);
        std::vector<counted_t<const document_t> > args;
        for (int i = 1; i < t->args_size(); ++i) {
            // don't insert redundant space
            if (i != 1) {
                args.push_back(comma);
                args.push_back(br);
            }
            args.push_back(visit_generic(t->mutable_args(i)));
        }
        if (!args.empty()) {
            args.push_back(comma);
            args.push_back(br);
        }
        args.push_back(visit_generic(t->mutable_args(0)));
        term.push_back(make_nest(make_concat(std::move(args))));
        term.push_back(rparen);
        return make_concat(std::move(term));
    }
    counted_t<const document_t> standard_funcall(Term *t) {
        std::vector<counted_t<const document_t> > term;
        term.push_back(make_text(to_js_name(t)));
        term.push_back(lparen);
        if (t->args_size() > 0 || t->optargs_size() > 0) {
            std::vector<counted_t<const document_t> > args;
            for (int i = 0; i < t->args_size(); ++i) {
                // don't insert redundant space
                if (args.size() != 0) {
                    args.push_back(comma);
                    args.push_back(br);
                }
                args.push_back(visit_generic(t->mutable_args(i)));
            }
            if (t->optargs_size() > 0) {
                if (args.size() > 0) {
                    args.push_back(comma);
                    args.push_back(br);
                }
                args.push_back(lbrace);
                args.push_back(render_optargs(t));
                args.push_back(rbrace);
            }
            term.push_back(make_nest(make_concat(std::move(args))));
        }
        term.push_back(rparen);
        return make_nest(make_concat(std::move(term)));
    }
    counted_t<const document_t> string_branches_together(Term *t) {
        std::vector<counted_t<const document_t> > term;
        term.push_back(lparen);
        term.push_back(cond);
        guarantee(t->args_size() == 3);
        term.push_back(sp);
        std::vector<counted_t<const document_t> > branches;
        Term *var = t;
        while (var->type() == Term::BRANCH) {
            std::vector<counted_t<const document_t> > branch;
            branch.push_back(lparen);
            branch.push_back(make_nest(make_concat(visit_generic(var->mutable_args(0)),
                                                   br,
                                                   visit_generic(var->mutable_args(1)))));
            branch.push_back(rparen);
            branches.push_back(make_concat(std::move(branch)));
            branches.push_back(br);
            var = var->mutable_args(2);
        }
        branches.push_back(make_concat(lparen,
                                       make_nest(make_concat(true_v,
                                                             br,
                                                             visit_generic(var))),
                                       rparen));
        term.push_back(make_nest(make_concat(std::move(branches))));
        term.push_back(rparen);
        return make_concat(std::move(term));
    }
    bool should_use_rdot(Term *t) {
        switch (t->type()) {
        case Term::VAR:
        case Term::DATUM:
            return false;
        default:
            return true;
        }
    }
    bool should_continue_string(Term *t) {
        switch (t->type()) {
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
            return false;
        case Term::FUNCALL:
            return t->args_size() == 2;
        default:
            return true;
        }
    }
    counted_t<const document_t> var_name(Datum *d) {
        guarantee(d->type() == Datum::R_NUM);
        return make_text("var" + std::to_string(lrint(d->r_num())));
    }
    counted_t<const document_t> to_js_func(Term *t) {
        guarantee(t->type() == Term::FUNC);
        guarantee(t->args_size() >= 2);
        counted_t<const document_t> arglist;
        if (t->mutable_args(0)->type() == Term::MAKE_ARRAY) {
            Term *args_term = t->mutable_args(0);
            std::vector<counted_t<const document_t> > args;
            for (int i = 0; i < args_term->args_size(); ++i) {
                if (i != 0) {
                    args.push_back(comma);
                    args.push_back(br);
                }
                Term *arg_term = args_term->mutable_args(i);
                guarantee(arg_term->type() == Term::DATUM);
                guarantee(arg_term->mutable_datum()->type() == Datum::R_NUM);
                args.push_back(var_name(arg_term->mutable_datum()));
            }
            arglist = make_concat(lparen,
                                  make_nest(make_concat(std::move(args))),
                                  rparen);
        } else if (t->mutable_args(0)->type() == Term::DATUM &&
                   t->mutable_args(0)->mutable_datum()->type() == Datum::R_ARRAY) {
            Datum *arg_term = t->mutable_args(0)->mutable_datum();
            std::vector<counted_t<const document_t> > args;
            for (int i = 0; i < arg_term->r_array_size(); ++i) {
                if (i != 0) {
                    args.push_back(comma);
                    args.push_back(br);
                }
                args.push_back(var_name(arg_term->mutable_r_array(i)));
            }
            arglist = make_concat(lparen,
                                  make_nest(make_concat(std::move(args))),
                                  rparen);
        } else {
            arglist = visit_generic(t->mutable_args(0));
        }
        std::vector<counted_t<const document_t> > body;
        for (int i = 1; i < t->args_size(); ++i) {
            if (i != 1) body.push_back(br);
            if (i == t->args_size() - 1) {
                body.push_back(make_concat(return_st,
                                           sp,
                                           make_nest(visit_generic(t->mutable_args(i))),
                                           semicolon));
            } else {
                body.push_back(make_nest(visit_generic(t->mutable_args(i))));
                body.push_back(semicolon);
            }
        }
        return make_concat(lambda_1,
                           make_nest(make_concat(lambda_2,
                                                 sp,
                                                 std::move(arglist),
                                                 sp,
                                                 lbrace,
                                                 cr,
                                                 make_concat(std::move(body)))),
                           cr,
                           rbrace);
    }

    static counted_t<const document_t> lparen, rparen, lbrack, rbrack, lbrace, rbrace;
    static counted_t<const document_t> colon, quote, sp, justdot, dotdotdot, comma, semicolon;
    static counted_t<const document_t> nil, true_v, false_v, r_st, json, row, do_st, return_st, cond, lambda_1, lambda_2;

    const unsigned int MAX_DEPTH = 15;
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
counted_t<const document_t> js_pretty_printer_t::true_v = make_text("true");
counted_t<const document_t> js_pretty_printer_t::false_v = make_text("false");
counted_t<const document_t> js_pretty_printer_t::sp = make_text(" ");
counted_t<const document_t> js_pretty_printer_t::quote = make_text("\"");
counted_t<const document_t> js_pretty_printer_t::r_st = make_text("r");
counted_t<const document_t> js_pretty_printer_t::json = make_text("json");
counted_t<const document_t> js_pretty_printer_t::row = make_text("row");
counted_t<const document_t> js_pretty_printer_t::return_st = make_text("return");
counted_t<const document_t> js_pretty_printer_t::do_st = make_text("do");
counted_t<const document_t> js_pretty_printer_t::cond = make_text("cond");
counted_t<const document_t> js_pretty_printer_t::lambda_1 = make_text("func");
counted_t<const document_t> js_pretty_printer_t::lambda_2 = make_text("tion");

counted_t<const document_t> render_as_javascript(Term *t) {
    return js_pretty_printer_t().walk(t);
}

} // namespace pprint

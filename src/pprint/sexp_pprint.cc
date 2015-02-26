#include "pprint/sexp_pprint.hpp"

#include <math.h>

#include <vector>
#include <memory>

#include "pprint/generic_term_walker.hpp"
#include "rdb_protocol/ql2_extensions.pb.h"

namespace pprint {

// we know what we're doing here, and I don't think 169 random
// Term types is going to clarify anything.
#pragma GCC diagnostic ignored "-Wswitch-enum"

class sexp_pretty_printer_t
    : public generic_term_walker_t<counted_t<const document_t> > {
    unsigned int depth;
    typedef std::vector<counted_t<const document_t> > v;
public:
    sexp_pretty_printer_t() : depth(0) {}
protected:
    counted_t<const document_t> visit_generic(const Term &t) override {
        ++depth;
        if (depth > MAX_DEPTH) return dotdotdot; // Crude attempt to avoid blowing stack
        counted_t<const document_t> doc;
        switch (t.type()) {
        case Term::DATUM:
            doc = to_lisp_datum(t.datum());
            break;
        case Term::BRACKET:
            doc = string_gets_together(t);
            break;
        case Term::BRANCH:
            doc = string_branches_together(t);
            break;
        case Term::FUNC:
            doc = to_lisp_func(t);
            break;
        case Term::VAR:
            guarantee(t.args_size() == 1);
            guarantee(t.args(0).type() == Term::DATUM);
            doc = var_name(t.args(0).datum());
            break;
        default:
        {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lparen);
            term.push_back(make_text(to_lisp_name(t)));
            if (t.args_size() > 0 || t.optargs_size() > 0) {
                term.push_back(sp);
                std::vector<counted_t<const document_t> > args;
                for (int i = 0; i < t.args_size(); ++i) {
                    // don't insert redundant space
                    if (args.size() != 0) args.push_back(cond_linebreak);
                    args.push_back(visit_generic(t.args(i)));
                }
                for (int i = 0; i < t.optargs_size(); ++i) {
                    // don't insert redundant space
                    if (args.size() != 0) args.push_back(cond_linebreak);
                    const Term_AssocPair &ap = t.optargs(i);
                    args.push_back(make_text(":" + to_lisp_name(ap.key())));
                    args.push_back(cond_linebreak);
                    args.push_back(visit_generic(ap.val()));
                }
                term.push_back(make_nest(make_concat(std::move(args))));
            }
            term.push_back(rparen);
            doc = make_nest(make_concat(std::move(term)));
            break;
        }
        }
        --depth;
        return doc;
    }
private:
    std::string to_lisp_name(const Term &t) {
        return to_lisp_name(Term_TermType_Name(t.type()));
    }
    std::string to_lisp_name(std::string s) {
        for (char &c : s) {
            c = tolower(c);
            if (c == '_') c = '-';
        }
        return s;
    }
    // Largely follow Clojure syntax here rather than CL, mostly
    // because it's easier to read.
    counted_t<const document_t> to_lisp_datum(const Datum &d) {
        switch (d.type()) {
        case Datum::R_NULL:
            return nil;
        case Datum::R_BOOL:
            return d.r_bool() ? true_v : false_v;
        case Datum::R_NUM:
        {
            double num = d.r_num();
            return make_text((fabs(num - trunc(num)) < 1e-10)
                             ? std::to_string(lrint(num))
                             : std::to_string(num));
        }
        case Datum::R_STR:
            return make_text("\"" + d.r_str() + "\"");
        case Datum::R_ARRAY:
        {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lbrack);
            for (int i = 0; i < d.r_array_size(); ++i) {
                if (i != 0) term.push_back(cond_linebreak);
                term.push_back(to_lisp_datum(d.r_array(i)));
            }
            term.push_back(rbrack);
            return make_nest(make_concat(std::move(term)));
        }
        case Datum::R_OBJECT:
        {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lbrace);
            for (int i = 0; i < d.r_object_size(); ++i) {
                if (i != 0) term.push_back(cond_linebreak);
                const Datum_AssocPair &ap = d.r_object(i);
                term.push_back(colon);
                term.push_back(make_text(ap.key()));
                term.push_back(sp);
                term.push_back(to_lisp_datum(ap.val()));
            }
            term.push_back(rbrace);
            return make_nest(make_concat(std::move(term)));
        }
        case Datum::R_JSON:
            return make_concat({lparen, json, cond_linebreak,
                                quote, make_text(d.r_str()), quote,
                                rparen});
        default:
            unreachable();
        }
    }
    counted_t<const document_t> string_gets_together(const Term &t) {
        std::vector<counted_t<const document_t> > term;
        term.push_back(lparen);
        term.push_back(dotdot);
        guarantee(t.args_size() == 2);
        term.push_back(sp);
        std::vector<counted_t<const document_t> > nest;
        const Term *var = &(t.args(0));
        if (should_continue_string(var->type())) {
            std::vector<counted_t<const document_t> > stack;
            while (should_continue_string(var->type())) {
                guarantee(var->args_size() == 2);
                stack.push_back(visit_stringing(var->type(), var->args(1)));
                stack.push_back(cond_linebreak);
                var = &(var->args(0));
            }
            stack.push_back(visit_generic(*var));
            nest.insert(nest.end(), stack.rbegin(), stack.rend());
        } else {
            nest.push_back(visit_generic(*var));
            nest.push_back(cond_linebreak);
            nest.push_back(visit_generic(t.args(1)));
        }
        term.push_back(make_nest(make_concat(std::move(nest))));
        term.push_back(rparen);
        return make_concat(std::move(term));
    }
    counted_t<const document_t> string_branches_together(const Term &t) {
        std::vector<counted_t<const document_t> > term;
        term.push_back(lparen);
        term.push_back(cond);
        guarantee(t.args_size() == 3);
        term.push_back(sp);
        std::vector<counted_t<const document_t> > branches;
        const Term *var = &t;
        while (var->type() == Term::BRANCH) {
            std::vector<counted_t<const document_t> > branch;
            branch.push_back(lparen);
            branch.push_back(make_nest(make_concat({visit_generic(var->args(0)),
                                                    cond_linebreak,
                                                    visit_generic(var->args(1))})));
            branch.push_back(rparen);
            branches.push_back(make_concat(std::move(branch)));
            branches.push_back(cond_linebreak);
            var = &(var->args(2));
        }
        branches.push_back(make_concat({lparen,
                                        make_nest(make_concat({true_v,
                                                               cond_linebreak,
                                                               visit_generic(*var)})),
                                        rparen}));
        term.push_back(make_nest(make_concat(std::move(branches))));
        term.push_back(rparen);
        return make_concat(std::move(term));
    }
    bool should_continue_string(Term_TermType type) {
        switch (type) {
        case Term::BRACKET:
        case Term::TABLE:
        case Term::GET:
        case Term::GET_FIELD:
            return true;
        default:
            return false;
        }
    }
    counted_t<const document_t> visit_stringing(Term_TermType type, const Term &t) {
        switch (type) {
        case Term::TABLE:
            return make_concat({lparen, table, sp, visit_generic(t), rparen});
        case Term::GET:
        case Term::GET_FIELD:
            return make_concat({lparen, get, sp, visit_generic(t), rparen});
        default:
            return visit_generic(t);
        }
    }
    counted_t<const document_t> var_name(const Datum &d) {
        guarantee(d.type() == Datum::R_NUM);
        return make_text("var" + std::to_string(lrint(d.r_num())));
    }
    counted_t<const document_t> to_lisp_func(const Term &t) {
        guarantee(t.type() == Term::FUNC);
        guarantee(t.args_size() >= 2);
        std::vector<counted_t<const document_t> > nest;
        if (t.args(0).type() == Term::MAKE_ARRAY) {
            const Term &args_term = t.args(0);
            std::vector<counted_t<const document_t> > args;
            for (int i = 0; i < args_term.args_size(); ++i) {
                if (i != 0) args.push_back(cond_linebreak);
                const Term &arg_term = args_term.args(i);
                guarantee(arg_term.type() == Term::DATUM);
                guarantee(arg_term.datum().type() == Datum::R_NUM);
                args.push_back(var_name(arg_term.datum()));
            }
            nest.push_back(make_concat({lparen,
                                        make_nest(make_concat(std::move(args))),
                                        rparen}));
        } else if (t.args(0).type() == Term::DATUM &&
                   t.args(0).datum().type() == Datum::R_ARRAY) {
            const Datum &arg_term = t.args(0).datum();
            std::vector<counted_t<const document_t> > args;
            for (int i = 0; i < arg_term.r_array_size(); ++i) {
                if (i != 0) args.push_back(cond_linebreak);
                args.push_back(var_name(arg_term.r_array(i)));
            }
            nest.push_back(make_concat({lparen,
                                        make_nest(make_concat(std::move(args))),
                                        rparen}));
        } else {
            nest.push_back(visit_generic(t.args(0)));
        }
        for (int i = 1; i < t.args_size(); ++i) {
            nest.push_back(cond_linebreak);
            nest.push_back(visit_generic(t.args(i)));
        }
        return make_concat({lparen, lambda, sp, make_nest(make_concat(std::move(nest))),
                            rparen});
    }

    static counted_t<const document_t> lparen, rparen, lbrack, rbrack, lbrace, rbrace;
    static counted_t<const document_t> colon, quote, sp, dotdot, dotdotdot;
    static counted_t<const document_t> nil, true_v, false_v, json, table, get, cond,
        lambda;

    static const unsigned int MAX_DEPTH = 20;
};

counted_t<const document_t> sexp_pretty_printer_t::lparen = make_text("(");
counted_t<const document_t> sexp_pretty_printer_t::rparen = make_text(")");
counted_t<const document_t> sexp_pretty_printer_t::lbrack = make_text("[");
counted_t<const document_t> sexp_pretty_printer_t::rbrack = make_text("]");
counted_t<const document_t> sexp_pretty_printer_t::lbrace = make_text("{");
counted_t<const document_t> sexp_pretty_printer_t::rbrace = make_text("}");
counted_t<const document_t> sexp_pretty_printer_t::colon = make_text(":");
counted_t<const document_t> sexp_pretty_printer_t::dotdot = make_text("..");
counted_t<const document_t> sexp_pretty_printer_t::dotdotdot = make_text("...");
counted_t<const document_t> sexp_pretty_printer_t::nil = make_text("nil");
counted_t<const document_t> sexp_pretty_printer_t::true_v = make_text("true");
counted_t<const document_t> sexp_pretty_printer_t::false_v = make_text("false");
counted_t<const document_t> sexp_pretty_printer_t::sp = make_text(" ");
counted_t<const document_t> sexp_pretty_printer_t::quote = make_text("\"");
counted_t<const document_t> sexp_pretty_printer_t::json = make_text("json");
counted_t<const document_t> sexp_pretty_printer_t::table = make_text("table");
counted_t<const document_t> sexp_pretty_printer_t::get = make_text("get");
counted_t<const document_t> sexp_pretty_printer_t::cond = make_text("cond");
counted_t<const document_t> sexp_pretty_printer_t::lambda = make_text("fn"); // follow Clojure here

counted_t<const document_t> render_as_sexp(const Term &t) {
    return sexp_pretty_printer_t().walk(t);
}

} // namespace pprint

// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "pprint/sexp_pprint.hpp"

#include <math.h>

#include <vector>
#include <memory>

#include "pprint/generic_term_walker.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/term_storage.hpp"
#include "rdb_protocol/pseudo_binary.hpp"

namespace pprint {

class sexp_pretty_printer_t
    : public generic_term_walker_t<counted_t<const document_t> > {
    unsigned int depth;
    typedef std::vector<counted_t<const document_t> > v;
public:
    sexp_pretty_printer_t() : depth(0) {}
protected:
    counted_t<const document_t> visit_generic(const ql::raw_term_t &t) override {
        ++depth;
        if (depth > MAX_DEPTH) return dotdotdot; // Crude attempt to avoid blowing stack
        counted_t<const document_t> doc;
        switch (static_cast<int>(t.type())) {
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
        case Term::VAR: {
            r_sanity_check(t.num_args() == 1);
            ql::raw_term_t arg0 = t.arg(0);
            r_sanity_check(arg0.type() == Term::DATUM);
            doc = var_name(arg0.datum());
            break;
        }
        default: {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lparen);
            term.push_back(make_text(to_lisp_name(t)));
            if (t.num_args() > 0 || t.num_optargs() > 0) {
                term.push_back(sp);
                std::vector<counted_t<const document_t> > args;
                for (size_t i = 0; i < t.num_args(); ++i) {
                    ql::raw_term_t item = t.arg(i);
                    if (!args.empty()) args.push_back(cond_linebreak);
                    args.push_back(visit_generic(item));
                }
                t.each_optarg([&](const ql::raw_term_t &item, const std::string &name) {
                        if (!args.empty()) args.push_back(cond_linebreak);
                        args.push_back(make_text(strprintf(
                            ":%s", to_lisp_name(name).c_str())));
                        args.push_back(cond_linebreak);
                        args.push_back(visit_generic(item));
                    });
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
    std::string to_lisp_name(const ql::raw_term_t &t) {
        return to_lisp_name(Term::TermType_Name(t.type()));
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
    counted_t<const document_t> to_lisp_datum(const ql::datum_t &d) {
        switch (d.get_type()) {
        case ql::datum_t::type_t::MINVAL:
            return minval;
        case ql::datum_t::type_t::MAXVAL:
            return maxval;
        case ql::datum_t::type_t::R_NULL:
            return nil;
        case ql::datum_t::type_t::R_BOOL:
            return d.as_bool() ? true_v : false_v;
        case ql::datum_t::type_t::R_NUM:
            return make_text((fabs(d.as_num() - trunc(d.as_num())) < 1e-10)
                             ? std::to_string(lrint(d.as_num()))
                             : std::to_string(d.as_num()));
        case ql::datum_t::type_t::R_BINARY: {
            // This prints the object straight, no linebreaks for each key
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            ql::pseudo::encode_base64_ptype(d.as_binary(), &writer);
            return make_text(std::string(buffer.GetString(), buffer.GetSize()));
        }
        case ql::datum_t::type_t::R_STR:
            return make_text(strprintf("\"%s\"", d.as_str().to_std().c_str()));
        case ql::datum_t::type_t::R_ARRAY: {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lbrack);
            for (size_t i = 0; i < d.arr_size(); ++i) {
                if (i != 0) term.push_back(cond_linebreak);
                term.push_back(to_lisp_datum(d.get(i)));
            }
            term.push_back(rbrack);
            return make_nest(make_concat(std::move(term)));
        }
        case ql::datum_t::type_t::R_OBJECT: {
            std::vector<counted_t<const document_t> > term;
            term.push_back(lbrace);
            for (size_t i = 0; i < d.obj_size(); ++i) {
                if (i != 0) term.push_back(cond_linebreak);
                auto pair = d.get_pair(i);
                term.push_back(colon);
                term.push_back(make_text(pair.first.to_std()));
                term.push_back(sp);
                term.push_back(to_lisp_datum(pair.second));
            }
            term.push_back(rbrace);
            return make_nest(make_concat(std::move(term)));
        }
        case ql::datum_t::type_t::UNINITIALIZED:
        default:
            unreachable();
        }
    }

    counted_t<const document_t> string_gets_together(const ql::raw_term_t &t) {
        std::vector<counted_t<const document_t> > term;
        term.push_back(lparen);
        term.push_back(dotdot);
        r_sanity_check(t.num_args() == 2);
        term.push_back(sp);
        std::vector<counted_t<const document_t> > nest;
        ql::raw_term_t arg0 = t.arg(0);
        if (should_continue_string(arg0.type())) {
            std::vector<counted_t<const document_t> > stack;
            while (should_continue_string(arg0.type())) {
                r_sanity_check(arg0.num_args() == 2);
                ql::raw_term_t next_arg = arg0.arg(0);
                stack.push_back(visit_stringing(arg0.type(), arg0.arg(1)));
                stack.push_back(cond_linebreak);
                arg0 = next_arg;
            }
            stack.push_back(visit_generic(arg0));
            nest.insert(nest.end(), stack.rbegin(), stack.rend());
        } else {
            nest.push_back(visit_generic(arg0));
            nest.push_back(cond_linebreak);
            nest.push_back(visit_generic(t.arg(1)));
        }
        term.push_back(make_nest(make_concat(std::move(nest))));
        term.push_back(rparen);
        return make_concat(std::move(term));
    }

    counted_t<const document_t> string_branches_together(const ql::raw_term_t &t) {
        r_sanity_check(t.num_args() == 3);

        std::vector<counted_t<const document_t> > term;
        term.push_back(lparen);
        term.push_back(cond);
        term.push_back(sp);

        std::vector<counted_t<const document_t> > branches;
        ql::raw_term_t var = t;
        while (var.type() == Term::BRANCH) {
            std::vector<counted_t<const document_t> > branch;
            branch.push_back(lparen);
            branch.push_back(make_nest(make_concat({visit_generic(var.arg(0)),
                                                    cond_linebreak,
                                                    visit_generic(var.arg(1))})));
            branch.push_back(rparen);
            branches.push_back(make_concat(std::move(branch)));
            branches.push_back(cond_linebreak);
            var = var.arg(2);
        }
        branches.push_back(make_concat({lparen,
                                        make_nest(make_concat({true_v,
                                                               cond_linebreak,
                                                               visit_generic(var)})),
                                        rparen}));
        term.push_back(make_nest(make_concat(std::move(branches))));
        term.push_back(rparen);
        return make_concat(std::move(term));
    }

    bool should_continue_string(Term::TermType type) {
        switch (static_cast<int>(type)) {
        case Term::BRACKET:
        case Term::TABLE:
        case Term::GET:
        case Term::GET_FIELD:
            return true;
        default:
            return false;
        }
    }

    counted_t<const document_t> visit_stringing(Term::TermType type,
                                                const ql::raw_term_t &t) {
        switch (static_cast<int>(type)) {
        case Term::TABLE:
            return make_concat({lparen, table, sp, visit_generic(t), rparen});
        case Term::GET:
        case Term::GET_FIELD:
            return make_concat({lparen, get, sp, visit_generic(t), rparen});
        default:
            return visit_generic(t);
        }
    }

    counted_t<const document_t> var_name(const ql::datum_t &d) {
        r_sanity_check(d.get_type() == ql::datum_t::type_t::R_NUM);
        return make_text("var" + std::to_string(lrint(d.as_num())));
    }

    counted_t<const document_t> to_lisp_func(const ql::raw_term_t &t) {
        r_sanity_check(t.type() == Term::FUNC);
        r_sanity_check(t.num_args() >= 2);
        std::vector<counted_t<const document_t> > nest;
        ql::raw_term_t arg0 = t.arg(0);
        if (arg0.type() == Term::MAKE_ARRAY) {
            std::vector<counted_t<const document_t> > args;
            for (size_t i = 0; i < arg0.num_args(); ++i) {
                ql::raw_term_t item = arg0.arg(i);
                if (!args.empty()) args.push_back(cond_linebreak);
                r_sanity_check(item.type() == Term::DATUM);
                ql::datum_t d = item.datum();
                r_sanity_check(d.get_type() == ql::datum_t::type_t::R_NUM);
                args.push_back(var_name(d));
            }
            nest.push_back(make_concat({lparen,
                                        make_nest(make_concat(std::move(args))),
                                        rparen}));
        } else if (arg0.type() == Term::DATUM &&
                   arg0.datum().get_type() == ql::datum_t::type_t::R_ARRAY) {
            ql::datum_t d = arg0.datum();
            std::vector<counted_t<const document_t> > args;
            for (size_t i = 0; i < d.arr_size(); ++i) {
                if (i != 0) args.push_back(cond_linebreak);
                args.push_back(var_name(d.get(i)));
            }
            nest.push_back(make_concat({lparen,
                                        make_nest(make_concat(std::move(args))),
                                        rparen}));
        } else {
            nest.push_back(visit_generic(arg0));
        }
        for (size_t i = 1; i < t.num_args(); ++i) {
            nest.push_back(cond_linebreak);
            nest.push_back(visit_generic(t.arg(i)));
        }
        return make_concat({lparen, lambda, sp, make_nest(make_concat(std::move(nest))),
                            rparen});
    }

    static counted_t<const document_t> lparen, rparen, lbrack, rbrack, lbrace, rbrace;
    static counted_t<const document_t> colon, quote, sp, dotdot, dotdotdot;
    static counted_t<const document_t> nil, minval, maxval, true_v, false_v;
    static counted_t<const document_t> json, table, get, cond, lambda;

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
counted_t<const document_t> sexp_pretty_printer_t::minval = make_text("minval");
counted_t<const document_t> sexp_pretty_printer_t::maxval = make_text("maxval");
counted_t<const document_t> sexp_pretty_printer_t::true_v = make_text("true");
counted_t<const document_t> sexp_pretty_printer_t::false_v = make_text("false");
counted_t<const document_t> sexp_pretty_printer_t::sp = make_text(" ");
counted_t<const document_t> sexp_pretty_printer_t::quote = make_text("\"");
counted_t<const document_t> sexp_pretty_printer_t::json = make_text("json");
counted_t<const document_t> sexp_pretty_printer_t::table = make_text("table");
counted_t<const document_t> sexp_pretty_printer_t::get = make_text("get");
counted_t<const document_t> sexp_pretty_printer_t::cond = make_text("cond");
counted_t<const document_t> sexp_pretty_printer_t::lambda =
    make_text("fn"); // follow Clojure here

counted_t<const document_t> render_as_sexp(const ql::raw_term_t &t) {
    return sexp_pretty_printer_t().walk(t);
}

} // namespace pprint

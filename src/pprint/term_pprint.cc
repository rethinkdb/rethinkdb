#include "pprint/pprint.hpp"

#include <vector>

#include "rdb_protocol/ql2_extensions.pb.h"

namespace pprint {

template <typename Accumulator>
class generic_term_walker_t {
public:
    virtual ~generic_term_walker_t() {}

    virtual Accumulator &&walk(Term *t) {
        return std::move(visit_generic(t));
    }
protected:
    virtual Accumulator &&visit_generic(Term *t) = 0;
};

class sexp_pretty_printer_t
    : public generic_term_walker_t<counted_t<const document_t> > {
protected:
    virtual counted_t<const document_t> &&visit_generic(Term *t) {
        static counted_t<const document_t> lparen = make_text("(");
        static counted_t<const document_t> rparen = make_text(")");
        static counted_t<const document_t> space = make_text(" ");
        std::vector<counted_t<const document_t> > term;
        term.push_back(lparen);
        term.push_back(make_text(Term_TermType_Name(t->type())));
        if (t->args_size() > 0 || t->optargs_size() > 0) {
            term.push_back(space);
            std::vector<counted_t<const document_t> > args;
            for (int i = 0; i < t->args_size(); ++i) {
                // don't insert redundant space
                if (args.size() != 0) args.push_back(br);
                args.push_back(visit_generic(t->mutable_args(i)));
            }
            for (int i = 0; i < t->optargs_size(); ++i) {
                // don't insert redundant space
                if (args.size() != 0) args.push_back(br);
                Term_AssocPair *ap = t->mutable_optargs(i);
                args.push_back(make_text(":" + ap->key()));
                args.push_back(space); // nonbreaking; don't separate key from value
                args.push_back(visit_generic(ap->mutable_val()));
            }
            term.push_back(make_nest(make_concat(std::move(args))));
        }
        term.push_back(rparen);
        return std::move(make_nest(make_concat(std::move(term))));
    }
};

counted_t<const document_t> render_as_sexp(Term *t) {
    return sexp_pretty_printer_t().walk(t);
}

} // namespace pprint

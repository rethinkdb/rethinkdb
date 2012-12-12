#include "rdb_protocol/ql2.hpp"

namespace ql {

class predicate_term_t : public simple_op_term_t {
public:
    predicate_term_t(env_t *env, const Term2 *term)
        : simple_op_term_t(env, term), namestr(0), invert(false), pred(0) {
        int predtype = term->type();
        switch(predtype) {
        case Term2_TermType_EQ: {
            namestr = "EQ";
            pred = &datum_t::operator==;
        }; break;
        case Term2_TermType_NE: {
            namestr = "NE";
            pred = &datum_t::operator==;
            invert = true; // we invert the == operator so (!= 1 2 3) makes sense
        }; break;
        case Term2_TermType_LT: {
            namestr = "LT";
            pred = &datum_t::operator<;
        }; break;
        case Term2_TermType_LE: {
            namestr = "LE";
            pred = &datum_t::operator<=;
        }; break;
        case Term2_TermType_GT: {
            namestr = "GT";
            pred = &datum_t::operator>;
        }; break;
        case Term2_TermType_GE: {
            namestr = "GE";
            pred = &datum_t::operator>=;
        }; break;
        default: unreachable();
        }
        guarantee(namestr && pred);
    }
    virtual val_t *simple_call_impl(std::vector<val_t *> *args) {
        const datum_t *lhs, *rhs;
        runtime_check(args->size() >= 2,
                      strprintf("Predicate %s requires at least 2 arguments.", name()));
        lhs = (*args)[0]->as_datum();
        for (size_t i = 1; i < args->size(); ++i) {
            rhs = (*args)[i]->as_datum();
            if (!(lhs->*pred)(*rhs)) return new_val(bool(false ^ invert));
            lhs = rhs;
        }
        return new_val(bool(true ^ invert));
    }
    virtual const char *name() const { return namestr; }
private:
    const char *namestr;
    bool invert;
    bool (datum_t::*pred)(const datum_t &rhs) const;
};

} //namespace ql

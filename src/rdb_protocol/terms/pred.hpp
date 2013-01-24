#include "rdb_protocol/ql2.hpp"

namespace ql {

class predicate_term_t : public op_term_t {
public:
    predicate_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(2, -1)), namestr(0), invert(false), pred(0) {
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
private:
    virtual val_t *eval_impl() {
        const datum_t *lhs, *rhs;
        lhs = arg(0)->as_datum();
        for (size_t i = 1; i < num_args(); ++i) {
            rhs = arg(i)->as_datum();
            if (!(lhs->*pred)(*rhs)) return new_val_bool(bool(false ^ invert));
            lhs = rhs;
        }
        return new_val_bool(bool(true ^ invert));
    }
    const char *namestr;
    virtual const char *name() const { return namestr; }
    bool invert;
    bool (datum_t::*pred)(const datum_t &rhs) const;
};

class not_term_t : public op_term_t {
public:
    not_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() { return new_val_bool(!arg(0)->as_datum()->as_bool()); }
    RDB_NAME("not")
};

} //namespace ql

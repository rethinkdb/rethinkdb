#ifndef RDB_PROTOCOL_TERMS_SORT_HPP_
#define RDB_PROTOCOL_TERMS_SORT_HPP_

#include <string>

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class orderby_term_t : public op_term_t {
public:
    orderby_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    class lt_cmp_t {
    public:
        explicit lt_cmp_t(const datum_t *_attrs) : attrs(_attrs) { }
        bool operator()(const datum_t *l, const datum_t *r) {
            for (size_t i = 0; i < attrs->size(); ++i) {
                std::string attrname = attrs->el(i)->as_str();
                bool invert = (attrname[0] == '-');
                if (attrname[0] == '-' || attrname[0] == '+') attrname.erase(0, 1);
                const datum_t *lattr = l->el(attrname, NOTHROW);
                const datum_t *rattr = r->el(attrname, NOTHROW);
                if (!lattr && !rattr) continue;
                if (!lattr) return static_cast<bool>(true ^ invert);
                if (!rattr) return static_cast<bool>(false ^ invert);
                // TODO: use datum_t::cmp instead to be faster
                if (*lattr == *rattr) continue;
                return static_cast<bool>((*lattr < *rattr) ^ invert);
            }
            return false;
        }
    private:
        const datum_t *attrs;
    };

    virtual val_t *eval_impl() {
        datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 1; i < num_args(); ++i) {
            arr->add(arg(i)->as_datum());
        }
        lt_cmp_t lt_cmp(arr);
        // We can't have datum_stream_t::sort because templates suck.
        datum_stream_t *s = new sort_datum_stream_t<lt_cmp_t>(
            env, lt_cmp, arg(0)->as_seq(), this);
        return new_val(s);
    }
    RDB_NAME("orderby");
};

class distinct_term_t : public op_term_t {
public:
    distinct_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    static bool lt_cmp(const datum_t *l, const datum_t *r) { return *l < *r; }
    virtual val_t *eval_impl() {
        datum_stream_t *s =
            new sort_datum_stream_t<bool (*)(const datum_t *, const datum_t *)>(
                env, lt_cmp, arg(0)->as_seq(), this);
        datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
        const datum_t *last = 0;
        while (const datum_t *d = s->next()) {
            if (last && *last == *d) continue;
            arr->add(last = d);
        }
        return new_val(new array_datum_stream_t(env, arr, this));
    }
    RDB_NAME("distinct");
};

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_SORT_HPP_

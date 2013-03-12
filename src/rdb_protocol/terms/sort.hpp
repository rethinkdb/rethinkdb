#ifndef RDB_PROTOCOL_TERMS_SORT_HPP_
#define RDB_PROTOCOL_TERMS_SORT_HPP_

#include <string>
#include <utility>

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

// NOTE: `asc` and `desc` don't fit into our type system (they're a hack for
// orderby to avoid string parsing), so we instead literally examine the
// protobuf to determine whether they're present.  This is a hack.  (This is
// implemented internally in terms of string concatenation because that's what
// the old string-parsing solution was, and this was a quick way to get the new
// behavior that also avoided dumping already-tested code paths.  I'm probably
// going to hell for it though.)

class asc_term_t : public op_term_t {
public:
    asc_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        return new_val("+" + arg(0)->as_str());
    }
    virtual const char *name() const { return "asc"; }
};

class desc_term_t : public op_term_t {
public:
    desc_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        return new_val("-" + arg(0)->as_str());
    }
    virtual const char *name() const { return "desc"; }
};

class orderby_term_t : public op_term_t {
public:
    orderby_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(1, -1)), src_term(term) { }
private:
    class lt_cmp_t {
    public:
        explicit lt_cmp_t(const datum_t *_attrs) : attrs(_attrs) { }
        bool operator()(const datum_t *l, const datum_t *r) {
            for (size_t i = 0; i < attrs->size(); ++i) {
                std::string attrname = attrs->el(i)->as_str();
                bool invert = (attrname[0] == '-');
                r_sanity_check(attrname[0] == '-' || attrname[0] == '+');
                attrname.erase(0, 1);
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
            Term::TermType type = src_term->args(i).type();
            if (type != Term::ASC && type != Term::DESC) {
                arr->add(new_val("+" + arg(i)->as_str())->as_datum());
            } else {
                arr->add(arg(i)->as_datum());
            }
        }
        lt_cmp_t lt_cmp(arr);
        // We can't have datum_stream_t::sort because templates suck.

        table_t *tbl = 0;
        datum_stream_t *seq = 0;
        val_t *v0 = arg(0);
        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<table_t *, datum_stream_t *> ts = v0->as_selection();
            tbl = ts.first;
            seq = ts.second;
        } else {
            seq = v0->as_seq();
        }
        datum_stream_t *s = new sort_datum_stream_t<lt_cmp_t>(
            env, lt_cmp, seq, this);
        return tbl ? new_val(tbl, s) : new_val(s);
    }
    virtual const char *name() const { return "orderby"; }

private:
    const Term *src_term;
};

class distinct_term_t : public op_term_t {
public:
    distinct_term_t(env_t *env, const Term *term)
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
    virtual const char *name() const { return "distinct"; }
};

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_SORT_HPP_

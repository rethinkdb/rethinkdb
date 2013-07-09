#include "rdb_protocol/terms/terms.hpp"

#include <string>
#include <utility>

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/error.hpp"
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
    asc_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        return new_val(make_counted<const datum_t>("+" + arg(0)->as_str()));
    }
    virtual const char *name() const { return "asc"; }
};

class desc_term_t : public op_term_t {
public:
    desc_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> arg0 = arg(0);
        return new_val(make_counted<const datum_t>("-" + arg(0)->as_str()));
    }
    virtual const char *name() const { return "desc"; }
};

class orderby_term_t : public op_term_t {
public:
    orderby_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1, -1)), src_term(term) { }
private:
    class lt_cmp_t {
    public:
        lt_cmp_t(std::vector<counted_t<val_t> > _comparisons,
                 term_t *_creator)
            : comparisons(_comparisons), creator(_creator) { }
        bool operator()(counted_t<const datum_t> l, counted_t<const datum_t> r) {
            for (auto it = comparisons.begin(); it != comparisons.end(); ++it) {
                counted_t<const datum_t> lval;
                counted_t<const datum_t> rval;
                if ((*it)->get_type().is_convertible(val_t::type_t::DATUM)) {
                    std::string attrname = (*it)->as_str();
                    lval = l->get(attrname, NOTHROW);
                    rval = r->get(attrname, NOTHROW);
                } else if ((*it)->get_type().is_convertible(val_t::type_t::FUNC)) {
                    lval = (*it)->as_func()->call(l)->as_datum();
                    rval = (*it)->as_func()->call(r)->as_datum();
                } else {
                    rfail_target(creator, base_exc_t::GENERIC,
                          "Must pass either DATUM or FUNCTION to %s\n", creator->name());
                }

                if (!lval.has() && !rval.has()) {
                    continue;
                }
                if (!lval.has()) {
                    return static_cast<bool>(true);
                }
                if (!rval.has()) {
                    return static_cast<bool>(false);
                }
                // TODO: use datum_t::cmp instead to be faster
                if (*lval == *rval) {
                    continue;
                }
                return static_cast<bool>((*lval < *rval));
            }
            return false;
        }
    private:
        std::vector<counted_t<val_t> > comparisons;
        term_t *creator;
    };

    virtual counted_t<val_t> eval_impl() {
        std::vector<counted_t<val_t> > comparisons;
        scoped_ptr_t<datum_t> arr(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 1; i < num_args(); ++i) {
            comparisons.push_back(arg(i));
        }
        lt_cmp_t lt_cmp(comparisons, this);
        // We can't have datum_stream_t::sort because templates suck.

        counted_t<table_t> tbl;
        counted_t<datum_stream_t> seq;
        counted_t<val_t> v0 = arg(0);
        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > ts
                = v0->as_selection();
            tbl = ts.first;
            seq = ts.second;
        } else {
            seq = v0->as_seq();
        }
        counted_t<datum_stream_t> s
            = make_counted<sort_datum_stream_t<lt_cmp_t> >(env, lt_cmp, seq, backtrace());
        return tbl.has() ? new_val(s, tbl) : new_val(s);
    }

    virtual const char *name() const { return "orderby"; }

private:
    protob_t<const Term> src_term;
};

class distinct_term_t : public op_term_t {
public:
    distinct_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    static bool lt_cmp(counted_t<const datum_t> l, counted_t<const datum_t> r) { return *l < *r; }
    virtual counted_t<val_t> eval_impl() {
        scoped_ptr_t<datum_stream_t> s(new sort_datum_stream_t<bool (*)(counted_t<const datum_t>, counted_t<const datum_t>)>(env, lt_cmp, arg(0)->as_seq(), backtrace()));
        scoped_ptr_t<datum_t> arr(new datum_t(datum_t::R_ARRAY));
        counted_t<const datum_t> last;
        while (counted_t<const datum_t> d = s->next()) {
            if (last.has() && *last == *d) {
                continue;
            }
            last = d;
            arr->add(last);
        }
        counted_t<datum_stream_t> out
            = make_counted<array_datum_stream_t>(env,
                                                 counted_t<const datum_t>(arr.release()),
                                                 backtrace());
        return new_val(out);
    }
    virtual const char *name() const { return "distinct"; }
};

counted_t<term_t> make_orderby_term(env_t *env, protob_t<const Term> term) {
    return make_counted<orderby_term_t>(env, term);
}
counted_t<term_t> make_distinct_term(env_t *env, protob_t<const Term> term) {
    return make_counted<distinct_term_t>(env, term);
}
counted_t<term_t> make_asc_term(env_t *env, protob_t<const Term> term) {
    return make_counted<asc_term_t>(env, term);
}
counted_t<term_t> make_desc_term(env_t *env, protob_t<const Term> term) {
    return make_counted<desc_term_t>(env, term);
}

} // namespace ql

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
        explicit lt_cmp_t(counted_t<const datum_t> _attrs) : attrs(_attrs) { }
        bool operator()(counted_t<const datum_t> l, counted_t<const datum_t> r) {
            for (size_t i = 0; i < attrs->size(); ++i) {
                std::string attrname = attrs->get(i)->as_str();
                bool invert = (attrname[0] == '-');
                r_sanity_check(attrname[0] == '-' || attrname[0] == '+');
                attrname.erase(0, 1);
                counted_t<const datum_t> lattr = l->get(attrname, NOTHROW);
                counted_t<const datum_t> rattr = r->get(attrname, NOTHROW);
                if (!lattr.has() && !rattr.has()) {
                    continue;
                }
                if (!lattr.has()) {
                    return static_cast<bool>(true ^ invert);
                }
                if (!rattr.has()) {
                    return static_cast<bool>(false ^ invert);
                }
                // TODO: use datum_t::cmp instead to be faster
                if (*lattr == *rattr) {
                    continue;
                }
                return static_cast<bool>((*lattr < *rattr) ^ invert);
            }
            return false;
        }
    private:
        counted_t<const datum_t> attrs;
    };

    virtual counted_t<val_t> eval_impl() {
        scoped_ptr_t<datum_t> arr(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 1; i < num_args(); ++i) {
            Term::TermType type = src_term->args(i).type();
            if (type != Term::ASC && type != Term::DESC) {
                auto datum = make_counted<const datum_t>("+" + arg(i)->as_str());
                arr->add(new_val(datum)->as_datum());
            } else {
                arr->add(arg(i)->as_datum());
            }
        }
        lt_cmp_t lt_cmp(counted_t<const datum_t>(arr.release()));
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

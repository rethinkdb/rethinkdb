#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

class append_term_t : public op_term_t {
public:
    append_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<const datum_t> arr = arg(0)->as_datum();
        counted_t<const datum_t> new_el = arg(1)->as_datum();
        arr->check_type(datum_t::R_ARRAY);
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        // TODO: this is horrendously inefficient.
        for (size_t i = 0; i < arr->size(); ++i) out->add(arr->get(i));
        out->add(new_el);
        return new_val(counted_t<const datum_t>(out.release()));
    }
    virtual const char *name() const { return "append"; }
};

// This gets the literal index of a (possibly negative) index relative to a
// fixed size.
size_t canonicalize(const term_t *t, int32_t index, size_t size, bool *oob_out = 0) {
    CT_ASSERT(sizeof(size_t) >= sizeof(int32_t));
    if (index >= 0) return index;
    if (size_t(index * -1) > size) {
        if (oob_out) {
            *oob_out = true;
        } else {
            rfail_target(t, base_exc_t::NON_EXISTENCE, "Index out of bounds: %d", index);
        }
        return 0;
    }
    return size + index;
}

class nth_term_t : public op_term_t {
public:
    nth_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(0);
        int32_t n = arg(1)->as_int<int32_t>();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> arr = v->as_datum();
            size_t real_n = canonicalize(this, n, arr->size());
            return new_val(arr->get(real_n));
        } else {
            counted_t<datum_stream_t> s = v->as_seq();
            rcheck(n >= -1,
                   base_exc_t::WELL_FORMEDNESS,
                   strprintf("Cannot use an index < -1 (%d) on a stream.", n));

            counted_t<const datum_t> last_d;
            for (int32_t i = 0; ; ++i) {
                counted_t<const datum_t> d = s->next();
                if (!d.has()) {
                    rcheck(n == -1 && last_d.has(),
                           base_exc_t::NOT_FOUND,
                           strprintf("Index out of bounds: %d", n));
                    return new_val(last_d);
                }
                if (i == n) return new_val(d);
                last_d = d;
                r_sanity_check(n == -1 || i < n);
            }
        }
    }
    virtual const char *name() const { return "nth"; }
};

// TODO: this kinda sucks.
class slice_term_t : public op_term_t {
public:
    slice_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(3)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(0);
        int32_t fake_l = arg(1)->as_int<int32_t>();
        int32_t fake_r = arg(2)->as_int<int32_t>();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> arr = v->as_datum();
            arr->check_type(datum_t::R_ARRAY);
            bool l_oob = false;
            size_t real_l = canonicalize(this, fake_l, arr->size(), &l_oob);
            if (l_oob) real_l = 0;
            bool r_oob = false;
            size_t real_r = canonicalize(this, fake_r, arr->size(), &r_oob);

            scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
            if (!r_oob) {
                for (size_t i = real_l; i <= real_r; ++i) {
                    if (i >= arr->size()) break;
                    out->add(arr->get(i));
                }
            }
            return new_val(counted_t<const datum_t>(out.release()));
        } else if (v->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            counted_t<table_t> t;
            counted_t<datum_stream_t> seq;
            if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
                std::pair<counted_t<table_t>, counted_t<datum_stream_t> > t_seq
                    = v->as_selection();
                t = t_seq.first;
                seq = t_seq.second;
            } else {
                seq = v->as_seq();
            }

            rcheck(fake_l >= 0, base_exc_t::WELL_FORMEDNESS,
                   "Cannot use a negative left index on a stream.");
            rcheck(fake_r >= -1, base_exc_t::WELL_FORMEDNESS,
                   "Cannot use a right index < -1 on a stream");
            counted_t<datum_stream_t> new_ds = seq->slice(fake_l, fake_r);
            return t.has() ? new_val(new_ds, t) : new_val(new_ds);
        }
        rcheck_typed_target(v, false, "Cannot slice non-sequences.");
        unreachable();
    }
    virtual const char *name() const { return "slice"; }
};

class limit_term_t : public op_term_t {
public:
    limit_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(0);
        counted_t<table_t> t;
        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            t = v->as_selection().first;
        }
        counted_t<datum_stream_t> ds = v->as_seq();
        int32_t r = arg(1)->as_int<int32_t>();
        rcheck(r >= 0, base_exc_t::NUMERIC_LIMIT,
               strprintf("LIMIT takes a non-negative argument (got %d)", r));
        counted_t<datum_stream_t> new_ds;
        if (r == 0) {
            new_ds = ds->slice(1, 0); // (0, -1) has a different meaning
        } else {
            new_ds = ds->slice(0, r-1); // note that both bounds are inclusive
        }
        return t.has() ? new_val(new_ds, t) : new_val(new_ds);
    }
    virtual const char *name() const { return "limit"; }
};


counted_t<term_t> make_append_term(env_t *env, protob_t<const Term> term) {
    return make_counted<append_term_t>(env, term);
}

counted_t<term_t> make_nth_term(env_t *env, protob_t<const Term> term) {
    return make_counted<nth_term_t>(env, term);
}

counted_t<term_t> make_slice_term(env_t *env, protob_t<const Term> term) {
    return make_counted<slice_term_t>(env, term);
}

counted_t<term_t> make_limit_term(env_t *env, protob_t<const Term> term) {
    return make_counted<limit_term_t>(env, term);
}

}  // namespace ql

#ifndef RDB_PROTOCOL_TERMS_ARR_HPP_
#define RDB_PROTOCOL_TERMS_ARR_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {
class append_term_t : public op_term_t {
public:
    append_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        const datum_t *arr    = arg(0)->as_datum();
        const datum_t *new_el = arg(1)->as_datum();
        arr->check_type(datum_t::R_ARRAY);
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        // TODO: this is horrendously inefficient.
        for (size_t i = 0; i < arr->size(); ++i) out->add(arr->el(i));
        out->add(new_el);
        return new_val(out.release());
    }
    RDB_NAME("append");
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
            rfail_target(t, "Index out of bounds: %d", index);
        }
        return 0;
    }
    return size + index;
}

class nth_term_t : public op_term_t {
public:
    nth_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v = arg(0);
        int32_t n = arg(1)->as_int<int32_t>();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *arr = v->as_datum();
            size_t real_n = canonicalize(this, n, arr->size());
            return new_val(arr->el(real_n));
        } else {
            datum_stream_t *s = v->as_seq();
            rcheck(n >= -1, strprintf("Cannot use an index < -1 (%d) on a stream.", n));

            const datum_t *last_d = 0;
            for (int32_t i = 0; ; ++i) {
                const datum_t *d = s->next();
                if (!d) {
                    rcheck(n == -1 && last_d, strprintf("Index out of bounds: %d", n));
                    return new_val(last_d);
                }
                if (i == n) return new_val(d);
                last_d = d;
                r_sanity_check(n == -1 || i < n);
            }
        }
    }
    RDB_NAME("nth");
};

// TODO: this kinda sucks.
class slice_term_t : public op_term_t {
public:
    slice_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(3)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v = arg(0);
        int32_t fake_l = arg(1)->as_int<int32_t>();
        int32_t fake_r = arg(2)->as_int<int32_t>();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *arr = v->as_datum();
            rcheck(arr->get_type() == datum_t::R_ARRAY, "Cannot slice non-sequences.");
            bool l_oob = false;
            size_t real_l = canonicalize(this, fake_l, arr->size(), &l_oob);
            if (l_oob) real_l = 0;
            bool r_oob = false;
            size_t real_r = canonicalize(this, fake_r, arr->size(), &r_oob);

            scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
            if (!r_oob) {
                for (size_t i = real_l; i <= real_r; ++i) {
                    if (i >= arr->size()) break;
                    out->add(arr->el(i));
                }
            }
            return new_val(out.release());
        } else if (v->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            table_t *t = 0;
            if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
                t = v->as_selection().first;
            }
            datum_stream_t *seq = v->as_seq();
            rcheck(fake_l >= 0, "Cannot use a negative left index on a stream.");
            rcheck(fake_r >= -1, "Cannot use a right index < -1 on a stream");
            datum_stream_t *new_ds = seq->slice(fake_l, fake_r);
            return t ? new_val(t, new_ds) : new_val(new_ds);
        }
        rfail("Cannot slice non-sequences.");
        unreachable();
    }
    RDB_NAME("slice");
};

class limit_term_t : public op_term_t {
public:
    limit_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v = arg(0);
        table_t *t = 0;
        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            t = v->as_selection().first;
        }
        datum_stream_t *ds = v->as_seq();
        int32_t r = arg(1)->as_int<int32_t>();
        rcheck(r >= 0, strprintf("LIMIT takes a non-negative argument (got %d)", r));
        datum_stream_t *new_ds;
        if (r == 0) {
            new_ds = ds->slice(1, 0); // (0, -1) has a different meaning
        } else {
            new_ds = ds->slice(0, r-1); // note that both bounds are inclusive
        }
        return t ? new_val(t, new_ds) : new_val(new_ds);
    }
    RDB_NAME("limit");
};

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_ARR_HPP_

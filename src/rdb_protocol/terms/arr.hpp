#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {
class append_term_t : public op_term_t {
public:
    append_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        const datum_t *arr    = arg(0)->as_datum();
        const datum_t *new_el = arg(1)->as_datum();
        rcheck(arr->get_type() == datum_t::R_ARRAY, "Cannot append to non-arrays.");
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        // TODO: this is horrendously inefficient.
        for (size_t i = 0; i < arr->size(); ++i) out->add(arr->el(i));
        out->add(new_el);
        return new_val(out.release());
    }
    RDB_NAME("append")
};

size_t canonicalize(int index, size_t size, bool *oob_out = 0) {
    if (index >= 0) return index;
    if (size_t(index * -1) > size) {
        if (oob_out) {
            *oob_out = true;
        } else {
            rfail("Index out of bounds: %d", index);
        }
        return 0;
    }
    return size + index;
}

class nth_term_t : public op_term_t {
public:
    nth_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v = arg(0);
        int n    = arg(1)->as_datum()->as_int();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *arr = v->as_datum();
            size_t real_n = canonicalize(n, arr->size());
            return new_val(arr->el(real_n));
        } else {
            datum_stream_t *s = v->as_seq();
            rcheck(n >= -1, strprintf("Cannot use an index < -1 (%d) on a stream.", n));

            const datum_t *last_d = 0;
            for (int i = 0; n >= 0 && i <= n; ++i) {
                env_checkpoint_t ect(env, &env_t::discard_checkpoint);
                const datum_t *d = s->next();
                if (d) {
                    last_d = d;
                } else {
                    if (n == -1) break;
                    rcheck(d, strprintf("Index out of bounds: %d", n));
                }
            }
            r_sanity_check(last_d);
            return new_val(last_d);
        }
    }
    RDB_NAME("nth")
};

class slice_term_t : public op_term_t {
public:
    slice_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(3)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v   = arg(0);
        int fake_l = arg(1)->as_datum()->as_int();
        int fake_r = arg(2)->as_datum()->as_int();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *arr = v->as_datum();
            rcheck(arr->get_type() == datum_t::R_ARRAY, "Cannot slice non-sequences.");
            bool l_oob = false;
            size_t real_l = canonicalize(fake_l, arr->size(), &l_oob);
            if (l_oob) real_l = 0;
            bool r_oob = false;
            size_t real_r = canonicalize(fake_r, arr->size(), &r_oob);
           //rcheck(real_l < arr->size(), strprintf("Index out of bounds: %lu", real_l));
           //rcheck(real_r < arr->size(), strprintf("Index out of bounds: %lu", real_r));

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
    RDB_NAME("slice")
};

class limit_term_t : public op_term_t {
public:
    limit_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v = arg(0);
        table_t *t = 0;
        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            t = v->as_selection().first;
        }
        datum_stream_t *ds = v->as_seq();
        int r = arg(1)->as_datum()->as_int();

        rcheck(r >= 0, strprintf("LIMIT takes a non-negative argument (got %d)", r));
        datum_stream_t *new_ds;
        if (r == 0) {
            new_ds = ds->slice(1, 0);
        } else {
            new_ds = ds->slice(0, r-1);
        }
        return t ? new_val(t, new_ds) : new_val(new_ds);
    }
    RDB_NAME("limit")
};

} // namespace ql

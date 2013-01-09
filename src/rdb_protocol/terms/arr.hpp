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

size_t canonicalize(int index, size_t size) {
    if (index >= 0) return index;
    rcheck(size_t(index * -1) <= size, strprintf("Index out of bounds: %d", index));
    return size + index;
}

class slice_term_t : public op_term_t {
public:
    slice_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(3)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v   = arg(0);
        int fake_l = arg(1)->as_datum()->as_int();
        int fake_r = arg(2)->as_datum()->as_int();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *arr = v->as_datum();
            rcheck(arr->get_type() == datum_t::R_ARRAY, "Cannot slice non-sequences.");
            size_t real_l = canonicalize(fake_l, arr->size());
            size_t real_r = canonicalize(fake_r, arr->size());
            rcheck(real_l < arr->size(), strprintf("Index out of bounds: %lu", real_l));
            rcheck(real_r < arr->size(), strprintf("Index out of bounds: %lu", real_r));

            scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
            for (size_t i = real_l; i <= real_r; ++i) out->add(arr->el(i));
            return new_val(out.release());
        } else if (v->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            datum_stream_t *seq = v->as_seq();
            rcheck(fake_l >= 0, "Cannot use a negative left index on a stream.");
            rcheck(fake_r >= -1, "Cannot use a right index < -1 on a stream");
            return new_val(new slice_datum_stream_t(env, fake_l, fake_r, seq));
        }
        rfail("Cannot slice non-sequences.");
        unreachable();
    }
    RDB_NAME("slice")
};

} // namespace ql

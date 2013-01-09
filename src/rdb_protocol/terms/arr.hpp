#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {
class append_term_t : public op_term_t {
public:
    append_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        const datum_t *arr = arg(0)->as_datum();
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
} // namespace ql

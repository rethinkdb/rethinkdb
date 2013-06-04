#ifndef RDB_PROTOCOL_TERMS_SET_HPP_
#define RDB_PROTOCOL_TERMS_SET_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/env.hpp"

namespace ql {
/* A set_adapter_term_t takes another term which is assumed to return an array
 * and turns the output in to a set. */
class set_adapter_term_t : public term_t {
public:
    set_adapter_term_t(env_t *env, protob_t<const Term> src,
            term_t *sub_term, const char *name)
        : term_t(env, src), sub_term_(sub_term), name_(name) { }
private:
    counted_t<val_t> eval_impl() {
        counted_t<val_t> res = sub_term_->eval();
        scoped_ptr_t<datum_t> out(new datum_t(res->as_datum()->as_array()));
        out->make_set();
        return new_val(counted_t<const datum_t>(out.release()));
    }
    const char *name() const { return name_; }
    virtual bool is_deterministic_impl() const { return sub_term_->is_deterministic(); }
    scoped_ptr_t<term_t> sub_term_;
    const char *name_;
};

} //namespace ql

#endif

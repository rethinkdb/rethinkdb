#ifndef RDB_PROTOCOL_TERMS_GMR_HPP_
#define RDB_PROTOCOL_TERMS_GMR_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class gmr_term_t : public op_term_t {
public:
    gmr_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(4)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<func_t> g = arg(1)->as_func();
        counted_t<func_t> m = arg(2)->as_func();
        counted_t<func_t> r = arg(3)->as_func();
        return new_val(arg(0)->as_seq()->gmr(g, m, counted_t<const datum_t>(), r));
    }
    virtual const char *name() const { return "grouped_map_reduce"; }
};

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_GMR_HPP_

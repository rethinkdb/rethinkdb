// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <stdint.h>

#include <string>

#include "containers/uuid.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {

class uuid_term_t : public op_term_t {
public:
    uuid_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0)) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *, args_t *, eval_flags_t) const {
        return new_val(datum_t(datum_string_t(uuid_to_str(generate_uuid()))));
    }
    virtual const char *name() const { return "uuid"; }

    // UUID generation is nondeterministic.
    virtual bool is_deterministic() const {
        return false;
    }
};

counted_t<term_t> make_uuid_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<uuid_term_t>(env, term);
}

}  // namespace ql

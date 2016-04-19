// Copyright 2010-2015 RethinkDB, all rights reserved.
#include <stdint.h>

#include <string>

#include "containers/uuid.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {

const uuid_u base_key_id =
    str_to_uuid("91461c99-f89d-49d2-af96-d8e2e14e9b58");

class uuid_term_t : public op_term_t {
public:
    uuid_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0, 1)) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        uuid_u id;

        if (args->num_args() == 1) {
            std::string root = args->arg(env, 0)->as_str().to_std();
            id = uuid_u::from_hash(base_key_id, root);
        } else {
            id = generate_uuid();
        }

        return new_val(datum_t(datum_string_t(uuid_to_str(id))));
    }
    virtual const char *name() const { return "uuid"; }

    // UUID generation is nondeterministic, unless it's actually a hash (then it depends
    // on its argument).
    virtual deterministic_t is_deterministic() const {
        // We are slightly over-cautious here. If the argument is `r.args`, we assume
        // that we're non-deterministic since we can't check whether it will evaluate to
        // 0 or more arguments.
        if (get_original_args().size() == 1
            && get_original_args()[0]->get_src().type() != Term::ARGS) {
            return op_term_t::is_deterministic();
        } else {
            return deterministic_t::no;
        }
    }
};

counted_t<term_t> make_uuid_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<uuid_term_t>(env, term);
}

}  // namespace ql

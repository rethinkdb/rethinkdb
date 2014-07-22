#include "rdb_protocol/configured_limits.hpp"
#include <limits>
#include "rdb_protocol/wire_func.hpp"
#include "rdb_protocol/func.hpp"

namespace ql {
configured_limits_t
from_optargs(rdb_context_t *ctx, signal_t *interruptor,
             const std::map<std::string, wire_func_t> &arguments)
{
    auto p = arguments.find("array_limit");
    if (p != arguments.end()) {
        // Fake an environment with no arguments.  We have to fake it
        // because of a chicken/egg problem; this function gets called
        // before there are any extant environments at all.  Only
        // because we use an empty argument list do we prevent an
        // infinite loop.
        env_t env(ctx, interruptor, std::map<std::string, wire_func_t>(),
                  nullptr);
        return configured_limits_t(p->second.compile_wire_func()->call(&env)->as_int());
    } else {
        return configured_limits_t();
    }
}

RDB_IMPL_ME_SERIALIZABLE_1(configured_limits_t, array_size_limit_);
INSTANTIATE_SERIALIZABLE_SELF_FOR_CLUSTER(configured_limits_t);

template <>
archive_result_t
ql::configured_limits_t::rdb_deserialize<cluster_version_t::v1_13>(read_stream_t *) {
    // this didn't exist in 1.13, so set it to default.
    array_size_limit_ = 100000;
    return archive_result_t::SUCCESS;
}

template <>
void ql::configured_limits_t::rdb_serialize<cluster_version_t::v1_13>(write_message_t *) const {
    // this didn't exist in 1.13, so don't write it.
}

const configured_limits_t configured_limits_t::unlimited(std::numeric_limits<size_t>::max());

} // namespace ql

#ifndef RDB_PROTOCOL_CONFIGURED_LIMITS_HPP_
#define RDB_PROTOCOL_CONFIGURED_LIMITS_HPP_

#include <map>
#include <string>
#include "rpc/serialize_macros.hpp"

class rdb_context_t;
class signal_t;

namespace ql {

class wire_func_t;
class global_optargs_t;

class configured_limits_t {
public:
    configured_limits_t() : array_size_limit_(100000) {}
    explicit configured_limits_t(const size_t limit) : array_size_limit_(limit) {}

    size_t array_size_limit() const { return array_size_limit_; }

    static const configured_limits_t unlimited;

    RDB_DECLARE_ME_SERIALIZABLE(configured_limits_t);
private:
    size_t array_size_limit_;
};

configured_limits_t from_optargs(rdb_context_t *ctx, signal_t *interruptor,
                                 global_optargs_t *optargs);

} // namespace ql

#endif /* RDB_PROTOCOL_CONFIGURED_LIMITS_HPP_ */

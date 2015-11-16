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
    configured_limits_t() :
        changefeed_queue_size_(default_changefeed_queue_size),
        array_size_limit_(default_array_size_limit) {}
    configured_limits_t(size_t changefeed_queue_size, size_t array_size_limit)
        : changefeed_queue_size_(changefeed_queue_size),
          array_size_limit_(array_size_limit) {}

    static const size_t default_changefeed_queue_size = 100000;
    static const size_t default_array_size_limit = 100000;
    static const configured_limits_t unlimited;

    size_t changefeed_queue_size() const { return changefeed_queue_size_; }
    size_t array_size_limit() const { return array_size_limit_; }
private:
    size_t changefeed_queue_size_;
    size_t array_size_limit_;
    RDB_DECLARE_ME_SERIALIZABLE(configured_limits_t);
};

configured_limits_t from_optargs(rdb_context_t *ctx, signal_t *interruptor,
                                 global_optargs_t *optargs);
size_t check_limit(const char *name, int64_t limit);

} // namespace ql

#endif /* RDB_PROTOCOL_CONFIGURED_LIMITS_HPP_ */

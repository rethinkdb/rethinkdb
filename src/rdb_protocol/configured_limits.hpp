#ifndef RDB_PROTOCOL_CONFIGURED_LIMITS_HPP_
#define RDB_PROTOCOL_CONFIGURED_LIMITS_HPP_

#include <map>
#include <string>

namespace ql {

class wire_func_t;

class configured_limits_t {
public:
    configured_limits_t() : _array_size_limit(100000) {}
    explicit configured_limits_t(const size_t limit) : _array_size_limit(limit) {}

    size_t array_size_limit() const { return _array_size_limit; }
private:
    const size_t _array_size_limit;
};

configured_limits_t from_optargs(const std::map<std::string, wire_func_t> &optargs);

} // namespace ql

#endif

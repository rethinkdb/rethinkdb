#include "ql2.hpp"

namespace ql {
term_t *compile_term(const Term2 *source, env_t *env) {
    runtime_check(false, strprintf("NOT IMPLEMENTED %p %p", source, env));
    return 0;
}
} // namespace ql

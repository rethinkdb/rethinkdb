#ifndef BUFFER_CACHE_ALT_SEMANTIC_CHECKER_HPP_
#define BUFFER_CACHE_ALT_SEMANTIC_CHECKER_HPP_

#include "containers/two_level_array.hpp"

namespace alt {

class alt_buf_lock_t;

typedef uint32_t crc_t;

class alt_semantic_checker_t {
public:
    void set(alt_buf_lock_t *lock);
    void check(alt_buf_lock_t *lock);
private:
    two_level_array_t<crc_t> crc_map;
};

} //namespace alt 

#endif

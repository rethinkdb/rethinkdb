#ifndef BUFFER_CACHE_ALT_SEMANTIC_CHECKER_HPP_
#define BUFFER_CACHE_ALT_SEMANTIC_CHECKER_HPP_

#include "containers/two_level_array.hpp"
#include "serializer/types.hpp"

namespace alt {

class alt_buf_lock_t;

#ifdef SEMANTIC_CACHE_CHECK
typedef uint32_t crc_t;

class alt_semantic_checker_t {
public:
    void set(block_id_t bid, const void *data, uint32_t size);
    void check(block_id_t bid, const void *data, uint32_t size);
private:
    two_level_array_t<crc_t> crc_map;
};
#endif //#ifdef SEMANTIC_CACHE_CHECK

} //namespace alt

#endif

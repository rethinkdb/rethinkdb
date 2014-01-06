
#include "utils.hpp"
#include <boost/crc.hpp>

#include "buffer_cache/alt/semantic_checker.hpp"
#include "buffer_cache/alt/alt.hpp"

namespace alt {

crc_t crc(const void *data, uint32_t size) {
    boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
    crc_computer.process_bytes(data, size);
    return crc_computer.checksum();
}

crc_t crc(alt_buf_lock_t *lock) {
    boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
    alt_buf_read_t read(lock);
    uint32_t block_size = 0;
    return crc(read.get_data_read(&block_size), block_size);
}

void alt_semantic_checker_t::set(block_id_t bid, const void *data, uint32_t size) {
    crc_map.set(bid, crc(data, size));
}

void alt_semantic_checker_t::check(block_id_t bid, const void *data, uint32_t size) {
    BREAKPOINT;
    debugf("check %ld\n", bid);
    guarantee(crc(data, size) == crc_map.get(bid));
}

} //namespace alt 

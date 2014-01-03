
#include "utils.hpp"
#include <boost/crc.hpp>

#include "buffer_cache/alt/semantic_checker.hpp"
#include "buffer_cache/alt/alt.hpp"

namespace alt {

crc_t crc(alt_buf_lock_t *lock) {
    boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
    alt_buf_read_t read(lock);
    uint32_t block_size = 0;
    crc_computer.process_bytes(read.get_data_read(&block_size), block_size);
    return crc_computer.checksum();
}

void alt_semantic_checker_t::set(alt_buf_lock_t *lock) {
    crc_map.set(lock->block_id(), crc(lock));
}

void alt_semantic_checker_t::check(alt_buf_lock_t *lock) {
    guarantee(crc(lock) == crc_map.get(lock->block_id()));
}

} //namespace alt 

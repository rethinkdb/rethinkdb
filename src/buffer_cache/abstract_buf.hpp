#ifndef _BUFFER_CACHE_BUF_HPP_
#define _BUFFER_CACHE_BUF_HPP_

#include <stdint.h>                   // size_t
#include "serializer/types.hpp"       // block_id_t
#include "buffer_cache/buf_patch.hpp" // buf_patch_t
#include "utils.hpp"                  // repli_timestamp_t

struct abstract_buf_t {
    virtual block_id_t get_block_id() const = 0;
    virtual const void *get_data_read() const = 0;
    // Use this only for writes which affect a large part of the block, as it bypasses the diff system
    virtual void *get_data_major_write() = 0;
    // Convenience function to set some address in the buffer acquired through get_data_read. (similar to memcpy)
    virtual void set_data(void *dest, const void *src, const size_t n) = 0;
    // Convenience function to move data within the buffer acquired through get_data_read. (similar to memmove)
    virtual void move_data(void *dest, const void *src, const size_t n) = 0;
    // This might delete the supplied patch, do not use patch after its application
    virtual void apply_patch(buf_patch_t *patch) = 0;
    virtual patch_counter_t get_next_patch_counter() = 0;
    virtual void release() = 0;
    virtual bool is_dirty() = 0;
    virtual ~abstract_buf_t() { }
};

#endif

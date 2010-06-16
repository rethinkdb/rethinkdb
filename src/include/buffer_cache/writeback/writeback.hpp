
#ifndef __BUFFER_CACHE_WRITEBACK_HPP__
#define __BUFFER_CACHE_WRITEBACK_HPP__

#include <set>

/*
 * TODO:
 * o Add data structure to track dirty block_id set.
 * o Take required locks to safely begin writeback procedure.
 * o Pass set of block_ids to the serializer in one group to allow for a flush
 *   transaction.
 * o Plumb through user setting for writeback frequency.
 */

template <class config_t>
struct writeback_tmpl_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef aio_context<config_t> aio_context_t;
    
    explicit writeback_tmpl_t(serializer_t *_serializer);

    void start();

    block_id_t mark_dirty(event_queue_t *event_queue, block_id_t block_id,
            void *block, void *state);

private:
    static void timer_callback(void *ctx);
    void writeback();

    serializer_t *serializer;
    std::set<block_id_t> dirty_blocks;
};

#include "buffer_cache/writeback/writeback_impl.hpp"

#endif // __buffer_cache_writeback_hpp__


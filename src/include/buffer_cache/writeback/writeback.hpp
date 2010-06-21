
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

    class buf_t {
    public:
        explicit buf_t(writeback_tmpl_t *wb) : writeback(wb), dirty(false) {}

        bool is_dirty() const { return dirty; }
        void set_dirty(typename config_t::buf_t *);

    protected:
        void set_clean() {
            assert(dirty);
            dirty = false;
        }

    private:
        writeback_tmpl_t *writeback;
        bool dirty;
    };

private:
    static void timer_callback(void *ctx);
    void writeback();

    serializer_t *serializer;
    std::set<typename config_t::transaction_t *> txns;
    std::set<typename config_t::buf_t *> dirty_bufs;
};

#include "buffer_cache/writeback/writeback_impl.hpp"

#endif // __buffer_cache_writeback_hpp__


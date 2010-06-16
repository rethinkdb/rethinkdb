
#ifndef __BUFFER_CACHE_WRITEBACK_IMPL_HPP__
#define __BUFFER_CACHE_WRITEBACK_IMPL_HPP__

template <class config_t>
writeback_tmpl_t<config_t>::writeback_tmpl_t(serializer_t *_serializer)
    : serializer(_serializer) {
}

template <class config_t>
void writeback_tmpl_t<config_t>::start() {
    timespec ts;
    ts.tv_sec = 5; /* TODO(NNW): This should be a user-specified value. */
    ts.tv_nsec = 0;
    get_cpu_context()->event_queue->set_timer(&ts, timer_callback, this);
}

template <class config_t>
typename writeback_tmpl_t<config_t>::block_id_t
writeback_tmpl_t<config_t>::mark_dirty(event_queue_t *event_queue,
        block_id_t block_id, void *block, void *state) {
    dirty_blocks.insert(block_id);
    aio_context_t *ctx = new aio_context_t();
    ctx->user_state = state;
    ctx->block_id = block_id;

    block_id_t new_block_id;
    new_block_id = serializer->do_write(event_queue, block_id, block, ctx);
    /* XXX What is this new_block_id for?? */

    return new_block_id;
}

template <class config_t>
void writeback_tmpl_t<config_t>::timer_callback(void *ctx) {
    static_cast<writeback_tmpl_t *>(ctx)->writeback();
}

template <class config_t>
void writeback_tmpl_t<config_t>::writeback() {
    /* TODO(NNW): Implement. */
}

#endif  // __BUFFER_CACHE_WRITEBACK_IMPL_HPP__

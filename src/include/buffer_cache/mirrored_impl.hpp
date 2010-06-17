
#ifndef __BUFFER_CACHE_MIRRORED_IMPL_HPP__
#define __BUFFER_CACHE_MIRRORED_IMPL_HPP__

template <class config_t>
void mirrored_cache_t<config_t>::buf_t::release(void *ctx) {
}

template <class config_t>
typename mirrored_cache_t<config_t>::node_t *
mirrored_cache_t<config_t>::buf_t::node() {
    /* TODO(NNW): Implement!! */
    return NULL;
}

template <class config_t>
void mirrored_cache_t<config_t>::transaction_t::commit() {
    /* TODO(NNW): Implement!! */
}

template <class config_t>
typename mirrored_cache_t<config_t>::buf_t *
mirrored_cache_t<config_t>::transaction_t::acquire(block_id_t block_id,
        void *ctx) {
    /* TODO(NNW): Implement!! */
    return NULL;
}

template <class config_t>
typename mirrored_cache_t<config_t>::buf_t *
mirrored_cache_t<config_t>::transaction_t::allocate(block_id_t *new_block_id) {
    /* TODO(NNW): Implement!! */
    return NULL;
}

#endif  // __BUFFER_CACHE_MIRRORED_IMPL_HPP__

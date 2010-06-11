
#ifndef __COREFWD_HPP__
#define __COREFWD_HPP__

// FSM
template<class config_t>
struct conn_fsm;

// Serializers
template <class config_t>
struct in_place_serializer_t;

// Caches
template <class config_t>
struct unlocked_hash_map_t;

template <class config_t>
struct page_repl_last_t;

template <class config_t>
struct fallthrough_cache_t;

template <class config_t>
struct mirrored_cache_t;

template <class config_t>
struct immediate_writeback_t;

// Btree
template <typename block_id_t>
struct array_node_t;

template <typename block_id_t>
class btree_fsm;

template <typename block_id_t>
class btree_get_fsm;

template <typename block_id_t>
class btree_set_fsm;

// Event queue
struct event_t;
struct event_queue_t;

// Worker pool
struct worker_pool_t;

// Request handler
template <class config_t>
class request_handler_t;

template <class config_t>
class memcached_handler_t;

// IOCB
struct iocb;

#endif // __COREFWD_HPP__


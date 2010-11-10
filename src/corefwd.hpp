
#ifndef __COREFWD_HPP__
#define __COREFWD_HPP__

/* TODO get rid of corefwd.hpp */

// Connection FSM
struct conn_fsm_t;

struct data_transferred_callback;

// Request
struct request_callback_t;

struct request_t;

// Serializers
struct in_place_serializer_t;
struct log_serializer_t;
template<class inner_serializer_t> struct semantic_checking_serializer_t;

// Btree

class btree_key_value_store_t;
struct btree_node;
class btree_modify_fsm_t;
class fill_large_value_msg_t;
class write_large_value_msg_t;
class btree_fsm_t;
class btree_get_fsm_t;
class btree_get_cas_fsm_t;
class btree_set_fsm_t;
class btree_delete_fsm_t;
class btree_incr_decr_fsm_t;
class btree_append_prepend_fsm_t;

// Worker pool
struct worker_pool_t;
struct worker_t;

// Request handler
class request_handler_t;
class txt_memcached_handler_t;
class bin_memcached_handler_t;
class memcached_handler_t;

// Context
struct iocb;

#endif // __COREFWD_HPP__


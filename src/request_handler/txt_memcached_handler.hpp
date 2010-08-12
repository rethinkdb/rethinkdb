
#ifndef __TXT_MEMCACHED_HANDLER_HPP__
#define __TXT_MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "config/code.hpp"
#include "alloc/alloc_mixin.hpp"

template<class config_t>
class txt_memcached_handler_t : public request_handler_t<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_handler_t<config_t> > {
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::btree_set_fsm_t btree_set_fsm_t;
    typedef typename config_t::btree_get_fsm_t btree_get_fsm_t;
    typedef typename config_t::btree_get_cas_fsm_t btree_get_cas_fsm_t;
    typedef typename config_t::btree_delete_fsm_t btree_delete_fsm_t;
    typedef typename config_t::btree_incr_decr_fsm_t btree_incr_decr_fsm_t;
    typedef typename config_t::btree_append_prepend_fsm_t btree_append_prepend_fsm_t;
    typedef typename config_t::req_handler_t req_handler_t;
    typedef typename req_handler_t::parse_result_t parse_result_t;
    using request_handler_t<config_t>::conn_fsm;
    
public:
    txt_memcached_handler_t(event_queue_t *eq, conn_fsm_t *conn_fsm)
        : req_handler_t(eq, conn_fsm), loading_data(false)
        {}
    
    virtual parse_result_t parse_request(event_t *event);

private:
    enum storage_command { SET, ADD, REPLACE, APPEND, PREPEND, CAS };
    storage_command cmd;

    union {
        char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
        btree_key key;
    };
    
    btree_value::mcflags_t mcflags;
    btree_value::exptime_t exptime;
    uint32_t bytes;
    btree_value::cas_t cas;
    bool noreply;
    bool loading_data;

    parse_result_t parse_storage_command(storage_command command, char *state, unsigned int line_len);
    parse_result_t parse_stat_command(char *state, unsigned int line_len);
    parse_result_t parse_adjustment(bool increment, char *state, unsigned int line_len);
    parse_result_t read_data();
    parse_result_t get(char *state, unsigned int line_len);
    parse_result_t get_cas(char *state, unsigned int line_len);
    parse_result_t remove(char *state, unsigned int line_len);
    parse_result_t malformed_request();
    parse_result_t unimplemented_request();
};


#include "request_handler/txt_memcached_handler.tcc"

#endif // __TXT_MEMCACHED_HANDLER_HPP__



#ifndef __TXT_MEMCACHED_HANDLER_HPP__
#define __TXT_MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "btree/key_value_store.hpp"
#include "config/alloc.hpp"

class server_t;

class txt_memcached_handler_t :
    public request_handler_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_handler_t> {
public:
    typedef request_handler_t::parse_result_t parse_result_t;
    using request_handler_t::conn_fsm;
    
public:
    txt_memcached_handler_t(server_t *server)
        : request_handler_t(), loading_data(false), server(server)
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

public:
    server_t *server;
};

#endif // __TXT_MEMCACHED_HANDLER_HPP__


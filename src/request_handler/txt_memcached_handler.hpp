
#ifndef __TXT_MEMCACHED_HANDLER_HPP__
#define __TXT_MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "btree/key_value_store.hpp"
#include "buffer_cache/large_buf.hpp"

class server_t;

class txt_memcached_handler_t :
    public request_handler_t,
    public large_value_completed_callback // Used for consuming data from the socket. XXX: Rename this.
{
public:
    typedef request_handler_t::parse_result_t parse_result_t;
    using request_handler_t::conn_fsm;
    
public:
    txt_memcached_handler_t(server_t *server)
        : request_handler_t(), loading_data(false), consuming(false), server(server)
        {}
    
    virtual parse_result_t parse_request(event_t *event);

    void on_large_value_completed(bool success); // TODO: Rename this. Only used for consuming a value.

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
    bool consuming;
    
    parse_result_t parse_storage_command(storage_command command, char *state, unsigned int line_len);
    parse_result_t parse_stat_command(unsigned int line_len, char *cmd_str);
    parse_result_t parse_adjustment(bool increment, char *state, unsigned int line_len);
    parse_result_t parse_gc_command(unsigned int line_len, char *state);
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


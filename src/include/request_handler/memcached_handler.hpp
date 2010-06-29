
#ifndef __MEMCACHED_HANDLER_HPP__
#define __MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "config/code.hpp"
#include "alloc/alloc_mixin.hpp"

template<class config_t>
class memcached_handler_t : public request_handler_t<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<typename config_t::alloc_t>, memcached_handler_t<config_t> > {
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::request_t request_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::btree_set_fsm_t btree_set_fsm_t;
    typedef typename config_t::btree_get_fsm_t btree_get_fsm_t;
    //typedef typename config_t::btree_delete_fsm_t btree_delete_fsm_t;
    typedef typename config_t::req_handler_t req_handler_t;
    typedef typename req_handler_t::parse_result_t parse_result_t;
    
public:
    memcached_handler_t(cache_t *_cache, event_queue_t *eq)
        : req_handler_t(eq), cache(_cache), key((btree_key*)key_memory), loading_data(false)
        {}
    
    virtual parse_result_t parse_request(event_t *event);
    virtual void build_response(request_t *request);

private:
    enum storage_command { SET, ADD, REPLACE, APPEND, PREPEND, CAS };
    cache_t *cache;
    storage_command cmd;

    char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
    btree_key * const key;
    uint32_t flags;
    uint32_t exptime;
    uint32_t bytes;
    uint64_t cas_unique; //must be at least 64 bits
    bool noreply;
    bool loading_data;

    parse_result_t parse_storage_command(storage_command command, char *state, unsigned int line_len, conn_fsm_t *fsm);

    parse_result_t read_data(char *data, unsigned int size, conn_fsm_t *fsm);

    parse_result_t set(char *data, conn_fsm_t *fsm);
    parse_result_t add(char *data, conn_fsm_t *fsm);
    parse_result_t replace(char *data, conn_fsm_t *fsm);
    parse_result_t append(char *data, conn_fsm_t *fsm);
    parse_result_t prepend(char *data, conn_fsm_t *fsm);
    parse_result_t cas(char *data, conn_fsm_t *fsm);
    void set_key(conn_fsm_t *fsm, btree_key *key, int value);

    parse_result_t get(char *state, bool include_unique, conn_fsm_t *fsm);

    parse_result_t remove(char *state, conn_fsm_t *fsm);
    parse_result_t adjust(char *state, bool inc, conn_fsm_t *fsm);

    void write_msg(conn_fsm_t *fsm, const char *str);
    parse_result_t malformed_request(conn_fsm_t *fsm);
    parse_result_t unimplemented_request(conn_fsm_t *fsm);
};

#include "request_handler/memcached_handler_impl.hpp"

#endif // __MEMCACHED_HANDLER_HPP__


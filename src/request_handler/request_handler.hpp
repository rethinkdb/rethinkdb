
#ifndef __REQUEST_HANDLER_HPP__
#define __REQUEST_HANDLER_HPP__

#include "event.hpp"
#include "config/code.hpp"

struct event_t;
struct event_queue_t;

template<class config_t>
class request_handler_t {
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::request_t request_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    
public:
    explicit request_handler_t(event_queue_t *eq) : event_queue(eq) {}
    virtual ~request_handler_t() {}

    enum parse_result_t {
        op_malformed,
        op_partial_packet,
        op_req_shutdown,
        op_req_quit,
        op_req_complex,
        op_req_parallelizable,
        op_req_send_now
    };
    
    virtual parse_result_t parse_request(event_t *event) = 0;
    virtual void build_response(request_t *request) = 0;
    
    // TODO: if we receive a small request from the user that can be
    // satisfied on the same CPU, we should probably special case it and
    // do it right away, no need to send it to itself, process it, and
    // then send it back to itself.
    
    void dispatch_btree_fsm(
        conn_fsm_t *conn_fsm,
        btree_fsm_t *btree_fsm)
    {
        request_t *request = conn_fsm->current_request;
        assert(request);
        
        assert (!btree_fsm->request);
        btree_fsm->request = request;
        
        assert(request->nstarted < MAX_OPS_IN_REQUEST);
        request->fsms[request->nstarted] = btree_fsm;
        request->nstarted++;
        
        // Add the fsm to the appropriate queue
        int shard_id = key_to_cpu(btree_fsm->key, event_queue->nqueues);
        event_queue->message_hub.store_message(shard_id, btree_fsm);
    }
    
protected:
    event_queue_t *event_queue;
};

#endif // __REQUEST_HANDLER_HPP__

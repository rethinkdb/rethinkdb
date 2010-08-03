
#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

#include "config/args.hpp"

struct request_callback_t
{
    virtual ~request_callback_t() {}
    virtual void on_request_completed() = 0;
};

// TODO: if we receive a small request from the user that can be
// satisfied on the same CPU, we should probably special case it and
// do it right away, no need to send it to itself, process it, and
// then send it back to itself.

/*
request_t abstracts the idea of dispatching one or more messages to the appropriate cores and
waiting for replies. Create a request_t with a request_callback_t and then call add() one or more
times. Then call dispatch(), and you will receive a callback when all of the tasks are done. Note
that the request will free itself.
*/

// TODO: Is there any real point in keeping track of the messages associated with the request?
// 'msgs' is written to, but never read from.

struct request_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, request_t >
{
public:
    explicit request_t(request_callback_t *cb) :
        nstarted(0), ncompleted(0), dispatched(false), callback(cb) {
        assert(callback);
    }
    ~request_t() {
    }
    
    bool can_add() {
        return nstarted < MAX_OPS_IN_REQUEST;
    }
    
    void add(cpu_message_t *msg, int cpu) {
        assert(!msg->request);
        assert(!dispatched);
        assert(can_add());
        msg->request = this;
        msg->return_cpu = get_cpu_context()->worker->workerid;
        msgs[nstarted] = msg;
        get_cpu_context()->worker->event_queue->message_hub.store_message(cpu, msg);
        nstarted ++;
    }
    
    void dispatch(void) {
        assert(!dispatched);
        assert(nstarted > 0);
        assert(ncompleted == 0);
        dispatched = true;
    }
    
    void on_request_part_completed() {
        assert(dispatched);
        ncompleted ++;
        assert(ncompleted <= nstarted);
        if (ncompleted == nstarted) {
            callback->on_request_completed();
            delete this;
        }
    }
    
private:
    unsigned int nstarted, ncompleted;
    bool dispatched;
    cpu_message_t *msgs[MAX_OPS_IN_REQUEST];
    
    request_callback_t *callback;
};

#endif // __REQUEST_HPP__

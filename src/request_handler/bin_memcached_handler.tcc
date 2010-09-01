#ifndef __BIN_MEMCACHED_HANDLER_TCC__
#define __BIN_MEMCACHED_HANDLER_TCC__

#include <string.h>
#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "request_handler/bin_memcached_handler.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"

template<class config_t>
class bin_memcached_request : public request_callback_t {

public:
    typedef typename bin_memcached_handler_t<config_t>::packet_t packet_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename bin_memcached_handler_t<config_t>::bin_opcode_t bin_opcode_t;
    typedef typename bin_memcached_handler_t<config_t>::bin_data_type_t bin_data_type_t;
    typedef typename bin_memcached_handler_t<config_t>::bin_opaque_t bin_opaque_t;

public:
    bin_memcached_request(bin_memcached_handler_t<config_t> *rh, packet_t *pkt)
        : request_callback_t(rh), opcode(pkt->opcode()), data_type(pkt->data_type()), opaque(pkt->opaque()),
          request(new request_t(this))
        {}

    /* build_response, necessary so that one request can send multiple packets */
    typedef enum {
        br_call_again,
        br_done,
    } br_result_t;
    
    virtual br_result_t build_response(packet_t *pkt) = 0;
    
    void on_request_completed() {
    
        conn_fsm_t *c = rh->conn_fsm;
        
        br_result_t res;
        do {
            byte tmpbuf[MAX_PACKET_SIZE] = {0x00};   // Make valgrind happy
            packet_t packet(tmpbuf);
            packet.opcode(opcode);
            packet.data_type(data_type);
            packet.opaque(opaque);
            packet.extra_length(0);
            packet.key_length(0);
            packet.value_length(0);
            packet.magic(bin_memcached_handler_t<config_t>::bin_magic_response);

            res = build_response(&packet);
            c->sbuf->append(packet.data, packet.size());
        } while (res == br_call_again);
        
        // TODO: Handle uncorking if we are non-quiet
        
        rh->request_complete();
        delete this;
    }
    
    bin_opcode_t opcode;
    bin_data_type_t data_type;
    bin_opaque_t opaque;
    request_t *request;
};

template<class config_t>
class bin_memcached_get_request : public bin_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, bin_memcached_get_request<config_t> > {

public:
    typedef typename bin_memcached_handler_t<config_t>::packet_t packet_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_get_fsm_t btree_get_fsm_t;
    typedef typename bin_memcached_request<config_t>::br_result_t br_result_t;
    using bin_memcached_request<config_t>::br_done;
    using bin_memcached_request<config_t>::request;
    using bin_memcached_request<config_t>::opcode;

public:
    bin_memcached_get_request(bin_memcached_handler_t<config_t> *rh, packet_t *pkt, btree_key *key)
        : bin_memcached_request<config_t>(rh, pkt), fsm(new btree_get_fsm_t(key, this))
        {
        request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        request->dispatch();
    }
    
    ~bin_memcached_get_request() {
        get_cpu_context()->worker->cmd_get++;
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        switch (fsm->status_code) {
            case btree_get_fsm_t::S_SUCCESS:
                get_cpu_context()->worker->get_hits++;
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_no_error);
                res_pkt->set_value(&fsm->value);
                res_pkt->set_extras(extra_flags, extra_flags_length);
                break;
            case btree_get_fsm_t::S_NOT_FOUND:
                get_cpu_context()->worker->get_misses++;
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_key_not_found);
                break;
            default:
                assert(0);
                break;
        }

        //check if the packet requires that a key be sent back
        if (bin_memcached_handler_t<config_t>::is_key_code(opcode)) {
            res_pkt->set_key(&fsm->key);
        }
        return br_done;
    }
private:
    btree_get_fsm_t *fsm;
};

template<class config_t>
class bin_memcached_set_request : public bin_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, bin_memcached_set_request<config_t> > {

public:
    typedef typename bin_memcached_handler_t<config_t>::packet_t packet_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_set_fsm_t btree_set_fsm_t;
    typedef typename bin_memcached_request<config_t>::br_result_t br_result_t;
    using bin_memcached_request<config_t>::br_done;
    using bin_memcached_request<config_t>::request;

public:
    bin_memcached_set_request(bin_memcached_handler_t<config_t> *rh, packet_t *pkt, btree_key *key, byte *data, uint32_t size, typename btree_set_fsm_t::set_type_t type, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, uint64_t req_cas)
        : bin_memcached_request<config_t>(rh, pkt), fsm(new btree_set_fsm_t(key, this, data, size, type, mcflags, exptime, req_cas))
        {
        assert(type == btree_set_fsm_t::set_type_set);   // We haven't hooked up ADD and REPLACE yet, and we're going to handle CAS differently.
        request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        request->dispatch();
    }
    
    ~bin_memcached_set_request() {
        get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_no_error);
        // Set responses require don't require anything to be set. When we hook up ADD and REPLACE
        // then we will need to check for errors.
        // XXX Hook up ADD and REPLACE and add all the appropriate error codes.
        assert(fsm->status_code == btree_set_fsm<config_t>::S_SUCCESS);
        return br_done;
    }
private:
    btree_set_fsm_t *fsm;
};

template<class config_t>
class bin_memcached_incr_decr_request : public bin_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, bin_memcached_incr_decr_request<config_t> > {

public:
    typedef typename bin_memcached_handler_t<config_t>::packet_t packet_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_incr_decr_fsm_t btree_incr_decr_fsm_t;
    typedef typename bin_memcached_request<config_t>::br_result_t br_result_t;
    using bin_memcached_request<config_t>::br_done;
    using bin_memcached_request<config_t>::request;

public:
    bin_memcached_incr_decr_request(bin_memcached_handler_t<config_t> *rh, packet_t *pkt, btree_key *key, bool increment, long long delta)
        : bin_memcached_request<config_t>(rh, pkt), fsm(new btree_incr_decr_fsm_t(key, increment, delta))
        {
        request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        request->dispatch();
    }
    
    ~bin_memcached_incr_decr_request() {
        get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        // TODO: This should return the value.
        switch (fsm->status_code) {
            case btree_incr_decr_fsm_t::S_SUCCESS:
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_no_error);
                break;
            case btree_incr_decr_fsm_t::S_NOT_NUMERIC:
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_incr_decr_on_non_numeric_value);
                break;
            case btree_incr_decr_fsm_t::S_NOT_FOUND:
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_key_not_found);
                break;
            default:
                // XXX
                assert(0);
                break;
        }
        return br_done;
    }
private:
    btree_incr_decr_fsm_t *fsm;
};

template<class config_t>
class bin_memcached_append_prepend_request : public bin_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, bin_memcached_append_prepend_request<config_t> > {

public:
    typedef typename bin_memcached_handler_t<config_t>::packet_t packet_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_append_prepend_fsm_t btree_append_prepend_fsm_t;
    typedef typename bin_memcached_request<config_t>::br_result_t br_result_t;
    using bin_memcached_request<config_t>::br_done;
    using bin_memcached_request<config_t>::request;

public:
    bin_memcached_append_prepend_request(bin_memcached_handler_t<config_t> *rh, packet_t *pkt, btree_key *key, byte *data, int size, bool append) : bin_memcached_request<config_t>(rh, pkt), fsm(new btree_append_prepend_fsm_t(key, data, size, append)) {
        request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        request->dispatch();
    }

    ~bin_memcached_append_prepend_request() {
        get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }

    br_result_t build_response(packet_t *res_pkt) {
        switch (fsm->status_code) {
            case btree_append_prepend_fsm_t::S_SUCCESS:
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_no_error);
                break;
            case btree_append_prepend_fsm_t::S_NOT_STORED:
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_item_not_stored);
                break;
            default:
                assert(0);
                break;
        }
        return br_done;
    }

private:
    btree_append_prepend_fsm_t *fsm;
};

template<class config_t>
class bin_memcached_delete_request : public bin_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, bin_memcached_delete_request<config_t> > {

public:
    typedef typename bin_memcached_handler_t<config_t>::packet_t packet_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_delete_fsm_t btree_delete_fsm_t;
    typedef typename bin_memcached_request<config_t>::br_result_t br_result_t;
    using bin_memcached_request<config_t>::br_done;
    using bin_memcached_request<config_t>::request;

public:
    bin_memcached_delete_request(bin_memcached_handler_t<config_t> *rh, packet_t *pkt, btree_key *key)
        : bin_memcached_request<config_t>(rh, pkt), fsm(new btree_delete_fsm_t(key))
        {
        request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        request->dispatch();
    }
    
    ~bin_memcached_delete_request() {
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        switch (fsm->status_code) {
            case btree_delete_fsm_t::S_DELETED:
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_no_error);
                break;
            case btree_delete_fsm_t::S_NOT_FOUND:
                res_pkt->status(bin_memcached_handler_t<config_t>::bin_status_key_not_found);
                break;
            default:
                assert(0);
                break;
        }
        return br_done;
    }
private:
    btree_delete_fsm_t *fsm;
};

template<class config_t>
class bin_memcached_perfmon_request : public bin_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, bin_memcached_perfmon_request<config_t> > {

public:
    typedef typename bin_memcached_handler_t<config_t>::packet_t packet_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename bin_memcached_request<config_t>::br_result_t br_result_t;
    using bin_memcached_request<config_t>::br_done;
    using bin_memcached_request<config_t>::br_call_again;
    using bin_memcached_request<config_t>::request;

public:
    bin_memcached_perfmon_request(bin_memcached_handler_t<config_t> *rh, packet_t *pkt)
        : bin_memcached_request<config_t>(rh, pkt), stats_collected(false) {

        int nworkers = (int)get_cpu_context()->worker->event_queue->parent_pool->nworkers;

        // Tell every single CPU core to pass their perfmon module *by copy*
        // to this CPU
        for (int i = 0; i < nworkers; i++)
        {
            assert(i < MAX_OPS_IN_REQUEST);
            msgs[i] = new perfmon_msg_t();
            this->request->add(msgs[i], i);
        }
        request->dispatch();
    }
    
    ~bin_memcached_perfmon_request() {}
    
    br_result_t build_response(packet_t *res_pkt) {
        // Combine all responses into one
        char tmpbuf[255];
#if defined(VALGRIND) || !defined(NDEBUG)
        // Fill the buffer with garbage in debug mode so valgrind doesn't complain, and to help
        // catch uninitialized memory errors.
        memset(tmpbuf, 0xBD, sizeof(tmpbuf));
#endif
        if (!stats_collected) {
            int nworkers = (int)get_cpu_context()->worker->event_queue->parent_pool->nworkers;
            for(int i = 0; i < nworkers; i++) {
                combined_perfmon.accumulate(msgs[i]->perfmon);
            }
            iter = combined_perfmon.registry.begin();
            stats_collected = true;
        }
        if (iter != combined_perfmon.registry.end()) {
            res_pkt->set_key(iter->first, strlen(iter->first));
            int val_len = iter->second.print(tmpbuf, 255);
            res_pkt->set_value(tmpbuf, val_len);
            iter++;
            return br_call_again;
        } else {
            /* we're also sending an empty packet here, but that doesn't require any code */
            return br_done;
        }
    }

public:
    perfmon_msg_t *msgs[MAX_OPS_IN_REQUEST];

private:
    bool stats_collected;
    perfmon_t combined_perfmon;
    perfmon_t::perfmon_map_t::iterator iter;
};

//! Parse a binary command received from the user
template<class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::parse_request(event_t *event)
{
    assert(event->state == conn_fsm);

    if (conn_fsm->nrbuf < sizeof(request_header_t))
        return req_handler_t::op_partial_packet;

    packet_t packet(conn_fsm->rbuf);
    packet_t *pkt = &packet;
    
    if (conn_fsm->nrbuf < pkt->size())
        return req_handler_t::op_partial_packet;

    if (!pkt->is_valid_request()) {
        conn_fsm->consume(pkt->size());
        return req_handler_t::op_malformed;
    }

    parse_result_t res;

    //Showtime!!
    switch (pkt->opcode()) {
        case bin_opcode_get:
        case bin_opcode_getq:
        case bin_opcode_getk:
        case bin_opcode_getkq:
        case bin_opcode_set:
        case bin_opcode_setq:
        case bin_opcode_add:
        case bin_opcode_addq:
        case bin_opcode_replace:
        case bin_opcode_replaceq:
        case bin_opcode_delete:
        case bin_opcode_deleteq:
        case bin_opcode_increment:
        case bin_opcode_incrementq:
        case bin_opcode_decrement:
        case bin_opcode_decrementq:
        case bin_opcode_append:
        case bin_opcode_appendq:
        case bin_opcode_prepend:
        case bin_opcode_prependq:
            res = dispatch_appropriate_fsm(pkt);
            break;
        case bin_opcode_quit:
        case bin_opcode_quitq:
            res = req_handler_t::op_req_quit;
            break;
        case bin_opcode_flush:
        case bin_opcode_flushq:
        case bin_opcode_no_op:
            res = no_op(pkt);
            break;
        case bin_opcode_version:
            res = version(pkt);
            break;
        case bin_opcode_stat:
            res = stat(pkt);
            break;
        default:
            res = malformed_request();
            break;
    }
    
#ifdef MEMCACHED_STRICT
    conn_fsm->corked = is_quiet_code(pkt->opcode()); //cork if the code is quiet
#endif

    conn_fsm->consume(pkt->size());
    return res;
}


template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::dispatch_appropriate_fsm(packet_t *pkt) {
    
    pkt->key(key);
    
    switch (pkt->opcode()) {
        case bin_opcode_get:
        case bin_opcode_getq:
        case bin_opcode_getk:
        case bin_opcode_getkq:
            new bin_memcached_get_request<config_t>(this, pkt, key);
            //TODO even a quiet get eventually requires a response, so pipelining is broken
            return req_handler_t::op_req_complex;
        case bin_opcode_set:
        case bin_opcode_setq:
            // TODO: Set the actual flags after the new packet-parsing code is written.
            new bin_memcached_set_request<config_t>(this, pkt, key, pkt->value(), pkt->value_length(), btree_set_fsm_t::set_type_set, 0, 0, false);
            break;
        case bin_opcode_add:
        case bin_opcode_addq:
            new bin_memcached_set_request<config_t>(this, pkt, key, pkt->value(), pkt->value_length(), btree_set_fsm_t::set_type_add, 0, 0, false);
            break;
        case bin_opcode_replace:
        case bin_opcode_replaceq:
            new bin_memcached_set_request<config_t>(this, pkt, key, pkt->value(), pkt->value_length(), btree_set_fsm_t::set_type_replace, 0, 0, false);
            break;
        case bin_opcode_increment:
        case bin_opcode_incrementq:
        {
            // FIXME delta is in extras, not the value.
            long long delta = atoll(pkt->value());
            new bin_memcached_incr_decr_request<config_t>(this, pkt, key, true, delta);
            break;
        }
        case bin_opcode_decrement:
        case bin_opcode_decrementq:
        {
            long long delta = atoll(pkt->value());
            new bin_memcached_incr_decr_request<config_t>(this, pkt, key, false, delta);
            break;
        }
        case bin_opcode_delete:
        case bin_opcode_deleteq:
            new bin_memcached_delete_request<config_t>(this, pkt, key);
            break;
        case bin_opcode_append:
        case bin_opcode_appendq:
            new bin_memcached_append_prepend_request<config_t>(this, pkt, key, pkt->value(), pkt->value_length(), true);
            break;
        case bin_opcode_prepend:
        case bin_opcode_prependq:
            new bin_memcached_append_prepend_request<config_t>(this, pkt, key, pkt->value(), pkt->value_length(), false);
            break;
        default:
            fail("Invalid opcode in bin_memcached_handler_t::dispatch_appropriate_fsm");
    }
    
    if (is_quiet_code(pkt->opcode())) return req_handler_t::op_req_parallelizable;
    else return req_handler_t::op_req_complex;
}

template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::no_op(packet_t *pkt) {
    //XXX this changes the passed packet good provided we don't need it anymore since it saves an allocation
    pkt->magic(bin_magic_response);

    conn_fsm->sbuf->append(pkt->data, pkt->size());
    return req_handler_t::op_req_send_now;
}

template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::stat(packet_t *pkt) {
    new bin_memcached_perfmon_request<config_t>(this, pkt);
    return req_handler_t::op_req_complex;
}

template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::version(packet_t *pkt) {
    byte tmpbuf[MAX_PACKET_SIZE] = {0x00};
    packet_t res_pkt(tmpbuf);

    res_pkt.magic(bin_magic_response);
    res_pkt.opcode(pkt->opcode());
    res_pkt.set_value((byte *) VERSION_STRING, strlen(VERSION_STRING) - 1);

    conn_fsm->sbuf->append(res_pkt.data, res_pkt.size());
    return req_handler_t::op_req_send_now;
}

template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::malformed_request() {
    // TODO: Send a packet back to the client
    return req_handler_t::op_malformed;
}

template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::unimplemented_request() {
    // TODO: Send a packet back to the client
    return req_handler_t::op_malformed;
}

#endif // __BIN_MEMCACHED_HANDLER_TCC__

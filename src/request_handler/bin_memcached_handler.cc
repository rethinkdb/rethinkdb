#include <string.h>
#include "arch/arch.hpp"
#include "request.hpp"
#include "request_handler/bin_memcached_handler.hpp"
#include "btree/append_prepend_fsm.hpp"
#include "btree/delete_fsm.hpp"
#include "btree/get_fsm.hpp"
#include "btree/get_cas_fsm.hpp"
#include "btree/incr_decr_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"
#include "server.hpp"

class bin_memcached_request_t :
    public request_callback_t,
    public btree_fsm_callback_t
{
public:
    typedef bin_memcached_handler_t::packet_t packet_t;
    typedef bin_memcached_handler_t::bin_opcode_t bin_opcode_t;
    typedef bin_memcached_handler_t::bin_data_type_t bin_data_type_t;
    typedef bin_memcached_handler_t::bin_opaque_t bin_opaque_t;

public:
    bin_memcached_request_t(bin_memcached_handler_t *rh, packet_t *pkt)
        : request_callback_t(rh), opcode(pkt->opcode()), data_type(pkt->data_type()), opaque(pkt->opaque())
        {}

    /* build_response, necessary so that one request can send multiple packets */
    typedef enum {
        br_call_again,
        br_done,
    } br_result_t;
    
    virtual br_result_t build_response(packet_t *pkt) = 0;
    virtual ~bin_memcached_request_t() { }
    
    void on_btree_fsm_complete(btree_fsm_t *fsm) {
        // NULL in the case of perfmon.

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
            packet.magic(bin_memcached_handler_t::bin_magic_response);

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
};

class bin_memcached_get_request_t :
    public bin_memcached_request_t
{

public:
    typedef bin_memcached_handler_t::packet_t packet_t;
    typedef bin_memcached_request_t::br_result_t br_result_t;
    using bin_memcached_request_t::br_done;
    using bin_memcached_request_t::opcode;

public:
    bin_memcached_get_request_t(bin_memcached_handler_t *rh, packet_t *pkt, btree_key *key)
        : bin_memcached_request_t(rh, pkt), fsm(new btree_get_fsm_t(key, rh->server->store, this))
        {
        fsm->run(this);
    }
    
    ~bin_memcached_get_request_t() {
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        switch (fsm->status_code) {
            case btree_get_fsm_t::S_SUCCESS:
                res_pkt->status(bin_memcached_handler_t::bin_status_no_error);
                res_pkt->set_value(&fsm->value);
                res_pkt->set_extras(extra_flags, extra_flags_length);
                break;
            case btree_get_fsm_t::S_NOT_FOUND:
                res_pkt->status(bin_memcached_handler_t::bin_status_key_not_found);
                break;
            default:
                assert(0);
                break;
        }

        //check if the packet requires that a key be sent back
        if (bin_memcached_handler_t::is_key_code(opcode)) {
            res_pkt->set_key(&fsm->key);
        }
        return br_done;
    }
private:
    btree_get_fsm_t *fsm;
};

class bin_memcached_set_request_t :
    public bin_memcached_request_t
{

public:
    typedef bin_memcached_handler_t::packet_t packet_t;
    typedef bin_memcached_request_t::br_result_t br_result_t;
    using bin_memcached_request_t::br_done;

public:
    bin_memcached_set_request_t(bin_memcached_handler_t *rh, packet_t *pkt, btree_key *key, byte *data, uint32_t size, btree_set_fsm_t::set_type_t type, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, uint64_t req_cas)
        : bin_memcached_request_t(rh, pkt), fsm(new btree_set_fsm_t(key, rh->server->store, this, false, size, data, type, mcflags, exptime, req_cas))
        {
        assert(type == btree_set_fsm_t::set_type_set);   // We haven't hooked up ADD and REPLACE yet, and we're going to handle CAS differently.
        fsm->run(this);
    }
    
    ~bin_memcached_set_request_t() {
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        res_pkt->status(bin_memcached_handler_t::bin_status_no_error);
        // Set responses require don't require anything to be set. When we hook up ADD and REPLACE
        // then we will need to check for errors.
        // XXX Hook up ADD and REPLACE and add all the appropriate error codes.
        assert(fsm->status_code == btree_set_fsm_t::S_SUCCESS);
        return br_done;
    }
private:
    btree_set_fsm_t *fsm;
};

class bin_memcached_incr_decr_request_t :
    public bin_memcached_request_t
{

public:
    typedef bin_memcached_handler_t::packet_t packet_t;
    typedef bin_memcached_request_t::br_result_t br_result_t;
    using bin_memcached_request_t::br_done;

public:
    bin_memcached_incr_decr_request_t(bin_memcached_handler_t *rh, packet_t *pkt, btree_key *key, bool increment, long long delta)
        : bin_memcached_request_t(rh, pkt), fsm(new btree_incr_decr_fsm_t(key, rh->server->store, increment, delta))
        {
        fsm->run(this);
    }
    
    ~bin_memcached_incr_decr_request_t() {
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        // TODO: This should return the value.
        switch (fsm->status_code) {
            case btree_incr_decr_fsm_t::S_SUCCESS:
                res_pkt->status(bin_memcached_handler_t::bin_status_no_error);
                break;
            case btree_incr_decr_fsm_t::S_NOT_NUMERIC:
                res_pkt->status(bin_memcached_handler_t::bin_status_incr_decr_on_non_numeric_value);
                break;
            case btree_incr_decr_fsm_t::S_NOT_FOUND:
                res_pkt->status(bin_memcached_handler_t::bin_status_key_not_found);
                break;
            default: fail("Bad status code.");
        }
        return br_done;
    }
private:
    btree_incr_decr_fsm_t *fsm;
};

class bin_memcached_append_prepend_request_t :
    public bin_memcached_request_t
{

public:
    typedef bin_memcached_handler_t::packet_t packet_t;
    typedef bin_memcached_request_t::br_result_t br_result_t;
    using bin_memcached_request_t::br_done;

public:
    bin_memcached_append_prepend_request_t(bin_memcached_handler_t *rh, packet_t *pkt, btree_key *key, byte *data, int size, bool append) :
        bin_memcached_request_t(rh, pkt), fsm(new btree_append_prepend_fsm_t(key, rh->server->store, this, false, size, data, append)) {
        fsm->run(this);
    }

    ~bin_memcached_append_prepend_request_t() {
        delete fsm;
    }

    br_result_t build_response(packet_t *res_pkt) {
        switch (fsm->status_code) {
            case btree_append_prepend_fsm_t::S_SUCCESS:
                res_pkt->status(bin_memcached_handler_t::bin_status_no_error);
                break;
            case btree_append_prepend_fsm_t::S_NOT_STORED:
                res_pkt->status(bin_memcached_handler_t::bin_status_item_not_stored);
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

class bin_memcached_delete_request_t :
    public bin_memcached_request_t
{

public:
    typedef bin_memcached_handler_t::packet_t packet_t;
    typedef bin_memcached_request_t::br_result_t br_result_t;
    using bin_memcached_request_t::br_done;

public:
    bin_memcached_delete_request_t(bin_memcached_handler_t *rh, packet_t *pkt, btree_key *key)
        : bin_memcached_request_t(rh, pkt), fsm(new btree_delete_fsm_t(key, rh->server->store))
        {
        fsm->run(this);
    }
    
    ~bin_memcached_delete_request_t() {
        delete fsm;
    }
    
    br_result_t build_response(packet_t *res_pkt) {
        switch (fsm->status_code) {
            case btree_delete_fsm_t::S_DELETED:
                res_pkt->status(bin_memcached_handler_t::bin_status_no_error);
                break;
            case btree_delete_fsm_t::S_NOT_FOUND:
                res_pkt->status(bin_memcached_handler_t::bin_status_key_not_found);
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

class bin_memcached_perfmon_request_t :
    public bin_memcached_request_t,
    public cpu_message_t,   // For call_later_on_this_cpu()
    public perfmon_callback_t
{

public:
    typedef bin_memcached_handler_t::packet_t packet_t;
    typedef bin_memcached_request_t::br_result_t br_result_t;
    using bin_memcached_request_t::br_done;
    using bin_memcached_request_t::br_call_again;

public:
    perfmon_stats_t stats;
    
    bin_memcached_perfmon_request_t(bin_memcached_handler_t *rh, packet_t *pkt)
        : bin_memcached_request_t(rh, pkt) {

        if (perfmon_get_stats(&stats, this)) {
        
            /* The world is not ready for the power of completing a request immediately.
            So we delay so that the request handler doesn't get confused. */
            call_later_on_this_cpu(this);
        }
    }
    
    void on_cpu_switch() {
        on_perfmon_stats();
    }
    
    void on_perfmon_stats() {
        iter = stats.begin();
        on_btree_fsm_complete(NULL);   // Hack
    }
    
    ~bin_memcached_perfmon_request_t() {}
    
    br_result_t build_response(packet_t *res_pkt) {
        if (iter != stats.end()) {
            res_pkt->set_key(iter->first.c_str(), iter->first.size());
            res_pkt->set_value(iter->second.c_str(), iter->second.size());
            iter++;
            return br_call_again;
        } else {
            // we're also sending an empty packet here, but that doesn't require any code
            return br_done;
        }
    }

private:
    perfmon_stats_t::iterator iter;
};

//! Parse a binary command received from the user
bin_memcached_handler_t::parse_result_t bin_memcached_handler_t::parse_request(event_t *event)
{
    assert(event->state == conn_fsm);

    if (conn_fsm->nrbuf < sizeof(request_header_t))
        return request_handler_t::op_partial_packet;

    packet_t packet(conn_fsm->rbuf);
    packet_t *pkt = &packet;
    
    if (conn_fsm->nrbuf < pkt->size())
        return request_handler_t::op_partial_packet;

    if (!pkt->is_valid_request()) {
        conn_fsm->consume(pkt->size());
        return request_handler_t::op_malformed;
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
            res = request_handler_t::op_req_quit;
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

bin_memcached_handler_t::parse_result_t bin_memcached_handler_t::dispatch_appropriate_fsm(packet_t *pkt) {
    
    pkt->key(key);
    
    switch (pkt->opcode()) {
        case bin_opcode_get:
        case bin_opcode_getq:
        case bin_opcode_getk:
        case bin_opcode_getkq:
            new bin_memcached_get_request_t(this, pkt, key);
            //TODO even a quiet get eventually requires a response, so pipelining is broken
            return request_handler_t::op_req_complex;
        case bin_opcode_set:
        case bin_opcode_setq:
            // TODO: Set the actual flags after the new packet-parsing code is written.
            new bin_memcached_set_request_t(this, pkt, key, pkt->value(), pkt->value_length(), btree_set_fsm_t::set_type_set, 0, 0, false);
            break;
        case bin_opcode_add:
        case bin_opcode_addq:
            new bin_memcached_set_request_t(this, pkt, key, pkt->value(), pkt->value_length(), btree_set_fsm_t::set_type_add, 0, 0, false);
            break;
        case bin_opcode_replace:
        case bin_opcode_replaceq:
            new bin_memcached_set_request_t(this, pkt, key, pkt->value(), pkt->value_length(), btree_set_fsm_t::set_type_replace, 0, 0, false);
            break;
        case bin_opcode_increment:
        case bin_opcode_incrementq:
        {
            // FIXME delta is in extras, not the value.
            long long delta = atoll(pkt->value());
            new bin_memcached_incr_decr_request_t(this, pkt, key, true, delta);
            break;
        }
        case bin_opcode_decrement:
        case bin_opcode_decrementq:
        {
            long long delta = atoll(pkt->value());
            new bin_memcached_incr_decr_request_t(this, pkt, key, false, delta);
            break;
        }
        case bin_opcode_delete:
        case bin_opcode_deleteq:
            new bin_memcached_delete_request_t(this, pkt, key);
            break;
        case bin_opcode_append:
        case bin_opcode_appendq:
            new bin_memcached_append_prepend_request_t(this, pkt, key, pkt->value(), pkt->value_length(), true);
            break;
        case bin_opcode_prepend:
        case bin_opcode_prependq:
            new bin_memcached_append_prepend_request_t(this, pkt, key, pkt->value(), pkt->value_length(), false);
            break;
        default:
            fail("Invalid opcode in bin_memcached_handler_t::dispatch_appropriate_fsm");
    }
    
    if (is_quiet_code(pkt->opcode())) return request_handler_t::op_req_parallelizable;
    else return request_handler_t::op_req_complex;
}

bin_memcached_handler_t::parse_result_t bin_memcached_handler_t::no_op(packet_t *pkt) {
    //XXX this changes the passed packet good provided we don't need it anymore since it saves an allocation
    pkt->magic(bin_magic_response);

    conn_fsm->sbuf->append(pkt->data, pkt->size());
    return request_handler_t::op_req_send_now;
}

bin_memcached_handler_t::parse_result_t bin_memcached_handler_t::stat(packet_t *pkt) {
    new bin_memcached_perfmon_request_t(this, pkt);
    return request_handler_t::op_req_complex;
}

bin_memcached_handler_t::parse_result_t bin_memcached_handler_t::version(packet_t *pkt) {
    byte tmpbuf[MAX_PACKET_SIZE] = {0x00};
    packet_t res_pkt(tmpbuf);

    res_pkt.magic(bin_magic_response);
    res_pkt.opcode(pkt->opcode());
    res_pkt.set_value((byte *) VERSION_STRING, strlen(VERSION_STRING) - 1);

    conn_fsm->sbuf->append(res_pkt.data, res_pkt.size());
    return request_handler_t::op_req_send_now;
}

bin_memcached_handler_t::parse_result_t bin_memcached_handler_t::malformed_request() {
    // TODO: Send a packet back to the client
    return request_handler_t::op_malformed;
}

bin_memcached_handler_t::parse_result_t bin_memcached_handler_t::unimplemented_request() {
    // TODO: Send a packet back to the client
    return request_handler_t::op_malformed;
}


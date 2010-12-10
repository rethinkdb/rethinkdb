#include <string.h>
#include "arch/arch.hpp"
#include "request_handler/bin_memcached_handler.hpp"
#include "btree/append_prepend_fsm.hpp"
#include "btree/delete_fsm.hpp"
#include "btree/get_fsm.hpp"
#include "btree/get_cas_fsm.hpp"
#include "btree/incr_decr_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "conn_fsm.hpp"
#include "server.hpp"

typedef bin_memcached_handler_t::packet_t packet_t;

struct response_t : public packet_t {
    byte data[MAX_PACKET_SIZE];
    explicit response_t(packet_t *req) : packet_t(data)
    {
        opcode(req->opcode());
        data_type(req->data_type());
        opaque(req->opaque());
        extra_length(0);
        key_length(0);
        value_length(0);
        magic(bin_memcached_handler_t::bin_magic_response);
    }
    
    void send(bin_memcached_handler_t *rh) {
        rh->conn_fsm->sbuf->append(data, size());
    }
};

class bin_memcached_get_request_t :
    public store_t::get_callback_t
{
    bin_memcached_handler_t *rh;
    response_t response;

public:
    bin_memcached_get_request_t(bin_memcached_handler_t *rh, packet_t *req, btree_key *key)
        : rh(rh), response(req)
    {
        if (bin_memcached_handler_t::is_key_code(req->opcode())) response.set_key(key);
        rh->server->store->get(key, this);
    }
    
    void value(const_buffer_group_t *buffer, store_t::get_callback_t::done_callback_t *cb,
        mcflags_t flags, cas_t cas) {
        
        response.status(bin_memcached_handler_t::bin_status_no_error);
        
        byte_t *value = new char[buffer->get_size()];
        size_t byte = 0;
        for (int i = 0; i < (int)buffer->buffers.size(); i++) {
            memcpy(value + byte, buffer->buffers[i].data, buffer->buffers[i].size);
            byte += buffer->buffers[i].size;
        }
        assert(byte == buffer->get_size());
        response.set_value(value, buffer->get_size());
        delete value;
        cb->have_copied_value();
        
        response.set_extras(extra_flags, extra_flags_length);
        
        response.send(rh);
        
        delete this;
    }
    
    void not_found() {
        response.status(bin_memcached_handler_t::bin_status_key_not_found);
        
        response.send(rh);
        rh->request_complete();
        delete this;
    }
};

class bin_memcached_set_request_t :
    public store_t::set_callback_t
{
    bin_memcached_handler_t *rh;
    response_t response;
    buffered_data_provider_t dp;
    
public:
    bin_memcached_set_request_t(bin_memcached_handler_t *rh, packet_t *req, btree_key *key, byte *data, uint32_t size, btree_set_fsm_t::set_type_t type, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, uint64_t req_cas)
        : rh(rh), response(req), dp(data, size)
    {
        switch (type) {
            case btree_set_fsm_t::set_type_set:
                rh->server->store->set(key, &dp, mcflags, exptime, this);
                break;
            case btree_set_fsm_t::set_type_add:
                rh->server->store->add(key, &dp, mcflags, exptime, this);
                break;
            case btree_set_fsm_t::set_type_replace:
                rh->server->store->replace(key, &dp, mcflags, exptime, this);
                break;
            case btree_set_fsm_t::set_type_cas:
            default:
                unreachable("Not implemented");
        }
    }
    
    void stored() {
        response.status(bin_memcached_handler_t::bin_status_no_error);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
    
    void not_stored() {
        not_implemented("Not implemented");
    }
        
    void not_found() {
        not_implemented("Not implemented");
    }
        
    void exists() {
        not_implemented("Not implemented");
    }
    
    void too_large() {
        not_implemented("Not implemented");
    }
    
    void data_provider_failed() {
        unreachable("Cannot happen with a buffered data provider");
    }
};

class bin_memcached_incr_decr_request_t :
    public store_t::incr_decr_callback_t
{
    bin_memcached_handler_t *rh;
    response_t response;

public:
    bin_memcached_incr_decr_request_t(bin_memcached_handler_t *rh, packet_t *req, btree_key *key, bool increment, long long delta)
        : rh(rh), response(req)
    {
        if (increment) rh->server->store->incr(key, delta, this);
        else rh->server->store->decr(key, delta, this);
    }
    
    void success(long long unsigned int v) {
        // TODO: Include value in response
        response.status(bin_memcached_handler_t::bin_status_no_error);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
    
    void not_found() {
        response.status(bin_memcached_handler_t::bin_status_key_not_found);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
    
    void not_numeric() {
        response.status(bin_memcached_handler_t::bin_status_incr_decr_on_non_numeric_value);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
};

class bin_memcached_append_prepend_request_t :
    public store_t::append_prepend_callback_t
{
    bin_memcached_handler_t *rh;
    response_t response;
    buffered_data_provider_t dp;

public:
    bin_memcached_append_prepend_request_t(bin_memcached_handler_t *rh, packet_t *req, btree_key *key, byte *data, int size, bool append) :
        rh(rh), response(req), dp(data, size)
    {
        if (append) rh->server->store->append(key, &dp, this);
        else rh->server->store->prepend(key, &dp, this);
    }
    
    void success() {
        response.status(bin_memcached_handler_t::bin_status_no_error);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
    
    void too_large() {
        not_implemented("Not implemented");
    }
    
    void not_found() {
        response.status(bin_memcached_handler_t::bin_status_item_not_stored);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
    
    void data_provider_failed() {
        unreachable("Cannot happen with a buffered data provider");
    }
};

class bin_memcached_delete_request_t :
    public store_t::delete_callback_t
{
    bin_memcached_handler_t *rh;
    response_t response;

public:
    bin_memcached_delete_request_t(bin_memcached_handler_t *rh, packet_t *req, btree_key *key)
        : rh(rh), response(req)
    {
        rh->server->store->delete_key(key, this);
    }
    
    void deleted() {
        response.status(bin_memcached_handler_t::bin_status_no_error);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
    
    void not_found() {
        response.status(bin_memcached_handler_t::bin_status_key_not_found);
        response.send(rh);
        rh->request_complete();
        delete this;
    }
};

class bin_memcached_perfmon_request_t :
    public perfmon_callback_t
{
    bin_memcached_handler_t *rh;
    response_t response;

public:
    perfmon_stats_t stats;
    
    bin_memcached_perfmon_request_t(bin_memcached_handler_t *rh, packet_t *pkt)
        : rh(rh), response(pkt)
    {
        if (perfmon_get_stats(&stats, this)) on_perfmon_stats();
    }
    
    void on_perfmon_stats() {
        for (perfmon_stats_t::iterator iter = stats.begin(); iter != stats.end(); iter++) {
            response.set_key(iter->first.c_str(), iter->first.size());
            response.set_value(iter->second.c_str(), iter->second.size());
            response.send(rh);
        }
        rh->request_complete();
        delete this;
    }
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
            unreachable("Invalid opcode in bin_memcached_handler_t::dispatch_appropriate_fsm");
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

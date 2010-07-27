#ifndef __BIN_MEMCACHED_HANDLER_TCC__
#define __BIN_MEMCACHED_HANDLER_TCC__

#include <string.h>
#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "request_handler/bin_memcached_handler.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"

//! Parse a binary command received from the user
template<class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::parse_request(event_t *event)
{
    conn_fsm_t *fsm = (conn_fsm_t*)event->state;
    char *rbuf = fsm->rbuf; 
    unsigned int size = fsm->nrbuf;
    parse_result_t res;

    if (size < sizeof(request_header_t))
        return req_handler_t::op_partial_packet;

    packet_t packet(rbuf);
    packet_t *pkt = &packet;
    
    if (size < pkt->size())
        return req_handler_t::op_partial_packet;

    if (!pkt->is_valid_request()) {
        fsm->consume(pkt->size());
        return req_handler_t::op_malformed;
    }

    //Showtime!!
    switch (pkt->opcode()) {
        case bin_opcode_get:
        case bin_opcode_getq:
        case bin_opcode_getk:
        case bin_opcode_getkq:
            dispatch_appropriate_fsm(pkt, fsm);
            res = req_handler_t::op_req_complex; //TODO even a quiet get eventually requires a response, so pipelining is broken
            break;
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
            res = dispatch_appropriate_fsm(pkt, fsm);
            break;
        case bin_opcode_quit:
        case bin_opcode_quitq:
            res = req_handler_t::op_req_quit;
            break;
        case bin_opcode_flush:
        case bin_opcode_flushq:
        case bin_opcode_no_op:
<<<<<<< HEAD:src/request_handler/bin_memcached_handler.tcc
            res = no_op(pkt, fsm);
            break;
=======
>>>>>>> Restructured btree_fsms. Now starting to restructure request handlers.:src/request_handler/bin_memcached_handler.tcc
        case bin_opcode_version:
        case bin_opcode_stat:
            res = unimplemented_request(fsm);
            break;
        default:
            res = malformed_request(fsm);
            break;
    }
#ifdef MEMCACHED_STRICT
    fsm->corked = is_quiet_code(pkt->opcode()); //cork if the code is quiet
#endif

    fsm->consume(pkt->size());
    return res;
}


template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t
bin_memcached_handler_t<config_t>::dispatch_appropriate_fsm(packet_t *pkt, conn_fsm_t *fsm) {
    
    pkt->key(key);
    
    btree_fsm_t *btree_fsm;
    switch (pkt->opcode()) {
        case bin_opcode_get:
        case bin_opcode_getq:
        case bin_opcode_getk:
        case bin_opcode_getkq:
            btree_fsm = new btree_get_fsm_t(key);
            break;
        case bin_opcode_set:
        case bin_opcode_setq:
            btree_fsm = new btree_set_fsm_t(key, pkt->value(), pkt->value_length(), true, true);
            break;
        case bin_opcode_add:
        case bin_opcode_addq:
            btree_fsm = new btree_set_fsm_t(key, pkt->value(), pkt->value_length(), true, false);
            break;
        case bin_opcode_replace:
        case bin_opcode_replaceq:
            btree_fsm = new btree_set_fsm_t(key, pkt->value(), pkt->value_length(), false, true);
            break;
        case bin_opcode_increment:
        case bin_opcode_incrementq:
            long long delta = atoll(pkt->value());
            btree_fsm = new btree_incr_decr_fsm_t(key, true, delta);
            break;
        case bin_opcode_decrement:
        case bin_opcode_decrementq:
            long long delta = atoll(pkt->value());
            btree_fsm = new btree_incr_decr_fsm_t(key, false, delta);
            break;
        case bin_opcode_delete:
        case bin_opcode_deleteq:
            btree_fsm = new btree_delete_fsm_t(key);
            break;
        case bin_opcode_append:
        case bin_opcode_appendq:
        case bin_opcode_prepend:
        case bin_opcode_prependq:
            return unimplemented_request(fsm);
        default:
            check("Invalid opcode in bin_memcached_handler_t::dispatch_appropriate_fsm", 0);
            return;   // Placate GCC
    }
    
    fsm->current_request = new request_t(fsm);
    fsm->current_request->handler_data = new bin_handler_data_t(pkt);
    dispatch_btree_fsm(fsm, btree_fsm);
    
    if (is_quiet_code(pkt->opcode()))
        return req_handler_t::op_req_parallelizable;
    else
        return req_handler_t::op_req_complex;
}

template <class config_t>
typename bin_memcached_handler_t<config_t>::parse_result_t bin_memcached_handler_t<config_t>::no_op(packet_t *pkt, conn_fsm_t *fsm) {
    //XXX this changes the passed packet good provided we don't need it anymore since it saves an allocation
    pkt->magic(bin_magic_response);

    fsm->sbuf->append(pkt->data, pkt->size());
    return req_handler_t::op_req_send_now;
}

template<class config_t>
void bin_memcached_handler_t<config_t>::build_response(request_t *request) {


    // Since we're in the middle of processing a command,
    // fsm->sbuf must exist at this point.
    conn_fsm_t *fsm = request->netfsm;
    btree_get_fsm_t *btree_get_fsm = NULL;
    btree_set_fsm_t *btree_set_fsm = NULL;
    btree_delete_fsm_t *btree_delete_fsm = NULL;

    //reload the handler specific data
    byte tmpbuf[MAX_PACKET_SIZE];
    memset(tmpbuf, 0, MAX_PACKET_SIZE); //calm yo bitch ass valgrind
    packet_t res_packet(tmpbuf, (bin_handler_data_t *) request->handler_data);
    delete (bin_handler_data_t *) request->handler_data;
    packet_t *res_pkt = &res_packet;
    res_pkt->magic(bin_magic_response);

    btree_key *key = NULL; //there must be a key
    btree_value *value = NULL; //there may not be a value

    assert(request->nstarted > 0 && request->nstarted == request->ncompleted);

    btree_fsm_t *btree = (btree_fsm_t*) request->msgs[0];
    switch(btree->fsm_type) {
        case btree_fsm_t::btree_get_fsm:
            // TODO: make sure we don't overflow the buffer with sprintf
            btree_get_fsm = (btree_get_fsm_t*) btree;

            if(btree_get_fsm->op_result == btree_get_fsm_t::btree_found) {
                res_pkt->status(bin_status_no_error);
                value = btree_get_fsm->value;
                res_pkt->set_value(value);
                res_pkt->set_extras(extra_flags, extra_flags_length);
            } else {
                res_pkt->status(bin_status_key_not_found);
            }

            //check if the packet requires that a key be sent back
            if (is_key_code(res_pkt->opcode())) {
                key = btree_get_fsm->key;
                res_pkt->set_key(key);
            }


            if (is_quiet_code(res_pkt->opcode())) {
                if (res_pkt->status() != bin_status_no_error) {
                    ;//mum on error with quiet code
                } else {
                    fsm->sbuf->append(tmpbuf, res_pkt->size());
                }
            } else {
                fsm->sbuf->append(tmpbuf, res_pkt->size());
            }
            
            break;
        case btree_fsm_t::btree_set_fsm:
            btree_set_fsm = (btree_set_fsm_t*) btree;
            key = btree_set_fsm->key;

            switch(res_pkt->opcode()) {
                case bin_opcode_increment:
                case bin_opcode_incrementq:
                case bin_opcode_decrement:
                case bin_opcode_decrementq:
                    break;
                case bin_opcode_set:
                case bin_opcode_setq:
                case bin_opcode_add:
                case bin_opcode_addq:
                case bin_opcode_replace:
                case bin_opcode_replaceq:
                    break;
                default:
                    break;
            }

            res_pkt->status(bin_status_no_error);

            //Set responses require don't require anything to be set

            fsm->sbuf->append(tmpbuf, res_pkt->size());
            break;
        case btree_fsm_t::btree_delete_fsm:
            btree_delete_fsm = (btree_delete_fsm_t*) btree;
            key = btree_delete_fsm->key;

            res_pkt->status(bin_status_no_error);

            fsm->sbuf->append(tmpbuf, res_pkt->size());
            break;
        default:
            check("bin_memcached_handler_t::build_response - Unknown btree op", 0);
            break;
    }

    delete request;
    fsm->current_request = NULL;
}
#endif // __BIN_MEMCACHED_HANDLER_TCC__

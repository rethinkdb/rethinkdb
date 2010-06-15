
#ifndef __MEMCACHED_HANDLER_IMPL_HPP__
#define __MEMCACHED_HANDLER_IMPL_HPP__

#include <string.h>
#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "request_handler/memcached_handler.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"

// TODO: we should have a nicer way of switching on the command than a
// giant if/else statement (at least break them out into functions).

// TODO: if we receive a small request from the user that can be
// satisfied on the same CPU, we should probably special case it and
// do it right away, no need to send it to itself, process it, and
// then send it back to itself.

// Process commands received from the user
template<class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::parse_request(event_t *event)
{
    conn_fsm_t *fsm = (conn_fsm_t*)event->state;
    char *buf = fsm->buf;
    unsigned int size = fsm->nbuf;

    // TODO: if we get incomplete packets, we retokenize the entire
    // message every time we get a new piece of the packet. Perhaps it
    // would be more efficient to store tokenizer state across
    // requests. In general, tokenizing a request is silly, we should
    // really provide a binary API.

    // TODO: we might end up getting a command, and a piece of the
    // next command. That means we can't check that the end of the
    // recv buffer in NULL terminated, we gotta scan. It also means
    // that we can't use one buffer for both recv and send, we need to
    // add a send buffer (assuming we want to support out of band
    // commands).
    
    // Make sure the string is properly terminated
    if(buf[size - 1] != '\n' && buf[size - 1] != '\r') {
        return req_handler_t::op_partial_packet;
    }

    // Grab the command out of the string
    unsigned int token_size;
    char delims[] = " \t\n\r\0"; //This will not work quite as expected - the \0 will be processed as the end of the cstring, not as another delimiter
                                 //also, this seems more suited to a constant or a #define
    char *token = tokenize(buf, size, delims, &token_size);
    if(token == NULL)
        return req_handler_t::op_malformed;
    
    // Execute command
    if(token_eq("quit", token, token_size)) {
        // Make sure there's no more tokens
        if (contains_tokens(token+token_size, buf+size, delims))
            return req_handler_t::op_malformed;
        // Quit the connection
        return req_handler_t::op_req_quit;

    } else if(token_eq("shutdown", token, token_size)) {
        // Make sure there's no more tokens
        if (contains_tokens(token+token_size, buf+size, delims))
            return req_handler_t::op_malformed;
        // Shutdown the server
        return req_handler_t::op_req_shutdown;

    } else if(token_eq("set", token, token_size)) {
        // Make sure we have two more tokens
        unsigned int key_size;
        char *key = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &key_size);
        if(key == NULL)
            return req_handler_t::op_malformed;
        key[key_size] = '\0';
        unsigned int value_size;
        char *value = tokenize(key + key_size + 1,
                               buf + size - (key + key_size + 1),
                               delims, &value_size);
        if(value == NULL)
            return req_handler_t::op_malformed;
        value[value_size] = '\0';
        if((token = tokenize(value + value_size + 1,
                             buf + size - (value + value_size + 1),
                             delims, &token_size)) != NULL)
            return req_handler_t::op_malformed;

        // Ok, we've got a key, a value, and no more tokens, add them
        // to the tree
        set_key(fsm, atoi(key), atoi(value));

        return req_handler_t::op_req_complex;

    } else if(token_eq("get", token, token_size)) {
        // Make sure we have at least one more token
        unsigned int key_size;
        char *key = tokenize(token + token_size,
                             buf + size - (token + token_size),
                             delims, &key_size);
        if(key == NULL)
            return req_handler_t::op_malformed;
        key[key_size] = '\0';

        get_key(fsm, key, key_size, delims);

        return req_handler_t::op_req_complex;

    } else {
        // Invalid command
        return req_handler_t::op_malformed;
    }
    
    // The command was processed successfully
    return req_handler_t::op_malformed;
}


	

template<class config_t> void memcached_handler_t<config_t>::set_key(conn_fsm_t *fsm, int key, int value){
    btree_set_fsm_t *btree_fsm = new btree_set_fsm_t(get_cpu_context()->event_queue->cache);
    btree_fsm->init_update(key, value);
    req_handler_t::event_queue->message_hub.store_message(key_to_cpu(key, req_handler_t::event_queue->nqueues),
            btree_fsm);

    // Create request
    request_t *request = new request_t(fsm);
    request->fsms[request->nstarted] = btree_fsm;
    request->nstarted++;
    fsm->current_request = request;
    btree_fsm->request = request;
}

template<class config_t> void memcached_handler_t<config_t>::get_key(conn_fsm_t *fsm, char *key, unsigned int key_size, const char *delims){
    // Create request
    request_t *request = new request_t(fsm);

    do {
        // See if we can fit one more request
        if(request->nstarted == MAX_OPS_IN_REQUEST) {
            // We can't fit any more operations, let's just break
            // and complete the ones we already sent out to other
            // cores.
            break;

            // TODO: to a user, it will look like some of his
            // requests aren't satisfied. We need to notify them
            // somehow.
        }

        key[key_size] = '\0';
        int key_int = atoi(key);

        // Ok, we've got a key, initialize the FSM and add it to
        // the request
        btree_get_fsm_t *btree_fsm = new btree_get_fsm_t(get_cpu_context()->event_queue->cache);
        btree_fsm->request = request;
        btree_fsm->init_lookup(key_int);
        request->fsms[request->nstarted] = btree_fsm;
        request->nstarted++;

        // Add the fsm to appropriate queue
        req_handler_t::event_queue->message_hub
            .store_message(key_to_cpu(key_int, req_handler_t::event_queue->nqueues), btree_fsm);

        // Grab the next token
        key = tokenize(key + key_size + 1,
                fsm->buf + fsm->nbuf - (key + key_size + 1),
                delims, &key_size);
    } while(key);

    // Set the current request in the connection fsm
    fsm->current_request = request;
}



template<class config_t>
void memcached_handler_t<config_t>::build_response(request_t *request) {
    // Since we're in the middle of processing a command,
    // fsm->buf must exist at this point.
    conn_fsm_t *fsm = request->netfsm;
    char msg_nil[] = "NIL\n";
    char msg_ok[] = "ok\n";
    btree_get_fsm_t *btree_get_fsm = NULL;
    btree_set_fsm_t *btree_set_fsm = NULL;
    char *buf = fsm->buf;
    fsm->nbuf = 0;
    int count;
    
    assert(request->nstarted > 0 && request->nstarted == request->ncompleted);
    switch(request->fsms[0]->fsm_type) {
    case btree_fsm_t::btree_get_fsm:
        // TODO: make sure we don't overflow the buffer with sprintf
        for(unsigned int i = 0; i < request->nstarted; i++) {
            btree_get_fsm = (btree_get_fsm_t*)request->fsms[i];
            if(btree_get_fsm->op_result == btree_get_fsm_t::btree_found) {
                count = sprintf(buf, "%d\n", btree_get_fsm->value);
                fsm->nbuf += count;
                buf += count;
            } else if(btree_get_fsm->op_result == btree_get_fsm_t::btree_not_found) {
                count = strlen(msg_nil);
                strcpy(buf, msg_nil);
                fsm->nbuf += count;
                buf += count;
            }
            delete btree_get_fsm;
        }
        fsm->nbuf++;
        break;

    case btree_fsm_t::btree_set_fsm:
        // For now we only support one set operation at a time
        assert(request->nstarted == 1);

        btree_set_fsm = (btree_set_fsm_t*)request->fsms[0];
        strcpy(buf, msg_ok);
        fsm->nbuf = strlen(msg_ok) + 1;
        delete btree_set_fsm;
        break;

    default:
        check("memcached_handler_t::build_response - Unknown btree op", 1);
        break;
    }
    delete request;
    fsm->current_request = NULL;
}

#endif // __MEMCACHED_HANDLER_IMPL_HPP__

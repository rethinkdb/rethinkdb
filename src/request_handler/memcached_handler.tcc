#ifndef __MEMCACHED_HANDLER_TCC__
#define __MEMCACHED_HANDLER_TCC__

#include <string.h>
#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "request_handler/memcached_handler.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"

#define DELIMS " \t\n\r"
#define MALFORMED_RESPONSE "ERROR\r\n"
#define UNIMPLEMENTED_RESPONSE "SERVER_ERROR functionality not supported\r\n"
#define STORAGE_SUCCESS "STORED\r\n"
#define STORAGE_FAILURE "NOT_STORED\r\n"
#define RETRIEVE_TERMINATOR "END\r\n"

// TODO: if we receive a small request from the user that can be
// satisfied on the same CPU, we should probably special case it and
// do it right away, no need to send it to itself, process it, and
// then send it back to itself.

// Process commands received from the user
template<class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::parse_request(event_t *event)
{
    conn_fsm_t *fsm = (conn_fsm_t*)event->state;
    char *rbuf = fsm->rbuf; 
    unsigned int size = fsm->nrbuf;

    // TODO: we might end up getting a command, and a piece of the
    // next command. It also means that we can't use one buffer
    //for both recv and send, we need to add a send buffer
    // (assuming we want to support out of band  commands).
    
    // Find the first line in the buffer
    char *line_end = (char *)memchr(rbuf, '\n', size);
    if (line_end == NULL) {   //make sure \n is in the buffer
        for (unsigned int i = 0; i < size; i++)
        return req_handler_t::op_partial_packet;    //if \n is at the beginning of the buffer, or if it is not preceeded by \r, the request is malformed
    }

    if (loading_data)
        return read_data(rbuf, size, fsm);

    if (line_end == rbuf || line_end[-1] != '\r')
        return malformed_request(fsm);

    // if we're not reading a binary blob, then the line will be a string - let's null terminate it
    *line_end = '\0';
    unsigned int line_len = line_end - rbuf + 1;

    // get the first token to determine the command
    char *state;
    char *cmd_str = strtok_r(rbuf, DELIMS, &state);

    if(cmd_str == NULL)
        return malformed_request(fsm);
    
    // Execute command
    if(!strcmp(cmd_str, "quit")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state))  //strtok will return NULL if there are no more tokens
            return malformed_request(fsm);
        // Quit the connection
        fsm->consume(fsm->nrbuf);
        return req_handler_t::op_req_quit;

    } else if(!strcmp(cmd_str, "shutdown")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state))  //strtok will return NULL if there are no more tokens
            return malformed_request(fsm);
        // Shutdown the server
        // clean out the rbuf
        fsm->consume(fsm->nrbuf);
        return req_handler_t::op_req_shutdown;

    } else if(!strcmp(cmd_str, "set")) {     // check for storage commands
            return parse_storage_command(SET, state, line_len, fsm);
    } else if(!strcmp(cmd_str, "add")) {
            return parse_storage_command(ADD, state, line_len, fsm);
    } else if(!strcmp(cmd_str, "replace")) {
            return parse_storage_command(REPLACE, state, line_len, fsm);
    } else if(!strcmp(cmd_str, "append")) {
            return parse_storage_command(APPEND, state, line_len, fsm);
    } else if(!strcmp(cmd_str, "prepend")) {
            return parse_storage_command(PREPEND, state, line_len, fsm);
    } else if(!strcmp(cmd_str, "cas")) {
            return parse_storage_command(CAS, state, line_len, fsm);

    } else if(!strcmp(cmd_str, "get")) {    // check for retrieval commands
            return get(state, false, fsm);
    } else if(!strcmp(cmd_str, "gets")) {
            return get(state, true, fsm);

    } else if(!strcmp(cmd_str, "delete")) {
        return remove(state, fsm);

    } else if(!strcmp(cmd_str, "incr")) {
        return adjust(state, true, fsm);
    } else if(!strcmp(cmd_str, "decr")) {
        return adjust(state, false, fsm);
    } else {
        // Invalid command
        return malformed_request(fsm);
    }
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::parse_storage_command(storage_command command, char *state, unsigned int line_len, conn_fsm_t *fsm) {
    char *key_tmp = strtok_r(NULL, DELIMS, &state);
    char *flags_str = strtok_r(NULL, DELIMS, &state);
    char *exptime_str = strtok_r(NULL, DELIMS, &state);
    char *bytes_str = strtok_r(NULL, DELIMS, &state);
    char *cas_unique_str = NULL;
    if (command == CAS)
        cas_unique_str = strtok_r(NULL, DELIMS, &state);
    char *noreply_str = strtok_r(NULL, DELIMS, &state); //optional

    if (key_tmp == NULL || flags_str == NULL || exptime_str == NULL || bytes_str == NULL || (command == CAS && cas_unique_str == NULL)) //check for proper number of arguments
        return malformed_request(fsm);

    cmd = command;
    node_handler::str_to_key(key_tmp, key);

    char *invalid_char;
    flags = strtoul(flags_str, &invalid_char, 10);  //a 32 bit integer.  int alone does not guarantee 32 bit length
    if (*invalid_char != '\0')  // ensure there were no improper characters in the token - i.e. parse was successful
        return malformed_request(fsm);

    exptime = strtoul(exptime_str, &invalid_char, 10);
    if (*invalid_char != '\0')
        return malformed_request(fsm);

    bytes = strtoul(bytes_str, &invalid_char, 10);
    if (*invalid_char != '\0')
        return malformed_request(fsm);

    if (cmd == CAS) {
        cas_unique = strtoull(cas_unique_str, &invalid_char, 10);
        if (*invalid_char != '\0')
            return malformed_request(fsm);
    }

    noreply = false;
    if (noreply_str != NULL) {
        if (!strcmp(noreply_str, "noreply")) {
            noreply = true;
        } else {
            return malformed_request(fsm);
        }
    }

    fsm->consume(line_len); //consume the line
    loading_data = true;

    return read_data(fsm->rbuf, fsm->nrbuf, fsm);
}
	
template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::read_data(char *data, unsigned int size, conn_fsm_t *fsm) {
    check("memcached handler should be in loading data state", !loading_data);
    if (size < bytes + 2){//check that the buffer contains enough data.  must also include \r\n
        return req_handler_t::op_partial_packet;
    }

    loading_data = false;
    parse_result_t ret;
    switch(cmd) {
        case SET:
            ret = set(data, fsm, btree_set_kind_set);
            break;
        case ADD:
            ret = set(data, fsm, btree_set_kind_add);
            break;
        case REPLACE:
            ret = set(data, fsm, btree_set_kind_replace);
            break;
        case APPEND:
            ret = append(data, fsm);
            break;
        case PREPEND:
            ret = prepend(data, fsm);
            break;
        case CAS:
            ret = prepend(data, fsm);
            break;
        default:
            ret = malformed_request(fsm);
            break;
    }

    return ret;
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::set(char *data, conn_fsm_t *fsm, btree_set_kind set_kind) {
    btree_set_fsm_t *btree_fsm = new btree_set_fsm_t(get_cpu_context()->event_queue->cache);
    btree_fsm->init_update(key, data, bytes, set_kind);
    req_handler_t::event_queue->message_hub.store_message(key_to_cpu(key, req_handler_t::event_queue->nqueues),
            btree_fsm);

    // Create request
    request_t *request = new request_t(fsm);
    request->fsms[request->nstarted] = btree_fsm;
    request->nstarted++;
    fsm->current_request = request;
    btree_fsm->request = request;

    fsm->consume(bytes+2);
    return req_handler_t::op_req_complex;
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::add(char *data, conn_fsm_t *fsm) {
    return unimplemented_request(fsm);
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::replace(char *data, conn_fsm_t *fsm) {
    return unimplemented_request(fsm);
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::append(char *data, conn_fsm_t *fsm) {
    return unimplemented_request(fsm);
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::prepend(char *data, conn_fsm_t *fsm) {
    return unimplemented_request(fsm);
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::cas(char *data, conn_fsm_t *fsm) {
    return unimplemented_request(fsm);
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::malformed_request(conn_fsm_t *fsm) {
    write_msg(fsm, MALFORMED_RESPONSE);
    return req_handler_t::op_malformed;
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::unimplemented_request(conn_fsm_t *fsm) {
    write_msg(fsm, UNIMPLEMENTED_RESPONSE);
    return req_handler_t::op_malformed;
}

template <class config_t>
void memcached_handler_t<config_t>::write_msg(conn_fsm_t *fsm, const char *str) {
    int len = strlen(str);
    memcpy(fsm->sbuf, str, len+1);
    fsm->nsbuf = len+1;
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::get(char *state, bool include_unique, conn_fsm_t *fsm) {
    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL)
        return malformed_request(fsm);

    if (include_unique)
        return unimplemented_request(fsm);

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

        node_handler::str_to_key(key_str, key);

        // Ok, we've got a key, initialize the FSM and add it to
        // the request
        btree_get_fsm_t *btree_fsm = new btree_get_fsm_t(get_cpu_context()->event_queue->cache);
        btree_fsm->request = request;
        btree_fsm->init_lookup(key);
        request->fsms[request->nstarted] = btree_fsm;
        request->nstarted++;

        // Add the fsm to appropriate queue
        req_handler_t::event_queue->message_hub.store_message(key_to_cpu(key, req_handler_t::event_queue->nqueues), btree_fsm);
        key_str = strtok_r(NULL, DELIMS, &state);
    } while(key_str);

    // Set the current request in the connection fsm
    fsm->current_request = request;

    //clean out the rbuf
    fsm->consume(fsm->nrbuf);
    return req_handler_t::op_req_complex;
}


template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::remove(char *state, conn_fsm_t *fsm) {
    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL)
        return malformed_request(fsm);

    unsigned long time = 0;
    bool noreply = false;
    char *time_or_noreply_str = strtok_r(NULL, DELIMS, &state);
    if (time_or_noreply_str != NULL) {
        if (!strcmp(time_or_noreply_str, "noreply")) {
            noreply = true;
        } else { //must represent a time, then
            char *invalid_char;
            time = strtoul(time_or_noreply_str, &invalid_char, 10);
            if (*invalid_char != '\0')  // ensure there were no improper characters in the token - i.e. parse was successful
                return unimplemented_request(fsm);

            // see if there's a noreply arg too
            char *noreply_str = strtok_r(NULL, DELIMS, &state);
            if (noreply_str != NULL) {
                if (!strcmp(noreply_str, "noreply")) {
                    noreply = true;
                } else {
                    return malformed_request(fsm);
                }
            }
        }
    }

    node_handler::str_to_key(key_str, key);

    // parsed successfully, but functionality not yet implemented
    //return unimplemented_request(fsm);
    // Create request
    btree_delete_fsm_t *btree_fsm = new btree_delete_fsm_t(get_cpu_context()->event_queue->cache);
    btree_fsm->init_delete(key);

    request_t *request = new request_t(fsm);
    request->fsms[request->nstarted] = btree_fsm;
    request->nstarted++;
    fsm->current_request = request;
    btree_fsm->request = request;
    fsm->current_request = request;

    req_handler_t::event_queue->message_hub.store_message(key_to_cpu(key, req_handler_t::event_queue->nqueues), btree_fsm);

    //clean out the rbuf
    fsm->consume(fsm->nrbuf);

    return req_handler_t::op_req_complex;
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::adjust(char *state, bool inc, conn_fsm_t *fsm) {
    char *key_str = strtok_r(NULL, DELIMS, &state);
    char *value_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL || value_str == NULL)
        return malformed_request(fsm);

    bool noreply = false;
    char *noreply_str = strtok_r(NULL, DELIMS, &state);
    if (noreply_str != NULL) {
        if (!strcmp(noreply_str, "noreply")) {
            noreply = true;
        } else {
            return malformed_request(fsm);
        }
    }

    // parsed successfully, but functionality not yet implemented
    return unimplemented_request(fsm);
}
    
template<class config_t>
void memcached_handler_t<config_t>::build_response(request_t *request) {
    // Since we're in the middle of processing a command,
    // fsm->buf must exist at this point.
    conn_fsm_t *fsm = request->netfsm;
    btree_get_fsm_t *btree_get_fsm = NULL;
    btree_set_fsm_t *btree_set_fsm = NULL;
    btree_delete_fsm_t *btree_delete_fsm = NULL;
    char *sbuf = fsm->sbuf;
    fsm->nsbuf = 0;
    int count;
    
    assert(request->nstarted > 0 && request->nstarted == request->ncompleted);
    switch(request->fsms[0]->fsm_type) {
    case btree_fsm_t::btree_get_fsm:
        // TODO: make sure we don't overflow the buffer with sprintf
        for(unsigned int i = 0; i < request->nstarted; i++) {
            btree_get_fsm = (btree_get_fsm_t*)request->fsms[i];
            if(btree_get_fsm->op_result == btree_get_fsm_t::btree_found) {
                //TODO: support flags
                btree_key *key = btree_get_fsm->key;
                btree_value *value = btree_get_fsm->value;
                count = sprintf(sbuf, "VALUE %*.*s %u %u\r\n%*.*s\r\n", key->size, key->size, key->contents, 0, value->size, value->size, value->size, value->contents);
                fsm->nsbuf += count;
                sbuf += count;
            } else if(btree_get_fsm->op_result == btree_get_fsm_t::btree_not_found) {
                // do nothing
            }
            delete btree_get_fsm;
        }
        count = sprintf(sbuf, RETRIEVE_TERMINATOR);
        fsm->nsbuf += count;
        break;

    case btree_fsm_t::btree_set_fsm:
        // For now we only support one set operation at a time
        assert(request->nstarted == 1);

        btree_set_fsm = (btree_set_fsm_t*)request->fsms[0];
        
        if (!btree_set_fsm->set_was_successful) {
            strcpy(sbuf,STORAGE_FAILURE);
            fsm->nsbuf = strlen(STORAGE_FAILURE);
        }else if (!noreply) {
            strcpy(sbuf, STORAGE_SUCCESS);
            fsm->nsbuf = strlen(STORAGE_SUCCESS);
        } else {
            fsm->nsbuf = 0;
        }
        delete btree_set_fsm;
        break;

    case btree_fsm_t::btree_delete_fsm:
        // For now we only support one delete operation at a time
        assert(request->nstarted == 1);

        btree_delete_fsm = (btree_delete_fsm_t*)request->fsms[0];

        if(btree_delete_fsm->op_result == btree_delete_fsm_t::btree_found) {
            count = sprintf(sbuf, "DELETED\r\n");
            fsm->nsbuf += count;
            sbuf += count; //for when we do support multiple deletes at a time
        } else if (btree_delete_fsm->op_result == btree_delete_fsm_t::btree_not_found) {
            count = sprintf(sbuf, "NOT_FOUND\r\n");
            fsm->nsbuf += count;
            sbuf += count;
        } else {
            check("memchached_handler_t::build_response - Uknown value for btree_delete_fsm->op_result\n", 0);
        }
        delete btree_delete_fsm;
        break;

    default:
        check("memcached_handler_t::build_response - Unknown btree op", 0);
        break;
    }
    delete request;
    fsm->current_request = NULL;
}

#endif // __MEMCACHED_HANDLER_TCC__

#ifndef __MEMCACHED_HANDLER_TCC__
#define __MEMCACHED_HANDLER_TCC__

#include <string.h>
#include <string>
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
#define NOT_FOUND "NOT_FOUND\r\n"
#define RETRIEVE_TERMINATOR "END\r\n"
#define BAD_BLOB "CLIENT_ERROR bad data chunk\r\n"

#include <iostream>
using namespace std;
// Please read and understand the memcached protocol before modifying this
// file. If you only do a cursory readthrough, please check with someone who
// has read it in depth before comitting.

// TODO: if we receive a small request from the user that can be
// satisfied on the same CPU, we should probably special case it and
// do it right away, no need to send it to itself, process it, and
// then send it back to itself.

// Process commands received from the user
typedef basic_string<char, char_traits<char>, gnew_alloc<char> > custom_string;

template<class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::parse_request(event_t *event)
{
    conn_fsm_t *fsm = (conn_fsm_t*)event->state;
    char *rbuf = fsm->rbuf; 
    unsigned int size = fsm->nrbuf;
    parse_result_t res;

    // TODO: we might end up getting a command, and a piece of the
    // next command. It also means that we can't use one buffer
    //for both recv and send, we need to add a send buffer
    // (assuming we want to support out of band  commands).
    
    // check if we're supposed to be reading a binary blob
    if (loading_data) {
        return read_data(rbuf, size, fsm);
    }

    // Find the first line in the buffer
    // This is only valid if we are not reading binary data
    char *line_end = (char *)memchr(rbuf, '\n', size);
    if (line_end == NULL) {   //make sure \n is in the buffer
        return req_handler_t::op_partial_packet;    //if \n is at the beginning of the buffer, or if it is not preceeded by \r, the request is malformed
    }
    unsigned int line_len = line_end - rbuf + 1;


    if (line_end == rbuf || line_end[-1] != '\r') {
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    // if we're not reading a binary blob, then the line will be a string - let's null terminate it
    *line_end = '\0';

    // get the first token to determine the command
    char *state;
    char *cmd_str = strtok_r(rbuf, DELIMS, &state);

    if(cmd_str == NULL) {
        fsm->consume(line_len);
        return malformed_request(fsm);
    }
    
    // Execute command
    if(!strcmp(cmd_str, "quit")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state)) {  //strtok will return NULL if there are no more tokens
            fsm->consume(line_len);
            return malformed_request(fsm);
        }
        // Quit the connection
        fsm->consume(fsm->nrbuf);
        return req_handler_t::op_req_quit;

    } else if(!strcmp(cmd_str, "shutdown")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state)) {  //strtok will return NULL if there are no more tokens
            fsm->consume(line_len);
            return malformed_request(fsm);
        }
        // Shutdown the server
        // clean out the rbuf
        fsm->consume(fsm->nrbuf);
        return req_handler_t::op_req_shutdown;
    } else if(!strcmp(cmd_str, "stats")) {
            return print_stats(fsm, line_len);
    } else if(!strcmp(cmd_str, "set")) {     // check for storage commands
            res = parse_storage_command(SET, state, line_len, fsm);
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
            return get(state, false, line_len, fsm);
    } else if(!strcmp(cmd_str, "gets")) {
            return get(state, true, line_len, fsm);

    } else if(!strcmp(cmd_str, "delete")) {
        return remove(state, line_len, fsm);

    } else if(!strcmp(cmd_str, "incr")) {
        return parse_adjustment(true, state, line_len, fsm);
    } else if(!strcmp(cmd_str, "decr")) {
        return parse_adjustment(false, state, line_len, fsm);
    } else {
        // Invalid command
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    if (loading_data && fsm->nrbuf > 0) {
        assert(res == req_handler_t::op_partial_packet);
        return parse_request(event);
    } else {
        return res;
    }
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::parse_adjustment(bool increment, char *state, unsigned int line_len, conn_fsm_t *fsm) {
    char *key_tmp = strtok_r(NULL, DELIMS, &state);
    char *value_str = strtok_r(NULL, DELIMS, &state);
    char *noreply_str = strtok_r(NULL, DELIMS, &state); //optional
    if (key_tmp == NULL || value_str == NULL) {
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    bool noreply = false;
    if (noreply_str != NULL) {
        if (!strcmp(noreply_str, "noreply")) {
            this->noreply = true;
        } else {
            fsm->consume(line_len);
            return malformed_request(fsm);
        }
    }

    node_handler::str_to_key(key_tmp, key);

    fsm->consume(line_len);
    return fire_set_fsm(increment ? btree_set_type_incr : btree_set_type_decr, key, value_str, strlen(value_str), noreply, fsm);
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

    //check for proper number of arguments
    if ((key_tmp == NULL || flags_str == NULL || exptime_str == NULL || bytes_str == NULL || (command == CAS && cas_unique_str == NULL))) {
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    cmd = command;
    node_handler::str_to_key(key_tmp, key);

    char *invalid_char;
    flags = strtoul(flags_str, &invalid_char, 10);  //a 32 bit integer.  int alone does not guarantee 32 bit length
    if (*invalid_char != '\0') {  // ensure there were no improper characters in the token - i.e. parse was successful
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    exptime = strtoul(exptime_str, &invalid_char, 10);
    if (*invalid_char != '\0') {
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    bytes = strtoul(bytes_str, &invalid_char, 10);
    if (*invalid_char != '\0') {
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    if (cmd == CAS) {
        cas_unique = strtoull(cas_unique_str, &invalid_char, 10);
        if (*invalid_char != '\0') {
            fsm->consume(line_len);
            return malformed_request(fsm);
        }
    }

    this->noreply = false;
    if (noreply_str != NULL) {
        if (!strcmp(noreply_str, "noreply")) {
            this->noreply = true;
        } else {
            fsm->consume(line_len);
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
    if (data[bytes] != '\r' || data[bytes+1] != '\n') {
        fsm->consume(bytes+2);
        write_msg(fsm, BAD_BLOB);
        return req_handler_t::op_malformed;
    }

    parse_result_t ret;
    switch(cmd) {
        case SET:
            ret = fire_set_fsm(btree_set_type_set, this->key, data, this->bytes, this->noreply, fsm);
            break;
        case ADD:
            ret = fire_set_fsm(btree_set_type_add, this->key, data, this->bytes, this->noreply, fsm);
            break;
        case REPLACE:
            ret = fire_set_fsm(btree_set_type_replace, this->key, data, this->bytes, this->noreply, fsm);
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

    fsm->consume(this->bytes+2);

    return ret;
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::fire_set_fsm(btree_set_type type, btree_key *key, char *value_data, uint32_t value_len, bool noreply, conn_fsm_t *conn_fsm) {
    btree_set_fsm_t *set_fsm = new btree_set_fsm_t(get_cpu_context()->event_queue->cache);
    set_fsm->noreply = noreply;
    set_fsm->init_update(key, value_data, value_len, type);
    req_handler_t::event_queue->message_hub.store_message(key_to_cpu(key, req_handler_t::event_queue->nqueues),
            set_fsm);

    // Create request
    request_t *request = new request_t(conn_fsm);
    request->fsms[0] = set_fsm;
    request->nstarted = 1;
    conn_fsm->current_request = request;
    set_fsm->request = request;
    
    if (noreply)
        return req_handler_t::op_req_parallelizable;
    else
        return req_handler_t::op_req_complex;
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
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::get(char *state, bool include_unique, unsigned int line_len, conn_fsm_t *fsm) {
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
    fsm->consume(line_len);
    return req_handler_t::op_req_complex;
}


template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::remove(char *state, unsigned int line_len, conn_fsm_t *fsm) {
    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL) {
        fsm->consume(line_len);
        return malformed_request(fsm);
    }

    unsigned long time = 0;
    this->noreply = false;
    char *time_or_noreply_str = strtok_r(NULL, DELIMS, &state);
    if (time_or_noreply_str != NULL) {
        if (!strcmp(time_or_noreply_str, "noreply")) {
            this->noreply = true;
        } else { //must represent a time, then
            char *invalid_char;
            time = strtoul(time_or_noreply_str, &invalid_char, 10);
            if (*invalid_char != '\0')  // ensure there were no improper characters in the token - i.e. parse was successful
                return unimplemented_request(fsm);

            // see if there's a noreply arg too
            char *noreply_str = strtok_r(NULL, DELIMS, &state);
            if (noreply_str != NULL) {
                if (!strcmp(noreply_str, "noreply")) {
                    this->noreply = true;
                } else {
                    fsm->consume(line_len);
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
    btree_fsm->noreply = this->noreply;
    btree_fsm->init_delete(key);

    request_t *request = new request_t(fsm);
    request->fsms[request->nstarted] = btree_fsm;
    request->nstarted++;
    fsm->current_request = request;
    btree_fsm->request = request;
    fsm->current_request = request;

    req_handler_t::event_queue->message_hub.store_message(key_to_cpu(key, req_handler_t::event_queue->nqueues), btree_fsm);

    //clean out the rbuf
    fsm->consume(line_len);

    if (this->noreply)
        return req_handler_t::op_req_parallelizable;
    else
        return req_handler_t::op_req_complex;
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
    
    if (request->fsms[0]->type == cpu_message_t::mt_btree)
    {
        btree_fsm_t *btree = dynamic_cast<btree_fsm_t*>(request->fsms[0]);
        switch(btree->fsm_type) {
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
            }
            count = sprintf(sbuf, RETRIEVE_TERMINATOR);
            fsm->nsbuf += count;
            break;
    
        case btree_fsm_t::btree_set_fsm:
            // For now we only support one set operation at a time
            assert(request->nstarted == 1);
    
            btree_set_fsm = (btree_set_fsm_t*)request->fsms[0];
    
            if (btree_set_fsm->noreply) {
                // if noreply is set do not reply regardless of success or failure
                fsm->nsbuf=0;
            } else {
                // noreply not set, send reply depending on type of request
    
                switch(btree_set_fsm->get_set_type()) {
                case btree_set_type_incr:
                case btree_set_type_decr:
                    if (btree_set_fsm->set_was_successful) {
                        strncpy(sbuf, btree_set_fsm->get_value()->contents, btree_set_fsm->get_value()->size);
                        sbuf[btree_set_fsm->get_value()->size + 0] = '\r';
                        sbuf[btree_set_fsm->get_value()->size + 1] = '\n';
                        fsm->nsbuf = btree_set_fsm->get_value()->size + 2;
                    } else {
                        strcpy(sbuf, NOT_FOUND);
                        fsm->nsbuf = strlen(NOT_FOUND);
                    }
                    break;
    
                case btree_set_type_set:
                case btree_set_type_add:
                case btree_set_type_replace:
                    if (btree_set_fsm->set_was_successful) {
                        strcpy(sbuf, STORAGE_SUCCESS);
                        fsm->nsbuf = strlen(STORAGE_SUCCESS);
                    } else {
                        strcpy(sbuf,STORAGE_FAILURE);
                        fsm->nsbuf = strlen(STORAGE_FAILURE);
                    }
                    break;
                }
            }
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
                count = sprintf(sbuf, NOT_FOUND);
                fsm->nsbuf += count;
                sbuf += count;
            } else {
                check("memchached_handler_t::build_response - Uknown value for btree_delete_fsm->op_result\n", 0);
            }
            break;
    
        default:
            check("memcached_handler_t::build_response - Unknown btree op", 0);
            break;
        }
    }else if (request->fsms[0]->type == cpu_message_t::mt_stats_response)
    {
        stats *request_stat = dynamic_cast<stats *>(request->fsms[0]);
        map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > *registry = request_stat->get();
        custom_string response = "";
        map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::iterator iter;
        for (iter=registry->begin();iter != registry->end();iter++)
        {
            response += "STAT " + iter->first + " " + iter->second->get_value() + "\r\n";
//                sprintf("STAT %s %s\r\n",iter->first.c_str(), iter->second->get_value().c_str());
        }
            strcpy(sbuf, response.c_str());
            fsm->nsbuf = strlen(response.c_str());
    }
    delete request;
    fsm->current_request = NULL;
}

template <class config_t>
typename memcached_handler_t<config_t>::parse_result_t memcached_handler_t<config_t>::print_stats(conn_fsm_t *fsm, unsigned int line_len) {

    // put text in sbuf and length in nsbuf
    worker_pool_t *worker_pool = get_cpu_context()->event_queue->parent_pool;
    int nworkers = (int)worker_pool->nworkers;

    // this creates a deep copy thanks to stat's operator= overload
/*
    stats stat = get_cpu_context()->event_queue->stat;
    map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > *registry = stat.get();
*/
    int id = get_cpu_context()->event_queue->queue_id;
    stats_request *req = new stats_request(id);
    
    req->conn_fsm = fsm;
    get_cpu_context()->event_queue->stat.conn_fsm = fsm;
    
    // tell every single CPU core to pass their stats module *by copy* to this CPU
    for (int i=0;i<nworkers;i++)
    {
        if (i != id) {
            req_handler_t::event_queue->message_hub.store_message(i, req);
        }
    }
    

    /* first, add the variables from each event_queue stat. */
/*
    for (unsigned int i=0;i<nworkers;i++)
    {
        map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > *cur_registry = worker_pool->workers[i]->stat.get();
        if (registry == cur_registry) continue;
        map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator cur_iter;        
        for (cur_iter=cur_registry->begin();cur_iter != cur_registry->end();cur_iter++)
        {
            (*registry)[cur_iter->first]->add(cur_iter->second);
        }
    }
    
    // now, print out those variables:
    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator iter;
    for (iter=registry->begin();iter != registry->end();iter++)
    {
        cout << "STAT " << iter->first << " " << iter->second->get_value() << endl;
    }
*/
    fsm->consume(line_len);
    return req_handler_t::op_req_complex;    
}

template <class config_t>
void memcached_handler_t<config_t>::accumulate_stats(stats *stat)
{
    /* accumulate */
    map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > *registry = get_cpu_context()->event_queue->stat.get();
    map<custom_string, base_type *, std::less<custom_string>, gnew_alloc<base_type*> > *cur_registry = stat->get();

    map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator cur_iter;        
    for (cur_iter=cur_registry->begin();cur_iter != cur_registry->end();cur_iter++)
    {
        (*registry)[cur_iter->first]->add(cur_iter->second);
    }
    stats_counter++;
    
    /* print out stats if we have received the stats module from all the cores */
    if (stats_counter == get_cpu_context()->event_queue->nqueues - 1)
    {
        map<custom_string, base_type *, less<custom_string>, gnew_alloc<base_type*> >::const_iterator iter;
        for (iter=registry->begin();iter != registry->end();iter++)
        {
            cout << "STAT " << iter->first << " " << iter->second->get_value() << endl;
        }
        stats_counter = 0;
        
        /* TODO: we need to clean out the stat values for all cores now. */
    }
}

#endif // __MEMCACHED_HANDLER_TCC__
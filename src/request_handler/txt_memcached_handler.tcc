#ifndef __TXT_MEMCACHED_HANDLER_TCC__
#define __TXT_MEMCACHED_HANDLER_TCC__

#include <string.h>
#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "request_handler/txt_memcached_handler.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"
#include "request.hpp"

#define DELIMS " \t\n\r"
#define MALFORMED_RESPONSE "ERROR\r\n"
#define UNIMPLEMENTED_RESPONSE "SERVER_ERROR functionality not supported\r\n"
#define STORAGE_SUCCESS "STORED\r\n"
#define STORAGE_FAILURE "NOT_STORED\r\n"
#define NOT_FOUND "NOT_FOUND\r\n"
#define DELETE_SUCCESS "DELETED\r\n"
#define RETRIEVE_TERMINATOR "END\r\n"
#define BAD_BLOB "CLIENT_ERROR bad data chunk\r\n"

// Please read and understand the memcached protocol before modifying this
// file. If you only do a cursory readthrough, please check with someone who
// has read it in depth before comitting.

template<class config_t>
class txt_memcached_request : public request_callback_t {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;

public:
    txt_memcached_request(txt_memcached_handler_t<config_t> *rh, bool noreply)
        : rh(rh), noreply(noreply), request(new request_t(this))
        {}
    
    virtual void build_response(typename conn_fsm_t::linked_buf_t *) = 0;
    
    void on_request_completed() {
        if (!noreply) {
            build_response(rh->conn_fsm->sbuf);
            rh->request_complete();
        }
        delete this;
    }
    
    txt_memcached_handler_t<config_t> *rh;
    bool noreply;
    request_t *request;
};

template<class config_t>
class txt_memcached_get_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_get_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_get_fsm_t btree_get_fsm_t;
    using txt_memcached_request<config_t>::rh;

public:
    // A txt_memcached_get_request supports more than one get at one time, so it takes several steps
    // to build it. First the constructor is called; then add_get() is called one or more times;
    // then dispatch() is called.
    
    txt_memcached_get_request(txt_memcached_handler_t<config_t> *rh)
        : txt_memcached_request<config_t>(rh, false), num_fsms(0) {}
    
    bool add_get(btree_key *key) {
        if (this->request->can_add()) {
            btree_get_fsm_t *fsm = new btree_get_fsm_t(key);
            this->request->add(fsm, key_to_cpu(key, rh->event_queue->nqueues));
            fsms[num_fsms ++] = fsm;
            return true;
        } else {
            return false;
        }
    }
    
    void dispatch() {
        this->request->dispatch();
    }
    
    ~txt_memcached_get_request() {
        for (int i = 0; i < num_fsms; i ++) {
            delete fsms[i];
        }
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        for(int i = 0; i < num_fsms; i++) {
            btree_get_fsm_t *fsm = fsms[i];
            if(fsm->op_result == btree_get_fsm_t::btree_found) {
                //TODO: support flags
                sbuf->printf("VALUE %*.*s %u %u\r\n",
                    fsm->key.size, fsm->key.size, fsm->key.contents,
                    0,
                    fsm->value.size);
                sbuf->append(fsm->value.contents, fsm->value.size);
                sbuf->printf("\r\n");
            } else if(fsms[i]->op_result == btree_get_fsm_t::btree_not_found) {
                // do nothing
            }
        }
        sbuf->printf(RETRIEVE_TERMINATOR);
    }

private:
    int num_fsms;
    btree_get_fsm_t *fsms[MAX_OPS_IN_REQUEST];
};

template<class config_t>
class txt_memcached_set_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_set_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_set_fsm_t btree_set_fsm_t;

public:
    txt_memcached_set_request(txt_memcached_handler_t<config_t> *rh, btree_key *key, byte *data, int length, bool add_ok, bool replace_ok, bool noreply)
        : txt_memcached_request<config_t>(rh, noreply),
          fsm(new btree_set_fsm_t(key, data, length, add_ok, replace_ok))
        {
        this->request->add(fsm, key_to_cpu(key, rh->event_queue->nqueues));
        this->request->dispatch();
    }
    
    ~txt_memcached_set_request() {
        delete fsm;
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        if (fsm->set_was_successful) {
            sbuf->printf(STORAGE_SUCCESS);
        } else {
            sbuf->printf(STORAGE_FAILURE);
        }
    }

private:
    btree_set_fsm_t *fsm;
};

template<class config_t>
class txt_memcached_incr_decr_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_incr_decr_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_incr_decr_fsm_t btree_incr_decr_fsm_t;

public:
    txt_memcached_incr_decr_request(txt_memcached_handler_t<config_t> *rh, btree_key *key, bool increment, long long delta, bool noreply)
        : txt_memcached_request<config_t>(rh, noreply),
          fsm(new btree_incr_decr_fsm_t(key, increment, delta))
        {
        this->request->add(fsm, key_to_cpu(key, rh->event_queue->nqueues));
        this->request->dispatch();
    }
    
    ~txt_memcached_incr_decr_request() {
        delete fsm;
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        if (fsm->set_was_successful) {
            sbuf->printf("%llu\r\n", (unsigned long long)fsm->new_number);
        } else {
            sbuf->printf(NOT_FOUND);
        }
        delete fsm;
    }

private:
    btree_incr_decr_fsm_t *fsm;
};

template<class config_t>
class txt_memcached_delete_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_delete_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_delete_fsm_t btree_delete_fsm_t;

public:
    txt_memcached_delete_request(txt_memcached_handler_t<config_t> *rh, btree_key *key, bool noreply)
        : txt_memcached_request<config_t>(rh, noreply),
          fsm(new btree_delete_fsm_t(key)) {
        this->request->add(fsm, key_to_cpu(key, rh->event_queue->nqueues));
        this->request->dispatch();
    }
    
    ~txt_memcached_delete_request() {
        delete fsm;
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        if(fsm->op_result == btree_delete_fsm_t::btree_found) {
            sbuf->printf(DELETE_SUCCESS);
        } else if (fsm->op_result == btree_delete_fsm_t::btree_not_found) {
            sbuf->printf(NOT_FOUND);
        } else assert(0);
    }

private:
    btree_delete_fsm_t *fsm;
};

template<class config_t>
class txt_memcached_perfmon_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_delete_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;

public:
    txt_memcached_perfmon_request(txt_memcached_handler_t<config_t> *rh)
        : txt_memcached_request<config_t>(rh, false) {
        
        int nworkers = (int)get_cpu_context()->event_queue->parent_pool->nworkers;
    
        // Tell every single CPU core to pass their perfmon module *by copy*
        // to this CPU
        for (int i = 0; i < nworkers; i++)
        {
            assert(i < MAX_OPS_IN_REQUEST);
            msgs[i] = new perfmon_msg_t();
            this->request->add(msgs[i], i);
        }
        
        this->request->dispatch();
    }
    
    ~txt_memcached_perfmon_request() {
        // Do NOT delete the perfmon messages. They are sent back to the cores that provided
        // the stats so that the perfmon objects can be freed.
        int nworkers = (int)get_cpu_context()->event_queue->parent_pool->nworkers;
        for (int i = 0; i < nworkers; i ++) {
            msgs[i]->send_back_to_free_perfmon();
        }
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
    
        // Combine all responses into one
        int nworkers = (int)get_cpu_context()->event_queue->parent_pool->nworkers;
        perfmon_t combined_perfmon;
        for(int i = 0; i < nworkers; i++) {
            combined_perfmon.accumulate(msgs[i]->perfmon);
        }
        
        // Print the resultings perfmon
        perfmon_t::perfmon_map_t *registry = &combined_perfmon.registry;
        for(perfmon_t::perfmon_map_t::iterator iter = registry->begin(); iter != registry->end(); iter++)
        {
            sbuf->printf("STAT %s ", iter->first);
            char tmpbuf[10];
            int val_len = iter->second.print(tmpbuf, 10);
            sbuf->append(tmpbuf, val_len);
            sbuf->printf("\r\n");
        }
    }
    
public:
    perfmon_msg_t *msgs[MAX_OPS_IN_REQUEST];
};

// Process commands received from the user
template<class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::parse_request(event_t *event)
{
    assert(event->state == conn_fsm);

    // TODO: we might end up getting a command, and a piece of the
    // next command. It also means that we can't use one buffer
    //for both recv and send, we need to add a send buffer
    // (assuming we want to support out of band  commands).
    
    // check if we're supposed to be reading a binary blob
    if (loading_data) {
        return read_data();
    }

    // Find the first line in the buffer
    // This is only valid if we are not reading binary data
    char *line_end = (char *)memchr(conn_fsm->rbuf, '\n', conn_fsm->nrbuf);
    if (line_end == NULL) {   //make sure \n is in the buffer
        return req_handler_t::op_partial_packet;
    }
    unsigned int line_len = line_end - conn_fsm->rbuf + 1;

    //if \n is at the beginning of the buffer, or if it is not preceeded by \r, the request is malformed
    if (line_end == conn_fsm->rbuf || line_end[-1] != '\r') {
        conn_fsm->consume(line_len);
        return malformed_request();
    }

    // if we're not reading a binary blob, then the line will be a string - let's null terminate it
    *line_end = '\0';

    // get the first token to determine the command
    char *state;
    char *cmd_str = strtok_r(conn_fsm->rbuf, DELIMS, &state);

    if(cmd_str == NULL) {
        conn_fsm->consume(line_len);
        return malformed_request();
    }
    
    // Execute command
    if(!strcmp(cmd_str, "quit")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state)) {  //strtok will return NULL if there are no more tokens
            conn_fsm->consume(line_len);
            return malformed_request();
        }
        // Quit the connection
        conn_fsm->consume(conn_fsm->nrbuf);
        return req_handler_t::op_req_quit;

    } else if(!strcmp(cmd_str, "shutdown")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state)) {  //strtok will return NULL if there are no more tokens
            conn_fsm->consume(line_len);
            return malformed_request();
        }
        // Shutdown the server
        // clean out the rbuf
        conn_fsm->consume(conn_fsm->nrbuf);
        // TODO: We sould send SIGINT to the main thread directly from here instead of by this
        // circuitous and indirect way of signalling via the return value.
        return req_handler_t::op_req_shutdown;
        
    } else if(!strcmp(cmd_str, "stats")) {
        conn_fsm->consume(line_len);
        new txt_memcached_perfmon_request<config_t>(this);
        return req_handler_t::op_req_complex;
    
    } else if(!strcmp(cmd_str, "set")) {     // check for storage commands
        return parse_storage_command(SET, state, line_len);
    } else if(!strcmp(cmd_str, "add")) {
        return parse_storage_command(ADD, state, line_len);
    } else if(!strcmp(cmd_str, "replace")) {
        return parse_storage_command(REPLACE, state, line_len);
    } else if(!strcmp(cmd_str, "append")) {
        return parse_storage_command(APPEND, state, line_len);
    } else if(!strcmp(cmd_str, "prepend")) {
        return parse_storage_command(PREPEND, state, line_len);
    } else if(!strcmp(cmd_str, "cas")) {
        return parse_storage_command(CAS, state, line_len);

    } else if(!strcmp(cmd_str, "get")) {    // check for retrieval commands
        return get(state, false, line_len);
    } else if(!strcmp(cmd_str, "gets")) {
        return get(state, true, line_len);

    } else if(!strcmp(cmd_str, "delete")) {
        return remove(state, line_len);

    } else if(!strcmp(cmd_str, "incr")) {
        return parse_adjustment(true, state, line_len);
    } else if(!strcmp(cmd_str, "decr")) {
        return parse_adjustment(false, state, line_len);
    } else {
        // Invalid command
        conn_fsm->consume(line_len);
        return malformed_request();
    }
}

template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::parse_adjustment(bool increment, char *state, unsigned int line_len) {
    char *key_tmp = strtok_r(NULL, DELIMS, &state);
    char *value_str = strtok_r(NULL, DELIMS, &state);
    char *noreply_str = strtok_r(NULL, DELIMS, &state); //optional
    
    if (key_tmp == NULL || value_str == NULL) {
        return malformed_request();
    }
    bool noreply = false;
    if (noreply_str != NULL) {
        if (!strcmp(noreply_str, "noreply")) {
            noreply = true;
        } else {
            conn_fsm->consume(line_len);
            return malformed_request();
        }
    }
    
    node_handler::str_to_key(key_tmp, &key);
    long long delta = atoll(value_str);
    new txt_memcached_incr_decr_request<config_t>(this, &key, increment, delta, noreply);
    
    conn_fsm->consume(line_len);
    
    if (noreply)
        return req_handler_t::op_req_parallelizable;
    else
        return req_handler_t::op_req_complex;
}

template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::parse_storage_command(storage_command command, char *state, unsigned int line_len) {
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
        conn_fsm->consume(line_len);
        return malformed_request();
    }

    cmd = command;
    node_handler::str_to_key(key_tmp, &key);

    char *invalid_char;
    flags = strtoul(flags_str, &invalid_char, 10);  //a 32 bit integer.  int alone does not guarantee 32 bit length
    if (*invalid_char != '\0') {  // ensure there were no improper characters in the token - i.e. parse was successful
        conn_fsm->consume(line_len);
        return malformed_request();
    }

    exptime = strtoul(exptime_str, &invalid_char, 10);
    if (*invalid_char != '\0') {
        conn_fsm->consume(line_len);
        return malformed_request();
    }

    bytes = strtoul(bytes_str, &invalid_char, 10);
    if (*invalid_char != '\0') {
        conn_fsm->consume(line_len);
        return malformed_request();
    }

    if (cmd == CAS) {
        cas_unique = strtoull(cas_unique_str, &invalid_char, 10);
        if (*invalid_char != '\0') {
            conn_fsm->consume(line_len);
            return malformed_request();
        }
    }

    this->noreply = false;
    if (noreply_str != NULL) {
        if (!strcmp(noreply_str, "noreply")) {
            this->noreply = true;
        } else {
            conn_fsm->consume(line_len);
            return malformed_request();
        }
    }

    conn_fsm->consume(line_len); //consume the line
    loading_data = true;
    return read_data();
}
	
template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::read_data() {
    check("memcached handler should be in loading data state", !loading_data);
    if (conn_fsm->nrbuf < bytes + 2){//check that the buffer contains enough data.  must also include \r\n
        return req_handler_t::op_partial_packet;
    }
    loading_data = false;
    if (conn_fsm->rbuf[bytes] != '\r' || conn_fsm->rbuf[bytes+1] != '\n') {
        conn_fsm->sbuf->printf(BAD_BLOB);
        conn_fsm->consume(bytes+2);
        return req_handler_t::op_malformed;
    }

    switch(cmd) {
        case SET:
            new txt_memcached_set_request<config_t>(this, &key, conn_fsm->rbuf, bytes, true, true, noreply);
            break;
        case ADD:
            new txt_memcached_set_request<config_t>(this, &key, conn_fsm->rbuf, bytes, true, false, noreply);
            break;
        case REPLACE:
            new txt_memcached_set_request<config_t>(this, &key, conn_fsm->rbuf, bytes, false, true, noreply);
            break;
        case APPEND:
        case PREPEND:
        case CAS:
            conn_fsm->consume(bytes+2);
            return unimplemented_request();
        default:
            conn_fsm->consume(bytes+2);
            return malformed_request();
    }
    
    conn_fsm->consume(bytes+2);
    
    if (noreply)
        return req_handler_t::op_req_parallelizable;
    else
        return req_handler_t::op_req_complex;
}

template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::malformed_request() {
    conn_fsm->sbuf->printf(MALFORMED_RESPONSE);
    return req_handler_t::op_malformed;
}

template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::unimplemented_request() {
    conn_fsm->sbuf->printf(UNIMPLEMENTED_RESPONSE);
    return req_handler_t::op_malformed;
}

template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::get(char *state, bool include_unique, unsigned int line_len) {
    
    if (include_unique)
        return unimplemented_request();
    
    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL) return malformed_request();
    
    txt_memcached_get_request<config_t> *rq = new txt_memcached_get_request<config_t>(this);
    
    do {
        node_handler::str_to_key(key_str, &key);
        
        if (!rq->add_get(&key)) {
            // We can't fit any more operations, let's just break
            // and complete the ones we already sent out to other
            // cores.
            break;

            // TODO: to a user, it will look like some of his
            // requests aren't satisfied. We need to notify them
            // somehow.
        }
        
        key_str = strtok_r(NULL, DELIMS, &state);
        
    } while(key_str);
    
    rq->dispatch();
    
    //clean out the rbuf
    conn_fsm->consume(line_len);
    return req_handler_t::op_req_complex;
}


template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::remove(char *state, unsigned int line_len) {
    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL) {
        conn_fsm->consume(line_len);
        return malformed_request();
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
                return unimplemented_request();

            // see if there's a noreply arg too
            char *noreply_str = strtok_r(NULL, DELIMS, &state);
            if (noreply_str != NULL) {
                if (!strcmp(noreply_str, "noreply")) {
                    this->noreply = true;
                } else {
                    conn_fsm->consume(line_len);
                    return malformed_request();
                }
            }
        }
    }

    node_handler::str_to_key(key_str, &key);

    // Create request
    new txt_memcached_delete_request<config_t>(this, &key, this->noreply);

    //clean out the rbuf
    conn_fsm->consume(line_len);

    if (this->noreply)
        return req_handler_t::op_req_parallelizable;
    else
        return req_handler_t::op_req_complex;
}

#endif // __MEMCACHED_HANDLER_TCC__

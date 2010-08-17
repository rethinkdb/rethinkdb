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
#define INCR_DECR_ON_NON_NUMERIC_VALUE "CLIENT_ERROR cannot increment or decrement non-numeric value\r\n"
#define KEY_EXISTS "EXISTS\r\n"
#define DELETE_SUCCESS "DELETED\r\n"
#define RETRIEVE_TERMINATOR "END\r\n"
#define BAD_BLOB "CLIENT_ERROR bad data chunk\r\n"

#define MAX_STATS_REQ_LEN 100

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
            this->request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
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
            get_cpu_context()->worker->cmd_get++;
            delete fsms[i];
        }
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        for(int i = 0; i < num_fsms; i++) {
            btree_get_fsm_t *fsm = fsms[i];
            if(fsm->op_result == btree_get_fsm_t::btree_found) {
                get_cpu_context()->worker->get_hits++;
                sbuf->printf("VALUE %*.*s %u %u\r\n",
                    fsm->key.size, fsm->key.size, fsm->key.contents,
                    fsm->value.mcflags(),
                    fsm->value.value_size());
                sbuf->append(fsm->value.value(), fsm->value.value_size());
                sbuf->printf("\r\n");
            } else if(fsms[i]->op_result == btree_get_fsm_t::btree_not_found) {
                get_cpu_context()->worker->get_misses++;
                // do nothing
            }
        }
        sbuf->printf(RETRIEVE_TERMINATOR);
    }

private:
    int num_fsms;
    btree_get_fsm_t *fsms[MAX_OPS_IN_REQUEST];
};

// FIXME horrible redundancy
template<class config_t>
class txt_memcached_get_cas_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_get_cas_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_get_cas_fsm_t btree_get_cas_fsm_t;
    using txt_memcached_request<config_t>::rh;

public:
    // A txt_memcached_get_request supports more than one get at one time, so it takes several steps
    // to build it. First the constructor is called; then add_get() is called one or more times;
    // then dispatch() is called.
    
    txt_memcached_get_cas_request(txt_memcached_handler_t<config_t> *rh)
        : txt_memcached_request<config_t>(rh, false), num_fsms(0) {}
    
    bool add_get(btree_key *key) {
        if (this->request->can_add()) {
            btree_get_cas_fsm_t *fsm = new btree_get_cas_fsm_t(key);
            this->request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
            fsms[num_fsms ++] = fsm;
            return true;
        } else {
            return false;
        }
    }
    
    void dispatch() {
        this->request->dispatch();
    }
    
    ~txt_memcached_get_cas_request() {
        for (int i = 0; i < num_fsms; i ++) {
            delete fsms[i];
        }
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        for(int i = 0; i < num_fsms; i++) {
            btree_get_cas_fsm_t *fsm = fsms[i];
            if(fsm->found) {
                get_cpu_context()->worker->get_hits++;
                sbuf->printf("VALUE %*.*s %u %u %llu\r\n",
                    fsm->key.size, fsm->key.size, fsm->key.contents,
                    fsm->value.mcflags(),
                    fsm->value.value_size(),
                    fsm->value.cas());
                sbuf->append(fsm->value.value(), fsm->value.value_size());
                sbuf->printf("\r\n");
            }
        }
        sbuf->printf(RETRIEVE_TERMINATOR);
    }

private:
    int num_fsms;
    btree_get_cas_fsm_t *fsms[MAX_OPS_IN_REQUEST];
};

template<class config_t>
class txt_memcached_set_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_set_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_set_fsm_t btree_set_fsm_t;

public:
    txt_memcached_set_request(txt_memcached_handler_t<config_t> *rh, btree_key *key, byte *data, int length, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, bool add_ok, bool replace_ok, btree_value::cas_t req_cas, bool check_cas, bool noreply)
        : txt_memcached_request<config_t>(rh, noreply),
          fsm(new btree_set_fsm_t(key, data, length, mcflags, convert_exptime(exptime), add_ok, replace_ok, req_cas, check_cas))
        {
        this->request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        this->request->dispatch();
    }
    
    ~txt_memcached_set_request() {
        get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        switch (fsm->status_code) {
            case btree_set_fsm_t::S_SUCCESS:
                sbuf->printf(STORAGE_SUCCESS);
                break;
            case btree_set_fsm_t::S_NOT_STORED:
                sbuf->printf(STORAGE_FAILURE);
                break;
            case btree_set_fsm_t::S_EXISTS:
                sbuf->printf(KEY_EXISTS);
                break;
            default:
                assert(0);
                break;
        }
    }

    // This is protocol.txt, verbatim:
    // Some commands involve a client sending some kind of expiration time
    // (relative to an item or to an operation requested by the client) to
    // the server. In all such cases, the actual value sent may either be
    // Unix time (number of seconds since January 1, 1970, as a 32-bit
    // value), or a number of seconds starting from current time. In the
    // latter case, this number of seconds may not exceed 60*60*24*30 (number
    // of seconds in 30 days); if the number sent by a client is larger than
    // that, the server will consider it to be real Unix time value rather
    // than an offset from current time.

    // TODO: Move this elsewhere (binary protocol and some other request handlers need it).
    btree_value::exptime_t convert_exptime(btree_value::exptime_t exptime) {
        if (exptime <= 60*60*24*30 && exptime > 0) {
            exptime += time(NULL);
        }
        // TODO: Do we need to handle exptimes in the past?
        return exptime;
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
        this->request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        this->request->dispatch();
    }
    
    ~txt_memcached_incr_decr_request() {
        get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        switch (fsm->status_code) {
            case btree_incr_decr_fsm_t::S_SUCCESS:
                sbuf->printf("%llu\r\n", (unsigned long long)fsm->new_number);
                break;
            case btree_incr_decr_fsm_t::S_NOT_FOUND:
                sbuf->printf(NOT_FOUND);
                break;
            case btree_incr_decr_fsm_t::S_NOT_NUMERIC:
                sbuf->printf(INCR_DECR_ON_NON_NUMERIC_VALUE);
                break;
            default:
                assert(0);
                break;
        }
    }

private:
    btree_incr_decr_fsm_t *fsm;
};

template<class config_t>
class txt_memcached_append_prepend_request : public txt_memcached_request<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_append_prepend_request<config_t> > {
public:
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::btree_append_prepend_fsm_t btree_append_prepend_fsm_t;

    txt_memcached_append_prepend_request(txt_memcached_handler_t<config_t> *rh, btree_key *key, byte *data, int length, bool append, bool noreply)
        : txt_memcached_request<config_t>(rh, noreply),
          fsm(new btree_append_prepend_fsm_t(key, data, length, append)) {
        this->request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
        this->request->dispatch();
    }

    ~txt_memcached_append_prepend_request() {
        get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }

    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        switch (fsm->status_code) {
            case btree_append_prepend_fsm_t::S_SUCCESS:
                sbuf->printf(STORAGE_SUCCESS);
                break;
            case btree_append_prepend_fsm_t::S_NOT_STORED:
                sbuf->printf(STORAGE_FAILURE);
                break;
            default:
                assert(0);
                break;
        }
    }

private:
    btree_append_prepend_fsm_t *fsm;
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
        this->request->add(fsm, key_to_cpu(key, rh->event_queue->parent->nworkers));
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
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_perfmon_request<config_t> > {

public:
    typedef typename config_t::conn_fsm_t       conn_fsm_t;

public:
    txt_memcached_perfmon_request(txt_memcached_handler_t<config_t> *rh)
        : txt_memcached_request<config_t>(rh, false) {
        
        int nworkers = (int)get_cpu_context()->worker->event_queue->parent_pool->nworkers;
    
        // Tell every single CPU core to pass their perfmon module *by copy*
        // to this CPU
        for (int i = 0; i < nworkers; i++)
        {
            assert(i < MAX_OPS_IN_REQUEST);
            msgs[i] = new perfmon_msg_t();
            this->request->add(msgs[i], i);
        }
    }

    void dispatch() {
        this->request->dispatch();
    }
    
    ~txt_memcached_perfmon_request() {
        // Do NOT delete the perfmon messages. They are sent back to the cores that provided
        // the stats so that the perfmon objects can be freed.
        int nworkers = (int)get_cpu_context()->worker->event_queue->parent_pool->nworkers;
        for (int i = 0; i < nworkers; i ++) {
            msgs[i]->send_back_to_free_perfmon();
        }
    }
    
    void build_response(typename conn_fsm_t::linked_buf_t *sbuf) {
        // Combine all responses into one
        int nworkers = (int)get_cpu_context()->worker->event_queue->parent_pool->nworkers;
        perfmon_t combined_perfmon;
        for(int i = 0; i < nworkers; i++) {
            combined_perfmon.accumulate(msgs[i]->perfmon);
        }
        
        // Print the resultings perfmon
        char tmpbuf[255];
#if defined(VALGRIND) || !defined(NDEBUG)
        // Fill the buffer with garbage in debug mode so valgrind doesn't complain, and to help
        // catch uninitialized memory errors.
        memset(tmpbuf, 0xBD, sizeof(tmpbuf));
#endif
        perfmon_t::perfmon_map_t *registry = &combined_perfmon.registry;
        if (strlen(fields) == 0){
            for(perfmon_t::perfmon_map_t::iterator iter = registry->begin(); iter != registry->end(); iter++)
            {
                sbuf->printf("STAT %s ", iter->first);
                int val_len = iter->second.print(tmpbuf, 10);
                sbuf->append(tmpbuf, val_len);
                sbuf->printf("\r\n");
            }
        }
        else {
            char *fields_ptr = fields;
            char *stat;
            while ((stat = strtok_r(NULL, DELIMS, &fields_ptr))) {
                sbuf->printf("STAT %s ", stat);
                perfmon_t::perfmon_map_t::iterator stat_entry = registry->find(stat);
                if (stat_entry == registry->end()) {
                    sbuf->printf("NOT FOUND\r\n");
                } else {
                    int val_len = stat_entry->second.print(tmpbuf, 10);
                    sbuf->append(tmpbuf, val_len);
                    sbuf->printf("\r\n");
                }
            }
        }
    }
    
public:
    perfmon_msg_t *msgs[MAX_OPS_IN_REQUEST];

public:
    /*! \brief which fields user is requesting, empty indicates all fields
     */
    char fields[MAX_STATS_REQ_LEN];
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
        
    } else if(!strcmp(cmd_str, "stats") || !strcmp(cmd_str, "stat")) {
        conn_fsm->consume(line_len);
        parse_stat_command(state, line_len);
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
        return get(state, line_len);
    } else if(!strcmp(cmd_str, "gets")) {
        return get_cas(state, line_len);

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
    char *mcflags_str = strtok_r(NULL, DELIMS, &state);
    char *exptime_str = strtok_r(NULL, DELIMS, &state);
    char *bytes_str = strtok_r(NULL, DELIMS, &state);
    char *cas_str = NULL;
    if (command == CAS)
        cas_str = strtok_r(NULL, DELIMS, &state);
    char *noreply_str = strtok_r(NULL, DELIMS, &state); //optional

    //check for proper number of arguments
    if ((key_tmp == NULL || mcflags_str == NULL || exptime_str == NULL || bytes_str == NULL || (command == CAS && cas_str == NULL))) {
        conn_fsm->consume(line_len);
        return malformed_request();
    }

    cmd = command;
    node_handler::str_to_key(key_tmp, &key);

    char *invalid_char;
    mcflags = strtoul(mcflags_str, &invalid_char, 10);  //a 32 bit integer.  int alone does not guarantee 32 bit length
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
        cas = strtoull(cas_str, &invalid_char, 10);
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
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::parse_stat_command(char *state, unsigned int line_len) {
    txt_memcached_perfmon_request<config_t> *rq = new txt_memcached_perfmon_request<config_t>(this);
    check("Too big of a stat request", line_len > MAX_STATS_REQ_LEN);
    memcpy(rq->fields, state, line_len);
    rq->dispatch();
    return req_handler_t::op_req_complex;
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
            new txt_memcached_set_request<config_t>(this, &key, conn_fsm->rbuf, bytes, mcflags, exptime, true, true, 0, false, noreply);
            break;
        case ADD:
            new txt_memcached_set_request<config_t>(this, &key, conn_fsm->rbuf, bytes, mcflags, exptime, true, false, 0, false, noreply);
            break;
        case REPLACE:
            new txt_memcached_set_request<config_t>(this, &key, conn_fsm->rbuf, bytes, mcflags, exptime, false, true, 0, false, noreply);
            break;
        case CAS:
            // TODO: CAS returns a different error code from REPLACE when a value isn't found.
            new txt_memcached_set_request<config_t>(this, &key, conn_fsm->rbuf, bytes, mcflags, exptime, false, true, cas, true, noreply);
            break;
        // APPEND and PREPEND always ignore flags and exptime.
        case APPEND:
            new txt_memcached_append_prepend_request<config_t>(this, &key, conn_fsm->rbuf, bytes, true, noreply);
            break;
        case PREPEND:
            new txt_memcached_append_prepend_request<config_t>(this, &key, conn_fsm->rbuf, bytes, false, noreply);
            break;
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
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::get(char *state, unsigned int line_len) {
    
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

// FIXME horrible redundancy
template <class config_t>
typename txt_memcached_handler_t<config_t>::parse_result_t txt_memcached_handler_t<config_t>::get_cas(char *state, unsigned int line_len) {
    
    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL) return malformed_request();
    
    txt_memcached_get_cas_request<config_t> *rq = new txt_memcached_get_cas_request<config_t>(this);
    
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

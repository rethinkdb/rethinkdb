#include "request_handler/txt_memcached_handler.hpp"

#include <string.h>
#include "arch/arch.hpp"
#include "conn_fsm.hpp"
#include "btree/append_prepend_fsm.hpp"
#include "btree/delete_fsm.hpp"
#include "btree/get_fsm.hpp"
#include "btree/get_cas_fsm.hpp"
#include "btree/incr_decr_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "server.hpp"
#include "perfmon.hpp"

#define DELIMS " \t\n\r"
#define MALFORMED_RESPONSE "ERROR\r\n"
#define UNIMPLEMENTED_RESPONSE "SERVER_ERROR functionality not supported\r\n"
#define STORAGE_SUCCESS "STORED\r\n"
#define STORAGE_FAILURE "NOT_STORED\r\n"
#define NOT_FOUND "NOT_FOUND\r\n"
#define INCR_DECR_ON_NON_NUMERIC_VALUE "CLIENT_ERROR cannot increment or decrement non-numeric value\r\n"
#define INCR_DECR_ON_NON_UINT_ARGUMENT "CLIENT_ERROR invalid numeric delta argument\r\n"
#define KEY_EXISTS "EXISTS\r\n"
#define DELETE_SUCCESS "DELETED\r\n"
#define RETRIEVE_TERMINATOR "END\r\n"
#define BAD_BLOB "CLIENT_ERROR bad data chunk\r\n"
#define TOO_LARGE "SERVER_ERROR object too large for cache\r\n"
#define MAX_COMMAND_SIZE 100
#define BAD_STAT "MALFORMED STAT REQUEST\r\n"

#define MAX_STATS_REQ_LEN 100

// Please read and understand the memcached protocol before modifying this
// file. If you only do a cursory readthrough, please check with someone who
// has read it in depth before committing.

class txt_memcached_get_request_t
{
public:
    // A txt_memcached_get_request_t supports more than one get at one time, so it takes several steps
    // to build it. First the constructor is called; then add_get() is called one or more times.
    
    txt_memcached_handler_t *rh;
    
    txt_memcached_get_request_t(txt_memcached_handler_t *rh, bool with_cas)
        : rh(rh), num_gets(0), curr_get(-1), with_cas(with_cas)
    {
    }

    bool add_get(btree_key *key) {
        if (num_gets < MAX_OPS_IN_REQUEST) {
            get_t *g = &gets[num_gets++];
            keycpy(&g->key, key);
            g->parent = this;
            g->ready = false;
            if (with_cas) rh->server->store->get_cas(key, g);
            else rh->server->store->get(key, g);
            return true;
        } else {
            return false;
        }
    }
    
    void dispatch() {
        curr_get = 0;   // allow pump() to go
        pump();
    }
    
    ~txt_memcached_get_request_t() {
    }

private:
    struct get_t :
        public store_t::get_callback_t,
        public data_transferred_callback
    {
        txt_memcached_get_request_t *parent;
        union {
            char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
            btree_key key;
        };
        
        const_buffer_group_t *buffer;   // Can be NULL
        store_t::get_callback_t::done_callback_t *callback;
        mcflags_t flags;
        cas_t cas;
        
        bool ready, sending;
        int buffer_being_sent;
        
        void value(const_buffer_group_t *b, store_t::get_callback_t::done_callback_t *cb, mcflags_t f, cas_t c) {
            assert(b);
            buffer = b;
            callback = cb;
            flags = f;
            cas = c;
            ready = true;
            sending = false;
            parent->pump();
        }
        
        void not_found() {
            buffer = NULL;
            ready = true;
            sending = false;
            parent->pump();
        }
        
        void write() {
            sending = true;
            if (buffer) {
                if (parent->with_cas) {
                    parent->rh->conn_fsm->sbuf->printf(
                        "VALUE %*.*s %u %u %llu\r\n",
                        key.size, key.size, key.contents, flags, buffer->get_size(), cas);
                } else {
                    assert(cas == 0);
                    parent->rh->conn_fsm->sbuf->printf(
                        "VALUE %*.*s %u %u\r\n",
                        key.size, key.size, key.contents, flags, buffer->get_size());
                }
                if (buffer->get_size() < MAX_BUFFERED_GET_SIZE) {
                    for (int i = 0; i < (signed)buffer->buffers.size(); i++) {
                        const_buffer_group_t::buffer_t b = buffer->buffers[i];
                        parent->rh->conn_fsm->sbuf->append((const char *)b.data, b.size);
                    }
                    callback->have_copied_value();
                    parent->rh->conn_fsm->sbuf->printf("\r\n");
                    done();
                } else {
                    buffer_being_sent = -1;
                    on_data_transferred();   // Start the data transfer loop
                }
            } else {
                done();
            }
        }
        
        void on_data_transferred() {
            buffer_being_sent++;
            if (buffer_being_sent == (int)buffer->buffers.size()) {
                callback->have_copied_value();
                parent->rh->conn_fsm->sbuf->printf("\r\n");
                done();
            } else {
                parent->rh->write_value((const char *)buffer->buffers[buffer_being_sent].data, buffer->buffers[buffer_being_sent].size, this);
            }
        }
        
        void done() {
            assert(&parent->gets[parent->curr_get] == this);
            parent->curr_get++;
            parent->pump();
        }
    };
    get_t gets[MAX_OPS_IN_REQUEST];
    int num_gets, curr_get;
    
    bool with_cas;
    
    void pump() {
        if (curr_get == -1) return;   // So we don't finish before we're done starting
        if (curr_get < num_gets) {
            get_t *g = &gets[curr_get];
            if (g->ready && !g->sending) {
                g->write();   // When this completes, it will increment curr_fsm and call pump()
            }
        } else {
            rh->conn_fsm->sbuf->printf(RETRIEVE_TERMINATOR);
            rh->request_complete();
            delete this;
        }
    }
};

class txt_memcached_set_request_t :
    public store_t::set_callback_t
{
    request_handler_t *rh;
    data_provider_t *data;
    bool noreply;
    
public:
    txt_memcached_set_request_t(
        txt_memcached_handler_t *rh,
        btree_key *key,
        data_provider_t *data,
        btree_set_fsm_t::set_type_t type,
        btree_value::mcflags_t flags,
        btree_value::exptime_t exptime,
        btree_value::cas_t req_cas,
        bool noreply)
        : rh(rh), data(data), noreply(noreply)
    {
        switch (type) {
            case btree_set_fsm_t::set_type_set:
                rh->server->store->set(key, data, flags, convert_exptime(exptime), this);
                break;
            case btree_set_fsm_t::set_type_add:
                rh->server->store->add(key, data, flags, convert_exptime(exptime), this);
                break;
            case btree_set_fsm_t::set_type_replace:
                rh->server->store->replace(key, data, flags, convert_exptime(exptime), this);
                break;
            case btree_set_fsm_t::set_type_cas:
                rh->server->store->cas(key, data, flags, convert_exptime(exptime), req_cas, this);
                break;
        }
    }

    ~txt_memcached_set_request_t() {
        delete data;
    }

    void stored() {
        if (!noreply) rh->conn_fsm->sbuf->printf(STORAGE_SUCCESS);
        done();
    }
    
    void not_stored() {
        if (!noreply) rh->conn_fsm->sbuf->printf(STORAGE_FAILURE);
        done();
    }
    
    void exists() {
        if (!noreply) rh->conn_fsm->sbuf->printf(KEY_EXISTS);
        done();
    }
    
    void not_found() {
        if (!noreply) rh->conn_fsm->sbuf->printf(NOT_FOUND);
        done();
    }
    
    void too_large() {
        if (!noreply) rh->conn_fsm->sbuf->printf(TOO_LARGE);
        done();
    }
    
    void data_provider_failed() {
        // read_data() handles printing the error
        done();
    }
    
    void done() {
        if (!noreply) rh->request_complete();
        delete this;
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
        // TODO: Do we need to handle exptimes in the past here?
        return exptime;
    }
};

class txt_memcached_incr_decr_request_t :
    public store_t::incr_decr_callback_t
{
    txt_memcached_handler_t *rh;
    bool noreply;
    
public:
    txt_memcached_incr_decr_request_t(txt_memcached_handler_t *rh, btree_key *key, bool increment, long long delta, bool noreply)
        : rh(rh), noreply(noreply)
    {
        if (increment) rh->server->store->incr(key, delta, this);
        else rh->server->store->decr(key, delta, this);
    }

    ~txt_memcached_incr_decr_request_t() {
    }
    
    void success(unsigned long long new_value) {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf("%llu\r\n", new_value);
            rh->request_complete();
        }
        delete this;
    }
    
    void not_found() {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf(NOT_FOUND);
            rh->request_complete();
        }
        delete this;
    }
    
    void not_numeric() {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf(INCR_DECR_ON_NON_NUMERIC_VALUE);
            rh->request_complete();
        }
        delete this;
    }
};

class txt_memcached_append_prepend_request_t :
    public store_t::append_prepend_callback_t
{
    txt_memcached_handler_t *rh;
    bool noreply;
    data_provider_t *data;
    
public:
    txt_memcached_append_prepend_request_t(txt_memcached_handler_t *rh, btree_key *key, data_provider_t *data, bool append, bool noreply)
        : rh(rh), noreply(noreply), data(data)
    {
        if (append) rh->server->store->append(key, data, this);
        else rh->server->store->prepend(key, data, this);
    }

    ~txt_memcached_append_prepend_request_t() {
        delete data;
    }

    void success() {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf(STORAGE_SUCCESS);
            rh->request_complete();
        }
        delete this;
    }
    
    void too_large() {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf(STORAGE_FAILURE);
            rh->request_complete();
        }
        delete this;
    }
    
    void not_found() {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf(STORAGE_FAILURE);
            rh->request_complete();
        }
        delete this;
    }
    
    void data_provider_failed() {
        // read_data() handles printing the error.
        if (!noreply) rh->request_complete();
        delete this;
    }
};

class txt_memcached_delete_request_t :
    public store_t::delete_callback_t
{
    request_handler_t *rh;
    bool noreply;

public:
    txt_memcached_delete_request_t(txt_memcached_handler_t *rh, btree_key *key, bool noreply)
        : rh(rh), noreply(noreply)
    {
        rh->server->store->delete_key(key, this);
    }

    ~txt_memcached_delete_request_t() {
    }

    void deleted() {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf(DELETE_SUCCESS);
            rh->request_complete();
        }
        delete this;
    }
    
    void not_found() {
        if (!noreply) {
            rh->conn_fsm->sbuf->printf(NOT_FOUND);
            rh->request_complete();
        }
        delete this;
    }
};

class txt_memcached_perfmon_request_t :
    public cpu_message_t,   // For call_later_on_this_cpu()
    public perfmon_callback_t
{
    txt_memcached_handler_t *rh;
    
public:
    txt_memcached_perfmon_request_t(txt_memcached_handler_t *rh, const char *fields_beg, size_t fields_len)
        : rh(rh) {

        assert(fields_len <= MAX_STATS_REQ_LEN);

        memcpy(fields, fields_beg, fields_len);

        if (perfmon_get_stats(&stats, this)) {
        
            /* The world is not ready for the power of completing a request immediately.
            So we delay so that the request handler doesn't get confused.  We don't know
            if this is necessary. */

            call_later_on_this_cpu(this);
        }
    }
    
    void on_cpu_switch() {
        on_perfmon_stats();
    }
    
    void on_perfmon_stats() {
        build_response(rh->conn_fsm->sbuf);
        rh->request_complete();
        delete this;
    }

    void build_response(linked_buf_t *sbuf) {

        if (strlen(fields) == 0) {
            for (perfmon_stats_t::iterator iter = stats.begin(); iter != stats.end(); iter++) {
                sbuf->printf("STAT %s %s\r\n", iter->first.c_str(), iter->second.c_str());
            }
            
        } else {
            char *fields_ptr = fields;
            char *stat;
            while ((stat = strtok_r(NULL, DELIMS, &fields_ptr))) {
                sbuf->printf("STAT %s ", stat);
                std::string s(stat);
                perfmon_stats_t::iterator stat_entry = stats.find(s);
                if (stat_entry == stats.end()) {
                    sbuf->printf("NOT FOUND\r\n");
                } else {
                    sbuf->printf("%s\r\n", stat_entry->second.c_str());
                }
            }
        }
        sbuf->printf("END\r\n");
    }

public:
    perfmon_stats_t stats;
    
    //! \brief which fields user is requesting, empty indicates all fields
    char fields[MAX_STATS_REQ_LEN];

private:
    DISABLE_COPYING(txt_memcached_perfmon_request_t);
};




class disable_gc_request_t :
    public server_t::all_gc_disabled_callback_t,  // gives us multiple_users_seen
    public cpu_message_t,   // As with txt_memcached_perfmon_request_t, for call_later_on_this_cpu()
    public home_cpu_mixin_t
{
    txt_memcached_handler_t *rh;
    
public:
    explicit disable_gc_request_t(txt_memcached_handler_t *rh)
        : rh(rh), done(false) {

        rh->server->disable_gc(this);
        done = true;
    }

    void on_cpu_switch() {
        on_gc_disabled();
    }

    void on_gc_disabled() {
        if (!continue_on_cpu(home_cpu, this)) {
            return;
        }

        if (!done) {
            call_later_on_this_cpu(this);
            return;
        }

        build_response(rh->conn_fsm->sbuf);
        rh->request_complete();
        delete this;
    }

    void build_response(linked_buf_t *sbuf) {
        // TODO: figure out what to print here.
        sbuf->printf("DISABLED%s\r\n", multiple_users_seen ? " (Warning: multiple users think they're the one who disabled the gc.)" : "");
    }
private:
    bool done;
    DISABLE_COPYING(disable_gc_request_t);
};

class enable_gc_request_t :
    public server_t::all_gc_enabled_callback_t,  // gives us multiple_users_seen
    public cpu_message_t,
    public home_cpu_mixin_t
{
    txt_memcached_handler_t *rh;
    
public:
    explicit enable_gc_request_t(txt_memcached_handler_t *rh)
        : rh(rh), done(false) {

        rh->server->enable_gc(this);
        done = true;
    }

    void on_cpu_switch() {
        on_gc_enabled();
    }

    void on_gc_enabled() {
        if (!continue_on_cpu(home_cpu, this)) {
            return;
        }

        if (!done) {
            call_later_on_this_cpu(this);
            return;
        }

        build_response(rh->conn_fsm->sbuf);
        rh->request_complete();
        delete this;
    }

    void build_response(linked_buf_t *sbuf) {
        // TODO: figure out what to print here.
        sbuf->printf("ENABLED%s\r\n", multiple_users_seen ? " (Warning: gc was already enabled.)" : "");
    }

private:
    bool done;
    DISABLE_COPYING(enable_gc_request_t);
};



// Process commands received from the user
txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::parse_request(event_t *event) {
    assert(event->state == conn_fsm);

    // check if we're supposed to be reading a binary blob
    if (loading_data) {
        return read_data();
    }

    // Find the first line in the buffer
    // This is only valid if we are not reading binary data (we're reading for a command)
    char *line_end = (char *)memchr(conn_fsm->rbuf, '\n', conn_fsm->nrbuf);
    if (line_end == NULL) {   //make sure \n is in the buffer
        if (conn_fsm->nrbuf > MAX_COMMAND_SIZE) {
            conn_fsm->consume(MAX_COMMAND_SIZE);
            return malformed_request();
        } else {
            return request_handler_t::op_partial_packet;
        }
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
    if(!strcmp(cmd_str, "get")) {    // check for retrieval commands
        return get(state, line_len, false);
    } else if(!strcmp(cmd_str, "gets")) {
        return get(state, line_len, true);

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

    } else if(!strcmp(cmd_str, "delete")) {
        return remove(state, line_len);

    } else if(!strcmp(cmd_str, "incr")) {
        return parse_adjustment(true, state, line_len);
    } else if(!strcmp(cmd_str, "decr")) {
        return parse_adjustment(false, state, line_len);
    } else if(!strcmp(cmd_str, "quit")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state)) {  //strtok will return NULL if there are no more tokens
            conn_fsm->consume(line_len);
            return malformed_request();
        }
        // Quit the connection
        conn_fsm->consume(conn_fsm->nrbuf);
        return request_handler_t::op_req_quit;

    } else if(!strcmp(cmd_str, "shutdown")) {
        // Make sure there's no more tokens
        if (strtok_r(NULL, DELIMS, &state)) {  //strtok will return NULL if there are no more tokens
            conn_fsm->consume(line_len);
            return malformed_request();
        }
        // Shutdown the server
        // clean out the rbuf
        conn_fsm->consume(conn_fsm->nrbuf);
        
        server->shutdown();
        
        return request_handler_t::op_req_quit;

    } else if(!strcmp(cmd_str, "stats") || !strcmp(cmd_str, "stat")) {
        return parse_stat_command(line_len, cmd_str);
    } else if(!strcmp(cmd_str, "rethinkdbctl")) {
        return parse_gc_command(line_len, state);

    } else {
        // Invalid command
        conn_fsm->consume(line_len);
        return malformed_request();
    }
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::parse_adjustment(bool increment, char *state, unsigned int line_len) {
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
    // First convert to signed long to catch negative arguments (strtoull handles them as valid unsigned numbers)
    signed long long signed_delta = strtoll(value_str, NULL, 10);
    char *endptr = NULL;
    unsigned long long delta = strtoull(value_str, &endptr, 10);
    if (*endptr != '\0' || signed_delta < 0) {
        if (!noreply) {
            conn_fsm->sbuf->printf(INCR_DECR_ON_NON_UINT_ARGUMENT);
            request_complete();
        }
    } else {
        new txt_memcached_incr_decr_request_t(this, &key, increment, delta, noreply);
    }

    conn_fsm->consume(line_len);

    if (noreply)
        return request_handler_t::op_req_parallelizable;
    else
        return request_handler_t::op_req_complex;
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::parse_storage_command(storage_command command, char *state, unsigned int line_len) {
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

void txt_memcached_handler_t::on_large_value_completed(bool success) {
    // Used for consuming data from the socket. XXX: This should be renamed.
    assert(success);
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::read_data() {
    check("memcached handler should be in loading data state", !loading_data);
    
    /* This function is a messy POS. It is possibly called many times per request, and it
    performs several different roles, some of which I don't entirely understand. */
    
    data_provider_t *dp;
    bool is_large = bytes > MAX_BUFFERED_SET_SIZE;
    if (!is_large) {
        if (conn_fsm->nrbuf < bytes + 2){ // check that the buffer contains enough data.  must also include \r\n
            return request_handler_t::op_partial_packet;
        }
        loading_data = false;
        if (conn_fsm->rbuf[bytes] != '\r' || conn_fsm->rbuf[bytes+1] != '\n') {
            conn_fsm->sbuf->printf(BAD_BLOB);
            conn_fsm->consume(bytes+2);
            return request_handler_t::op_malformed;
        }
        dp = new buffered_data_provider_t(conn_fsm->rbuf, bytes);
        conn_fsm->consume(bytes+2);
    } else {
        if (this->data_provider) {
            /* We already dispatched a request to the KVS and the rh_data_provider_t fed it data.
            Now we're checking to make sure that it ends in "\r\n" so we can sign off on the
            rh_data_provider_t so that it sends a "success" rather than "failure" to its caller. */
            if (conn_fsm->nrbuf < 2) {
                return request_handler_t::op_partial_packet;
            }
            bool well_formed = conn_fsm->rbuf[0] == '\r' && conn_fsm->rbuf[1] == '\n';
            conn_fsm->consume(2);
            this->data_provider->fill_complete(well_formed);
            this->data_provider = NULL;
            loading_data = false;
            if (well_formed) {
                return request_handler_t::op_req_complex;
            } else {
                conn_fsm->sbuf->printf(BAD_BLOB);
                return request_handler_t::op_malformed;
            }
        } else {
            dp = new rh_data_provider_t(this, bytes);
        }
    }
    
    /* We might not reach this point in the function; there's a good chance of having bailed out
    above for various reasons, like if we had already started a request or something. */
    
    switch(cmd) {
        case SET:
            new txt_memcached_set_request_t(this, &key, dp, btree_set_fsm_t::set_type_set, mcflags, exptime, 0, noreply);
            break;
        case ADD:
            new txt_memcached_set_request_t(this, &key, dp, btree_set_fsm_t::set_type_add, mcflags, exptime, 0, noreply);
            break;
        case REPLACE:
            new txt_memcached_set_request_t(this, &key, dp, btree_set_fsm_t::set_type_replace, mcflags, exptime, 0, noreply);
            break;
        case CAS:
            new txt_memcached_set_request_t(this, &key, dp, btree_set_fsm_t::set_type_cas, mcflags, exptime, cas, noreply);
            break;
        // APPEND and PREPEND always ignore flags and exptime.
        case APPEND:
            new txt_memcached_append_prepend_request_t(this, &key, dp, true, noreply);
            break;
        case PREPEND:
            new txt_memcached_append_prepend_request_t(this, &key, dp, false, noreply);
            break;
        default:
            fail("Bad storage command.");
    }
    
    if (!is_large) {
        if (noreply)
            return request_handler_t::op_req_parallelizable;
        else
            return request_handler_t::op_req_complex;
    } else {
        return request_handler_t::op_req_complex;
    }
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::malformed_request() {
    conn_fsm->sbuf->printf(MALFORMED_RESPONSE);
    return request_handler_t::op_malformed;
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::unimplemented_request() {
    conn_fsm->sbuf->printf(UNIMPLEMENTED_RESPONSE);
    return request_handler_t::op_malformed;
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::get(char *state, unsigned int line_len, bool with_cas) {
    txt_memcached_handler_t::parse_result_t res;

    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL) { 
        res = malformed_request();
    } else {
        txt_memcached_get_request_t *rq = new txt_memcached_get_request_t(this, with_cas);

        do {
            if (strlen(key_str) >  MAX_KEY_SIZE) {
                //check to make sure the key isn't too long
                res = malformed_request();
                delete rq;
                goto error_breakout;
            }
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

        res = request_handler_t::op_req_complex;
    }
    //clean out the rbuf
error_breakout:
    conn_fsm->consume(line_len); //XXX this line must always be called (no returning from anywhere else)
    return res;
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::remove(char *state, unsigned int line_len) {
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
    new txt_memcached_delete_request_t(this, &key, this->noreply);

    //clean out the rbuf
    conn_fsm->consume(line_len);

    if (this->noreply)
        return request_handler_t::op_req_parallelizable;
    else
        return request_handler_t::op_req_complex;
}

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::parse_stat_command(unsigned int line_len, char *cmd_str) {
    if(line_len > MAX_STATS_REQ_LEN) {
        //stat line is too big (if we run in to this alot increase MAX_STATS_REQ_LEN
        conn_fsm->sbuf->printf("%sTOO BIG\n", BAD_STAT);
        return request_handler_t::op_req_send_now;
    }
    size_t offset = strlen(cmd_str) + 1 + (cmd_str - conn_fsm->rbuf);
    if (offset < line_len) {
        new txt_memcached_perfmon_request_t(this, conn_fsm->rbuf + offset, line_len - offset);
    } else {
        new txt_memcached_perfmon_request_t(this, "0", 1);
    }
    conn_fsm->consume(line_len);
    return request_handler_t::op_req_complex;
}

static store_t::replicant_t *test_replicant;

class test_replicant_t :
    public store_t::replicant_t
{
    void value(store_key_t *key, const_buffer_group_t *bg, done_callback_t *cb, mcflags_t flags, exptime_t exptime, cas_t cas) {
        flockfile(stderr);
        debugf("VALUE '%*.*s': '", key->size, key->size, key->contents);
        for (int i = 0; i < (int)bg->buffers.size(); i++) {
            fwrite(bg->buffers[i].data, 1, bg->buffers[i].size, stderr);
        }
        fprintf(stderr, "' %d %d %d\n", (int)flags, (int)exptime, (int)cas);
        funlockfile(stderr);
        cb->have_copied_value();
    }
    
    void stopped() {
        debugf("Stopped replicating.\n");
        delete this;
    }
};

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::parse_gc_command(unsigned int line_len, char *state) {
    
    const char *subcommand = strtok_r(NULL, DELIMS, &state);

    if (subcommand == NULL) {
        conn_fsm->consume(line_len);
        return malformed_request();
    }

    if (!strcmp(subcommand, "gc")) {
        const char *gc_subcommand = strtok_r(NULL, DELIMS, &state);

        if (gc_subcommand == NULL) {
            conn_fsm->consume(line_len);
            return malformed_request();
        } else if (!strcmp(gc_subcommand, "disable")) {
            if (strtok_r(NULL, DELIMS, &state)) {
                conn_fsm->consume(line_len);
                return malformed_request();
            }
            conn_fsm->consume(line_len);
                
            new disable_gc_request_t(this);
            return request_handler_t::op_req_complex;
        } else if (!strcmp(gc_subcommand, "enable")) {
            if (strtok_r(NULL, DELIMS, &state)) {
                conn_fsm->consume(line_len);
                return malformed_request();
            }
            conn_fsm->consume(line_len);
                
            new enable_gc_request_t(this);
            return request_handler_t::op_req_complex;
        } else {
            conn_fsm->consume(line_len);
            return malformed_request();
        }
        
    } else if (!strcmp(subcommand, "replicate")) {
        conn_fsm->consume(line_len);
        if (!test_replicant) {
            test_replicant = new test_replicant_t;
            server->store->replicate(test_replicant);
            conn_fsm->sbuf->printf("Replicating to stderr.\r\n");
        } else {
            conn_fsm->sbuf->printf("Already replicating.\r\n");
        }
        return request_handler_t::op_req_send_now;
        
    } else if (!strcmp(subcommand, "unreplicate")) {
        if (test_replicant) {
            server->store->stop_replicating(test_replicant);
            test_replicant = NULL;
            conn_fsm->sbuf->printf("Stopped replicating.\r\n");
        } else {
            conn_fsm->sbuf->printf("Already not replicating.\r\n");
        }
        return request_handler_t::op_req_send_now;
        
    } else if (!strcmp(subcommand, "help")) {
        if (strtok_r(NULL, DELIMS, &state)) {
            conn_fsm->consume(line_len);
            return malformed_request();
        }

        conn_fsm->consume(line_len);

        conn_fsm->sbuf->printf("Commonly used commands:\n  rethinkdbctl gc disable\n  rethinkdbctl gc enable\n");
        return request_handler_t::op_req_send_now;
    } else {
        // Invalid command.
        conn_fsm->consume(line_len);
        return malformed_request();
    }
}

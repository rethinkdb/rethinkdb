#include "request_handler/txt_memcached_handler.hpp"

#include <string.h>
#include "arch/arch.hpp"
#include "conn_fsm.hpp"
#include "corefwd.hpp"
#include "request.hpp"
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
#define KEY_EXISTS "EXISTS\r\n"
#define DELETE_SUCCESS "DELETED\r\n"
#define RETRIEVE_TERMINATOR "END\r\n"
#define BAD_BLOB "CLIENT_ERROR bad data chunk\r\n"
#define TOO_LARGE "SERVER_ERROR object too large for cache\r\n"
#define MAX_COMMAND_SIZE 100

#define MAX_STATS_REQ_LEN 100

// Please read and understand the memcached protocol before modifying this
// file. If you only do a cursory readthrough, please check with someone who
// has read it in depth before committing.

// Used for making a bunch of calls to some btree_fsm_t's and waiting
// for them to complete.
class txt_memcached_request_t :
    public request_callback_t,
    public btree_fsm_callback_t
{
public:
    txt_memcached_request_t(txt_memcached_handler_t *rh, bool noreply)
        : request_callback_t(rh), rh(rh), noreply(noreply), nfsms(0)
        {}
    virtual ~txt_memcached_request_t() { }

    void on_btree_fsm_complete(btree_fsm_t *fsm) {
        if (fsm) on_fsm_ready(fsm); // Hack
        nfsms--;
        if (nfsms == 0) {
            if (!noreply) {
                build_response(rh->conn_fsm->sbuf);
                rh->request_complete();
            }
            delete this;
        }
    }

protected:
    virtual void build_response(linked_buf_t *) = 0;

    txt_memcached_handler_t * const rh;
    bool const noreply;
    int nfsms;

private:
    DISABLE_COPYING(txt_memcached_request_t);
};

class txt_memcached_get_request_t :
    public txt_memcached_request_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_get_request_t> {

public:
    using txt_memcached_request_t::rh;

public:
    // A txt_memcached_get_request_t supports more than one get at one time, so it takes several steps
    // to build it. First the constructor is called; then add_get() is called one or more times.

    txt_memcached_get_request_t(txt_memcached_handler_t *rh)
        : txt_memcached_request_t(rh, false), num_fsms(0), curr_fsm(0) {}

    bool add_get(btree_key *key) {
        if (num_fsms < MAX_OPS_IN_REQUEST) {
            btree_get_fsm_t *fsm = new btree_get_fsm_t(key, rh->server->store, this);
            ready_fsms[num_fsms] = false;
            fsms[num_fsms ++] = fsm;
            nfsms++;
            return true;
        } else {
            return false;
        }
    }
    
    void dispatch() {
        for (int i = 0; i < num_fsms; i ++) {
            fsms[i]->run(this);
        }
    }
    
    ~txt_memcached_get_request_t() {
        for (int i = 0; i < num_fsms; i ++) {
            // TODO PERFMON get_cpu_context()->worker->cmd_get++;
            delete fsms[i];
        }
    }

    void value_header(linked_buf_t *sbuf, btree_get_fsm_t *fsm) {
        // TODO PERFMON get_cpu_context()->worker->get_hits++;
        sbuf->printf("VALUE %*.*s %u %u\r\n",
                fsm->key.size, fsm->key.size, fsm->key.contents,
                fsm->value.mcflags(),
                fsm->value.value_size());
    }

    void on_fsm_ready(btree_fsm_t *ready_fsm) {
        int i;
        for (i = 0; i < num_fsms; i++) { // XXX: Is this the right way to do this?
            if (ready_fsm == fsms[i]) {
                ready_fsms[i] = true;
                break;
            }
        }
        assert(i < num_fsms);
        linked_buf_t *sbuf = rh->conn_fsm->sbuf; // XXX
        send_next_value(sbuf);
    }

    void send_next_value(linked_buf_t *sbuf) {
        while (curr_fsm < num_fsms) {
            if (!ready_fsms[curr_fsm]) return; // The next FSM we're supposed to be sending isn't ready yet.
            btree_get_fsm_t *fsm = fsms[curr_fsm];
            switch (fsm->status_code) {
                case btree_get_fsm_t::S_SUCCESS:
                    if (fsm->value.is_large()) {
                        switch (fsm->state) { // XXX
                            case btree_get_fsm_t::large_value_acquired:
                                value_header(sbuf, fsm);
                                fsm->begin_lv_write();
                                return;
                                break;
                            case btree_get_fsm_t::lookup_complete:
                                sbuf->printf("\r\n");
                                break;
                            case btree_get_fsm_t::large_value_writing:
                                return;
                                break;
                            default:
                                assert(0);
                                break;
                        }
                    } else {
                        value_header(sbuf, fsm);
                        sbuf->append(fsm->value.value(), fsm->value.value_size());
                        sbuf->printf("\r\n");
                    }
                    break;
                case btree_get_fsm_t::S_NOT_FOUND:
                    // TODO PERFMON get_cpu_context()->worker->get_misses++;
                    break;
                default:
                    assert(0);
                    break;
            }
            curr_fsm++;
        }
    }

    void build_response(linked_buf_t *sbuf) {
#ifndef NDEBUG
        for (int i = 0; i < num_fsms; i++) {
            assert(ready_fsms[i]);
        }
#endif
        send_next_value(sbuf);
        assert(curr_fsm == num_fsms);
        sbuf->printf(RETRIEVE_TERMINATOR);
    }

private:
    int num_fsms;
    int curr_fsm;
    btree_get_fsm_t *fsms[MAX_OPS_IN_REQUEST];
    bool ready_fsms[MAX_OPS_IN_REQUEST];
};

// FIXME horrible redundancy
class txt_memcached_get_cas_request_t :
    public txt_memcached_request_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_get_cas_request_t> {
public:
    using txt_memcached_request_t::rh;

public:
    // A txt_memcached_get_request supports more than one get at one time, so it takes several steps
    // to build it. First the constructor is called; then add_get() is called one or more times;
    // then dispatch() is called.

    txt_memcached_get_cas_request_t(txt_memcached_handler_t *rh)
        : txt_memcached_request_t(rh, false), num_fsms(0), curr_fsm(0) {}

    bool add_get(btree_key *key) {
        if (num_fsms < MAX_OPS_IN_REQUEST) {
            btree_get_cas_fsm_t *fsm = new btree_get_cas_fsm_t(key, rh->server->store, this);
            ready_fsms[num_fsms] = false;
            fsms[num_fsms ++] = fsm;
            nfsms++;
            return true;
        } else {
            return false;
        }
    }
    
    void dispatch() {
        for (int i = 0; i < num_fsms; i ++) {
            fsms[i]->run(this);
        }
    }

    ~txt_memcached_get_cas_request_t() {
        for (int i = 0; i < num_fsms; i ++) {
            delete fsms[i];
        }
    }

    void value_header(linked_buf_t *sbuf, btree_get_cas_fsm_t *fsm) {
        // TODO: Figure out stats.
        //get_cpu_context()->worker->get_hits++; // XXX: Perfmon is broken.
        sbuf->printf("VALUE %*.*s %u %u %llu\r\n",
            fsm->key.size, fsm->key.size, fsm->key.contents,
            fsm->value.mcflags(),
            fsm->value.value_size(),
            fsm->value.cas());
    }

    void on_fsm_ready(btree_fsm_t *ready_fsm) {
        int i;
        for (i = 0; i < num_fsms; i++) { // XXX: Is this the right way to do this?
            if (ready_fsm == fsms[i]) {
                ready_fsms[i] = true;
                break;
            }
        }
        assert(i < num_fsms);
        linked_buf_t *sbuf = rh->conn_fsm->sbuf; // XXX
        send_next_value(sbuf);
    }

    void send_next_value(linked_buf_t *sbuf) {
        while (curr_fsm < num_fsms) {
            if (!ready_fsms[curr_fsm]) return; // The next FSM we're supposed to be sending isn't ready yet.
            btree_get_cas_fsm_t *fsm = fsms[curr_fsm];
            switch (fsm->status_code) {
                case btree_get_fsm_t::S_SUCCESS:
                    if (fsm->value.is_large()) {
                        switch (fsm->lv_state) {
                            case btree_get_cas_fsm_t::lv_state_ready:
                                value_header(sbuf, fsm);
                                fsm->begin_lv_write();
                                return;
                                break;
                            case btree_get_cas_fsm_t::lv_state_done:
                                sbuf->printf("\r\n");
                                // XXX: Set ready back to false?
                                break;
                            case btree_get_cas_fsm_t::lv_state_writing:
                                return;
                                break;
                            default:
                                assert(0);
                                break;
                        }
                    } else {
                        value_header(sbuf, fsm);
                        sbuf->append(fsm->value.value(), fsm->value.value_size());
                        sbuf->printf("\r\n");
                    }
                    break;
                case btree_get_fsm_t::S_NOT_FOUND:
                    //get_cpu_context()->worker->get_misses++; // XXX Perfmon is broken.
                    break;
                default:
                    assert(0);
                    break;
            }
            curr_fsm++;
        }
    }

    void build_response(linked_buf_t *sbuf) {
#ifndef NDEBUG
            for (int i = 0; i < num_fsms; i++) {
                assert(ready_fsms[i]);
            }
#endif
        send_next_value(sbuf);
        assert(curr_fsm == num_fsms);
        sbuf->printf(RETRIEVE_TERMINATOR);
    }

private:
    int num_fsms;
    int curr_fsm;
    btree_get_cas_fsm_t *fsms[MAX_OPS_IN_REQUEST];
    bool ready_fsms[MAX_OPS_IN_REQUEST];
};

class txt_memcached_set_request_t :
    public txt_memcached_request_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_set_request_t> {
public:
    txt_memcached_set_request_t(txt_memcached_handler_t *rh, btree_key *key, bool is_large, uint32_t length, byte *data, btree_set_fsm_t::set_type_t type, btree_value::mcflags_t mcflags, btree_value::exptime_t exptime, btree_value::cas_t req_cas, bool noreply)
        : txt_memcached_request_t(rh, noreply),
          fsm(new btree_set_fsm_t(key, rh->server->store, this, is_large, length, data, type, mcflags, convert_exptime(exptime), req_cas)) {
        fsm->run(this);
        nfsms = 1;
    }

    ~txt_memcached_set_request_t() {
        // TODO PERFMON get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }

    void build_response(linked_buf_t *sbuf) {
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
            case btree_set_fsm_t::S_NOT_FOUND:
                sbuf->printf(NOT_FOUND);
                break;
            case btree_set_fsm_t::S_READ_FAILURE:
                // read_data() handles printing the error.
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
        // TODO: Do we need to handle exptimes in the past here?
        return exptime;
    }

private:
    btree_set_fsm_t *fsm;
};

class txt_memcached_incr_decr_request_t :
    public txt_memcached_request_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_incr_decr_request_t> {

public:
    txt_memcached_incr_decr_request_t(txt_memcached_handler_t *rh, btree_key *key, bool increment, long long delta, bool noreply)
        : txt_memcached_request_t(rh, noreply),
          fsm(new btree_incr_decr_fsm_t(key, rh->server->store, increment, delta))
        {
        fsm->run(this);
        nfsms = 1;
    }

    ~txt_memcached_incr_decr_request_t() {
        // TODO PERFMON get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }

    void build_response(linked_buf_t *sbuf) {
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

class txt_memcached_append_prepend_request_t :
    public txt_memcached_request_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_append_prepend_request_t> {
public:
    txt_memcached_append_prepend_request_t(txt_memcached_handler_t *rh, btree_key *key, bool got_large, int length, byte *data, bool append, bool noreply)
        : txt_memcached_request_t(rh, noreply),
          fsm(new btree_append_prepend_fsm_t(key, rh->server->store, this, got_large, length, data, append)) {
        fsm->run(this);
        nfsms = 1;
    }

    ~txt_memcached_append_prepend_request_t() {
        // TODO PERFMON get_cpu_context()->worker->cmd_set++;
        delete fsm;
    }

    void build_response(linked_buf_t *sbuf) {
        switch (fsm->status_code) {
            case btree_append_prepend_fsm_t::S_SUCCESS:
                sbuf->printf(STORAGE_SUCCESS);
                break;
            case btree_append_prepend_fsm_t::S_TOO_LARGE:
            case btree_append_prepend_fsm_t::S_NOT_FOUND:
                // memcached pronounces these "NOT_STORED".
                sbuf->printf(STORAGE_FAILURE);
                break;
            case btree_append_prepend_fsm_t::S_READ_FAILURE:
                // read_data() handles printing the error.
                break;
            default:
                assert(0);
                break;
        }
    }

private:
    btree_append_prepend_fsm_t *fsm;
};

class txt_memcached_delete_request_t :
    public txt_memcached_request_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_delete_request_t> {

public:
    txt_memcached_delete_request_t(txt_memcached_handler_t *rh, btree_key *key, bool noreply)
        : txt_memcached_request_t(rh, noreply),
          fsm(new btree_delete_fsm_t(key, rh->server->store)) {
        fsm->run(this);
        nfsms = 1;
    }

    ~txt_memcached_delete_request_t() {
        delete fsm;
    }

    void build_response(linked_buf_t *sbuf) {
        switch (fsm->status_code) {
            case btree_delete_fsm_t::S_DELETED:
                sbuf->printf(DELETE_SUCCESS);
                break;
            case btree_delete_fsm_t::S_NOT_FOUND:
                sbuf->printf(NOT_FOUND);
                break;
            default:
                assert(0);
                break;
        }
    }

private:
    btree_delete_fsm_t *fsm;
};

class txt_memcached_perfmon_request_t :
    public request_callback_t,
    public cpu_message_t,   // For call_later_on_this_cpu()
    public perfmon_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, txt_memcached_perfmon_request_t> {

public:
    txt_memcached_perfmon_request_t(txt_memcached_handler_t *rh)
        : request_callback_t(rh) {

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
                std_string_t s(stat);
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
    public home_cpu_mixin_t,
    public request_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, disable_gc_request_t> {
public:
    disable_gc_request_t(txt_memcached_handler_t *rh)
        : request_callback_t(rh), done(false) {

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
    public home_cpu_mixin_t,
    public request_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, enable_gc_request_t> {

public:
    enable_gc_request_t(txt_memcached_handler_t *rh)
        : request_callback_t(rh), done(false) {

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

    // TODO: we might end up getting a command, and a piece of the
    // next command. It also means that we can't use one buffer
    //for both recv and send, we need to add a send buffer
    // (assuming we want to support out of band  commands).

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
        return get(state, line_len);
    } else if(!strcmp(cmd_str, "gets")) {
        return get_cas(state, line_len);

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
        txt_memcached_perfmon_request_t *rq = new txt_memcached_perfmon_request_t(this);
        check("Too big of a stat request", line_len > MAX_STATS_REQ_LEN);
        size_t offset = strlen(cmd_str) + 1 + (cmd_str - conn_fsm->rbuf);
        if (offset < line_len) {
            memcpy(rq->fields, conn_fsm->rbuf + offset, line_len - offset);
        } else {
            *rq->fields = '\0';
        }
        conn_fsm->consume(line_len);
        return request_handler_t::op_req_complex;
    } else if(!strcmp(cmd_str, "rethinkdbctl")) {
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
    long long delta = atoll(value_str);
    new txt_memcached_incr_decr_request_t(this, &key, increment, delta, noreply);

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
    if (consuming) {
        int bytes_to_consume = std::min(conn_fsm->nrbuf, bytes);
        conn_fsm->consume(bytes_to_consume);
        bytes -= bytes_to_consume;
        if (bytes == 0) {
            consuming = false;
            loading_data = false;
            return request_handler_t::op_req_send_now;
        } else {
            return request_handler_t::op_partial_packet;
        }
    }
    bool is_large = bytes > MAX_IN_NODE_VALUE_SIZE;
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
    } else {
        if (this->fill_lv_msg) {
            if (conn_fsm->nrbuf < 2) {
                return request_handler_t::op_partial_packet;
            }
            bool well_formed = conn_fsm->rbuf[0] == '\r' && conn_fsm->rbuf[1] == '\n';
            conn_fsm->consume(2);
            this->fill_lv_msg->fill_complete(well_formed);
            this->fill_lv_msg = NULL;
            loading_data = false;
            if (well_formed) {
                return request_handler_t::op_req_complex;
            } else {
                conn_fsm->sbuf->printf(BAD_BLOB);
                return request_handler_t::op_malformed;
            }
        } else if (bytes > MAX_VALUE_SIZE) {
                conn_fsm->sbuf->printf(TOO_LARGE);
                consuming = true;
                bytes += 2; // \r\n
                return request_handler_t::op_req_send_now;
        } else {
            // Will be handled in the FSM.
        }
    }

    switch(cmd) {
        case SET:
            new txt_memcached_set_request_t(this, &key, is_large, bytes, conn_fsm->rbuf, btree_set_fsm_t::set_type_set, mcflags, exptime, 0, noreply);
            break;
        case ADD:
            new txt_memcached_set_request_t(this, &key, is_large, bytes, conn_fsm->rbuf, btree_set_fsm_t::set_type_add, mcflags, exptime, 0, noreply);
            break;
        case REPLACE:
            new txt_memcached_set_request_t(this, &key, is_large, bytes, conn_fsm->rbuf, btree_set_fsm_t::set_type_replace, mcflags, exptime, 0, noreply);
            break;
        case CAS:
            new txt_memcached_set_request_t(this, &key, is_large, bytes, conn_fsm->rbuf, btree_set_fsm_t::set_type_cas, mcflags, exptime, cas, noreply);
            break;
        // APPEND and PREPEND always ignore flags and exptime.
        case APPEND:
            new txt_memcached_append_prepend_request_t(this, &key, is_large, bytes, conn_fsm->rbuf, true, noreply);
            break;
        case PREPEND:
            new txt_memcached_append_prepend_request_t(this, &key, is_large, bytes, conn_fsm->rbuf, false, noreply);
            break;
        default:
            conn_fsm->consume(bytes+2);
            return malformed_request();
    }

    if (!is_large) {
        conn_fsm->consume(bytes+2);

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

txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::get(char *state, unsigned int line_len) {
    txt_memcached_handler_t::parse_result_t res;

    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL) { 
        res = malformed_request();
    } else {
        txt_memcached_get_request_t *rq = new txt_memcached_get_request_t(this);

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

        res = request_handler_t::op_req_complex;
    }
    //clean out the rbuf
    conn_fsm->consume(line_len); //XXX this line must always be called (no returning from anywhere else)
    return res;
}

// FIXME horrible redundancy
txt_memcached_handler_t::parse_result_t txt_memcached_handler_t::get_cas(char *state, unsigned int line_len) {
    txt_memcached_handler_t::parse_result_t res;

    char *key_str = strtok_r(NULL, DELIMS, &state);
    if (key_str == NULL) {
        res = malformed_request();
    } else {
        txt_memcached_get_cas_request_t *rq = new txt_memcached_get_cas_request_t(this);

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
        
        res = request_handler_t::op_req_complex;
    }

    //clean out the rbuf
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

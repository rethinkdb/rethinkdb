#include <stdexcept>
#include <stdarg.h>
#include "memcached/memcached.hpp"
#include "concurrency/task.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/cond_var.hpp"
#include "arch/arch.hpp"
#include "server/server.hpp"

/* txt_memcached_handler_t is basically defunct; it only exists as a convenient thing to pass
around to do_get(), do_storage(), and the like. */

struct txt_memcached_handler_t {

    tcp_conn_t *conn;
    server_t *server;

    txt_memcached_handler_t(tcp_conn_t *conn, server_t *server) :
        conn(conn),
        server(server),
        requests_out_sem(MAX_CONCURRENT_QUERIES_PER_CONNECTION)
    { }

    /* Wrappers around conn->write*() that ignore write errors, because we want to continue
    processing requests even if the client is no longer listening for replies */
    void write(const char *buffer, size_t bytes) {
        try {
            conn->write_buffered(buffer, bytes);
        } catch (tcp_conn_t::write_closed_exc_t) {
        }
    }
    void writef(const char *format, ...) {
        va_list args;
        va_start(args, format);
        char buffer[1000];
        size_t bytes = vsnprintf(buffer, sizeof(buffer), format, args);
        rassert(bytes < sizeof(buffer));
        write(buffer, bytes);
        va_end(args);
    }
    void write_unbuffered(const char *buffer, size_t bytes) {
        try {
            conn->write(buffer, bytes);
        } catch (tcp_conn_t::write_closed_exc_t) {
        }
    }

    // Used to limit number of concurrent noreply requests
    semaphore_t requests_out_sem;

    /* If a client sends a bunch of noreply requests and then a 'quit', we cannot delete ourself
    immediately because the noreply requests still hold the 'requests_out' semaphore. We use this
    lock to figure out when we can delete ourself. Each request acquires this lock in non-exclusive
    (read) mode, and when we want to shut down we acquire it in exclusive (write) mode. That way
    we don't shut down until everything that holds a reference to the semaphore is gone. */
    rwi_lock_t prevent_shutdown_lock;

    // Used to implement throttling. Write requests should call begin_write_command() and
    // end_write_command() to make sure that not too many write requests are sent
    // concurrently.
    void begin_write_command() {
        prevent_shutdown_lock.co_lock(rwi_read);
        requests_out_sem.co_lock();
    }
    void end_write_command() {
        prevent_shutdown_lock.unlock();
        requests_out_sem.unlock();
    }
};

/* do_get() is used for "get" and "gets" commands. */

struct get_t {
    union {
        char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
        btree_key key;
    };
    task_t<store_t::get_result_t> *result;
};

void do_get(txt_memcached_handler_t *rh, bool with_cas, int argc, char **argv) {

    rassert(argc >= 1);
    rassert(strcmp(argv[0], "get") == 0 || strcmp(argv[0], "gets") == 0);

    /* Vector to store the keys and the task-objects */
    std::vector<get_t> gets;

    /* First parse all of the keys to get */
    gets.reserve(argc - 1);
    for (int i = 1; i < argc; i++) {
        gets.push_back(get_t());
        if (!node::str_to_key(argv[i], &gets.back().key)) {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            return;
        }
    }
    if (gets.size() == 0) {
        rh->writef("ERROR\r\n");
        return;
    }

    /* Now that we're sure they're all valid, send off the requests */
    for (int i = 0; i < (int)gets.size(); i++) {
        gets[i].result = task<store_t::get_result_t>(
            with_cas ? &store_t::co_get_cas : &store_t::co_get,
            rh->server->store,
            &gets[i].key);
    }

    /* Handle the results in sequence */
    for (int i = 0; i < (int)gets.size(); i++) {

        store_t::get_result_t res = gets[i].result->join();

        /* If the write half of the connection has been closed, there's no point in trying
        to send anything */
        if (rh->conn->is_write_open()) {
            btree_key &key = gets[i].key;

            /* If res.buffer is NULL that means the value was not found so we don't write
            anything */
            if (res.buffer) {

                /* Write the "VALUE ..." header */
                if (with_cas) {
                    rh->writef("VALUE %*.*s %u %u %llu\r\n",
                        key.size, key.size, key.contents, res.flags, res.buffer->get_size(), res.cas);
                } else {
                    rassert(res.cas == 0);
                    rh->writef("VALUE %*.*s %u %u\r\n",
                        key.size, key.size, key.contents, res.flags, res.buffer->get_size());
                }

                /* Write the value itself. If the value is small, write it into the send buffer;
                otherwise, stream it. */
                for (int i = 0; i < (signed)res.buffer->buffers.size(); i++) {
                    const_buffer_group_t::buffer_t b = res.buffer->buffers[i];
                    if (res.buffer->get_size() < MAX_BUFFERED_GET_SIZE) {
                        rh->write((const char *)b.data, b.size);
                    } else {
                        rh->write_unbuffered((const char *)b.data, b.size);
                    }
                }
                rh->writef("\r\n");
            }
        }

        /* Call the callback so that the value is released */
        if (res.buffer) res.cb->have_copied_value();
    }

    rh->writef("END\r\n");
};

/* "set", "add", "replace", "cas", "append", and "prepend" command logic */

/* The buffered_data_provider_t provides data from an internal buffer. */

struct buffered_data_provider_t :
    public data_provider_t
{
    /* We copy the data we are given, not refer to it. */
    buffered_data_provider_t(void *buf, size_t length)
        : buffer(malloc(length)), length(length)
    {
        memcpy(buffer, buf, length);
    }
    ~buffered_data_provider_t() {
        free(buffer);
    }
    size_t get_size() {
        return length;
    }
    void get_value(buffer_group_t *bg, data_provider_t::done_callback_t *cb) {
        if (bg) {
            int pos = 0;
            for (int i = 0; i < (signed)bg->buffers.size(); i++) {
                memcpy(bg->buffers[i].data, (char*)buffer + pos, bg->buffers[i].size);
                pos += bg->buffers[i].size;
            }
            rassert(pos == (signed)length);
        }
        cb->have_provided_value();
    }
private:
    void *buffer;
    size_t length;
};

/* The memcached_streaming_data_provider_t is a data_provider_t that gets its data by
reading from a tcp_conn_t. In general it is used for big values that would be expensive
to buffer in memory. The disadvantage is that it requires going back and forth between
the core where the cache is and the core where the request handler is.

It also takes care of checking for a CRLF at the end of the value (the
buffered_data_provider_t doesn't do that). */

class memcached_streaming_data_provider_t :
    public data_provider_t,
    public home_thread_mixin_t
{
private:
    txt_memcached_handler_t *rh;   // Request handler for us to read from
    size_t length;   // Our length

public:
    memcached_streaming_data_provider_t(txt_memcached_handler_t *rh, uint32_t length)
        : rh(rh), length(length)
    {
    }

    ~memcached_streaming_data_provider_t() {
    }

    size_t get_size() {
        return length;
    }

    void get_value(buffer_group_t *b, data_provider_t::done_callback_t *c) {
        coro_t::spawn(&memcached_streaming_data_provider_t::run, this, b, c);
    }

    void run(buffer_group_t *b, data_provider_t::done_callback_t *c) {
        bool success;
        {
            on_thread_t thread_switcher(home_thread);
            try {
                if (b) {
                    for (int i = 0; i < (int)b->buffers.size(); i++) {
                        rh->conn->read(b->buffers[i].data, b->buffers[i].size);
                    }
                } else {
                    /* Allocate a buffer from the heap instead of the stack because we are in a
                    coroutine */
                    std::vector<char> dummy(IO_BUFFER_SIZE);
                    int consumed = 0;
                    while (consumed < (int)length) {
                        int chunk = std::min((int)length - consumed, (int)IO_BUFFER_SIZE);
                        rh->conn->read(dummy.data(), chunk);
                        consumed += chunk;
                    }
                }
                char crlf[2];
                rh->conn->read(crlf, 2);
                success = (memcmp(crlf, "\r\n", 2) == 0);
            } catch (tcp_conn_t::read_closed_exc_t) {
                success = false;
            }
        }
        if (success) c->have_provided_value();
        else c->have_failed();
    }
};

enum storage_command_t {
    set_command,
    add_command,
    replace_command,
    cas_command,
    append_command,
    prepend_command
};

enum set_result_t {
    sr_stored,
    sr_not_stored,
    sr_not_found,
    sr_exists,
    sr_too_large,
    sr_data_provider_failed
};

struct set_callback_t
    : public store_t::set_callback_t, public value_cond_t<set_result_t>
{
    void stored() { pulse(sr_stored); }
    void not_stored() { pulse(sr_not_stored); }
    void not_found() { pulse(sr_not_found); }
    void exists() { pulse(sr_exists); }
    void too_large() { pulse(sr_too_large); }
    void data_provider_failed() { pulse(sr_data_provider_failed); }
};

set_result_t co_set(store_t *store, store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime) {
    set_callback_t cb;
    store->set(key, data, flags, exptime, &cb);
    return cb.wait();
};

set_result_t co_add(store_t *store, store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime) {
    set_callback_t cb;
    store->add(key, data, flags, exptime, &cb);
    return cb.wait();
}

set_result_t co_replace(store_t *store, store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime) {
    set_callback_t cb;
    store->replace(key, data, flags, exptime, &cb);
    return cb.wait();
}

set_result_t co_cas(store_t *store, store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique) {
    set_callback_t cb;
    store->cas(key, data, flags, exptime, unique, &cb);
    return cb.wait();
}

enum append_prepend_result_t {
    apr_success,
    apr_too_large,
    apr_not_found,
    apr_data_provider_failed
};

struct append_prepend_callback_t
    : public store_t::append_prepend_callback_t, public value_cond_t<append_prepend_result_t>
{
    void success() { pulse(apr_success); }
    void not_found() { pulse(apr_not_found); }
    void too_large() { pulse(apr_too_large); }
    void data_provider_failed() { pulse(apr_data_provider_failed); }
};

append_prepend_result_t co_append(store_t *store, store_key_t *key, data_provider_t *data) {
    append_prepend_callback_t cb;
    store->append(key, data, &cb);
    return cb.wait();
}

append_prepend_result_t co_prepend(store_t *store, store_key_t *key, data_provider_t *data) {
    append_prepend_callback_t cb;
    store->prepend(key, data, &cb);
    return cb.wait();
}

void run_storage_command(txt_memcached_handler_t *rh, storage_command_t sc, store_key_and_buffer_t key, data_provider_t *data, mcflags_t mcflags, exptime_t exptime, cas_t unique, bool noreply) {

    if (sc != append_command && sc != prepend_command) {
        set_result_t res;
        switch (sc) {
            case set_command: res = co_set(rh->server->store, &key.key, data, mcflags, exptime); break;
            case add_command: res = co_add(rh->server->store, &key.key, data, mcflags, exptime); break;
            case replace_command: res = co_replace(rh->server->store, &key.key, data, mcflags, exptime); break;
            case cas_command: res = co_cas(rh->server->store, &key.key, data, mcflags, exptime, unique); break;
            case append_command:
            case prepend_command:
            default:
                unreachable();
        }
        /* Ignore 'noreply' if we got sr_data_provider_failed because that's considered a syntax
        error. Note that we can only get sr_data_provider_failed if we had a streaming data
        provider, so there is no risk of interfering with the output from the next command. */
        if (!noreply || res == sr_data_provider_failed) {
            switch (res) {
                case sr_stored: rh->writef("STORED\r\n"); break;
                case sr_not_stored: rh->writef("NOT_STORED\r\n"); break;
                case sr_exists: rh->writef("EXISTS\r\n"); break;
                case sr_not_found: rh->writef("NOT_FOUND\r\n"); break;
                case sr_too_large:
                    rh->writef("SERVER_ERROR object too large for cache\r\n");
                    break;
                case sr_data_provider_failed:
                    rh->writef("CLIENT_ERROR bad data chunk\r\n");
                    break;
                default: unreachable();
            }
        }
    } else {
        append_prepend_result_t res;
        switch (sc) {
            case append_command: res = co_append(rh->server->store, &key.key, data); break;
            case prepend_command: res = co_prepend(rh->server->store, &key.key, data); break;
            case set_command:
            case add_command:
            case replace_command:
            case cas_command:
            default:
                unreachable();
        }
        if (!noreply || res == apr_data_provider_failed) {
            switch (res) {
                case apr_success: rh->writef("STORED\r\n"); break;
                case apr_not_found: rh->writef("NOT_FOUND\r\n"); break;
                case apr_too_large:
                    rh->writef("SERVER_ERROR object too large for cache\r\n");
                    break;
                case apr_data_provider_failed:
                    rh->writef("CLIENT_ERROR bad data chunk\r\n");
                    break;
                default: unreachable();
            }
        }
    }

    delete data;
    rh->end_write_command();
}

void do_storage(txt_memcached_handler_t *rh, storage_command_t sc, int argc, char **argv) {

    char *invalid_char;

    /* cmd key flags exptime size [noreply]
    OR "cas" key flags exptime size cas [noreply] */
    if ((sc != cas_command && (argc != 5 && argc != 6)) ||
        (sc == cas_command && (argc != 6 && argc != 7))) {
        rh->writef("ERROR\r\n");
        return;
    }

    /* First parse the key */
    store_key_and_buffer_t key;
    if (!node::str_to_key(argv[1], &key.key)) {
        rh->writef("CLIENT_ERROR bad command line format\r\n");
        return;
    }

    /* Next parse the flags */
    mcflags_t mcflags = strtoul_strict(argv[2], &invalid_char, 10);
    if (*invalid_char != '\0') {
        rh->writef("CLIENT_ERROR bad command line format\r\n");
        return;
    }

    /* Now parse the expiration time */
    exptime_t exptime = strtoul_strict(argv[3], &invalid_char, 10);
    if (*invalid_char != '\0') {
        rh->writef("CLIENT_ERROR bad command line format\r\n");
        return;
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
    if (exptime <= 60*60*24*30 && exptime > 0) {
        // TODO: Do we need to handle exptimes in the past here?
        exptime += time(NULL);
    }

    /* Now parse the value length */
    size_t value_size = strtoul_strict(argv[4], &invalid_char, 10);
    // Check for signed 32 bit max value for Memcached compatibility...
    if (*invalid_char != '\0' || value_size >= (1u << 31) - 1) {
        rh->writef("CLIENT_ERROR bad command line format\r\n");
        return;
    }

    /* If a "cas", parse the cas_command unique */
    cas_t unique = 0;
    if (sc == cas_command) {
        unique = strtoull_strict(argv[5], &invalid_char, 10);
        if (*invalid_char != '\0') {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            return;
        }
    }

    /* Check for noreply */
    int offset = (sc == cas_command ? 1 : 0);
    bool noreply;
    if (argc == 6 + offset) {
        if (strcmp(argv[5 + offset], "noreply") == 0) {
            noreply = true;
        } else {
            // Memcached 1.4.5 ignores invalid tokens here
            noreply = false;
        }
    } else {
        noreply = false;
    }

    /* Make a data provider */
    data_provider_t *data;
    bool nowait;
    if (value_size <= MAX_BUFFERED_SET_SIZE) {
        /* Buffer the value */
        char value_buffer[value_size+2];   // +2 for the CRLF

        try {
            rh->conn->read(value_buffer, value_size + 2);
        } catch (tcp_conn_t::read_closed_exc_t) {
            return;    // serve_memcached() will see it is closed
        }

        /* The buffered_data_provider_t doesn't check the CRLF, so we must check it here.
        The memcached_streaming_data_provider_t checks its own CRLF. */
        if (value_buffer[value_size] != '\r' || value_buffer[value_size + 1] != '\n') {
            rh->writef("CLIENT_ERROR bad data chunk\r\n");
            return;
        }

        /* Copies value_buffer */
        data = new buffered_data_provider_t(value_buffer, value_size);
        nowait = noreply;

    } else {
        /* Stream the value from the socket */
        data = new memcached_streaming_data_provider_t(rh, value_size);
        nowait = false;
    }

    /* This operation may block if a lot of sets were streamed. The corresponding call to
    end_write_command() is in run_storage_command(). */
    rh->begin_write_command();

    /* If nowait is true, then we will wait for the command to complete before returning. There are
    two reasons why this might be true. The first is that noreply was not specified, so we need
    to send a reply before reading the next command. The second is that we are streaming a large
    value off the socket, so we can't read the next command until we're sure this one is done. */

    if (nowait) {
        coro_t::spawn(&run_storage_command, rh, sc, key, data, mcflags, exptime, unique, noreply);
    } else {
        run_storage_command(rh, sc, key, data, mcflags, exptime, unique, noreply);
    }
}

/* "incr" and "decr" commands */

struct incr_decr_result_t {
    enum result_t {
        idr_success,
        idr_not_found,
        idr_not_numeric
    } res;
    unsigned long long new_value;   // Valid only if idr_success
    incr_decr_result_t(result_t r, unsigned long long n = 0) : res(r), new_value(n) { }
};

struct incr_decr_callback_t :
    public store_t::incr_decr_callback_t,
    public value_cond_t<incr_decr_result_t>
{
    void success(unsigned long long new_value) {
        pulse(incr_decr_result_t(incr_decr_result_t::idr_success, new_value));
    }
    void not_found() {
        pulse(incr_decr_result_t(incr_decr_result_t::idr_not_found));
    }
    void not_numeric() {
        pulse(incr_decr_result_t(incr_decr_result_t::idr_not_numeric));
    }
};

incr_decr_result_t co_incr(store_t *store, store_key_t *key, unsigned long long amount) {
    incr_decr_callback_t cb;
    store->incr(key, amount, &cb);
    return cb.wait();
}

incr_decr_result_t co_decr(store_t *store, store_key_t *key, unsigned long long amount) {
    incr_decr_callback_t cb;
    store->decr(key, amount, &cb);
    return cb.wait();
}

void run_incr_decr(txt_memcached_handler_t *rh, store_key_and_buffer_t key, unsigned long long amount, bool incr, bool noreply) {

    incr_decr_result_t res = incr ?
        co_incr(rh->server->store, &key.key, amount) :
        co_decr(rh->server->store, &key.key, amount);

    if (!noreply) {
        switch (res.res) {
            case incr_decr_result_t::idr_success:
                rh->writef("%llu\r\n", res.new_value);
                break;
            case incr_decr_result_t::idr_not_found:
                rh->writef("NOT_FOUND\r\n");
                break;
            case incr_decr_result_t::idr_not_numeric:
                rh->writef("CLIENT_ERROR cannot increment or decrement non-numeric value\r\n");
                break;
            default: unreachable();
        }
    }

    rh->end_write_command();
}

void do_incr_decr(txt_memcached_handler_t *rh, bool i, int argc, char **argv) {

    /* cmd key delta [noreply] */
    if (argc != 3 && argc != 4) {
        rh->writef("ERROR\r\n");
        return;
    }

    /* Parse key */
    store_key_and_buffer_t key;
    if (!node::str_to_key(argv[1], &key.key)) {
        rh->writef("CLIENT_ERROR bad command line format\r\n");
        return;
    }

    /* Parse amount to change by */
    char *invalid_char;
    unsigned long long delta = strtoull_strict(argv[2], &invalid_char, 10);
    if (*invalid_char != '\0') {
        rh->writef("CLIENT_ERROR bad command line format\r\n");
        return;
    }

    /* Parse "noreply" */
    bool noreply;
    if (argc == 4) {
        if (strcmp(argv[3], "noreply") == 0) {
            noreply = true;
        } else {
            // Memcached 1.4.5 ignores invalid tokens here
            noreply = false;
        }
    } else {
        noreply = false;
    }

    rh->begin_write_command();

    if (noreply) {
        coro_t::spawn(&run_incr_decr, rh, key, delta, i, true);
    } else {
        run_incr_decr(rh, key, delta, i, false);
    }
}

/* "delete" commands */

enum delete_result_t {
    dr_deleted,
    dr_not_found
};

struct delete_callback_t :
    public store_t::delete_callback_t, public value_cond_t<delete_result_t>
{
    void deleted() { pulse(dr_deleted); }
    void not_found() { pulse(dr_not_found); }
};

delete_result_t co_delete(store_t *store, store_key_t *key) {
    delete_callback_t cb;
    store->delete_key(key, &cb);
    return cb.wait();
}

void run_delete(txt_memcached_handler_t *rh, store_key_and_buffer_t key, bool noreply) {

    delete_result_t res = co_delete(rh->server->store, &key.key);

    if (!noreply) {
        switch (res) {
            case dr_deleted: rh->writef("DELETED\r\n"); break;
            case dr_not_found: rh->writef("NOT_FOUND\r\n"); break;
            default: unreachable();
        }
    }

    rh->end_write_command();
}

void do_delete(txt_memcached_handler_t *rh, int argc, char **argv) {

    /* "delete" key [a number] ["noreply"] */
    if (argc < 2 || argc > 4) {
        rh->writef("ERROR\r\n");
        return;
    }

    /* Parse key */
    store_key_and_buffer_t key;
    if (!node::str_to_key(argv[1], &key.key)) {
        rh->writef("CLIENT_ERROR bad command line format\r\n");
        return;
    }

    /* Parse "noreply" */
    bool noreply;
    if (argc > 2) {
        noreply = strcmp(argv[argc - 1], "noreply") == 0;

        /* We don't support the delete queue, but we do tolerate weird bits of syntax that are
        related to it */
        bool zero = strcmp(argv[2], "0") == 0;

        bool valid = (argc == 3 && (zero || noreply))
                  || (argc == 4 && (zero && noreply));

        if (!valid) {
            if (!noreply) rh->writef("CLIENT_ERROR bad command line format.  Usage: delete <key> [noreply]\r\n");
            return;
        }
    } else {
        noreply = false;
    }

    rh->begin_write_command();

    if (noreply) {
        coro_t::spawn(&run_delete, rh, key, true);
    } else {
        run_delete(rh, key, false);
    }
};

/* "stats"/"stat" commands */

void do_stats(txt_memcached_handler_t *rh, int argc, char **argv) {

    perfmon_stats_t stats;
    perfmon_get_stats(&stats);

    if (argc == 1) {
        for (perfmon_stats_t::iterator iter = stats.begin(); iter != stats.end(); iter++) {
            rh->writef("STAT %s %s\r\n", iter->first.c_str(), iter->second.c_str());
        }
    } else {
        for (int i = 1; i < argc; i++) {
            perfmon_stats_t::iterator stat_entry = stats.find(argv[i]);
            if (stat_entry == stats.end()) {
                rh->writef("NOT FOUND\r\n");
            } else {
                rh->writef("%s\r\n", stat_entry->second.c_str());
            }
        }
    }
    rh->writef("END\r\n");
};

perfmon_duration_sampler_t
    pm_conns_reading("conns_reading", secs_to_ticks(1)),
    pm_conns_writing("conns_writing", secs_to_ticks(1)),
    pm_conns_acting("conns_acting", secs_to_ticks(1));

void read_line(tcp_conn_t *conn, std::vector<char> *dest) {

    for (;;) {
        tcp_conn_t::bufslice sl = conn->peek();
        void *crlf_loc = memmem(sl.buf, sl.len, "\r\n", 2);
        size_t threshold = MEGABYTE;

        if (crlf_loc) {
            // We have a valid line.
            size_t line_size = (char *)crlf_loc - (char *)sl.buf;

            dest->resize(line_size + 2);  // +2 for CRLF
            memcpy(dest->data(), sl.buf, line_size + 2);
            conn->pop(line_size + 2);
            return;
        } else if (sl.len > threshold) {
            // If a malfunctioning client sends a lot of data without a
            // CRLF, we cut them off.  (This doesn't apply to large values
            // because they are read from the socket via a different
            // mechanism.)  There are better ways to handle this
            // situation.
            logERR("Aborting connection %p because we got more than %u bytes without a CRLF\n",
                   coro_t::self(), threshold);
            conn->shutdown_read();
            throw tcp_conn_t::read_closed_exc_t();
        }

        // Keep trying until we get a complete line.
        conn->read_more_buffered();
    }

};

#ifndef NDEBUG
typedef union {
    char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
    btree_key key;
} key_with_memory;

std::vector<key_with_memory> key_array_to_btree_keys_vector(int argc, char **argv) {
    std::vector<key_with_memory> keys(argc);
    for (int i = 0; i < argc; i++) {
        if (!node::str_to_key(argv[i], &keys[i].key)) {
            throw std::runtime_error("bad key");
        }
    }
    return keys;
}

bool parse_debug_command(txt_memcached_handler_t *rh, int argc, char **argv) {
    if (argc < 1)
        return false;

    if (!strcmp(argv[0], ".h") && argc >= 2) {  // .h prints hash and slice of the passed keys
        try {
            std::vector<key_with_memory> keys = key_array_to_btree_keys_vector(argc-1, argv+1);
            for (std::vector<key_with_memory>::iterator it = keys.begin(); it != keys.end(); ++it) {
                btree_key& k = (*it).key;
                uint32_t hash = btree_key_value_store_t::hash(&k);
                uint32_t slice = rh->server->store->slice_nr(&k);
                rh->writef("%*s: %08lx [%lu]\r\n", k.size, k.contents, hash, slice);
            }
            rh->writef("END\r\n");
            return true;
        } catch (std::runtime_error& e) {
            rh->writef("CLIENT_ERROR bad command line format\r\n");
            return false;
        }
    } else {
        return false;
    }
}
#endif

void serve_memcache(tcp_conn_t *conn, server_t *server) {

    // logINF("Opened connection %p\n", coro_t::self());

    /* Object that we pass around to subroutines (is there a better way to do this?) */
    txt_memcached_handler_t rh(conn, server);

    /* Declared outside the while-loop so it doesn't repeatedly reallocate its buffer */
    std::vector<char> line;

    while (true) {

        /* Flush if necessary (no reason to do this the first time around, but it's easier
        to put it here than after every thing that could need to flush */
        block_pm_duration flush_timer(&pm_conns_writing);
        try {
            conn->flush_buffer();
        } catch (tcp_conn_t::write_closed_exc_t) {
            /* Ignore errors; it's OK for the write end of the connection to be closed. */
        }
        flush_timer.end();

        /* Read a line off the socket */
        block_pm_duration read_timer(&pm_conns_reading);
        try {
            read_line(conn, &line);
        } catch (tcp_conn_t::read_closed_exc_t) {
            break;
        }
        read_timer.end();

        block_pm_duration action_timer(&pm_conns_acting);

        /* Tokenize the line */
        line.push_back('\0');   // Null terminator
        std::vector<char *> args;
        char *l = line.data(), *state = NULL;
        while (char *cmd_str = strtok_r(l, " \r\n\t", &state)) {
            args.push_back(cmd_str);
            l = NULL;
        }

        if (args.size() == 0) {
            rh.writef("ERROR\r\n");
            continue;
        }

        /* Dispatch to the appropriate subclass */
        if(!strcmp(args[0], "get")) {    // check for retrieval commands
            do_get(&rh, false, args.size(), args.data());
        } else if(!strcmp(args[0], "gets")) {
            do_get(&rh, true, args.size(), args.data());
        } else if(!strcmp(args[0], "set")) {     // check for storage commands
            do_storage(&rh, set_command, args.size(), args.data());
        } else if(!strcmp(args[0], "add")) {
            do_storage(&rh, add_command, args.size(), args.data());
        } else if(!strcmp(args[0], "replace")) {
            do_storage(&rh, replace_command, args.size(), args.data());
        } else if(!strcmp(args[0], "append")) {
            do_storage(&rh, append_command, args.size(), args.data());
        } else if(!strcmp(args[0], "prepend")) {
            do_storage(&rh, prepend_command, args.size(), args.data());
        } else if(!strcmp(args[0], "cas")) {
            do_storage(&rh, cas_command, args.size(), args.data());
        } else if(!strcmp(args[0], "delete")) {
            do_delete(&rh, args.size(), args.data());
        } else if(!strcmp(args[0], "incr")) {
            do_incr_decr(&rh, true, args.size(), args.data());
        } else if(!strcmp(args[0], "decr")) {
            do_incr_decr(&rh, false, args.size(), args.data());
        } else if(!strcmp(args[0], "quit")) {
            // Make sure there's no more tokens
            if (args.size() > 1) {
                rh.writef("ERROR\r\n");
            } else {
                break;
            }
        } else if(!strcmp(args[0], "stats") || !strcmp(args[0], "stat")) {
            do_stats(&rh, args.size(), args.data());
        } else if(!strcmp(args[0], "rethinkdb")) {
            if (args.size() == 2 && !strcmp(args[1], "shutdown")) {
                // Shut down the server
                server->shutdown();
                break;
            } else {
                rh.writef("Available commands:\r\n");
                rh.writef("rethinkdb shutdown\r\n");
            }
        } else if(!strcmp(args[0], "version")) {
            if (args.size() == 2) {
                rh.writef("VERSION rethinkdb-%s\r\n", RETHINKDB_VERSION);
            } else {
                rh.writef("ERROR\r\n");
            }
#ifndef NDEBUG
        } else if (!parse_debug_command(&rh, args.size(), args.data())) {
#else
        } else {
#endif
            rh.writef("ERROR\r\n");
        }

        action_timer.end();
    }

    /* Acquire the prevent-shutdown lock to make sure that anything that might refer to us is
    done by now */
    rh.prevent_shutdown_lock.co_lock(rwi_write);

    // logINF("Closed connection %p\n", coro_t::self());
}


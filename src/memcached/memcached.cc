#include <stdexcept>
#include <stdarg.h>
#include "memcached/memcached.hpp"
#include "concurrency/task.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/cond_var.hpp"
#include "arch/arch.hpp"
#include "store.hpp"
#include "logger.hpp"

/* txt_memcached_handler_t is basically defunct; it only exists as a convenient thing to pass
around to do_get(), do_storage(), and the like. */

struct txt_memcached_handler_t {

    tcp_conn_t *conn;
    store_t *store;

    txt_memcached_handler_t(tcp_conn_t *conn, store_t *store) :
        conn(conn),
        store(store),
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
        char key_memory[MAX_KEY_SIZE+sizeof(store_key_t)];
        store_key_t key;
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
        if (!str_to_key(argv[i], &gets.back().key)) {
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
            with_cas ? &store_t::get_cas : &store_t::get,
            rh->store,
            &gets[i].key);
    }

    /* Handle the results in sequence */
    for (int i = 0; i < (int)gets.size(); i++) {

        store_t::get_result_t res = gets[i].result->join();

        /* If the write half of the connection has been closed, there's no point in trying
        to send anything */
        if (rh->conn->is_write_open()) {
            store_key_t &key = gets[i].key;

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

/* The memcached_data_provider_t is a data_provider_t that gets its data by
reading from a tcp_conn_t. It also reads the CRLF at the end of the value and fails if
it does not find the CRLF.

Its constructor takes a promise_t<bool> that it pulse()s when it's done reading, with the boolean
value indicating if it found the CRLF or not. This way the request handler knows if it should print
an error and when it can move on to the next command. */

class memcached_data_provider_t :
    public auto_buffering_data_provider_t,   // So we don't have to implement get_data_as_buffers()
    public home_thread_mixin_t
{
private:
    txt_memcached_handler_t *rh;   // Request handler for us to read from
    size_t length;   // Our length
    promise_t<bool> *to_signal;

public:
    memcached_data_provider_t(txt_memcached_handler_t *rh, uint32_t length, promise_t<bool> *to_signal)
        : rh(rh), length(length), to_signal(to_signal)
    { }

    size_t get_size() const {
        return length;
    }

    void get_data_into_buffers(const buffer_group_t *b) throw (data_provider_failed_exc_t) {
        on_thread_t thread_switcher(home_thread);
        try {
            for (int i = 0; i < (int)b->buffers.size(); i++) {
                rh->conn->read(b->buffers[i].data, b->buffers[i].size);
            }
            char crlf[2];
            rh->conn->read(crlf, 2);
            if (memcmp(crlf, "\r\n", 2) != 0) {
                if (to_signal) to_signal->pulse(false);
                throw data_provider_failed_exc_t();   // Cancel the set/append/prepend operation
            }
            if (to_signal) to_signal->pulse(true);
        } catch (tcp_conn_t::read_closed_exc_t) {
            if (to_signal) to_signal->pulse(false);
            throw data_provider_failed_exc_t();
        }
        // Crazy wizardry happens here: cross-thread exception throw. As stack is unwound,
        // thread_switcher's destructor causes us to switch threads.
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

void run_storage_command(txt_memcached_handler_t *rh, storage_command_t sc, store_key_and_buffer_t key, data_provider_t *unbuffered_data, mcflags_t mcflags, exptime_t exptime, cas_t unique, bool noreply) {

    rh->begin_write_command();

    /* unbuffered_data is guaranteed to remain valid as long as we need it because do_storage()
    waits on the promise_t that it gave to unbuffered_data, which won't be triggered until we
    consume unbuffered_data's value. */
    maybe_buffered_data_provider_t data(unbuffered_data, MAX_BUFFERED_SET_SIZE);

    if (sc != append_command && sc != prepend_command) {
        store_t::set_result_t res;
        switch (sc) {
            case set_command: res = rh->store->set(&key.key, &data, mcflags, exptime); break;
            case add_command: res = rh->store->add(&key.key, &data, mcflags, exptime); break;
            case replace_command: res = rh->store->replace(&key.key, &data, mcflags, exptime); break;
            case cas_command: res = rh->store->cas(&key.key, &data, mcflags, exptime, unique); break;
            case append_command:
            case prepend_command:
            default:
                unreachable();
        }
        if (!noreply) {
            switch (res) {
                case store_t::sr_stored: rh->writef("STORED\r\n"); break;
                case store_t::sr_not_stored: rh->writef("NOT_STORED\r\n"); break;
                case store_t::sr_exists: rh->writef("EXISTS\r\n"); break;
                case store_t::sr_not_found: rh->writef("NOT_FOUND\r\n"); break;
                case store_t::sr_too_large:
                    rh->writef("SERVER_ERROR object too large for cache\r\n");
                    break;
                case store_t::sr_data_provider_failed:
                    /* The error message will be written by do_storage() */
                    break;
                /* case store_t::sr_not_allowed:
                    rh->writef("CLIENT_ERROR writing not allowed on this store (probably a slave)\r\n");
                    break; */
                default: unreachable();
            }
        }
    } else {
        store_t::append_prepend_result_t res;
        switch (sc) {
            case append_command: res = rh->store->append(&key.key, &data); break;
            case prepend_command: res = rh->store->prepend(&key.key, &data); break;
            case set_command:
            case add_command:
            case replace_command:
            case cas_command:
            default:
                unreachable();
        }
        if (!noreply) {
            switch (res) {
                case store_t::apr_success: rh->writef("STORED\r\n"); break;
                case store_t::apr_not_found: rh->writef("NOT_FOUND\r\n"); break;
                case store_t::apr_too_large:
                    rh->writef("SERVER_ERROR object too large for cache\r\n");
                    break;
                case store_t::apr_data_provider_failed:
                    /* The error message will be written by do_storage() */
                    break;
                /* case apr_not_allowed:
                    rh->writef("CLIENT_ERROR writing not allowed on this store (probably a slave)\r\n");
                    break; */
                default: unreachable();
            }
        }
    }

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
    if (!str_to_key(argv[1], &key.key)) {
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
    promise_t<bool> value_read_promise;
    memcached_data_provider_t mcdp(rh, value_size, &value_read_promise);

    if (noreply) {
        coro_t::spawn(&run_storage_command, rh, sc, key, &mcdp, mcflags, exptime, unique, true);
    } else {
        run_storage_command(rh, sc, key, &mcdp, mcflags, exptime, unique, false);
    }

    /* We can't move on to the next command until the value has been read off the socket. This also
    ensures that 'mcdp' is not destroyed until run_storage_command() is done with it. */
    bool ok = value_read_promise.wait();
    if (!ok) {
        rh->writef("CLIENT_ERROR bad data chunk\r\n");
    }
}

/* "incr" and "decr" commands */
void run_incr_decr(txt_memcached_handler_t *rh, store_key_and_buffer_t key, unsigned long long amount, bool incr, bool noreply) {

    store_t::incr_decr_result_t res = incr ?
        rh->store->incr(&key.key, amount) :
        rh->store->decr(&key.key, amount);

    if (!noreply) {
        switch (res.res) {
            case store_t::incr_decr_result_t::idr_success:
                rh->writef("%llu\r\n", res.new_value);
                break;
            case store_t::incr_decr_result_t::idr_not_found:
                rh->writef("NOT_FOUND\r\n");
                break;
            case store_t::incr_decr_result_t::idr_not_numeric:
                rh->writef("CLIENT_ERROR cannot increment or decrement non-numeric value\r\n");
                break;
            /* case incr_decr_result_t::idr_not_allowed:
                rh->writef("CLIENT_ERROR writing not allowed on this store (probably a slave)\r\n");
                break; */
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
    if (!str_to_key(argv[1], &key.key)) {
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

void run_delete(txt_memcached_handler_t *rh, store_key_and_buffer_t key, bool noreply) {

    store_t::delete_result_t res = rh->store->delete_key(&key.key);

    if (!noreply) {
        switch (res) {
            case store_t::dr_deleted: rh->writef("DELETED\r\n"); break;
            case store_t::dr_not_found: rh->writef("NOT_FOUND\r\n"); break;
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
    if (!str_to_key(argv[1], &key.key)) {
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

    /* The corresponding call to end_write_command() is in do_delete() */
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
        const_charslice sl = conn->peek();
        void *crlf_loc = memmem(sl.beg, sl.end - sl.beg, "\r\n", 2);
        ssize_t threshold = MEGABYTE;

        if (crlf_loc) {
            // We have a valid line.
            size_t line_size = (char *)crlf_loc - (char *)sl.beg;

            dest->resize(line_size + 2);  // +2 for CRLF
            memcpy(dest->data(), sl.beg, line_size + 2);
            conn->pop(line_size + 2);
            return;
        } else if (sl.end - sl.beg > threshold) {
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

std::vector<store_key_and_buffer_t> key_array_to_keys_vector(int argc, char **argv) {
    std::vector<store_key_and_buffer_t> keys(argc);
    for (int i = 0; i < argc; i++) {
        if (!str_to_key(argv[i], &keys[i].key)) {
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
            std::vector<store_key_and_buffer_t> keys = key_array_to_keys_vector(argc-1, argv+1);
            for (std::vector<store_key_and_buffer_t>::iterator it = keys.begin(); it != keys.end(); ++it) {
                store_key_t& k = (*it).key;
                uint32_t hash = 0; // btree_key_value_store_t::hash(&k);
                uint32_t slice = -1; //rh->store->slice_nr(&k);  // TODO fix this.  FIX THIS.
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

void serve_memcache(tcp_conn_t *conn, store_t *store) {

    // logINF("Opened connection %p\n", coro_t::self());

    /* Object that we pass around to subroutines (is there a better way to do this?) */
    txt_memcached_handler_t rh(conn, store);

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
                thread_message_t *old_interrupt_msg = thread_pool_t::set_interrupt_message(NULL);
                /* If the interrupt message already was NULL, that means that either shutdown()
                was for some reason called before we finished starting up or shutdown() was called
                twice and this is the second time. */
                if (old_interrupt_msg) {
                    if (continue_on_thread(get_num_threads()-1, old_interrupt_msg))
                        call_later_on_this_thread(old_interrupt_msg);
                }
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


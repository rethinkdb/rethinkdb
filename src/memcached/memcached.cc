#include <stdexcept>
#include <stdarg.h>
#include "memcached/memcached.hpp"
#include "concurrency/task.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "containers/unique_ptr.hpp"
#include "arch/arch.hpp"
#include "store.hpp"
#include "logger.hpp"
#include "control.hpp"

/* txt_memcached_handler_t is basically defunct; it only exists as a convenient thing to pass
around to do_get(), do_storage(), and the like. */

static const char *crlf = "\r\n";

struct txt_memcached_handler_t : public home_thread_mixin_t {
    tcp_conn_t *conn;
    get_store_t *get_store;
    set_store_interface_t *set_store;

    txt_memcached_handler_t(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) :
        conn(conn),
        get_store(get_store),
        set_store(set_store),
        requests_out_sem(MAX_CONCURRENT_QUERIES_PER_CONNECTION)
    { }

    /* Wrappers around conn->write*() that ignore write errors, because we want to continue
    processing requests even if the client is no longer listening for replies */
    void write(const thread_saver_t& saver, const std::string& buffer) {
        write(saver, buffer.c_str(), buffer.length());
    }

    void write(const thread_saver_t& saver, const char *buffer, size_t bytes) {
        try {
            // TODO: Stop needing this ensure_thread thing, so we
            // don't have to pass a thread_saver_t to everything.
            ensure_thread(saver);
            conn->write_buffered(buffer, bytes);
        } catch (tcp_conn_t::write_closed_exc_t) {
        }
    }
    void vwritef(const thread_saver_t& saver, const char *format, va_list args) {
        char buffer[1000];
        size_t bytes = vsnprintf(buffer, sizeof(buffer), format, args);
        rassert(bytes < sizeof(buffer));
        write(saver, buffer, bytes);
    }
    void writef(const thread_saver_t& saver, const char *format, ...)
        __attribute__ ((format (printf, 3, 4))) {
        va_list args;
        va_start(args, format);
        vwritef(saver, format, args);
        va_end(args);
    }
    void write_unbuffered(const char *buffer, size_t bytes) {
        try {
            conn->write(buffer, bytes);
        } catch (tcp_conn_t::write_closed_exc_t) {
        }
    }
    void write_from_data_provider(const thread_saver_t& saver, data_provider_t *dp) {
        /* Write the value itself. If the value is small, write it into the send buffer;
        otherwise, stream it. */
        const const_buffer_group_t *bg;
        {
            thread_saver_t thread_saver;
            bg = dp->get_data_as_buffers();
        }
        for (size_t i = 0; i < bg->num_buffers(); i++) {
            const_buffer_group_t::buffer_t b = bg->get_buffer(i);
            if (dp->get_size() < MAX_BUFFERED_GET_SIZE) {
                write(saver, ptr_cast<const char>(b.data), b.size);
            } else {
                write_unbuffered(ptr_cast<const char>(b.data), b.size);
            }
        }
    }
    void write_value_header(const thread_saver_t& saver, const char *key, size_t key_size, mcflags_t mcflags, size_t value_size) {
        writef(saver, "VALUE %*.*s %u %zu\r\n",
               int(key_size), int(key_size), key, mcflags, value_size);
    }
    void write_value_header(const thread_saver_t& saver, const char *key, size_t key_size, mcflags_t mcflags, size_t value_size, cas_t cas) {
        writef(saver, "VALUE %*.*s %u %zu %llu\r\n",
               int(key_size), int(key_size), key, mcflags, value_size, (long long unsigned int)cas);
    }

    void error(const thread_saver_t& saver) {
        writef(saver, "ERROR\r\n");
    }
    void write_crlf(const thread_saver_t& saver) {
        write(saver, crlf, 2);
    }
    void write_end(const thread_saver_t& saver) {
        writef(saver, "END\r\n");
    }
    void client_error(const thread_saver_t& saver, const char *format, ...)
        __attribute__ ((format (printf, 3, 4))) {
        writef(saver, "CLIENT_ERROR ");
        va_list args;
        va_start(args, format);
        vwritef(saver, format, args);
        va_end(args);
    }
    void server_error(const thread_saver_t& saver, const char *format, ...)
        __attribute__ ((format (printf, 3, 4))) {
        writef(saver, "SERVER_ERROR ");
        va_list args;
        va_start(args, format);
        vwritef(saver, format, args);
        va_end(args);
    }

    void client_error_bad_command_line_format(const thread_saver_t& saver) {
        client_error(saver, "bad command line format\r\n");
    }

    void client_error_bad_data(const thread_saver_t& saver) {
        client_error(saver, "bad data chunk\r\n");
    }

    void client_error_writing_not_allowed(const thread_saver_t& saver) {
        client_error(saver, "writing not allowed on this store (probably a slave)\r\n");
    }

    void server_error_object_too_large_for_cache(const thread_saver_t& saver) {
        server_error(saver, "object too large for cache\r\n");
    }

    // Used to limit number of concurrent noreply requests
    semaphore_t requests_out_sem;

    /* If a client sends a bunch of noreply requests and then a 'quit', we cannot delete ourself
    immediately because the noreply requests still hold the 'requests_out' semaphore. We use this
    semaphore to figure out when we can delete ourself. */
    drain_semaphore_t drain_semaphore;

    // Used to implement throttling. Write requests should call begin_write_command() and
    // end_write_command() to make sure that not too many write requests are sent
    // concurrently.
    void begin_write_command() {
        drain_semaphore.acquire();
        requests_out_sem.co_lock();
    }
    void end_write_command() {
        drain_semaphore.release();
        requests_out_sem.unlock();
    }
};

perfmon_duration_sampler_t
    pm_cmd_set("cmd_set", secs_to_ticks(1.0)),
    pm_cmd_get("cmd_get", secs_to_ticks(1.0)),
    pm_cmd_rget("cmd_rget", secs_to_ticks(1.0));

/* do_get() is used for "get" and "gets" commands. */

struct get_t {
    store_key_t key;
    get_result_t res;
};

void do_one_get(txt_memcached_handler_t *rh, bool with_cas, get_t *gets, int i) {
    if (with_cas) {
        gets[i].res = rh->set_store->get_cas(gets[i].key);
    } else {
        gets[i].res = rh->get_store->get(gets[i].key);
    }
}

void do_get(const thread_saver_t& saver, txt_memcached_handler_t *rh, bool with_cas, int argc, char **argv) {
    rassert(argc >= 1);
    rassert(strcmp(argv[0], "get") == 0 || strcmp(argv[0], "gets") == 0);

    /* Vector to store the keys and the task-objects */
    std::vector<get_t> gets;

    /* First parse all of the keys to get */
    gets.reserve(argc - 1);
    for (int i = 1; i < argc; i++) {
        gets.push_back(get_t());
        if (!str_to_key(argv[i], &gets.back().key)) {
            rh->client_error_bad_command_line_format(saver);
            return;
        }
    }
    if (gets.size() == 0) {
        rh->error(saver);
        return;
    }

    block_pm_duration get_timer(&pm_cmd_get);

    /* Now that we're sure they're all valid, send off the requests */
    pmap(gets.size(), boost::bind(&do_one_get, rh, with_cas, gets.data(), _1));

    /* Handle the results in sequence */
    for (int i = 0; i < (int)gets.size(); i++) {
        get_result_t &res = gets[i].res;

        /* If res.value is NULL that means the value was not found so we don't write
        anything */
        if (res.value) {
            /* If the write half of the connection has been closed, there's no point in trying
            to send anything */
            if (rh->conn->is_write_open()) {
                const store_key_t &key = gets[i].key;

                /* Write the "VALUE ..." header */
                if (with_cas) {
                    rh->write_value_header(saver, key.contents, key.size, res.flags, res.value->get_size(), res.cas);
                } else {
                    rassert(res.cas == 0);
                    rh->write_value_header(saver, key.contents, key.size, res.flags, res.value->get_size());
                }

                rh->write_from_data_provider(saver, res.value.get());
                rh->write_crlf(saver);
            }
        }
        if (res.to_signal_when_done) {
            res.to_signal_when_done->pulse();
        }
    }

    rh->write_end(saver);
};

static const char *rget_null_key = "null";

static bool rget_parse_bound(char *flag, char *key, rget_bound_mode_t *mode_out, store_key_t *key_out) {
    if (!str_to_key(key, key_out)) return false;

    char *invalid_char;
    long open_flag = strtol_strict(flag, &invalid_char, 10);
    if (*invalid_char != '\0') return false;

    switch (open_flag) {
    case 0:
        *mode_out = rget_bound_closed;
        return true;
    case 1:
        *mode_out = rget_bound_open;
        return true;
    case -1:
        if (strcasecmp(rget_null_key, key) == 0) {
            *mode_out = rget_bound_none;
            key_out->size = 0;   // Key is irrelevant
            return true;
        }
        // Fall through
    default:
        return false;
    }
}

void do_rget(const thread_saver_t& saver, txt_memcached_handler_t *rh, int argc, char **argv) {
    if (argc != 6) {
        rh->client_error_bad_command_line_format(saver);
        return;
    }

    /* Parse left/right boundary keys and flags */
    rget_bound_mode_t left_mode, right_mode;
    store_key_t left_key, right_key;

    if (!rget_parse_bound(argv[3], argv[1], &left_mode, &left_key) ||
        !rget_parse_bound(argv[4], argv[2], &right_mode, &right_key)) {\
        rh->client_error_bad_command_line_format(saver);
        return;
    }

    /* Parse max items count */
    char *invalid_char;
    uint64_t max_items = strtoull_strict(argv[5], &invalid_char, 10);
    if (*invalid_char != '\0') {
        rh->client_error_bad_command_line_format(saver);
        return;
    }

    block_pm_duration rget_timer(&pm_cmd_rget);

    rget_result_t results_iterator = rh->get_store->rget(left_mode, left_key, right_mode, right_key);

    boost::optional<key_with_data_provider_t> pair;
    uint64_t count = 0;
    while (++count <= max_items && (pair = results_iterator->next())) {
        const key_with_data_provider_t& kv = pair.get();

        const std::string& key = kv.key;
        const boost::shared_ptr<data_provider_t>& dp = kv.value_provider;

        rh->write_value_header(saver, key.c_str(), key.length(), kv.mcflags, dp->get_size());
        rh->write_from_data_provider(saver, dp.get());
        rh->write_crlf(saver);
    }
    rh->write_end(saver);
}

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
    bool was_read;
    promise_t<bool> *to_signal;

public:
    memcached_data_provider_t(txt_memcached_handler_t *rh, uint32_t length, promise_t<bool> *to_signal)
        : rh(rh), length(length), was_read(false), to_signal(to_signal)
    { }

    ~memcached_data_provider_t() {
        if (!was_read) {
            /* We have to clear the data out of the socket, even if we have nowhere to put it. */
            get_data_as_buffers();
        }
    }

    size_t get_size() const {
        return length;
    }

    void get_data_into_buffers(const buffer_group_t *b) throw (data_provider_failed_exc_t) {
        rassert(!was_read);
        was_read = true;
        on_thread_t thread_switcher(home_thread);
        try {
            for (int i = 0; i < (int)b->num_buffers(); i++) {
                rh->conn->read(b->get_buffer(i).data, b->get_buffer(i).size);
            }
            char expected_crlf[2];
            rh->conn->read(expected_crlf, 2);
            if (memcmp(expected_crlf, crlf, 2) != 0) {
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

struct storage_metadata_t {
    const mcflags_t mcflags;
    const exptime_t exptime;
    const cas_t unique;
    storage_metadata_t(mcflags_t _mcflags, exptime_t _exptime, cas_t _unique)
        : mcflags(_mcflags), exptime(_exptime), unique(_unique) { }
};

void run_storage_command(txt_memcached_handler_t *rh,
                         storage_command_t sc,
                         store_key_t key,
                         size_t value_size, promise_t<bool> *value_read_promise,
                         storage_metadata_t metadata,
                         bool noreply) {

    thread_saver_t saver;

    memcached_data_provider_t unbuffered_data(rh, value_size, value_read_promise);
    maybe_buffered_data_provider_t data(&unbuffered_data, MAX_BUFFERED_SET_SIZE);

    block_pm_duration set_timer(&pm_cmd_set);

    if (sc != append_command && sc != prepend_command) {
        add_policy_t add_policy;
        replace_policy_t replace_policy;

        switch (sc) {
        case set_command:
            add_policy = add_policy_yes;
            replace_policy = replace_policy_yes;
            break;
        case add_command:
            add_policy = add_policy_yes;
            replace_policy = replace_policy_no;
            break;
        case replace_command:
            add_policy = add_policy_no;
            replace_policy = replace_policy_yes;
            break;
        case cas_command:
            add_policy = add_policy_no;
            replace_policy = replace_policy_if_cas_matches;
            break;
        case append_command:
        case prepend_command:
        default:
            unreachable();
        }

        set_result_t res = rh->set_store->sarc(key, &data, metadata.mcflags, metadata.exptime,
                                               add_policy, replace_policy, metadata.unique);
        
        if (!noreply) {
            switch (res) {
            case sr_stored:
                rh->writef(saver, "STORED\r\n");
                break;
            case sr_didnt_add:
                if (sc == replace_command) rh->writef(saver, "NOT_STORED\r\n");
                else if (sc == cas_command) rh->writef(saver, "NOT_FOUND\r\n");
                else unreachable();
                break;
            case sr_didnt_replace:
                if (sc == add_command) rh->writef(saver, "NOT_STORED\r\n");
                else if (sc == cas_command) rh->writef(saver, "EXISTS\r\n");
                else unreachable();
                break;
            case sr_too_large:
                rh->server_error_object_too_large_for_cache(saver);
                break;
            case sr_data_provider_failed:
                /* The error message will be written by do_storage() */
                break;
            case sr_not_allowed:
                rh->client_error_writing_not_allowed(saver);
                break;
            default: unreachable();
            }
        }

    } else {
        append_prepend_result_t res =
            rh->set_store->append_prepend(
                sc == append_command ? append_prepend_APPEND : append_prepend_PREPEND,
                key, &data);

        if (!noreply) {
            switch (res) {
            case apr_success: rh->writef(saver, "STORED\r\n"); break;
            case apr_not_found: rh->writef(saver, "NOT_FOUND\r\n"); break;
            case apr_too_large:
                rh->server_error_object_too_large_for_cache(saver);
                break;
            case apr_data_provider_failed:
                /* The error message will be written by do_storage() */
                break;
            case apr_not_allowed:
                rh->client_error_writing_not_allowed(saver);
                break;
            default: unreachable();
            }
        }
    }

    rh->end_write_command();

    /* If the key-value store never read our value for whatever reason, then the
    memcached_data_provider_t's destructor will read it off the socket and signal
    read_value_promise here */
}

void do_storage(const thread_saver_t& saver, txt_memcached_handler_t *rh, storage_command_t sc, int argc, char **argv) {
    char *invalid_char;

    /* cmd key flags exptime size [noreply]
    OR "cas" key flags exptime size cas [noreply] */
    if ((sc != cas_command && (argc != 5 && argc != 6)) ||
        (sc == cas_command && (argc != 6 && argc != 7))) {
        rh->error(saver);
        return;
    }

    /* First parse the key */
    store_key_t key;
    if (!str_to_key(argv[1], &key)) {
        rh->client_error_bad_command_line_format(saver);
        return;
    }

    /* Next parse the flags */
    mcflags_t mcflags = strtoul_strict(argv[2], &invalid_char, 10);
    if (*invalid_char != '\0') {
        rh->client_error_bad_command_line_format(saver);
        return;
    }

    /* Now parse the expiration time */
    exptime_t exptime = strtoul_strict(argv[3], &invalid_char, 10);
    if (*invalid_char != '\0') {
        rh->client_error_bad_command_line_format(saver);
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
        rh->client_error_bad_command_line_format(saver);
        return;
    }

    /* If a "cas", parse the cas_command unique */
    cas_t unique = NO_CAS_SUPPLIED;
    if (sc == cas_command) {
        unique = strtoull_strict(argv[5], &invalid_char, 10);
        if (*invalid_char != '\0') {
            rh->client_error_bad_command_line_format(saver);
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

    /* run_storage_command() will signal this when it's done reading the value off the socket. */
    promise_t<bool> value_read_promise;
    storage_metadata_t metadata(mcflags, exptime, unique);

    /* We must do this out here instead of in run_storage_command() so that we stop reading off
    the socket if it blocks */
    rh->begin_write_command();

    if (noreply) {
        coro_t::spawn_now(boost::bind(&run_storage_command, rh, sc, key, value_size, &value_read_promise, metadata, true));
    } else {
        run_storage_command(rh, sc, key, value_size, &value_read_promise, metadata, false);
    }

    /* We can't move on to the next command until the value has been read off the socket. */
    bool ok = value_read_promise.wait();
    if (!ok) {
        /* We get here if there was no CRLF */
        rh->client_error(saver, "bad data chunk\r\n");
    }
}

/* "incr" and "decr" commands */
void run_incr_decr(txt_memcached_handler_t *rh, store_key_t key, uint64_t amount, bool incr, bool noreply) {
    thread_saver_t saver;

    block_pm_duration set_timer(&pm_cmd_set);

    incr_decr_result_t res = rh->set_store->incr_decr(
        incr ? incr_decr_INCR : incr_decr_DECR,
        key, amount);

    if (!noreply) {
        switch (res.res) {
            case incr_decr_result_t::idr_success:
                rh->writef(saver, "%llu\r\n", (unsigned long long)res.new_value);
                break;
            case incr_decr_result_t::idr_not_found:
                rh->writef(saver, "NOT_FOUND\r\n");
                break;
            case incr_decr_result_t::idr_not_numeric:
                rh->client_error(saver, "cannot increment or decrement non-numeric value\r\n");
                break;
            case incr_decr_result_t::idr_not_allowed:
                rh->client_error_writing_not_allowed(saver);
                break;
            default: unreachable();
        }
    }

    rh->end_write_command();
}

void do_incr_decr(const thread_saver_t& saver, txt_memcached_handler_t *rh, bool i, int argc, char **argv) {
    /* cmd key delta [noreply] */
    if (argc != 3 && argc != 4) {
        rh->error(saver);
        return;
    }

    /* Parse key */
    store_key_t key;
    if (!str_to_key(argv[1], &key)) {
        rh->client_error_bad_command_line_format(saver);
        return;
    }

    /* Parse amount to change by */
    char *invalid_char;
    uint64_t delta = strtoull_strict(argv[2], &invalid_char, 10);
    if (*invalid_char != '\0') {
        rh->client_error_bad_command_line_format(saver);
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
        coro_t::spawn_now(boost::bind(&run_incr_decr, rh, key, delta, i, true));
    } else {
        run_incr_decr(rh, key, delta, i, false);
    }
}

/* "delete" commands */

void run_delete(txt_memcached_handler_t *rh, store_key_t key, bool noreply) {
    thread_saver_t saver;

    block_pm_duration set_timer(&pm_cmd_set);

    delete_result_t res = rh->set_store->delete_key(key);

    if (!noreply) {
        switch (res) {
            case dr_deleted:
                rh->writef(saver, "DELETED\r\n");
                break;
            case dr_not_found:
                rh->writef(saver, "NOT_FOUND\r\n");
                break;
            case dr_not_allowed:
                rh->client_error_writing_not_allowed(saver);
                break;
            default: unreachable();
        }
    }

    rh->end_write_command();
}

void do_delete(const thread_saver_t& saver, txt_memcached_handler_t *rh, int argc, char **argv) {
    /* "delete" key [a number] ["noreply"] */
    if (argc < 2 || argc > 4) {
        rh->error(saver);
        return;
    }

    /* Parse key */
    store_key_t key;
    if (!str_to_key(argv[1], &key)) {
        rh->client_error_bad_command_line_format(saver);
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
            if (!noreply)
                rh->client_error_bad_command_line_format(saver);
            return;
        }
    } else {
        noreply = false;
    }

    rh->begin_write_command();

    if (noreply) {
        coro_t::spawn_now(boost::bind(&run_delete, rh, key, true));
    } else {
        run_delete(rh, key, false);
    }
};

/* "stats"/"stat" commands */

void do_stats(const thread_saver_t& saver, txt_memcached_handler_t *rh, int argc, char **argv) {
    perfmon_stats_t stats;
    perfmon_get_stats(&stats);

    if (argc == 1) {
        for (perfmon_stats_t::iterator iter = stats.begin(); iter != stats.end(); iter++) {
            rh->writef(saver, "STAT %s %s\r\n", iter->first.c_str(), iter->second.c_str());
        }
    } else {
        for (int i = 1; i < argc; i++) {
            perfmon_stats_t::iterator stat_entry = stats.find(argv[i]);
            if (stat_entry == stats.end()) {
                rh->writef(saver, "NOT FOUND\r\n");
            } else {
                rh->writef(saver, "%s\r\n", stat_entry->second.c_str());
            }
        }
    }
    rh->write_end(saver);
};

perfmon_duration_sampler_t
    pm_conns_reading("conns_reading", secs_to_ticks(1)),
    pm_conns_writing("conns_writing", secs_to_ticks(1)),
    pm_conns_acting("conns_acting", secs_to_ticks(1));

void read_line(tcp_conn_t *conn, std::vector<char> *dest) {
    for (;;) {
        const_charslice sl = conn->peek();
        void *crlf_loc = memmem(sl.beg, sl.end - sl.beg, crlf, 2);
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
            logERR("Aborting connection %p because we got more than %ld bytes without a CRLF\n",
                   coro_t::self(), threshold);
            conn->shutdown_read();
            throw tcp_conn_t::read_closed_exc_t();
        }

        // Keep trying until we get a complete line.
        conn->read_more_buffered();
    }

};

std::string join_strings(std::string separator, std::vector<char*>::iterator begin, std::vector<char*>::iterator end) {
    std::string res(*begin);
    for (std::vector<char *>::iterator it = begin + 1; it != end; it++)
        res += separator + std::string(*it); // sigh
    return res;
}

#ifndef NDEBUG
void do_quickset(const thread_saver_t& saver, txt_memcached_handler_t *rh, std::vector<char*> args) {
    if (args.size() < 2 || args.size() % 2 == 0) {
        // The connection will be closed if more than a megabyte or so is sent
        // over without a newline, so we don't really need to worry about large
        // values.
        rh->write(saver, "CLIENT_ERROR Usage: .s k1 v1 [k2 v2...] (no whitespace in values)\r\n");
        return;
    }

    for (size_t i = 1; i < args.size(); i += 2) {
        store_key_t key;
        if (!str_to_key(args[i], &key)) {
            rh->writef(saver, "CLIENT_ERROR Invalid key %s\r\n", args[i]);
            return;
        }
        buffered_data_provider_t value(args[i + 1], strlen(args[i + 1]));

        set_result_t res = rh->set_store->sarc(key, &value, 0, 0, add_policy_yes, replace_policy_yes, 0);

        if (res == sr_stored) {
            rh->writef(saver, "STORED key %s\r\n", args[i]);
        } else {
            rh->writef(saver, "MYSTERIOUS_ERROR key %s\r\n", args[i]);
        }
    }
}

bool parse_debug_command(const thread_saver_t& saver, txt_memcached_handler_t *rh, std::vector<char*> args) {
    if (args.size() < 1) {
        return false;
    }

    // TODO: A nicer way to do short aliases like this.
    if (!strcmp(args[0], ".h") && args.size() >= 2) {       // .h is an alias for "rdb hash"
        std::vector<char *> ctrl_args = args;               // It's ugly, but typing that command out in full is just stupid
        ctrl_args[0] = (char *) "hash"; // This should be const but it's just for debugging and it won't be modified anyway.
        rh->write(saver, control_t::exec(ctrl_args.size(), ctrl_args.data()));
        return true;
    } else if (!strcmp(args[0], ".s")) { // There should be a better way of doing this, but it doesn't really matter.
        do_quickset(saver, rh, args);
        return true;
    } else {
        return false;
    }
}
#endif  // NDEBUG

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store) {
    logDBG("Opened connection %p\n", coro_t::self());

    /* Object that we pass around to subroutines (is there a better way to do this?) */
    txt_memcached_handler_t rh(conn, get_store, set_store);

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
            thread_saver_t saver;
            rh.error(saver);
            continue;
        }

        /* Dispatch to the appropriate subclass */
        thread_saver_t saver;
        if (!strcmp(args[0], "get")) {    // check for retrieval commands
            do_get(saver, &rh, false, args.size(), args.data());
        } else if (!strcmp(args[0], "gets")) {
            do_get(saver, &rh, true, args.size(), args.data());
        } else if (!strcmp(args[0], "rget")) {
            do_rget(saver, &rh, args.size(), args.data());
        } else if (!strcmp(args[0], "set")) {     // check for storage commands
            do_storage(saver, &rh, set_command, args.size(), args.data());
        } else if (!strcmp(args[0], "add")) {
            do_storage(saver, &rh, add_command, args.size(), args.data());
        } else if (!strcmp(args[0], "replace")) {
            do_storage(saver, &rh, replace_command, args.size(), args.data());
        } else if (!strcmp(args[0], "append")) {
            do_storage(saver, &rh, append_command, args.size(), args.data());
        } else if (!strcmp(args[0], "prepend")) {
            do_storage(saver, &rh, prepend_command, args.size(), args.data());
        } else if (!strcmp(args[0], "cas")) {
            do_storage(saver, &rh, cas_command, args.size(), args.data());
        } else if (!strcmp(args[0], "delete")) {
            do_delete(saver, &rh, args.size(), args.data());
        } else if (!strcmp(args[0], "incr")) {
            do_incr_decr(saver, &rh, true, args.size(), args.data());
        } else if (!strcmp(args[0], "decr")) {
            do_incr_decr(saver, &rh, false, args.size(), args.data());
        } else if (!strcmp(args[0], "quit")) {
            // Make sure there's no more tokens
            if (args.size() > 1) {
                rh.error(saver);
            } else {
                break;
            }
        } else if (!strcmp(args[0], "stats") || !strcmp(args[0], "stat")) {
            do_stats(saver, &rh, args.size(), args.data());
        } else if(!strcmp(args[0], "rethinkdb") || !strcmp(args[0], "rdb")) {
            rh.write(saver, control_t::exec(args.size() - 1, args.data() + 1));
        } else if (!strcmp(args[0], "version")) {
            if (args.size() == 2) {
                rh.writef(saver, "VERSION rethinkdb-%s\r\n", RETHINKDB_VERSION);
            } else {
                rh.error(saver);
            }
#ifndef NDEBUG
        } else if (!parse_debug_command(saver, &rh, args)) {
#else
        } else {
#endif
            rh.error(saver);
        }

        action_timer.end();
    }

    /* Acquire the drain semaphore to make sure that anything that might refer to us is
    done by now */
    rh.drain_semaphore.drain();

    logDBG("Closed connection %p\n", coro_t::self());
}


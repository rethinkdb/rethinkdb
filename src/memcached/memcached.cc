#include "memcached/memcached.hpp"

#include <stdexcept>
#include <stdarg.h>
#include <unistd.h>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

#include "concurrency/coro_fifo.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/promise.hpp"
#include "containers/buffer_group.hpp"
#include "containers/iterators.hpp"
#include "stats/control.hpp"
#include "memcached/store.hpp"
#include "logger.hpp"
#include "arch/os_signal.hpp"
#include "perfmon.hpp"
#include "stats/persist.hpp"

/* txt_memcached_handler_t only exists as a convenient thing to pass around to do_get(),
do_storage(), and the like. */

static const char *crlf = "\r\n";

struct txt_memcached_handler_t : public home_thread_mixin_t {

    txt_memcached_handler_t(memcached_interface_t *_interface, get_store_t *_get_store,
            set_store_interface_t *_set_store, int _max_concurrent_queries_per_connection)
        : interface(_interface), get_store(_get_store), set_store(_set_store), max_concurrent_queries_per_connection(_max_concurrent_queries_per_connection)
    { }

    memcached_interface_t *interface;

    get_store_t *get_store;
    set_store_interface_t *set_store;

    const int max_concurrent_queries_per_connection;

    void write(const std::string& buffer) {
        write(buffer.data(), buffer.length());
    }

    void write(const char *buffer, size_t bytes) {
        interface->write(buffer, bytes);
    }

    void vwritef(const char *format, va_list args) {
        char buffer[1000];
        size_t bytes = vsnprintf(buffer, sizeof(buffer), format, args);
        rassert(bytes < sizeof(buffer));
        write(buffer, bytes);
    }

    void writef(const char *format, ...)
        __attribute__ ((format (printf, 2, 3))) {
        va_list args;
        va_start(args, format);
        vwritef(format, args);
        va_end(args);
    }

    void write_unbuffered(const char *buffer, size_t bytes) {
        interface->write_unbuffered(buffer, bytes);
    }

    void write_from_data_provider(data_buffer_t *dp) {
        if (dp->size() < MAX_BUFFERED_GET_SIZE) {
            write(dp->buf(), dp->size());
        } else {
            write_unbuffered(dp->buf(), dp->size());
        }
    }

    void write_value_header(const char *key, size_t key_size, mcflags_t mcflags, size_t value_size) {
        writef("VALUE %*.*s %u %zu\r\n",
               int(key_size), int(key_size), key, mcflags, value_size);
    }

    void write_value_header(const char *key, size_t key_size, mcflags_t mcflags, size_t value_size, cas_t cas) {
        writef("VALUE %*.*s %u %zu %llu\r\n",
               int(key_size), int(key_size), key, mcflags, value_size, (long long unsigned int)cas);
    }

    void error() {
        writef("ERROR\r\n");
    }

    void write_crlf() {
        write(crlf, 2);
    }

    void write_end() {
        writef("END\r\n");
    }

    void client_error(const char *format, ...)
        __attribute__ ((format (printf, 2, 3))) {
        writef("CLIENT_ERROR ");
        va_list args;
        va_start(args, format);
        vwritef(format, args);
        va_end(args);
    }

    void server_error(const char *format, ...)
        __attribute__ ((format (printf, 2, 3))) {
        writef("SERVER_ERROR ");
        va_list args;
        va_start(args, format);
        vwritef(format, args);
        va_end(args);
    }

    void client_error_bad_command_line_format() {
        client_error("bad command line format\r\n");
    }

    void client_error_bad_data() {
        client_error("bad data chunk\r\n");
    }

    void client_error_not_allowed(bool op_is_write) {
        // TODO: Do we have a way of figuring out what's going on more specifically?
        // Like: Are we running in a replication setup? Are we actually shutting down?
        std::string explanations;
        if (op_is_write) {
            explanations = "Maybe you are trying to write to a slave? We might also be shutting down, or master and slave are out of sync.";
        } else {
            explanations = "We might be shutting down, or master and slave are out of sync.";
        }

        client_error("operation not allowed; %s\r\n", explanations.c_str());
    }

    void server_error_object_too_large_for_cache() {
        server_error("object too large for cache\r\n");
    }

    void flush_buffer() {
        interface->flush_buffer();
    }

    bool is_write_open() {
        return interface->is_write_open();
    }

    void read(void *buf, size_t nbytes) {
        interface->read(buf, nbytes);
    }

    void read_line(std::vector<char> *dest) {
        interface->read_line(dest);
    }
};


perfmon_duration_sampler_t
    pm_conns_reading("conns_reading", secs_to_ticks(1), false),
    pm_conns_writing("conns_writing", secs_to_ticks(1), false),
    pm_conns_acting("conns_acting", secs_to_ticks(1), false);


class pipeliner_t {
public:
    pipeliner_t(txt_memcached_handler_t *rh) : requests_out_sem(rh->max_concurrent_queries_per_connection), rh_(rh) { }
    ~pipeliner_t() { }
private:
    friend class pipeliner_acq_t;
    coro_fifo_t fifo;

    // Used to limit number of concurrent requests
    semaphore_t requests_out_sem;

    mutex_t mutex;
    txt_memcached_handler_t *rh_;

    DISABLE_COPYING(pipeliner_t);
};

class pipeliner_acq_t {
public:
    pipeliner_acq_t(pipeliner_t *pipeliner)
        : pipeliner_(pipeliner)
#ifndef NDEBUG
        , state_(untouched)
#endif
    { }
    ~pipeliner_acq_t() {
        rassert(state_ == has_ended_write);
    }

    void begin_operation() {
        rassert(state_ == untouched);
        DEBUG_ONLY(state_ = has_begun_operation);
        fifo_acq_.enter(&pipeliner_->fifo);
    }

    void done_argparsing() {
        rassert(state_ == has_begun_operation);
        DEBUG_ONLY(state_ = has_done_argparsing);
        pipeliner_->requests_out_sem.co_lock();
    }

    void begin_write() {
        rassert(state_ == has_done_argparsing);
        DEBUG_ONLY(state_ = has_begun_write);
        fifo_acq_.leave();
        mutex_acq_.reset(&pipeliner_->mutex);
    }

    void end_write() {
        rassert(state_ == has_begun_write);
        DEBUG_ONLY(state_ = has_ended_write);

        block_pm_duration flush_timer(&pm_conns_writing);
        pipeliner_->rh_->flush_buffer();
        flush_timer.end();

        mutex_acq_.reset();
        pipeliner_->requests_out_sem.unlock();
    }

private:
    pipeliner_t *pipeliner_;
    mutex_t::acq_t mutex_acq_;
    coro_fifo_acq_t fifo_acq_;
#ifndef NDEBUG
    enum { untouched, has_begun_operation, has_done_argparsing, has_begun_write, has_ended_write } state_;
#endif
    DISABLE_COPYING(pipeliner_acq_t);
};


perfmon_duration_sampler_t
    pm_cmd_set("cmd_set", secs_to_ticks(1.0), false),
    pm_cmd_get("cmd_get", secs_to_ticks(1.0), false),
    pm_cmd_rget("cmd_rget", secs_to_ticks(1.0), false);

perfmon_persistent_stddev_t
    pm_get_key_size("cmd_get_key_size"),
    pm_storage_key_size("cmd_set_key_size"),
    pm_storage_value_size("cmd_set_val_size"),
    pm_delete_key_size("cmd_delete_key_size");

/* do_get() is used for "get" and "gets" commands. */

struct get_t {
    store_key_t key;
    get_result_t res;
};

void do_one_get(txt_memcached_handler_t *rh, bool with_cas, get_t *gets, int i, order_token_t token) {
    if (with_cas) {
        gets[i].res = rh->set_store->get_cas(gets[i].key, token);
    } else {
        gets[i].res = rh->get_store->get(gets[i].key, token);
    }
}

void do_get(txt_memcached_handler_t *rh, pipeliner_t *pipeliner, bool with_cas, int argc, char **argv, order_token_t token) {
    // We should already be spawned within a coroutine.
    pipeliner_acq_t pipeliner_acq(pipeliner);
    pipeliner_acq.begin_operation();

    rassert(argc >= 1);
    rassert(strcmp(argv[0], "get") == 0 || strcmp(argv[0], "gets") == 0);

    /* Vector to store the keys and the task-objects */
    std::vector<get_t> gets;

    /* First parse all of the keys to get */
    gets.reserve(argc - 1);
    for (int i = 1; i < argc; i++) {
        gets.push_back(get_t());
        if (!str_to_key(argv[i], &gets.back().key)) {
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh->client_error_bad_command_line_format();
            pipeliner_acq.end_write();
            return;
        }
        pm_get_key_size.record((float) gets.back().key.size);
    }
    if (gets.size() == 0) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->error();
        pipeliner_acq.end_write();
        return;
    }

    pipeliner_acq.done_argparsing();

    block_pm_duration get_timer(&pm_cmd_get);

    /* Now that we're sure they're all valid, send off the requests */
    pmap(gets.size(), boost::bind(&do_one_get, rh, with_cas, gets.data(), _1, token));

    pipeliner_acq.begin_write();

    /* Check if they hit a gated_get_store_t. */
    if (gets.size() > 0 && gets[0].res.is_not_allowed) {
        /* They all should have gotten the same error */
        for (int i = 0; i < (int)gets.size(); i++) {
            rassert(gets[i].res.is_not_allowed);
        }

        rh->client_error_not_allowed(with_cas);

    } else {

        /* Handle the results in sequence */
        for (int i = 0; i < (int)gets.size(); i++) {
            get_result_t &res = gets[i].res;

            rassert(!res.is_not_allowed);

            /* If res.value is NULL that means the value was not found so we don't write
               anything */
            if (res.value) {
                /* If the write half of the connection has been closed, there's no point in trying
                   to send anything */
                if (rh->is_write_open()) {
                    const store_key_t &key = gets[i].key;

                    /* Write the "VALUE ..." header */
                    if (with_cas) {
                        rh->write_value_header(key.contents, key.size, res.flags, res.value->size(), res.cas);
                    } else {
                        rassert(res.cas == 0);
                        rh->write_value_header(key.contents, key.size, res.flags, res.value->size());
                    }

                    rh->write_from_data_provider(res.value.get());
                    rh->write_crlf();
                }
            }
        }

        rh->write_end();
    }

    pipeliner_acq.end_write();
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

perfmon_duration_sampler_t rget_iteration_next("rget_iteration_next", secs_to_ticks(1));

void do_rget(txt_memcached_handler_t *rh, pipeliner_t *pipeliner, int argc, char **argv, order_token_t token) {
    // We should already be spawned within a coroutine.
    pipeliner_acq_t pipeliner_acq(pipeliner);
    pipeliner_acq.begin_operation();

    if (argc != 6) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }

    /* Parse left/right boundary keys and flags */
    rget_bound_mode_t left_mode, right_mode;
    store_key_t left_key, right_key;

    if (!rget_parse_bound(argv[3], argv[1], &left_mode, &left_key) ||
        !rget_parse_bound(argv[4], argv[2], &right_mode, &right_key)) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }

    /* Parse max items count */
    char *invalid_char;
    uint64_t max_items = strtoull_strict(argv[5], &invalid_char, 10);
    if (*invalid_char != '\0') {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }

    pipeliner_acq.done_argparsing();

    block_pm_duration rget_timer(&pm_cmd_rget);

    rget_result_t results_iterator = rh->get_store->rget(left_mode, left_key, right_mode, right_key, token);

    pipeliner_acq.begin_write();

    /* Check if the query hit a gated_get_store_t */
    if (!results_iterator) {
        rh->client_error_not_allowed(false);
    } else {

        boost::optional<key_with_data_buffer_t> pair;
        uint64_t count = 0;
        ticks_t next_time;
        while (++count <= max_items && (rget_iteration_next.begin(&next_time), pair = results_iterator->next())) {
            rget_iteration_next.end(&next_time);
            const key_with_data_buffer_t& kv = pair.get();

            const std::string& key = kv.key;
            const boost::intrusive_ptr<data_buffer_t>& dp = kv.value_provider;

            rh->write_value_header(key.c_str(), key.length(), kv.mcflags, dp->size());
            rh->write_from_data_provider(dp.get());
            rh->write_crlf();
        }

        rh->write_end();
    }
    pipeliner_acq.end_write();
}

/* "set", "add", "replace", "cas", "append", and "prepend" command logic */

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
                         pipeliner_acq_t *pipeliner_acq_raw,
                         storage_command_t sc,
                         store_key_t key,
                         const boost::intrusive_ptr<data_buffer_t>& data,
                         storage_metadata_t metadata,
                         bool noreply,
                         order_token_t token) {
    boost::scoped_ptr<pipeliner_acq_t> pipeliner_acq(pipeliner_acq_raw);

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

        set_result_t res = rh->set_store->sarc(key, data, metadata.mcflags, metadata.exptime,
                                               add_policy, replace_policy, metadata.unique, token);

        pipeliner_acq->begin_write();


        if (!noreply) {
            switch (res) {
            case sr_stored:
                rh->writef("STORED\r\n");
                break;
            case sr_didnt_add:
                if (sc == replace_command) {
                    rh->writef("NOT_STORED\r\n");
                } else if (sc == cas_command) {
                    rh->writef("NOT_FOUND\r\n");
                } else {
                    unreachable();
                }
                break;
            case sr_didnt_replace:
                if (sc == add_command) {
                    rh->writef("NOT_STORED\r\n");
                } else if (sc == cas_command) {
                    rh->writef("EXISTS\r\n");
                } else {
                    unreachable();
                }
                break;
            case sr_too_large:
                rh->server_error_object_too_large_for_cache();
                break;
            case sr_not_allowed:
                rh->client_error_not_allowed(true);
                break;
            default: unreachable();
            }
        }

    } else {
        append_prepend_result_t res =
            rh->set_store->append_prepend(
                sc == append_command ? append_prepend_APPEND : append_prepend_PREPEND,
                key, data, token);

        pipeliner_acq->begin_write();

        if (!noreply) {
            switch (res) {
            case apr_success: rh->writef("STORED\r\n"); break;
            case apr_not_found: rh->writef("NOT_FOUND\r\n"); break;
            case apr_too_large:
                rh->server_error_object_too_large_for_cache();
                break;
            case apr_not_allowed:
                rh->client_error_not_allowed(true);
                break;
            default: unreachable();
            }
        }
    }

    pipeliner_acq->end_write();

    /* If the key-value store never read our value for whatever reason, then the
    memcached_data_provider_t's destructor will read it off the socket and signal
    read_value_promise here */
}

void do_storage(txt_memcached_handler_t *rh, pipeliner_t *pipeliner, storage_command_t sc, int argc, char **argv, order_token_t token) {
    // This is _not_ spawned yet.

    pipeliner_acq_t *pipeliner_acq = new pipeliner_acq_t(pipeliner);
    pipeliner_acq->begin_operation();

    char *invalid_char;

    /* cmd key flags exptime size [noreply]
       OR "cas" key flags exptime size cas [noreply] */
    if ((sc != cas_command && (argc != 5 && argc != 6)) ||
        (sc == cas_command && (argc != 6 && argc != 7))) {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->error();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }

    /* First parse the key */
    store_key_t key;
    if (!str_to_key(argv[1], &key)) {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }
    pm_storage_key_size.record((float) key.size);

    /* Next parse the flags */
    mcflags_t mcflags = strtoul_strict(argv[2], &invalid_char, 10);
    if (*invalid_char != '\0') {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }

    /* Now parse the expiration time */
    exptime_t exptime = strtoul_strict(argv[3], &invalid_char, 10);
    if (*invalid_char != '\0') {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
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
        // If 60*60*24*30 < exptime <= time(NULL), that's fine, the
        // btree code needs to handle that case gracefully anyway
        // (since the clock can tick in the middle of an insert
        // anyway...).  We have tests in expiration.py.
        exptime += time(NULL);
    }

    /* Now parse the value length */
    size_t value_size = strtoul_strict(argv[4], &invalid_char, 10);
    // Check for signed 32 bit max value for Memcached compatibility...
    if (*invalid_char != '\0' || value_size >= (1u << 31) - 1) {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }
    pm_storage_value_size.record((float) value_size);

    /* If a "cas", parse the cas_command unique */
    cas_t unique = NO_CAS_SUPPLIED;
    if (sc == cas_command) {
        unique = strtoull_strict(argv[5], &invalid_char, 10);
        if (*invalid_char != '\0') {
            pipeliner_acq->done_argparsing();
            pipeliner_acq->begin_write();
            rh->client_error_bad_command_line_format();
            pipeliner_acq->end_write();
            delete pipeliner_acq;
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

    /* Read the data off the socket. For now we always read the data into a buffer. In the
       future we may want to be able to stream the data to its destination. */
    boost::intrusive_ptr<data_buffer_t> dp = data_buffer_t::create(value_size);
    char crlf_buf[2];
    try {
        rh->read(dp->buf(), value_size);
        rh->read(crlf_buf, 2);
    } catch (memcached_interface_t::no_more_data_exc_t) {
        rh->client_error("bad data chunk\r\n");
        return;
    }

    if (memcmp(crlf_buf, crlf, 2) != 0) {
        rh->client_error("bad data chunk\r\n");
        return;
    }

    /* Bundle metadata into one object so we don't try to pass too many arguments
    to `boost::bind()` */
    storage_metadata_t metadata(mcflags, exptime, unique);

    pipeliner_acq->done_argparsing();

    coro_t::spawn_now(boost::bind(&run_storage_command, rh, pipeliner_acq, sc, key, dp, metadata, noreply, token));
}

/* "incr" and "decr" commands */
void run_incr_decr(txt_memcached_handler_t *rh, pipeliner_acq_t *pipeliner_acq, store_key_t key, uint64_t amount, bool incr, bool noreply, order_token_t token) {
    block_pm_duration set_timer(&pm_cmd_set);

    incr_decr_result_t res = rh->set_store->incr_decr(
        incr ? incr_decr_INCR : incr_decr_DECR,
        key, amount, token);

    pipeliner_acq->begin_write();
    if (!noreply) {
        switch (res.res) {
            case incr_decr_result_t::idr_success:
                rh->writef("%llu\r\n", (unsigned long long)res.new_value);
                break;
            case incr_decr_result_t::idr_not_found:
                rh->writef("NOT_FOUND\r\n");
                break;
            case incr_decr_result_t::idr_not_numeric:
                rh->client_error("cannot increment or decrement non-numeric value\r\n");
                break;
            case incr_decr_result_t::idr_not_allowed:
                rh->client_error_not_allowed(true);
                break;
            default: unreachable();
        }
    }

    pipeliner_acq->end_write();
}

void do_incr_decr(txt_memcached_handler_t *rh, pipeliner_t *pipeliner, bool i, int argc, char **argv, order_token_t token) {
    // We should already be spawned in a coroutine.
    pipeliner_acq_t pipeliner_acq(pipeliner);
    pipeliner_acq.begin_operation();

    /* cmd key delta [noreply] */
    if (argc != 3 && argc != 4) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->error();
        pipeliner_acq.end_write();
        return;
    }

    /* Parse key */
    store_key_t key;
    if (!str_to_key(argv[1], &key)) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }

    /* Parse amount to change by */
    char *invalid_char;
    uint64_t delta = strtoull_strict(argv[2], &invalid_char, 10);
    if (*invalid_char != '\0') {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
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

    pipeliner_acq.done_argparsing();

    run_incr_decr(rh, &pipeliner_acq, key, delta, i, noreply, token);
}

/* "delete" commands */

void run_delete(txt_memcached_handler_t *rh, pipeliner_acq_t *pipeliner_acq, store_key_t key, bool noreply, order_token_t token) {

    block_pm_duration set_timer(&pm_cmd_set);

    delete_result_t res = rh->set_store->delete_key(key, token);

    pipeliner_acq->begin_write();
    if (!noreply) {
        switch (res) {
            case dr_deleted:
                rh->writef("DELETED\r\n");
                break;
            case dr_not_found:
                rh->writef("NOT_FOUND\r\n");
                break;
            case dr_not_allowed:
                rh->client_error_not_allowed(true);
                break;
            default: unreachable();
        }
    }

    pipeliner_acq->end_write();
}

void do_delete(txt_memcached_handler_t *rh, pipeliner_t *pipeliner, int argc, char **argv, order_token_t token) {
    // This should already be spawned within a coroutine.
    pipeliner_acq_t pipeliner_acq(pipeliner);
    pipeliner_acq.begin_operation();

    /* "delete" key [a number] ["noreply"] */
    if (argc < 2 || argc > 4) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->error();
        pipeliner_acq.end_write();
        return;
    }

    /* Parse key */
    store_key_t key;
    if (!str_to_key(argv[1], &key)) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }
    pm_delete_key_size.record((float) key.size);

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
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            if (!noreply) {
                rh->client_error_bad_command_line_format();
            }
            pipeliner_acq.end_write();
            return;
        }
    } else {
        noreply = false;
    }

    pipeliner_acq.done_argparsing();

    run_delete(rh, &pipeliner_acq, key, noreply, token);
}

/* "stats" command */

std::string memcached_stats(int argc, char **argv, bool include_internal) {
    std::string res;
    perfmon_stats_t stats;

    perfmon_get_stats(&stats, include_internal);

    if (argc == 1) {
        for (perfmon_stats_t::iterator iter = stats.begin(); iter != stats.end(); iter++) {
            res += strprintf("STAT %s %s\r\n", iter->first.c_str(), iter->second.c_str());
        }
    } else {
        for (int i = 1; i < argc; i++) {
            perfmon_stats_t::iterator iter = stats.find(argv[i]);
            res += strprintf("STAT %s %s\r\n", argv[i], iter == stats.end()
                                                        ? "NOT_FOUND"
                                                        : iter->second.c_str());

        }
    }
    res += "END\r\n";
    return res;
}


// Control for showing internal stats.
struct memcached_stats_control_t : public control_t {
    memcached_stats_control_t() :
        control_t("stats", "Show all stats (including internal stats).", true) {}
    std::string call(int argc, char **argv) {
        return memcached_stats(argc, argv, true);
    }
} memcached_stats_control;

#ifndef NDEBUG
void do_quickset(txt_memcached_handler_t *rh, pipeliner_acq_t *pipeliner_acq, std::vector<char*> args) {
    if (args.size() < 2 || args.size() % 2 == 0) {
        // The connection will be closed if more than a megabyte or so is sent
        // over without a newline, so we don't really need to worry about large
        // values.
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->write("CLIENT_ERROR Usage: .s k1 v1 [k2 v2...] (no whitespace in values)\r\n");
        pipeliner_acq->end_write();
        return;
    }

    std::vector<std::string> args_copy(args.begin(), args.end());

    pipeliner_acq->done_argparsing();
    pipeliner_acq->begin_write();
    for (size_t i = 1; i < args.size(); i += 2) {
        store_key_t key;
        if (!str_to_key(args_copy[i].c_str(), &key)) {
            rh->writef("CLIENT_ERROR Invalid key %s\r\n", args_copy[i].c_str());
            pipeliner_acq->end_write();
            return;
        }
        boost::intrusive_ptr<data_buffer_t> value = data_buffer_t::create(args_copy[i + 1].size());
        memcpy(value->buf(), args_copy[i + 1].data(), value->size());

        set_result_t res = rh->set_store->sarc(key, value, 0, 0, add_policy_yes, replace_policy_yes, 0, order_token_t::ignore);

        if (res == sr_stored) {
            rh->writef("STORED key %s\r\n", args_copy[i].c_str());
        } else {
            rh->writef("MYSTERIOUS_ERROR key %s\r\n", args_copy[i].c_str());
        }
    }
    pipeliner_acq->end_write();
}

bool parse_debug_command(txt_memcached_handler_t *rh, pipeliner_t *pipeliner, std::vector<char*> args) {
    pipeliner_acq_t pipeliner_acq(pipeliner);
    pipeliner_acq.begin_operation();

    if (args.size() < 1) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        pipeliner_acq.end_write();
        return false;
    }

    // .h is an alias for "rdb hash"
    if (!strcmp(args[0], ".h") && args.size() >= 2) {
        // We can't use our reference to args after we've called
        // done_argparsing().
        std::vector<std::string> args_copy(args.begin(), args.end());

        std::vector<char *> ctrl_args;
        for (int i = 0, e = args_copy.size(); i < e; ++i) {
            ctrl_args.push_back(const_cast<char *>(args_copy[i].c_str()));
        }

        static char hashstring[] = "hash";
        ctrl_args[0] = hashstring;

        pipeliner_acq.done_argparsing();
        std::string control_result = control_t::exec(ctrl_args.size(), ctrl_args.data());

        pipeliner_acq.begin_write();
        rh->write(control_result);
        pipeliner_acq.end_write();
        return true;
    } else if (!strcmp(args[0], ".s")) {
        do_quickset(rh, &pipeliner_acq, args);
        return true;
    } else {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        pipeliner_acq.end_write();
        return false;
    }
}
#endif  // NDEBUG

/* Handle memcached, takes a txt_memcached_handler_t and handles the memcached commands that come in on it */
void handle_memcache(memcached_interface_t *interface, get_store_t *get_store,
        set_store_interface_t *set_store, int max_concurrent_queries_per_connection) {
    logDBG("Opened memcached stream: %p\n", coro_t::self());

    /* This object just exists to group everything together so we don't have to pass a lot of
    context around. */
    txt_memcached_handler_t rh(interface, get_store, set_store, max_concurrent_queries_per_connection);

    /* The commands from each individual memcached handler must be performed in the order
    that the handler parses them. This `order_source_t` is used to guarantee that. */
    order_source_t order_source;

    /* Declared outside the while-loop so it doesn't repeatedly reallocate its buffer */
    std::vector<char> line;

    pipeliner_t pipeliner(&rh);

    while (true) {

        /* Read a line off the socket */
        block_pm_duration read_timer(&pm_conns_reading);
        try {
            rh.read_line(&line);
        } catch (memcached_interface_t::no_more_data_exc_t) {
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
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh.error();
            pipeliner_acq.end_write();
            continue;
        }

        /* Dispatch to the appropriate subclass */
	order_token_t token = order_source.check_in(std::string("handle_memcache+") + args[0]);
        if (!strcmp(args[0], "get")) {    // check for retrieval commands
            coro_t::spawn_now(boost::bind(do_get, &rh, &pipeliner, false, args.size(), args.data(), token.with_read_mode()));
        } else if (!strcmp(args[0], "gets")) {
            coro_t::spawn_now(boost::bind(do_get, &rh, &pipeliner, true, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "rget")) {
            coro_t::spawn_now(boost::bind(do_rget, &rh, &pipeliner, args.size(), args.data(), token.with_read_mode()));
        } else if (!strcmp(args[0], "set")) {     // check for storage commands
            do_storage(&rh, &pipeliner, set_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "add")) {
            do_storage(&rh, &pipeliner, add_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "replace")) {
            do_storage(&rh, &pipeliner, replace_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "append")) {
            do_storage(&rh, &pipeliner, append_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "prepend")) {
            do_storage(&rh, &pipeliner, prepend_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "cas")) {
            do_storage(&rh, &pipeliner, cas_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "delete")) {
            coro_t::spawn_now(boost::bind(do_delete, &rh, &pipeliner, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "incr")) {
            coro_t::spawn_now(boost::bind(do_incr_decr, &rh, &pipeliner, true, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "decr")) {
            coro_t::spawn_now(boost::bind(do_incr_decr, &rh, &pipeliner, false, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "quit")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            // Make sure there's no more tokens (the kind in args, not
            // order tokens)
            if (args.size() > 1) {
                // We block everybody, but who cares?
                pipeliner_acq.done_argparsing();
                pipeliner_acq.begin_write();
                rh.error();
                pipeliner_acq.end_write();
            } else {
                // It's very important that we actually block everybody here :)
                pipeliner_acq.done_argparsing();
                pipeliner_acq.begin_write();
                pipeliner_acq.end_write();
                break;
            }
        } else if (!strcmp(args[0], "stats") || !strcmp(args[0], "stat")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            std::string the_stats = memcached_stats(args.size(), args.data(), false);

            // We block everybody before writing.  I don't think we care.
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh.write(the_stats); // Only shows "public" stats; to see all stats, use "rdb stats".
            pipeliner_acq.end_write();
        } else if (!strcmp(args[0], "help")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            // Slightly hacky -- just treat this as a control. This assumes that a control named "help" exists (which it does).
            std::string help_text = control_t::exec(args.size(), args.data());

            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh.write(help_text);
            pipeliner_acq.end_write();
        } else if(!strcmp(args[0], "rethinkdb") || !strcmp(args[0], "rdb")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            std::string control_text = control_t::exec(args.size() - 1, args.data() + 1);

            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh.write(control_text);
            pipeliner_acq.end_write();
        } else if (!strcmp(args[0], "version")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            if (args.size() == 1) {
                rh.writef("VERSION rethinkdb-%s\r\n", RETHINKDB_VERSION);
            } else {
                rh.error();
            }
            pipeliner_acq.end_write();
#ifndef NDEBUG
        } else if (!parse_debug_command(&rh, &pipeliner, args)) {
#else
        } else {
#endif
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh.error();
            pipeliner_acq.end_write();
        }

        action_timer.end();
    }

    // Make sure anything that would be running has finished.
    pipeliner_acq_t pipeliner_acq(&pipeliner);
    pipeliner_acq.begin_operation();
    pipeliner_acq.done_argparsing();
    pipeliner_acq.begin_write();
    pipeliner_acq.end_write();

    logDBG("Closed memcached stream: %p\n", coro_t::self());
}


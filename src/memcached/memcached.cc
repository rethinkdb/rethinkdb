#include "memcached/memcached.hpp"

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include <stdexcept>
#include <stdarg.h>
#include <unistd.h>

#include "arch/arch.hpp"
#include "concurrency/coro_fifo.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/task.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "containers/unique_ptr.hpp"
#include "server/control.hpp"
#include "store.hpp"
#include "logger.hpp"
#include "progress/progress.hpp"
#include "arch/os_signal.hpp"

/* txt_memcached_handler_t is basically defunct; it only exists as a convenient thing to pass
around to do_get(), do_storage(), and the like. */

static const char *crlf = "\r\n";
static const char crlf_array[] = { '\r', '\n' };

struct txt_memcached_handler_t : public txt_memcached_handler_if, public home_thread_mixin_t {
    tcp_conn_t *conn;

    txt_memcached_handler_t(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store, int num_slices) :
        txt_memcached_handler_if(get_store, set_store, num_slices),
        conn(conn)
    { }

    /* Wrappers around conn->write*() that ignore write errors, because we want to continue
    processing requests even if the client is no longer listening for replies */
    void write(const std::string& buffer) {
        write(buffer.data(), buffer.length());
    }

    void write(const char *buffer, size_t bytes) {
        try {
            assert_thread();
            conn->write(buffer, bytes);
        } catch (tcp_conn_t::write_closed_exc_t) {
        }
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
        try {
            conn->get_raw_connection().write(buffer, bytes, NULL);
        } catch (tcp_conn_t::write_closed_exc_t) {
        }
    }

    void write_from_data_provider(const boost::shared_ptr<data_provider_t>& dp, memcached_write_callback_t *callback) {
        /* Write the value itself. If the value is small, write it into the send buffer;
        otherwise, stream it. */
        const const_buffer_group_t *bg = dp->get_data_as_buffers();
        struct iovec iov[bg->num_buffers() + 1]; // We add an index to contain the end of line at the end of the write

        for (size_t i = 0; i < bg->num_buffers(); i++) {
            const_buffer_group_t::buffer_t b = bg->get_buffer(i);
            iov[i].iov_base = const_cast<void *>(b.data);
            iov[i].iov_len = b.size;
        }
        iov[bg->num_buffers()].iov_base = const_cast<void *>(reinterpret_cast<const void *>(crlf_array));
        iov[bg->num_buffers()].iov_len = sizeof(crlf_array);

        conn->write_vectored(iov, bg->num_buffers() + 1, callback);
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
        try {
            conn->flush_write_buffer();
        } catch (tcp_conn_t::write_closed_exc_t) {
            /* Ignore errors; it's OK for the write end of the connection to be closed. */
        }
    }

    bool is_write_open() {
        return conn->is_write_open();
    }

    void read(void *buf, size_t nbytes) {
        try {
            conn->read_exactly(buf, nbytes);
        } catch(tcp_conn_t::read_closed_exc_t) {
            throw no_more_data_exc_t();
        }
    }

    void read_line(std::vector<char> *dest) {
        try {
            for (;;) {
                const_charslice sl = conn->peek_read_buffer();
                void *crlf_loc = memmem(sl.beg, sl.end - sl.beg, crlf, 2);
                ssize_t threshold = MEGABYTE;

                if (crlf_loc) {
                    // We have a valid line.
                    size_t line_size = (char *)crlf_loc - (char *)sl.beg;

                    dest->resize(line_size + 2);  // +2 for CRLF
                    memcpy(dest->data(), sl.beg, line_size + 2);
                    conn->pop_read_buffer(line_size + 2);
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

        } catch(tcp_conn_t::read_closed_exc_t) {
            throw no_more_data_exc_t();
        }
    }
};

/* Warning this is a bit of a hack, it's like the memcached handler but instead
 * of a conn, it has a file which it reads from, it implements alot of the
 * interface with dummy functions (like all of the values that would be sent
 * back to user), this feels kind of dumb since we have an abstract interface
 * with 2 subclasses, one of which basically has dummies for 90% of the
 * functions. This is really mostly out of convience, the right way to do this
 * would be to have more abstraction in txt_memcached_handler_t... but for now
 * it doesn't seem worth it. It's also worth noting that it's not entirely
 * outside the realm of possibility that we could want this output at some
 * later date (in which case we would replace these dummies with real
 * functions). One use case that jumps to mind is that we could potentially use
 * a big rget as a more efficient (I think) form of abstraction, in which case
 * we would need the output */

class txt_memcached_file_importer_t : public txt_memcached_handler_if
{
private:
    FILE *file;
    file_progress_bar_t progress_bar;

public:

    txt_memcached_file_importer_t(std::string filename, get_store_t *get_store, set_store_interface_t *set_store, int num_slices) :
        txt_memcached_handler_if(get_store, set_store, num_slices, MAX_CONCURRENT_QUEURIES_ON_IMPORT),
        file(fopen(filename.c_str(), "r")), progress_bar(std::string("Import"), file)
    { }
    ~txt_memcached_file_importer_t() {
        fclose(file);
    }
    void write(UNUSED const std::string& buffer) { }
    void write(UNUSED const char *buffer, UNUSED size_t bytes) { }
    void vwritef(UNUSED const char *format, UNUSED va_list args) { }
    void writef(UNUSED const char *format, ...) { }
    void write_unbuffered(UNUSED const char *buffer, UNUSED size_t bytes) { }
    void write_from_data_provider(UNUSED const boost::shared_ptr<data_provider_t>& dp, UNUSED memcached_write_callback_t *callback) { }
    void write_value_header(UNUSED const char *key, UNUSED size_t key_size, UNUSED mcflags_t mcflags, UNUSED size_t value_size) { }
    void write_value_header(UNUSED const char *key, UNUSED size_t key_size, UNUSED mcflags_t mcflags, UNUSED size_t value_size, UNUSED cas_t cas) { }
    void error() { }
    void write_crlf() { }
    void write_end() { }
    void client_error(UNUSED const char *format, ...) { }
    void server_error(UNUSED const char *format, ...) { }
    void client_error_bad_command_line_format() { }
    void client_error_bad_data() { }
    void client_error_not_allowed(UNUSED bool op_is_write) { }
    void server_error_object_too_large_for_cache() { }
    void flush_buffer() { }

    bool is_write_open() { return false; }

    void read(void *buf, size_t nbytes) {
        if (fread(buf, nbytes, 1, file) == 0)
            throw no_more_data_exc_t();
    }

    void read_line(std::vector<char> *dest) {
        int limit = MEGABYTE;
        dest->clear();
        char c;
        char *head = (char *) crlf;
        while ((*head) && ((c = getc(file)) != EOF) && (limit--) > 0) {
            dest->push_back(c);
            if (c == *head) head++;
            else head = (char *) crlf;
        }
        //we didn't every find a crlf unleash the exception
        if (*head) throw no_more_data_exc_t();
    }

public:
    /* In our current use of import we ignore gets, the easiest way to do this
     * is with a dummyed get_store */
    class dummy_get_store_t : public get_store_t {
        get_result_t get(UNUSED const store_key_t &key, UNUSED sequence_group_t *seq_group, UNUSED order_token_t token) { return get_result_t(); }
        rget_result_t rget(UNUSED sequence_group_t *seq_group,
                           UNUSED rget_bound_mode_t left_mode, UNUSED const store_key_t &left_key,
                           UNUSED rget_bound_mode_t right_mode, UNUSED const store_key_t &right_key,
                           UNUSED order_token_t token)
        {
            return rget_result_t();
        }
    };
};

perfmon_duration_sampler_t
    pm_conns_reading("conns_reading", secs_to_ticks(1), false),
    pm_conns_writing("conns_writing", secs_to_ticks(1), false),
    pm_conns_acting("conns_acting", secs_to_ticks(1), false);


class pipeliner_t {
public:
    pipeliner_t(txt_memcached_handler_if *rh) : requests_out_sem(rh->max_concurrent_queries_per_connection), rh_(rh) { }
    ~pipeliner_t() { }

    void lock_argparsing() {
        co_lock_mutex(&argparsing_mutex);
    }
private:
    friend class pipeliner_acq_t;
    coro_fifo_t fifo;

    // This should have no effect (because we don't block coroutines
    // until after done argparsing), but
    mutex_t argparsing_mutex;

    // Used to limit number of concurrent requests
    semaphore_t requests_out_sem;

    mutex_t mutex;
    txt_memcached_handler_if *rh_;

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

        pipeliner_->argparsing_mutex.unlock(false);
        pipeliner_->requests_out_sem.co_lock();
    }

    void begin_write() {
        rassert(state_ == has_done_argparsing);
        DEBUG_ONLY(state_ = has_begun_write);
        fifo_acq_.leave();
        co_lock_mutex(&pipeliner_->mutex);
    }

    void end_write() {
        rassert(state_ == has_begun_write);
        DEBUG_ONLY(state_ = has_ended_write);

        block_pm_duration flush_timer(&pm_conns_writing);
        pipeliner_->rh_->flush_buffer();
        flush_timer.end();

        pipeliner_->mutex.unlock(true);
        pipeliner_->requests_out_sem.unlock();
    }

private:
    pipeliner_t *pipeliner_;
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




/* do_get() is used for "get" and "gets" commands. */

struct get_t {
    store_key_t key;
    get_result_t res;
};

void do_one_get(txt_memcached_handler_if *rh, bool with_cas, get_t *gets, int i, order_token_t token) {
    if (with_cas) {
        gets[i].res = rh->set_store->get_cas(&rh->seq_group, gets[i].key, token);
    } else {
        gets[i].res = rh->get_store->get(gets[i].key, &rh->seq_group, token);
    }
}

void do_get(txt_memcached_handler_if *rh, pipeliner_t *pipeliner, bool with_cas, int argc, char **argv, order_token_t token) {
    try { // RSI: try catch all the places
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
            pipeliner_acq.end_write();

        } else {
            txt_memcached_handler_if::memcached_write_callback_t callbacks[gets.size()];
            cond_t callback_cond;
            size_t callbacks_to_run = gets.size();

            /* Handle the results in sequence */
            for (int i = 0; i < (int)gets.size(); i++) {
                get_result_t &res = gets[i].res;

                rassert(!res.is_not_allowed);

                /* If res.value is NULL that means the value was not found so we don't write
                   anything */
                if (res.value_provider) {
                    /* If the write half of the connection has been closed, there's no point in trying
                       to send anything */
                    if (rh->is_write_open()) {
                        const store_key_t &key = gets[i].key;

                        /* Write the "VALUE ..." header */
                        if (with_cas) {
                            rh->write_value_header(key.contents, key.size, res.flags, res.value_provider->get_size(), res.cas);
                        } else {
                            rassert(res.cas == 0);
                            rh->write_value_header(key.contents, key.size, res.flags, res.value_provider->get_size());
                        }

                        callbacks[i].dp = res.value_provider;
                        callbacks[i].cond = &callback_cond;
                        callbacks[i].callbacks_to_run = &callbacks_to_run;
                        rh->write_from_data_provider(res.value_provider, &callbacks[i]);
                    } else {
                        --callbacks_to_run;
                    }
                } else {
                    --callbacks_to_run;
                }
            }

            rh->write_end();
            pipeliner_acq.end_write();

            // Wait for the last callback to be called
            callback_cond.wait();
            guarantee(callbacks_to_run == 0);
        }
        
    } catch (tcp_conn_t::write_closed_exc_t &ex) {
        // the other side has closed the socket, don't do anything
    }
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
void do_rget(txt_memcached_handler_if *rh, pipeliner_t *pipeliner, int argc, char **argv, order_token_t token) {
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

    rget_result_t results_iterator = rh->get_store->rget(&rh->seq_group, left_mode, left_key, right_mode, right_key, token);

    pipeliner_acq.begin_write();

    /* Check if the query hit a gated_get_store_t */
    if (!results_iterator) {
        rh->client_error_not_allowed(false);
    } else {

        boost::optional<key_with_data_provider_t> pair;
        uint64_t count = 0;
        ticks_t next_time;
        while (++count <= max_items && (rget_iteration_next.begin(&next_time), pair = results_iterator->next())) {
            rget_iteration_next.end(&next_time);
            const key_with_data_provider_t& kv = pair.get();
            const std::string& key = kv.key;

            rh->write_value_header(key.c_str(), key.length(), kv.mcflags, kv.value_provider->get_size());
            rh->write_from_data_provider(kv.value_provider, NULL); // RSI - this will always copy instead of using zerocopy
        }

        rh->write_end();
    }
    pipeliner_acq.end_write();
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
    txt_memcached_handler_if *rh;   // Request handler for us to read from
    size_t length;   // Our length
    bool was_read;
    promise_t<bool> *to_signal;

public:
    memcached_data_provider_t(txt_memcached_handler_if *rh, uint32_t length, promise_t<bool> *to_signal)
        : rh(rh), length(length), was_read(false), to_signal(to_signal)
    { }

    ~memcached_data_provider_t() {
        if (!was_read) {
            /* We have to clear the data out of the socket, even if we have nowhere to put it. */
            try {
                // TODO: Really we should do this on the home_thread()
                // (if we actually did any kind of streaming), neh?  Yeh.
                get_data_as_buffers();
            } catch (data_provider_failed_exc_t) {
                // If the connection was closed then we don't need to clear the
                // data out of the socket.
            }
        }

        // TODO: This is crufty, really we should just not be a home_thread_mixin_t.
        // This is a harmless hack to get ~home_thread_mixin_t to not fail its assertion.
#ifndef NDEBUG
        const_cast<int&>(real_home_thread) = get_thread_id();
#endif
    }

    size_t get_size() const {
        return length;
    }

    void get_data_into_buffers(const buffer_group_t *b) throw (data_provider_failed_exc_t) {
        rassert(!was_read);
        was_read = true;
        on_thread_t thread_switcher(home_thread());
        try {
            for (int i = 0; i < (int)b->num_buffers(); i++) {
                rh->read(b->get_buffer(i).data, b->get_buffer(i).size);
            }
            char expected_crlf[2];
            rh->read(expected_crlf, 2);
            if (memcmp(expected_crlf, crlf, 2) != 0) {
                if (to_signal) to_signal->pulse(false);
                throw data_provider_failed_exc_t();   // Cancel the set/append/prepend operation
            }
            if (to_signal) to_signal->pulse(true);
        } catch (txt_memcached_handler_if::no_more_data_exc_t) {
            if (to_signal) to_signal->pulse(false);
            throw data_provider_failed_exc_t();
        }
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

void run_storage_command(txt_memcached_handler_if *rh,
                         pipeliner_acq_t *pipeliner_acq_raw,
                         storage_command_t sc,
                         store_key_t key,
                         size_t value_size, promise_t<bool> *value_read_promise,
                         storage_metadata_t metadata,
                         bool noreply,
                         order_token_t token) {
    boost::scoped_ptr<pipeliner_acq_t> pipeliner_acq(pipeliner_acq_raw);

    unique_ptr_t<memcached_data_provider_t> unbuffered_data(new memcached_data_provider_t(rh, value_size, value_read_promise));
    unique_ptr_t<maybe_buffered_data_provider_t> data(new maybe_buffered_data_provider_t(unbuffered_data, MAX_BUFFERED_SET_SIZE));

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

        set_result_t res = rh->set_store->sarc(&rh->seq_group, key, data, metadata.mcflags, metadata.exptime,
                                               add_policy, replace_policy, metadata.unique, token);

        pipeliner_acq->begin_write();


        if (!noreply || res == sr_data_provider_failed) {
            switch (res) {
            case sr_stored:
                rh->writef("STORED\r\n");
                break;
            case sr_didnt_add:
                if (sc == replace_command) rh->writef("NOT_STORED\r\n");
                else if (sc == cas_command) rh->writef("NOT_FOUND\r\n");
                else unreachable();
                break;
            case sr_didnt_replace:
                if (sc == add_command) rh->writef("NOT_STORED\r\n");
                else if (sc == cas_command) rh->writef("EXISTS\r\n");
                else unreachable();
                break;
            case sr_too_large:
                rh->server_error_object_too_large_for_cache();
                break;
            case sr_data_provider_failed:
                /* We get here if there was no CRLF */
                rh->client_error("bad data chunk\r\n");
                break;
            case sr_not_allowed:
                rh->client_error_not_allowed(true);
                break;
            default: unreachable();
            }
        }

    } else {
        append_prepend_result_t res =
            rh->set_store->append_prepend(&rh->seq_group,
                sc == append_command ? append_prepend_APPEND : append_prepend_PREPEND,
                key, data, token);

        pipeliner_acq->begin_write();

        if (!noreply || res == apr_data_provider_failed) {
            switch (res) {
            case apr_success: rh->writef("STORED\r\n"); break;
            case apr_not_found: rh->writef("NOT_FOUND\r\n"); break;
            case apr_too_large:
                rh->server_error_object_too_large_for_cache();
                break;
            case apr_data_provider_failed:
                /* We get here if there was no CRLF */
                rh->client_error("bad data chunk\r\n");
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

void do_storage(txt_memcached_handler_if *rh, pipeliner_t *pipeliner, storage_command_t sc, int argc, char **argv, order_token_t token) {
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

    /* run_storage_command() will signal this when it's done reading the value off the socket. */
    promise_t<bool> value_read_promise;
    storage_metadata_t metadata(mcflags, exptime, unique);

    pipeliner_acq->done_argparsing();

    coro_t::spawn_now(boost::bind(&run_storage_command, rh, pipeliner_acq, sc, key, value_size, &value_read_promise, metadata, noreply, token));

    /* We can't move on to the next command until the value has been read off the socket. */
    UNUSED bool ok = value_read_promise.wait();

    // We no longer care whether reading the value had no CRLF or not,
    // so ok is unused.
}

/* "incr" and "decr" commands */
void run_incr_decr(txt_memcached_handler_if *rh, pipeliner_acq_t *pipeliner_acq, store_key_t key, uint64_t amount, bool incr, bool noreply, order_token_t token) {
    block_pm_duration set_timer(&pm_cmd_set);

    incr_decr_result_t res = rh->set_store->incr_decr(&rh->seq_group,
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

void do_incr_decr(txt_memcached_handler_if *rh, pipeliner_t *pipeliner, bool i, int argc, char **argv, order_token_t token) {
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

void run_delete(txt_memcached_handler_if *rh, pipeliner_acq_t *pipeliner_acq, store_key_t key, bool noreply, order_token_t token) {
    block_pm_duration set_timer(&pm_cmd_set);

    delete_result_t res = rh->set_store->delete_key(&rh->seq_group, key, token);

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

void do_delete(txt_memcached_handler_if *rh, UNUSED pipeliner_t *pipeliner, int argc, char **argv, order_token_t token) {
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
void do_quickset(txt_memcached_handler_if *rh, pipeliner_acq_t *pipeliner_acq, std::vector<char*> args) {
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
        unique_ptr_t<buffered_data_provider_t> value(new buffered_data_provider_t(args_copy[i + 1].data(), args_copy[i + 1].size()));

        set_result_t res = rh->set_store->sarc(&rh->seq_group, key, value, 0, 0, add_policy_yes, replace_policy_yes, 0, order_token_t::ignore);

        if (res == sr_stored) {
            rh->writef("STORED key %s\r\n", args_copy[i].c_str());
        } else {
            rh->writef("MYSTERIOUS_ERROR key %s\r\n", args_copy[i].c_str());
        }
    }
    pipeliner_acq->end_write();
}

bool parse_debug_command(txt_memcached_handler_if *rh, pipeliner_t *pipeliner, std::vector<char*> args) {

    if (args.size() < 1) {
        return false;
    }

    // .h is an alias for "rdb hash"
    if (!strcmp(args[0], ".h") && args.size() >= 2) {
        pipeliner_acq_t pipeliner_acq(pipeliner);
        pipeliner_acq.begin_operation();

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
        pipeliner_acq_t pipeliner_acq(pipeliner);
        pipeliner_acq.begin_operation();
        do_quickset(rh, &pipeliner_acq, args);
        return true;
    } else {
        return false;
    }
}
#endif  // NDEBUG

/* Handle memcached, takes a txt_memcached_handler_if and handles the memcached commands that come in on it */
void handle_memcache(txt_memcached_handler_if *rh, order_source_t *order_source) {
    logDBG("Opened memcached stream: %p\n", coro_t::self());

    /* Declared outside the while-loop so it doesn't repeatedly reallocate its buffer */
    std::vector<char> line;
    std::vector<char *> args;

    /* Create a linked sigint cond on my thread */
    sigint_indicator_t sigint_has_happened;

    pipeliner_t pipeliner(rh);

    while (pipeliner.lock_argparsing(), !sigint_has_happened.get_value()) {
        /* Read a line off the socket */
        block_pm_duration read_timer(&pm_conns_reading);
        try {
            rh->read_line(&line);
        } catch (txt_memcached_handler_if::no_more_data_exc_t) {
            break;
        }
        read_timer.end();

        block_pm_duration action_timer(&pm_conns_acting);

        /* Tokenize the line */
        line.push_back('\0');   // Null terminator
        args.clear();
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
            rh->error();
            pipeliner_acq.end_write();
            continue;
        }

        /* Dispatch to the appropriate subclass */
        order_token_t token = order_source->check_in(std::string("handle_memcache+") + args[0]);
        if (!strcmp(args[0], "get")) {    // check for retrieval commands
            coro_t::spawn_now(boost::bind(do_get, rh, &pipeliner, false, args.size(), args.data(), token.with_read_mode()));
        } else if (!strcmp(args[0], "gets")) {
            coro_t::spawn_now(boost::bind(do_get, rh, &pipeliner, true, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "rget")) {
            coro_t::spawn_now(boost::bind(do_rget, rh, &pipeliner, args.size(), args.data(), token.with_read_mode()));
        } else if (!strcmp(args[0], "set")) {     // check for storage commands
            do_storage(rh, &pipeliner, set_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "add")) {
            do_storage(rh, &pipeliner, add_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "replace")) {
            do_storage(rh, &pipeliner, replace_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "append")) {
            do_storage(rh, &pipeliner, append_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "prepend")) {
            do_storage(rh, &pipeliner, prepend_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "cas")) {
            do_storage(rh, &pipeliner, cas_command, args.size(), args.data(), token);
        } else if (!strcmp(args[0], "delete")) {
            coro_t::spawn_now(boost::bind(do_delete, rh, &pipeliner, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "incr")) {
            coro_t::spawn_now(boost::bind(do_incr_decr, rh, &pipeliner, true, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "decr")) {
            coro_t::spawn_now(boost::bind(do_incr_decr, rh, &pipeliner, false, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "quit")) {
            // Make sure there's no more tokens (the kind in args, not
            // order tokens)
            if (args.size() > 1) {
                pipeliner_acq_t pipeliner_acq(&pipeliner);
                pipeliner_acq.begin_operation();
                // We block everybody, but who cares?
                pipeliner_acq.done_argparsing();
                pipeliner_acq.begin_write();
                rh->error();
                pipeliner_acq.end_write();
            } else {
                break;
            }
        } else if (!strcmp(args[0], "stats") || !strcmp(args[0], "stat")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            std::string the_stats = memcached_stats(args.size(), args.data(), false);

            // We block everybody before writing.  I don't think we care.
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh->write(the_stats); // Only shows "public" stats; to see all stats, use "rdb stats".
            pipeliner_acq.end_write();
        } else if (!strcmp(args[0], "help")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            // Slightly hacky -- just treat this as a control. This assumes that a control named "help" exists (which it does).
            std::string help_text = control_t::exec(args.size(), args.data());

            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh->write(help_text);
            pipeliner_acq.end_write();
        } else if(!strcmp(args[0], "rethinkdb") || !strcmp(args[0], "rdb")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            std::string control_text = control_t::exec(args.size() - 1, args.data() + 1);

            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh->write(control_text);
            pipeliner_acq.end_write();
        } else if (!strcmp(args[0], "version")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            if (args.size() == 1) {
                rh->writef("VERSION rethinkdb-%s\r\n", RETHINKDB_VERSION);
            } else {
                rh->error();
            }
            pipeliner_acq.end_write();
#ifndef NDEBUG
        } else if (!parse_debug_command(rh, &pipeliner, args)) {

#else
        } else {
#endif

            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh->error();
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

/* serve_memcache serves memcache over a tcp_conn_t */
void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store, int num_slices, order_source_t *order_source) {
    /* Object that we pass around to subroutines (is there a better way to do this?) */
    txt_memcached_handler_t rh(conn, get_store, set_store, num_slices);

    handle_memcache(&rh, order_source);
}

void import_memcache(std::string filename, set_store_interface_t *set_store, int num_slices, order_source_t *order_source) {
    /* Object that we pass around to subroutines (is there a better way to do this?) - copy pasta */
    txt_memcached_file_importer_t::dummy_get_store_t dummy_get_store;
    txt_memcached_file_importer_t rh(filename, &dummy_get_store, set_store, num_slices);

    handle_memcache(&rh, order_source);
}


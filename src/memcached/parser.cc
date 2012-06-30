#include "memcached/parser.hpp"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdexcept>
#include <stdarg.h>
#include <unistd.h>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

#include <vector>
#include <set>

#include "concurrency/coro_fifo.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/semaphore.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/promise.hpp"
#include "containers/buffer_group.hpp"
#include "containers/iterators.hpp"
#include "containers/printf_buffer.hpp"
#include "logger.hpp"
#include "arch/os_signal.hpp"
#include "perfmon/collect.hpp"
#include "memcached/stats.hpp"

static const char *crlf = "\r\n";

/* txt_memcached_handler_t only exists as a convenient thing to pass around to do_get(),
do_storage(), and the like. */

struct txt_memcached_handler_t : public home_thread_mixin_t {
    txt_memcached_handler_t(memcached_interface_t *_interface,
                            namespace_interface_t<memcached_protocol_t> *_nsi,
                            int _max_concurrent_queries_per_connection,
                            memcached_stats_t *_stats)
        : interface(_interface), nsi(_nsi), max_concurrent_queries_per_connection(_max_concurrent_queries_per_connection), stats(_stats)
    { }

    memcached_interface_t *interface;

    namespace_interface_t<memcached_protocol_t> *nsi;

    const int max_concurrent_queries_per_connection;

    memcached_stats_t *stats;

    cas_t generate_cas() {
        // TODO we have to do better than this. CASes need to be generated in a
        // way that is very fast but also gives a reasonably good guarantee of
        // uniqueness across time and space.
        return random();
    }

    void write(const std::string& buffer) {
        write(buffer.data(), buffer.length());
    }

    void write(const char *buffer, size_t bytes) {
        interface->write(buffer, bytes);
    }

    void vwritef(const char *format, va_list args) {
        printf_buffer_t<1000> buffer(args, format);
        write(buffer.data(), buffer.size());
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
        writef("VALUE %*.*s %u %zu %" PRIu64 "\r\n",
               int(key_size), int(key_size), key, mcflags, value_size, cas);
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
        printf_buffer_t<1000> buffer(args, format);
        write(buffer.data(), buffer.size());
        va_end(args);
        writef("\r\n");
        debugf("Client request returned SERVER_ERROR %s\n", buffer.data());
    }

    void client_error_bad_command_line_format() {
        client_error("bad command line format\r\n");
    }

    void client_error_bad_data() {
        client_error("bad data chunk\r\n");
    }

    void server_error_object_too_large_for_cache() {
        server_error("object too large for cache");
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

class pipeliner_t {
public:
    explicit pipeliner_t(txt_memcached_handler_t *rh) : requests_out_sem(rh->max_concurrent_queries_per_connection), rh_(rh) { }
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
    txt_memcached_handler_t *rh_;

    DISABLE_COPYING(pipeliner_t);
};

class pipeliner_acq_t {
public:
    explicit pipeliner_acq_t(pipeliner_t *pipeliner)
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
        DEBUG_ONLY_CODE(state_ = has_begun_operation);
        fifo_acq_.enter(&pipeliner_->fifo);
    }

    void done_argparsing() {
        rassert(state_ == has_begun_operation);
        DEBUG_ONLY_CODE(state_ = has_done_argparsing);

        unlock_mutex(&pipeliner_->argparsing_mutex);
        pipeliner_->requests_out_sem.co_lock();
    }

    void begin_write() {
        rassert(state_ == has_done_argparsing);
        DEBUG_ONLY_CODE(state_ = has_begun_write);
        fifo_acq_.leave();
        mutex_acq_.reset(&pipeliner_->mutex);
    }

    void end_write() {
        rassert(state_ == has_begun_write);
        DEBUG_ONLY_CODE(state_ = has_ended_write);

        {
            block_pm_duration flush_timer(&pipeliner_->rh_->stats->pm_conns_writing); // FIXME: race condition here
            pipeliner_->rh_->flush_buffer();
        }

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
/* do_get() is used for "get" and "gets" commands. */

struct get_t {
    store_key_t key;
    get_result_t res;
    std::string error_message;
    bool ok;
};

void do_one_get(txt_memcached_handler_t *rh, bool with_cas, get_t *gets, int i, order_token_t token) {
    try {
        if (with_cas) {
            get_cas_mutation_t get_cas_mutation(gets[i].key);
            memcached_protocol_t::write_t write(get_cas_mutation, rh->generate_cas(), time(NULL));
            cond_t non_interruptor;
            memcached_protocol_t::write_response_t response = rh->nsi->write(write, token, &non_interruptor);
            gets[i].res = boost::get<get_result_t>(response.result);
        } else {
            get_query_t get_query(gets[i].key);
            memcached_protocol_t::read_t read(get_query, time(NULL));
            cond_t non_interruptor;
            memcached_protocol_t::read_response_t response = rh->nsi->read(read, token, &non_interruptor);
            gets[i].res = boost::get<get_result_t>(response.result);
        }
        gets[i].ok = true;
    } catch (cannot_perform_query_exc_t e) {
        gets[i].error_message = e.what();
        gets[i].ok = false;
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
        if (!unescaped_str_to_key(argv[i], strlen(argv[i]), &gets.back().key)) {
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            rh->client_error_bad_command_line_format();
            pipeliner_acq.end_write();
            return;
        }
        rh->stats->pm_get_key_size.record((float) gets.back().key.size());
    }
    if (gets.size() == 0) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->error();
        pipeliner_acq.end_write();
        return;
    }

    pipeliner_acq.done_argparsing();

    block_pm_duration get_timer(&rh->stats->pm_cmd_get);

    /* Now that we're sure they're all valid, send off the requests */
    pmap(gets.size(), boost::bind(&do_one_get, rh, with_cas, gets.data(), _1, token));

    pipeliner_acq.begin_write();

    /* Check if any failed */
    for (int i = 0; i < (int)gets.size(); i++) {
        if (!gets[i].ok) {
            rh->server_error("%s", gets[i].error_message.c_str());
            pipeliner_acq.end_write();
            return;
        }
    }

    /* Handle the results in sequence */
    for (int i = 0; i < (int)gets.size(); i++) {
        get_result_t &res = gets[i].res;

        /* If res.value is NULL that means the value was not found so we don't write
           anything */
        if (res.value) {
            /* If the write half of the connection has been closed, there's no point in trying
               to send anything */
            if (rh->is_write_open()) {
                const store_key_t &key = gets[i].key;

                /* Write the "VALUE ..." header */
                if (with_cas) {
                    rh->write_value_header(reinterpret_cast<const char *>(key.contents()), key.size(), res.flags, res.value->size(), res.cas);
                } else {
                    rassert(res.cas == 0);
                    rh->write_value_header(reinterpret_cast<const char *>(key.contents()), key.size(), res.flags, res.value->size());
                }

                rh->write_from_data_provider(res.value.get());
                rh->write_crlf();
            }
        }
    }

    rh->write_end();

    pipeliner_acq.end_write();
};

static const char *rget_null_key = "null";

static bool rget_parse_bound(char *flag, char *key, key_range_t::bound_t *mode_out, store_key_t *key_out) {
    if (!unescaped_str_to_key(key, strlen(key), key_out)) return false;

    const char *invalid_char;
    int64_t open_flag = strtoi64_strict(flag, &invalid_char, 10);
    if (*invalid_char != '\0') return false;

    switch (open_flag) {
    case 0:
        *mode_out = key_range_t::closed;
        return true;
    case 1:
        *mode_out = key_range_t::open;
        return true;
    case -1:
        if (strcasecmp(rget_null_key, key) == 0) {
            *mode_out = key_range_t::none;
            // key is irrelevant
            return true;
        }
        // Fall through
    default:
        return false;
    }
}

void do_rget(txt_memcached_handler_t *rh, pipeliner_t *pipeliner, int argc, char **argv) {
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
    key_range_t::bound_t left_mode, right_mode;
    store_key_t left_key, right_key;
    if (!rget_parse_bound(argv[3], argv[1], &left_mode, &left_key) ||
        !rget_parse_bound(argv[4], argv[2], &right_mode, &right_key)) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }
    key_range_t range(left_mode, left_key, right_mode, right_key);

    /* Parse max items count */
    const char *invalid_char;
    uint64_t max_items = strtou64_strict(argv[5], &invalid_char, 10);
    if (*invalid_char != '\0') {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }

    pipeliner_acq.done_argparsing();

    block_pm_duration rget_timer(&rh->stats->pm_cmd_rget);

    pipeliner_acq.begin_write();

    std::string error_message;
    try {
        /* Range scans can potentially request enormous amounts of data. To
        avoid having to store all that data in memory at once or transfer it all
        over the network at once, we break the range scan into many
        sub-requests. `rget_query_t` will automatically stop the range scan once
        it has read `rget_max_chunk_size` bytes of data. When we get the
        `rget_result_t` back, if the query was truncated, we dispatch another
        `rget_query_t` starting where the last one left off. */

        /* The naive approach has a problem, though. Suppose that we request a
        range from 'a' to 'z', and the database is sharded at 'm'. Both the
        'a'-'m' shard and the 'm'-'z' shard will get a request. Because the
        semantics of `rget_query_t` are that a `rget_max_chunk_size`-sized chunk
        will be returned, and the `m'-'z' shard doesn't know how much data might
        be present in the 'a'-'m' shard, both shards return an
        `rget_max_chunk_size`-sized chunk of data, and then the `unshard()`
        function discards the second one. This means that the rget internally
        transfers (m + 1)/2 times as much data as it should have to, where "m"
        is the number of shards. To avoid this, we need to be aware of the
        sharding scheme and not dispatch `rget_query_t`s that cross shard
        boundaries */
        const std::set< hash_region_t<key_range_t> > shards = rh->nsi->get_sharding_scheme();

        /* Check that we have a valid set of hash ranges, that don't use their hashing parts. */
        {
            key_range_t::right_bound_t bound(store_key_t::min());
            for (std::set< hash_region_t<key_range_t> >::const_iterator it = shards.begin(); it != shards.end(); it++) {
                guarantee(!bound.unbounded);
                guarantee(it->beg == 0);
                guarantee(it->end == HASH_REGION_HASH_SIZE);
                guarantee(bound.key == it->inner.left);
                bound = it->inner.right;
            }
            rassert(bound.unbounded);
        }

        /* Find the shard that we're going to start in. */
        std::set<hash_region_t<key_range_t> >::const_iterator shard_it = shards.begin();
        while (!shard_it->inner.contains_key(range.left)) {
            ++shard_it;
        }

        while (max_items > 0) {
            rget_query_t rget_query(region_intersection(range, shard_it->inner), max_items);
            memcached_protocol_t::read_t read(rget_query, time(NULL));
            cond_t non_interruptor;
            memcached_protocol_t::read_response_t response = rh->nsi->read(read, order_token_t::ignore, &non_interruptor);
            rget_result_t results = boost::get<rget_result_t>(response.result);

            for (std::vector<key_with_data_buffer_t>::iterator it = results.pairs.begin(); it != results.pairs.end(); it++) {
                rh->write_value_header(reinterpret_cast<const char *>(it->key.contents()), it->key.size(), it->mcflags, it->value_provider->size());
                rh->write_from_data_provider(it->value_provider.get());
                rh->write_crlf();
            }

            if (results.truncated) {
                /* This round of the range scan stopped because the chunk was
                getting too big. We need to submit another range scan with the left
                key equal to the rightmost key of the results we got */
                rassert(!results.pairs.empty());
                range.left = results.pairs.back().key;
                range.left.increment();
            } else {
                /* This round of the range scan stopped for some other reason... */
                if (shard_it->inner.right < range.right) {
                    /* The `rget_query_t` hit the right-hand bound of the range we
                    passed to it. But because we were filtering by shard, the
                    right-hand bound of the range we passed to it isn't the
                    right-hand bound of the range scan as a whole. Switch to the
                    next shard and keep going. (It's also possible that we hit
                    the maximum number of rows that the user requested, but in that
                    case the `while` loop will stop, so we don't have to distinguish
                    between the two cases.) */
                    ++shard_it;
                    range.left = shard_it->inner.left;
                } else {
                    /* The `rget_query_t` hit the right-hand bound of the range we
                    passed to it. Because the right-hand bound of the range scan
                    as a whole lies within the shard we're currently at, the
                    right-hand bound we passed to the `rget_query_t` is the
                    right-hand bound of the range scan as a whole. So abort. (As
                    above, it's also possible that we hit `max_items`.) */
                    break;
                }
            }

            rassert(results.pairs.size() <= max_items);
            max_items -= results.pairs.size();
        }
        rh->write_end();

    } catch (cannot_perform_query_exc_t e) {
        /* We can't call `server_error()` directly from within here because
        it's not safe to call `coro_t::wait()` from inside a `catch`-block.
        */
        error_message = e.what();
    }

    if (error_message != "") {
        rh->server_error("%s", error_message.c_str());
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
                         const intrusive_ptr_t<data_buffer_t>& data,
                         storage_metadata_t metadata,
                         bool noreply,
                         order_token_t token) {

    boost::scoped_ptr<pipeliner_acq_t> pipeliner_acq(pipeliner_acq_raw);

    block_pm_duration set_timer(&rh->stats->pm_cmd_set);

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

        set_result_t res;
        std::string error_message;
        bool ok;

        try {
            sarc_mutation_t sarc_mutation(key, data, metadata.mcflags, metadata.exptime,
                add_policy, replace_policy, metadata.unique);
            memcached_protocol_t::write_t write(sarc_mutation, rh->generate_cas(), time(NULL));
            cond_t non_interruptor;
            memcached_protocol_t::write_response_t result = rh->nsi->write(write, token, &non_interruptor);
            res = boost::get<set_result_t>(result.result);
            ok = true;
        } catch (cannot_perform_query_exc_t e) {
            error_message = e.what();
            ok = false;
        }

        pipeliner_acq->begin_write();

        if (!noreply) {
            if (ok) {
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
                default: unreachable();
                }
            } else {
                rh->server_error("%s", error_message.c_str());
            }
        }

    } else {
        append_prepend_result_t res;
        std::string error_message;
        bool ok;

        try {
            append_prepend_mutation_t append_prepend_mutation(
                sc == append_command ? append_prepend_APPEND : append_prepend_PREPEND,
                key, data);
            memcached_protocol_t::write_t write(append_prepend_mutation, rh->generate_cas(), time(NULL));
            cond_t non_interruptor;
            memcached_protocol_t::write_response_t result = rh->nsi->write(write, token, &non_interruptor);
            res = boost::get<append_prepend_result_t>(result.result);
            ok = true;
        } catch (cannot_perform_query_exc_t e) {
            error_message = e.what();
            ok = false;
        }

        pipeliner_acq->begin_write();

        if (!noreply) {
            if (ok) {
                switch (res) {
                case apr_success: rh->writef("STORED\r\n"); break;
                case apr_not_found: rh->writef("NOT_FOUND\r\n"); break;
                case apr_too_large:
                    rh->server_error_object_too_large_for_cache();
                    break;
                default: unreachable();
                }
            } else {
                rh->server_error("%s", error_message.c_str());
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

    const char *invalid_char;

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
    if (!unescaped_str_to_key(argv[1], strlen(argv[1]), &key)) {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }
    rh->stats->pm_storage_key_size.record((float) key.size());

    /* Next parse the flags */
    mcflags_t mcflags = strtou64_strict(argv[2], &invalid_char, 10);
    if (*invalid_char != '\0') {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }

    /* Now parse the expiration time */
    exptime_t exptime = strtou64_strict(argv[3], &invalid_char, 10);
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
    size_t value_size = strtou64_strict(argv[4], &invalid_char, 10);
    // Check for signed 32 bit max value for Memcached compatibility...
    if (*invalid_char != '\0' || value_size >= (1u << 31) - 1) {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }
    rh->stats->pm_storage_value_size.record((float) value_size);

    /* If a "cas", parse the cas_command unique */
    cas_t unique = NO_CAS_SUPPLIED;
    if (sc == cas_command) {
        unique = strtou64_strict(argv[5], &invalid_char, 10);
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
    intrusive_ptr_t<data_buffer_t> dp = data_buffer_t::create(value_size);
    char crlf_buf[2];
    try {
        rh->read(dp->buf(), value_size);
        rh->read(crlf_buf, 2);
    } catch (memcached_interface_t::no_more_data_exc_t) {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error_bad_data();
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }

    if (memcmp(crlf_buf, crlf, 2) != 0) {
        pipeliner_acq->done_argparsing();
        pipeliner_acq->begin_write();
        rh->client_error("bad data chunk\r\n");
        pipeliner_acq->end_write();
        delete pipeliner_acq;
        return;
    }

    /* Bundle metadata into one object so we don't try to pass too many arguments
    to `boost::bind()` */
    storage_metadata_t metadata(mcflags, exptime, unique);

    pipeliner_acq->done_argparsing();

    coro_t::spawn_now_deprecated(boost::bind(&run_storage_command, rh, pipeliner_acq, sc, key, dp, metadata, noreply, token));
}

/* "incr" and "decr" commands */
void run_incr_decr(txt_memcached_handler_t *rh, pipeliner_acq_t *pipeliner_acq, store_key_t key, uint64_t amount, bool incr, bool noreply, order_token_t token) {
    block_pm_duration set_timer(&rh->stats->pm_cmd_set);

    incr_decr_result_t res;
    std::string error_message;
    bool ok;

    try {
        incr_decr_mutation_t incr_decr_mutation(
            incr ? incr_decr_INCR : incr_decr_DECR,
            key, amount);
        memcached_protocol_t::write_t write(incr_decr_mutation, rh->generate_cas(), time(NULL));
        cond_t non_interruptor;
        memcached_protocol_t::write_response_t result = rh->nsi->write(write, token, &non_interruptor);
        res = boost::get<incr_decr_result_t>(result.result);
        ok = true;
    } catch (cannot_perform_query_exc_t e) {
        error_message = e.what();
        res = incr_decr_result_t();   /* shut up compiler warnings */
        ok = false;
    }

    pipeliner_acq->begin_write();
    if (!noreply) {
        if (ok) {
            switch (res.res) {
                case incr_decr_result_t::idr_success:
                    rh->writef("%" PRIu64 "\r\n", res.new_value);
                    break;
                case incr_decr_result_t::idr_not_found:
                    rh->writef("NOT_FOUND\r\n");
                    break;
                case incr_decr_result_t::idr_not_numeric:
                    rh->client_error("cannot increment or decrement non-numeric value\r\n");
                    break;
                default: unreachable();
            }
        } else {
            rh->server_error("%s", error_message.c_str());
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
    if (!unescaped_str_to_key(argv[1], strlen(argv[1]), &key)) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }

    /* Parse amount to change by */
    const char *invalid_char;
    uint64_t delta = strtou64_strict(argv[2], &invalid_char, 10);
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

    block_pm_duration set_timer(&rh->stats->pm_cmd_set);

    delete_result_t res;
    std::string error_message;
    bool ok;

    try {
        delete_mutation_t delete_mutation(key, false);
        memcached_protocol_t::write_t write(delete_mutation, INVALID_CAS, time(NULL));
        cond_t non_interruptor;
        memcached_protocol_t::write_response_t result = rh->nsi->write(write, token, &non_interruptor);
        res = boost::get<delete_result_t>(result.result);
        ok = true;
    } catch (cannot_perform_query_exc_t e) {
        error_message = e.what();
        ok = false;
    }

    pipeliner_acq->begin_write();
    if (!noreply) {
        if (ok) {
            switch (res) {
                case dr_deleted:
                    rh->writef("DELETED\r\n");
                    break;
                case dr_not_found:
                    rh->writef("NOT_FOUND\r\n");
                    break;
                default: unreachable();
            }
        } else {
            rh->server_error("%s", error_message.c_str());
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
    if (!unescaped_str_to_key(argv[1], strlen(argv[1]), &key)) {
        pipeliner_acq.done_argparsing();
        pipeliner_acq.begin_write();
        rh->client_error_bad_command_line_format();
        pipeliner_acq.end_write();
        return;
    }
    rh->stats->pm_delete_key_size.record((float) key.size());

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

void format_stats(const perfmon_result_t *stats, const std::string& name, const std::set<std::string>& names_to_match, std::vector<std::string>& result) {
    // `switch` is used instead of `if` with `is_map` and `is_string` checks
    // because that way the compiler guarantees us an error message if someone
    // adds another type of `perfmon_results_t` and forgets to change this code
    switch (stats->get_type()) {
        case perfmon_result_t::type_value:
             // This is not super-efficient (better to only scan for the stats
             // that match the name), but we don't care right now
            if (names_to_match.empty() || names_to_match.count(name) != 0) {
                result.push_back(strprintf("STAT %s %s\r\n", name.c_str(), stats->get_string()->c_str()));
            }
            break;
        case perfmon_result_t::type_map:
            for (perfmon_result_t::const_iterator i = stats->begin(); i != stats->end(); i++) {
                std::string sub_name(name.empty() ? i->first : name + "." + i->first);
                format_stats(i->second, sub_name, names_to_match, result);
            }
            break;
        default:
            unreachable("Bad perfmon result type");
    }
}

void memcached_stats(int argc, char **argv, std::vector<std::string>& stat_response_lines) {
    static const std::string end_marker("END\r\n");

    // parse args, if any
    std::set<std::string> names_to_match;
    for (int i = 1; i < argc; i++) {
        names_to_match.insert(argv[i]);
    }

    boost::scoped_ptr<perfmon_result_t> stats(perfmon_get_stats());
    format_stats(stats.get(), std::string(), names_to_match, stat_response_lines);
    stat_response_lines.push_back(end_marker);
}

/* Handle memcached, takes a txt_memcached_handler_t and handles the memcached commands that come in on it */
void handle_memcache(memcached_interface_t *interface, namespace_interface_t<memcached_protocol_t> *nsi, int max_concurrent_queries_per_connection, memcached_stats_t *stats) {
    logDBG("Opened memcached stream: %p", coro_t::self());

    /* This object just exists to group everything together so we don't have to pass a lot of
    context around. */
    txt_memcached_handler_t rh(interface, nsi, max_concurrent_queries_per_connection, stats);

    /* The commands from each individual memcached handler must be performed in the order
    that the handler parses them. This `order_source_t` is used to guarantee that. */
    order_source_t order_source;

    /* Declared outside the while-loop so it doesn't repeatedly reallocate its buffer */
    std::vector<char> line;
    std::vector<char*> args;

    pipeliner_t pipeliner(&rh);

    while (pipeliner.lock_argparsing(), true) {
        /* Read a line off the socket */
        block_pm_duration read_timer(&rh.stats->pm_conns_reading);
        try {
            rh.read_line(&line);
        } catch (memcached_interface_t::no_more_data_exc_t) {
            break;
        }
        read_timer.end();

        block_pm_duration action_timer(&rh.stats->pm_conns_acting);

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
            rh.error();
            pipeliner_acq.end_write();
            continue;
        }

        /* Dispatch to the appropriate subclass */
        order_token_t token = order_source.check_in(std::string("handle_memcache+") + args[0]);
        if (!strcmp(args[0], "get")) {    // check for retrieval commands
            coro_t::spawn_now_deprecated(boost::bind(do_get, &rh, &pipeliner, false, args.size(), args.data(), token.with_read_mode()));
        } else if (!strcmp(args[0], "gets")) {
            coro_t::spawn_now_deprecated(boost::bind(do_get, &rh, &pipeliner, true, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "rget")) {
            coro_t::spawn_now_deprecated(boost::bind(do_rget, &rh, &pipeliner, args.size(), args.data()));
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
            coro_t::spawn_now_deprecated(boost::bind(do_delete, &rh, &pipeliner, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "incr")) {
            coro_t::spawn_now_deprecated(boost::bind(do_incr_decr, &rh, &pipeliner, true, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "decr")) {
            coro_t::spawn_now_deprecated(boost::bind(do_incr_decr, &rh, &pipeliner, false, args.size(), args.data(), token));
        } else if (!strcmp(args[0], "quit")) {
            // Make sure there's no more tokens (the kind in args, not
            // order tokens)
            if (args.size() > 1) {
                pipeliner_acq_t pipeliner_acq(&pipeliner);
                pipeliner_acq.begin_operation();
                // We block everybody, but who cares?
                pipeliner_acq.done_argparsing();
                pipeliner_acq.begin_write();
                rh.error();
                pipeliner_acq.end_write();
            } else {
                break;
            }
        } else if (!strcmp(args[0], "stats") || !strcmp(args[0], "stat")) {
            pipeliner_acq_t pipeliner_acq(&pipeliner);
            pipeliner_acq.begin_operation();

            std::vector<std::string> stat_response_lines;
            memcached_stats(args.size(), args.data(), stat_response_lines);

            // We block everybody before writing.  I don't think we care.
            pipeliner_acq.done_argparsing();
            pipeliner_acq.begin_write();
            for (std::vector<std::string>::const_iterator i = stat_response_lines.begin(); i != stat_response_lines.end(); ++i) {
                rh.write(*i);
            }
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
        } else {
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

    logDBG("Closed memcached stream: %p", coro_t::self());
}


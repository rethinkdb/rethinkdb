#ifndef RDB_PROTOCOL_DATUM_STREAM_UNION_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_UNION_HPP_

#include "rdb_protocol/datum_stream.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"

namespace ql {

class coro_stream_t;

class union_datum_stream_t : public datum_stream_t, public home_thread_mixin_t {
public:
    union_datum_stream_t(env_t *env,
                         std::vector<counted_t<datum_stream_t> > &&_streams,
                         backtrace_id_t bt,
                         size_t expected_states = 0);

    virtual void add_transformation(transform_variant_t &&tv,
                                    backtrace_id_t bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    virtual bool is_array() const;
    virtual datum_t as_array(env_t *env);
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const;
    virtual bool is_infinite() const;

private:
    friend class coro_stream_t;

    virtual std::vector<changespec_t> get_changespecs();
    std::vector<datum_t >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    // We need to keep these around to apply transformations to even though we
    // spawn coroutines to read from them.
    std::vector<scoped_ptr_t<coro_stream_t> > coro_streams;
    feed_type_t union_type;
    bool is_infinite_union;

    // Set during construction.
    scoped_ptr_t<profile::trace_t> trace;
    scoped_ptr_t<profile::disabler_t> disabler;
    scoped_ptr_t<env_t> coro_env;
    // Set the first time `next_batch_impl` is called.
    scoped_ptr_t<batchspec_t> coro_batchspec;

    bool sent_init;
    size_t ready_needed;

    // A coro pool for launching reads on the individual coro_streams.
    // If the union is not a changefeed, coro_stream_t::maybe_launch_read() is going
    // to put reads into `read_queue` which will then be processed by `read_coro_pool`.
    // This is to limit the degree of parallelism if a union stream is created
    // over a large number of substreams (like in a getAll with many arguments).
    // If the union is a changefeed, we must launch parallel reads on all streams,
    // and this is not used (instead coro_stream_t::maybe_launch_read() will launch
    // a coroutine directly).
    unlimited_fifo_queue_t<std::function<void()> > read_queue;
    calling_callback_t read_coro_callback;
    coro_pool_t<std::function<void()> > read_coro_pool;

    size_t active;
    // We recompute this only when `next_batch_impl` returns to retain the
    // invariant that a stream won't change from unexhausted to exhausted
    // without attempting to read more from it.
    bool coros_exhausted;
    promise_t<std::exception_ptr> abort_exc;
    scoped_ptr_t<cond_t> data_available;

    std::queue<std::vector<datum_t> > queue; // FIFO

    auto_drainer_t drainer;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_UNION_HPP_

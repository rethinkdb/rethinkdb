// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_STREAM_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_HPP_

#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/real_table.hpp"
#include "rdb_protocol/shards.hpp"

namespace ql {

class env_t;
class scope_env_t;
class func_t;
class response_t;

enum class return_empty_normal_batches_t { NO, YES };

enum class feed_type_t { not_feed, point, stream, orderby_limit, unioned };

// Handle unions of changefeeds; if there's no plausible unioned type then we
// just return `feed_type_t::unioned`.
inline feed_type_t union_of(feed_type_t a, feed_type_t b) {
    switch (a) {
    case feed_type_t::stream: // fallthru
    case feed_type_t::point: // fallthru
    case feed_type_t::orderby_limit:
        return (b == a || b == feed_type_t::not_feed) ? a : feed_type_t::unioned;
    case feed_type_t::unioned: return a;
    case feed_type_t::not_feed: return b;
    default: unreachable();
    }
    unreachable();
}

struct active_state_t {
    std::map<uuid_u, std::pair<key_range_t, uint64_t> > shard_last_read_stamps;
    boost::optional<reql_version_t> reql_version; // none for pkey
    DEBUG_ONLY(boost::optional<std::string> sindex;)
};

struct changespec_t {
    changespec_t(changefeed::keyspec_t _keyspec,
                 counted_t<datum_stream_t> _stream)
        : keyspec(std::move(_keyspec)),
          stream(std::move(_stream)) { }
    changefeed::keyspec_t keyspec;
    counted_t<datum_stream_t> stream;
};

class datum_stream_t : public single_threaded_countable_t<datum_stream_t>,
                       public bt_rcheckable_t {
public:
    virtual ~datum_stream_t() { }
    virtual void set_notes(response_t *) const { }

    virtual std::vector<changespec_t> get_changespecs() = 0;
    virtual void add_transformation(transform_variant_t &&tv, backtrace_id_t bt) = 0;
    virtual bool add_stamp(changefeed_stamp_t stamp);
    virtual boost::optional<active_state_t> get_active_state();
    void add_grouping(transform_variant_t &&tv,
                      backtrace_id_t bt);

    scoped_ptr_t<val_t> run_terminal(env_t *env, const terminal_variant_t &tv);
    scoped_ptr_t<val_t> to_array(env_t *env);

    // stream -> stream (always eager)
    counted_t<datum_stream_t> slice(size_t l, size_t r);
    counted_t<datum_stream_t> offsets_of(counted_t<const func_t> f);
    counted_t<datum_stream_t> ordered_distinct();

    // Returns false or NULL respectively if stream is lazy.
    virtual bool is_array() const = 0;
    virtual datum_t as_array(env_t *env) = 0;

    bool is_grouped() const { return grouped; }

    // Gets the next elements from the stream.  (Returns zero elements only when
    // the end of the stream has been reached.  Otherwise, returns at least one
    // element.)  (Wrapper around `next_batch_impl`.)
    std::vector<datum_t>
    next_batch(env_t *env, const batchspec_t &batchspec);
    // Prefer `next_batch`.  Cannot be used in conjunction with `next_batch`.
    virtual datum_t next(env_t *env, const batchspec_t &batchspec);
    virtual bool is_exhausted() const = 0;
    virtual feed_type_t cfeed_type() const = 0;
    virtual bool is_infinite() const = 0;

    virtual void accumulate(
        env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) = 0;
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;

protected:
    bool batch_cache_exhausted() const;
    void check_not_grouped(const char *msg);
    explicit datum_stream_t(backtrace_id_t bt);

private:
    virtual std::vector<datum_t>
    next_batch_impl(env_t *env, const batchspec_t &batchspec) = 0;

    std::vector<datum_t> batch_cache;
    size_t batch_cache_index;
    bool grouped;
};

class eager_datum_stream_t : public datum_stream_t {
protected:
    explicit eager_datum_stream_t(backtrace_id_t bt)
        : datum_stream_t(bt) { }
    virtual datum_t as_array(env_t *env);
    bool ops_to_do() { return ops.size() != 0; }

protected:
    virtual std::vector<changespec_t> get_changespecs() {
        rfail(base_exc_t::LOGIC, "%s", "Cannot call `changes` on an eager stream.");
    }
    std::vector<transform_variant_t> transforms;

    virtual void add_transformation(transform_variant_t &&tv,
                                    backtrace_id_t bt);

private:
    enum class done_t { YES, NO };

    virtual bool is_array() const = 0;

    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    done_t next_grouped_batch(env_t *env, const batchspec_t &bs, groups_t *out);
    virtual std::vector<datum_t>
    next_batch_impl(env_t *env, const batchspec_t &bs);
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &bs) = 0;

    std::vector<scoped_ptr_t<op_t> > ops;
};

class wrapper_datum_stream_t : public eager_datum_stream_t {
protected:
    explicit wrapper_datum_stream_t(counted_t<datum_stream_t> _source)
        : eager_datum_stream_t(_source->backtrace()), source(_source) { }
private:
    virtual bool is_array() const { return source->is_array(); }
    virtual datum_t as_array(env_t *env) {
        return source->is_array() && !source->is_grouped()
            ? eager_datum_stream_t::as_array(env)
            : datum_t();
    }
    virtual bool is_exhausted() const {
        return source->is_exhausted() && batch_cache_exhausted();
    }
    virtual feed_type_t cfeed_type() const {
        return source->cfeed_type();
    }
    virtual bool is_infinite() const {
        return source->is_infinite();
    }

protected:
    const counted_t<datum_stream_t> source;
};

class offsets_of_datum_stream_t : public wrapper_datum_stream_t {
public:
    offsets_of_datum_stream_t(counted_t<const func_t> _f, counted_t<datum_stream_t> _source);

private:
    std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    counted_t<const func_t> f;
    int64_t index;
};

class ordered_distinct_datum_stream_t : public wrapper_datum_stream_t {
public:
    explicit ordered_distinct_datum_stream_t(counted_t<datum_stream_t> _source);
private:
    std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
    datum_t last_val;
};

class array_datum_stream_t : public eager_datum_stream_t {
public:
    array_datum_stream_t(datum_t _arr,
                         backtrace_id_t bt);
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const;
    virtual bool is_infinite() const;

private:
    virtual bool is_array() const;
    virtual datum_t as_array(env_t *env) {
        return is_array()
            ? (ops_to_do() ? eager_datum_stream_t::as_array(env) : arr)
            : datum_t();
    }
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, UNUSED const batchspec_t &batchspec);
    datum_t next(env_t *env, const batchspec_t &batchspec);
    datum_t next_arr_el();

    size_t index;
    datum_t arr;
};

class slice_datum_stream_t : public wrapper_datum_stream_t {
public:
    slice_datum_stream_t(uint64_t left, uint64_t right, counted_t<datum_stream_t> src);
private:
    virtual std::vector<changespec_t> get_changespecs();
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const;
    virtual bool is_infinite() const;
    uint64_t index, left, right;
};

class indexed_sort_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexed_sort_datum_stream_t(
        counted_t<datum_stream_t> stream, // Must be a table with a sorting applied.
        std::function<bool(env_t *,  // NOLINT(readability/casting)
                           profile::sampler_t *,
                           const datum_t &,
                           const datum_t &)> lt_cmp);
private:
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    std::function<bool(env_t *,  // NOLINT(readability/casting)
                       profile::sampler_t *,
                       const datum_t &,
                       const datum_t &)> lt_cmp;
    size_t index;
    std::vector<datum_t> data;
};

struct coro_info_t;
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

class range_datum_stream_t : public eager_datum_stream_t {
public:
    range_datum_stream_t(bool _is_infite_range,
                         int64_t _start,
                         int64_t _stop,
                         backtrace_id_t);

    virtual std::vector<datum_t>
    next_raw_batch(env_t *, const batchspec_t &batchspec);

    virtual bool is_array() const {
        return false;
    }
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const {
        return feed_type_t::not_feed;
    }
    virtual bool is_infinite() const {
        return is_infinite_range;
    }

private:
    bool is_infinite_range;
    int64_t start, stop;
};

class map_datum_stream_t : public eager_datum_stream_t {
public:
    map_datum_stream_t(std::vector<counted_t<datum_stream_t> > &&_streams,
                       counted_t<const func_t> &&_func,
                       backtrace_id_t bt);

    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    virtual bool is_array() const {
        return is_array_map;
    }
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const {
        return union_type;
    }
    virtual bool is_infinite() const {
        return is_infinite_map;
    }

private:
    std::vector<counted_t<datum_stream_t> > streams;
    counted_t<const func_t> func;
    feed_type_t union_type;
    bool is_array_map, is_infinite_map;

    // We need to preserve this between calls because we might have gotten data
    // from a few substreams before having to abort because we timed out on a
    // changefeed stream and return a partial batch.  (We still time out and
    // return partial batches, or even empty batches, for the web UI in the case
    // of a changefeed stream.)
    std::vector<datum_t> args;
};

// Every shard is in a particular state.  ACTIVE means we should read more data
// from it, SATURATED means that there's more data to read but we didn't
// actually use any of the data we read last time so don't bother issuing
// another read, and `EXHAUSTED` means there's no more to read.  (Generally when
// iterating over a stream ordered by the primary key, range shards we've
// already read will be EXHAUSTED, the range shard we're currently reading will
// be ACTIVE, and the range shards we've yet to read will be SATURATED.)
//
// We track these on a per-hash-shard basis because it's easier, but we decide
// whether or not to issue reads on a per-range-shard basis.
enum class range_state_t { ACTIVE, SATURATED, EXHAUSTED };

void debug_print(printf_buffer_t *buf, const range_state_t &rs);
struct hash_range_with_cache_t {
    uuid_u cfeed_shard_id;
    // This is the range of values that we have yet to read from the shard.  We
    // store a range instead of just a `store_key_t` because this range is only
    // a restriction of the indexed range for primary key reads.
    key_range_t key_range;
    raw_stream_t cache; // Entries we weren't able to unshard.
    range_state_t state;
    // No data in the cache and nothing to read from the shards.
    bool totally_exhausted() const;
};
void debug_print(printf_buffer_t *buf, const hash_range_with_cache_t &hrwc);

struct hash_ranges_t {
    std::map<hash_range_t, hash_range_with_cache_t> hash_ranges;
    // Composite state of all hash shards.
    range_state_t state() const;
    // True if all hash shards are totally exhausted.
    bool totally_exhausted() const;
};
void debug_print(printf_buffer_t *buf, const hash_ranges_t &hr);

struct active_ranges_t {
    std::map<key_range_t, hash_ranges_t> ranges;
    bool totally_exhausted() const;
};
void debug_print(printf_buffer_t *buf, const active_ranges_t &ar);

// This class generates the `read_t`s used in range reads.  It's used by
// `reader_t` below.  Its subclasses are the different types of range reads we
// need to do.
class readgen_t {
public:
    explicit readgen_t(
        global_optargs_t global_optargs,
        std::string table_name,
        profile_bool_t profile,
        read_mode_t read_mode,
        sorting_t sorting);
    virtual ~readgen_t() { }

    virtual read_t terminal_read(
        const std::vector<transform_variant_t> &transform,
        const terminal_variant_t &_terminal,
        const batchspec_t &batchspec) const = 0;
    // This has to be on `readgen_t` because we sort differently depending on
    // the kinds of reads we're doing.
    virtual void sindex_sort(std::vector<rget_item_t> *vec,
                             const batchspec_t &batchspec) const = 0;

    virtual read_t next_read(
        const boost::optional<active_ranges_t> &active_ranges,
        const boost::optional<reql_version_t> &reql_version,
        boost::optional<changefeed_stamp_t> stamp,
        std::vector<transform_variant_t> transform,
        const batchspec_t &batchspec) const = 0;

    virtual key_range_t original_keyrange(reql_version_t rv) const = 0;
    virtual void restrict_active_ranges(
        sorting_t sorting, active_ranges_t *ranges_inout) const = 0;
    virtual boost::optional<std::string> sindex_name() const = 0;

    virtual changefeed::keyspec_t::range_t get_range_spec(
        std::vector<transform_variant_t>) const = 0;

    const std::string &get_table_name() const { return table_name; }
    read_mode_t get_read_mode() const { return read_mode; }
    // Returns `sorting_` unless the batchspec overrides it.
    sorting_t sorting(const batchspec_t &batchspec) const;
protected:
    const global_optargs_t global_optargs;
    const std::string table_name;
    const profile_bool_t profile;
    const read_mode_t read_mode;
    const sorting_t sorting_;
};

class rget_readgen_t : public readgen_t {
public:
    explicit rget_readgen_t(
        global_optargs_t global_optargs,
        std::string table_name,
        const datumspec_t &datumspec,
        profile_bool_t profile,
        read_mode_t read_mode,
        sorting_t sorting);

    virtual read_t terminal_read(
        const std::vector<transform_variant_t> &transform,
        const terminal_variant_t &_terminal,
        const batchspec_t &batchspec) const;

    virtual read_t next_read(
        const boost::optional<active_ranges_t> &active_ranges,
        const boost::optional<reql_version_t> &reql_version,
        boost::optional<changefeed_stamp_t> stamp,
        std::vector<transform_variant_t> transform,
        const batchspec_t &batchspec) const;

private:
    virtual rget_read_t next_read_impl(
        const boost::optional<active_ranges_t> &active_ranges,
        const boost::optional<reql_version_t> &reql_version,
        boost::optional<changefeed_stamp_t> stamp,
        std::vector<transform_variant_t> transforms,
        const batchspec_t &batchspec) const = 0;

protected:
    datumspec_t datumspec;
};

class primary_readgen_t : public rget_readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        std::string table_name,
        read_mode_t read_mode,
        const datumspec_t &datumspec = datumspec_t(datum_range_t::universe()),
        sorting_t sorting = sorting_t::UNORDERED);

private:
    primary_readgen_t(global_optargs_t global_optargs,
                      std::string table_name,
                      const datumspec_t &datumspec,
                      profile_bool_t profile,
                      read_mode_t read_mode,
                      sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const boost::optional<active_ranges_t> &active_ranges,
        const boost::optional<reql_version_t> &reql_version,
        boost::optional<changefeed_stamp_t> stamp,
        std::vector<transform_variant_t> transform,
        const batchspec_t &batchspec) const;
    virtual void sindex_sort(std::vector<rget_item_t> *vec,
                             const batchspec_t &batchspec) const;
    virtual key_range_t original_keyrange(reql_version_t rv) const;
    virtual boost::optional<std::string> sindex_name() const;
    void restrict_active_ranges(
        sorting_t sorting, active_ranges_t *active_ranges_inout) const final;

    virtual changefeed::keyspec_t::range_t get_range_spec(
            std::vector<transform_variant_t> transforms) const;

    boost::optional<std::map<store_key_t, uint64_t> > store_keys;
};

class sindex_readgen_t : public rget_readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        std::string table_name,
        read_mode_t read_mode,
        const std::string &sindex,
        const datumspec_t &datumspec = datumspec_t(datum_range_t::universe()),
        sorting_t sorting = sorting_t::UNORDERED);

    virtual void sindex_sort(std::vector<rget_item_t> *vec,
                             const batchspec_t &batchspec) const;
    virtual key_range_t original_keyrange(reql_version_t rv) const;
    virtual boost::optional<std::string> sindex_name() const;
    void restrict_active_ranges(sorting_t, active_ranges_t *) const final { }
private:
    sindex_readgen_t(
        global_optargs_t global_optargs,
        std::string table_name,
        const std::string &sindex,
        const datumspec_t &datumspec,
        profile_bool_t profile,
        read_mode_t read_mode,
        sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const boost::optional<active_ranges_t> &active_ranges,
        const boost::optional<reql_version_t> &reql_version,
        boost::optional<changefeed_stamp_t> stamp,
        std::vector<transform_variant_t> transform,
        const batchspec_t &batchspec) const;

    virtual changefeed::keyspec_t::range_t get_range_spec(
            std::vector<transform_variant_t> transforms) const;

    const std::string sindex;
    bool sent_first_read;
};

// For geospatial intersection queries
class intersecting_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        std::string table_name,
        read_mode_t read_mode,
        const std::string &sindex,
        const datum_t &query_geometry);

    virtual read_t terminal_read(
        const std::vector<transform_variant_t> &transform,
        const terminal_variant_t &_terminal,
        const batchspec_t &batchspec) const;

    virtual read_t next_read(
        const boost::optional<active_ranges_t> &active_ranges,
        const boost::optional<reql_version_t> &reql_version,
        boost::optional<changefeed_stamp_t> stamp,
        std::vector<transform_variant_t> transform,
        const batchspec_t &batchspec) const;

    virtual void sindex_sort(std::vector<rget_item_t> *vec,
                             const batchspec_t &batchspec) const;
    virtual key_range_t original_keyrange(reql_version_t rv) const;
    virtual boost::optional<std::string> sindex_name() const;
    void restrict_active_ranges(sorting_t, active_ranges_t *) const final { }

    virtual changefeed::keyspec_t::range_t get_range_spec(
        std::vector<transform_variant_t>) const {
        rfail_datum(base_exc_t::LOGIC,
                    "%s", "Cannot call `changes` on an intersection read.");
        unreachable();
    }
private:
    intersecting_readgen_t(
        global_optargs_t global_optargs,
        std::string table_name,
        const std::string &sindex,
        const datum_t &query_geometry,
        profile_bool_t profile,
        read_mode_t read_mode);

    // Analogue to rget_readgen_t::next_read_impl(), but generates an intersecting
    // geo read.
    intersecting_geo_read_t next_read_impl(
        const boost::optional<active_ranges_t> &active_ranges,
        const boost::optional<reql_version_t> &reql_version,
        boost::optional<changefeed_stamp_t> stamp,
        std::vector<transform_variant_t> transforms,
        const batchspec_t &batchspec) const;

    const std::string sindex;
    const datum_t query_geometry;
};

class reader_t {
public:
    virtual ~reader_t() { }
    virtual void add_transformation(transform_variant_t &&tv) = 0;
    virtual bool add_stamp(changefeed_stamp_t stamp) = 0;
    virtual boost::optional<active_state_t> get_active_state() = 0;
    virtual void accumulate(env_t *env, eager_acc_t *acc,
                            const terminal_variant_t &tv) = 0;
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;
    virtual std::vector<datum_t> next_batch(
        env_t *env, const batchspec_t &batchspec) = 0;
    virtual bool is_finished() const = 0;

    virtual changefeed::keyspec_t get_changespec() const = 0;
};

// To handle empty range on getAll
class empty_reader_t : public reader_t {
public:
    explicit empty_reader_t(counted_t<real_table_t> _table, std::string _table_name)
      : table(std::move(_table)), table_name(std::move(_table_name)) {}
    virtual ~empty_reader_t() {}
    virtual void add_transformation(transform_variant_t &&) {}
    virtual bool add_stamp(changefeed_stamp_t) {
        r_sanity_fail();
    }
    virtual boost::optional<active_state_t> get_active_state() {
        r_sanity_fail();
    }
    virtual void accumulate(env_t *, eager_acc_t *, const terminal_variant_t &) {}
    virtual void accumulate_all(env_t *, eager_acc_t *) {}
    virtual std::vector<datum_t> next_batch(env_t *, const batchspec_t &) {
        return std::vector<datum_t>();
    }
    virtual bool is_finished() const {
        return true;
    }
    virtual changefeed::keyspec_t get_changespec() const;

private:
    counted_t<real_table_t> table;
    std::string table_name;
};

// For reads that generate read_response_t results.
class rget_response_reader_t : public reader_t {
public:
    rget_response_reader_t(
        const counted_t<real_table_t> &table,
        scoped_ptr_t<readgen_t> &&readgen);
    virtual void add_transformation(transform_variant_t &&tv);
    virtual bool add_stamp(changefeed_stamp_t stamp);
    virtual boost::optional<active_state_t> get_active_state();
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;
    virtual std::vector<datum_t> next_batch(env_t *env, const batchspec_t &batchspec);
    virtual bool is_finished() const;

    virtual changefeed::keyspec_t get_changespec() const {
        return changefeed::keyspec_t(
            readgen->get_range_spec(transforms),
            table,
            readgen->get_table_name());
    }

protected:
    raw_stream_t unshard(sorting_t sorting, rget_read_response_t &&res);
    bool shards_exhausted() const {
        return active_ranges ? active_ranges->totally_exhausted() : false;
    }
    void mark_shards_exhausted() {
        r_sanity_check(!active_ranges);
        active_ranges = active_ranges_t();
    }

    // Returns `true` if there's data in `items`.
    // Overwrite this in an implementation
    virtual bool load_items(env_t *env, const batchspec_t &batchspec) = 0;
    rget_read_response_t do_read(env_t *env, const read_t &read);

    counted_t<real_table_t> table;
    std::vector<transform_variant_t> transforms;
    boost::optional<changefeed_stamp_t> stamp;

    bool started;
    const scoped_ptr_t<const readgen_t> readgen;
    boost::optional<active_ranges_t> active_ranges;
    boost::optional<reql_version_t> reql_version;
    std::map<uuid_u, shard_stamp_info_t> shard_stamp_infos;

    // We need this to handle the SINDEX_CONSTANT case.
    std::vector<rget_item_t> items;
    size_t items_index;
};

class rget_reader_t : public rget_response_reader_t {
public:
    rget_reader_t(
        const counted_t<real_table_t> &_table,
        scoped_ptr_t<readgen_t> &&readgen);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

protected:
    // Loads new items into the `items` field of rget_response_reader_t.
    // Returns `true` if there's data in `items`.
    virtual bool load_items(env_t *env, const batchspec_t &batchspec);

private:
    std::vector<rget_item_t> do_range_read(env_t *env, const read_t &read);
};

// intersecting_reader_t performs filtering for duplicate documents in the stream,
// assuming it is read in batches (otherwise that's not necessary, because the
// shards will already provide distinct results).
class intersecting_reader_t : public rget_response_reader_t {
public:
    intersecting_reader_t(
        const counted_t<real_table_t> &_table,
        scoped_ptr_t<readgen_t> &&readgen);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

protected:
    // Loads new items into the `items` field of rget_response_reader_t.
    // Returns `true` if there's data in `items`.
    virtual bool load_items(env_t *env, const batchspec_t &batchspec);

private:
    std::vector<rget_item_t> do_intersecting_read(env_t *env, const read_t &read);

    // To detect duplicates
    std::set<store_key_t> processed_pkeys;
};

class lazy_datum_stream_t : public datum_stream_t {
public:
    lazy_datum_stream_t(
        scoped_ptr_t<reader_t> &&_reader,
        backtrace_id_t bt);

    virtual bool is_array() const { return false; }
    virtual datum_t as_array(UNUSED env_t *env) {
        return datum_t();  // Cannot be converted implicitly.
    }

    bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const;
    virtual bool is_infinite() const;

    virtual bool add_stamp(changefeed_stamp_t stamp) {
        return reader->add_stamp(std::move(stamp));
    }
    virtual boost::optional<active_state_t> get_active_state() {
        return reader->get_active_state();
    }

private:
    virtual std::vector<changespec_t> get_changespecs() {
        return std::vector<changespec_t>{changespec_t(
                reader->get_changespec(), counted_from_this())};
    }

    std::vector<datum_t >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    virtual void add_transformation(transform_variant_t &&tv,
                                    backtrace_id_t bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    // We use these to cache a batch so that `next` works.  There are a lot of
    // things that are most easily written in terms of `next` that would
    // otherwise have to do this caching themselves.
    size_t current_batch_offset;
    std::vector<datum_t> current_batch;

    scoped_ptr_t<reader_t> reader;
};

class vector_datum_stream_t : public eager_datum_stream_t {
public:
    vector_datum_stream_t(
            backtrace_id_t bt,
            std::vector<datum_t> &&_rows,
            boost::optional<changefeed::keyspec_t> &&_changespec);
private:
    datum_t next(env_t *env, const batchspec_t &bs);
    datum_t next_impl(env_t *);
    std::vector<datum_t> next_raw_batch(env_t *env, const batchspec_t &bs);

    void add_transformation(
        transform_variant_t &&tv, backtrace_id_t bt);

    bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const;
    bool is_array() const;
    bool is_infinite() const;

    std::vector<changespec_t> get_changespecs();

    std::vector<datum_t> rows;
    size_t index;
    boost::optional<changefeed::keyspec_t> changespec;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_

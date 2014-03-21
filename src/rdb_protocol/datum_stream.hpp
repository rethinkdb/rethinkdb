// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_STREAM_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_HPP_

#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {

class env_t;

/* This wraps a namespace_interface_t and makes it automatically handle getting
 * profiling information from them. It acheives this by doing the following in
 * its methods:
 * - Set the explain field in the read_t/write_t object so that the shards know
 whether or not to do profiling
 * - Construct a splitter_t
 * - Call the corresponding method on internal_
 * - splitter_t::give_splits with the event logs from the shards
 */
class rdb_namespace_interface_t {
public:
    rdb_namespace_interface_t(
        namespace_interface_t<rdb_protocol_t> *internal, env_t *env);

    void read(const rdb_protocol_t::read_t &,
              rdb_protocol_t::read_response_t *response,
              order_token_t tok,
              signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);
    void read_outdated(const rdb_protocol_t::read_t &,
                       rdb_protocol_t::read_response_t *response,
                       signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);
    void write(rdb_protocol_t::write_t *,
               rdb_protocol_t::write_response_t *response,
               order_token_t tok,
               signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    /* These calls are for the sole purpose of optimizing queries; don't rely
       on them for correctness. They should not block. */
    std::set<rdb_protocol_t::region_t> get_sharding_scheme()
        THROWS_ONLY(cannot_perform_query_exc_t);
    signal_t *get_initial_ready_signal();
    /* Check if the internal value is null. */
    bool has();
private:
    namespace_interface_t<rdb_protocol_t> *internal_;
    env_t *env_;
};

class rdb_namespace_access_t {
public:
    rdb_namespace_access_t(uuid_u id, env_t *env);
    rdb_namespace_interface_t get_namespace_if();
private:
    base_namespace_repo_t<rdb_protocol_t>::access_t internal_;
    env_t *env_;
};

// This is a more selective subset of the list at the top of protocol.cc.
typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::sindex_rangespec_t sindex_rangespec_t;
typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::read_response_t read_response_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;
typedef rdb_protocol_t::region_t region_t;

class scope_env_t;
class datum_stream_t : public single_threaded_countable_t<datum_stream_t>,
                       public pb_rcheckable_t {
public:
    virtual ~datum_stream_t() { }

    virtual counted_t<datum_stream_t> add_transformation(
        env_t *env, transform_variant_t &&tv, const protob_t<const Backtrace> &bt) = 0;
    counted_t<datum_stream_t> add_grouping(
        env_t *env, transform_variant_t &&tv, const protob_t<const Backtrace> &bt);

    counted_t<val_t> run_terminal(env_t *env, const terminal_variant_t &tv);
    counted_t<val_t> to_array(env_t *env);

    // stream -> stream (always eager)
    counted_t<datum_stream_t> slice(size_t l, size_t r);
    counted_t<datum_stream_t> zip();
    counted_t<datum_stream_t> indexes_of(counted_t<func_t> f);

    // Returns false or NULL respectively if stream is lazy.
    virtual bool is_array() = 0;
    virtual counted_t<const datum_t> as_array(env_t *env) = 0;

    bool is_grouped() { return grouped; }

    // Gets the next elements from the stream.  (Returns zero elements only when
    // the end of the stream has been reached.  Otherwise, returns at least one
    // element.)  (Wrapper around `next_batch_impl`.)
    std::vector<counted_t<const datum_t> >
    next_batch(env_t *env, const batchspec_t &batchspec);
    // Prefer `next_batch`.  Cannot be used in conjunction with `next_batch`.
    virtual counted_t<const datum_t> next(env_t *env, const batchspec_t &batchspec);
    virtual bool is_exhausted() const = 0;

    virtual void accumulate(
        env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) = 0;
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;

protected:
    bool batch_cache_exhausted() const;
    void check_not_grouped(const char *msg);
    explicit datum_stream_t(const protob_t<const Backtrace> &bt_src);

private:
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batchspec_t &batchspec) = 0;

    std::vector<counted_t<const datum_t> > batch_cache;
    size_t batch_cache_index;
    bool grouped;
};

class eager_datum_stream_t : public datum_stream_t {
protected:
    explicit eager_datum_stream_t(const protob_t<const Backtrace> &bt)
        : datum_stream_t(bt) { }
    virtual counted_t<const datum_t> as_array(env_t *env);
    bool ops_to_do() { return ops.size() != 0; }

private:
    enum class done_t { YES, NO };

    virtual bool is_array() = 0;

    virtual counted_t<datum_stream_t> add_transformation(
        env_t *env, transform_variant_t &&tv, const protob_t<const Backtrace> &bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    done_t next_grouped_batch(env_t *env, const batchspec_t &bs, groups_t *out);
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batchspec_t &bs);
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &bs) = 0;

    std::vector<scoped_ptr_t<op_t> > ops;
};

class wrapper_datum_stream_t : public eager_datum_stream_t {
protected:
    explicit wrapper_datum_stream_t(counted_t<datum_stream_t> _source)
        : eager_datum_stream_t(_source->backtrace()), source(_source) { }
private:
    virtual bool is_array() { return source->is_array(); }
    virtual counted_t<const datum_t> as_array(env_t *env) {
        return source->is_array() && !source->is_grouped()
            ? eager_datum_stream_t::as_array(env)
            : counted_t<const datum_t>();
    }
    virtual bool is_exhausted() const {
        return source->is_exhausted() && batch_cache_exhausted();
    }

protected:
    const counted_t<datum_stream_t> source;
};

class indexes_of_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexes_of_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    counted_t<func_t> f;
    int64_t index;
};

class array_datum_stream_t : public eager_datum_stream_t {
public:
    array_datum_stream_t(counted_t<const datum_t> _arr,
                         const protob_t<const Backtrace> &bt_src);
    virtual bool is_exhausted() const;

private:
    virtual bool is_array();
    virtual counted_t<const datum_t> as_array(env_t *env) {
        return is_array()
            ? (ops_to_do() ? eager_datum_stream_t::as_array(env) : arr)
            : counted_t<const datum_t>();
    }
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, UNUSED const batchspec_t &batchspec);
    counted_t<const datum_t> next(env_t *env, const batchspec_t &batchspec);
    counted_t<const datum_t> next_arr_el();

    size_t index;
    counted_t<const datum_t> arr;
};

class slice_datum_stream_t : public wrapper_datum_stream_t {
public:
    slice_datum_stream_t(uint64_t left, uint64_t right, counted_t<datum_stream_t> src);
private:
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
    virtual bool is_exhausted() const;
    uint64_t index, left, right;
};

class zip_datum_stream_t : public wrapper_datum_stream_t {
public:
    explicit zip_datum_stream_t(counted_t<datum_stream_t> src);
private:
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
};

class indexed_sort_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexed_sort_datum_stream_t(
        counted_t<datum_stream_t> stream, // Must be a table with a sorting applied.
        std::function<bool(env_t *,  // NOLINT(readability/casting)
                           profile::sampler_t *,
                           const counted_t<const datum_t> &,
                           const counted_t<const datum_t> &)> lt_cmp);
private:
virtual std::vector<counted_t<const datum_t> >
next_raw_batch(env_t *env, const batchspec_t &batchspec);

std::function<bool(env_t *,  // NOLINT(readability/casting)
                   profile::sampler_t *,
                   const counted_t<const datum_t> &,
                   const counted_t<const datum_t> &)> lt_cmp;
size_t index;
std::vector<counted_t<const datum_t> > data;
};

class union_datum_stream_t : public datum_stream_t {
public:
    union_datum_stream_t(std::vector<counted_t<datum_stream_t> > &&_streams,
                         const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src), streams(_streams), streams_index(0) { }

    virtual counted_t<datum_stream_t> add_transformation(
        env_t *env, transform_variant_t &&tv, const protob_t<const Backtrace> &bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    virtual bool is_array();
    virtual counted_t<const datum_t> as_array(env_t *env);
    virtual bool is_exhausted() const;

private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    std::vector<counted_t<datum_stream_t> > streams;
    size_t streams_index;
};

// This class generates the `read_t`s used in range reads.  It's used by
// `reader_t` below.  Its subclasses are the different types of range reads we
// need to do.
class readgen_t {
public:
    explicit readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        const datum_range_t &original_datum_range,
        profile_bool_t profile,
        sorting_t sorting);
    virtual ~readgen_t() { }
    read_t terminal_read(
        const std::vector<transform_variant_t> &transform,
        const terminal_variant_t &_terminal,
        const batchspec_t &batchspec) const;
    // This has to be on `readgen_t` because we sort differently depending on
    // the kinds of reads we're doing.
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const = 0;

    virtual read_t next_read(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;
    // This generates a read that will read as many rows as we need to be able
    // to do an sindex sort, or nothing if no such read is necessary.  Such a
    // read should only be necessary when we're ordering by a secondary index
    // and the last element read has a truncated value for that secondary index.
    virtual boost::optional<read_t> sindex_sort_read(
        const key_range_t &active_range,
        const std::vector<rget_item_t> &items,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const = 0;

    virtual key_range_t original_keyrange() const = 0;
    virtual std::string sindex_name() const = 0; // Used for error checking.

    // Returns `true` if there is no more to read.
    bool update_range(key_range_t *active_range,
                      const store_key_t &last_key) const;
protected:
    const std::map<std::string, wire_func_t> global_optargs;
    const datum_range_t original_datum_range;
    const profile_bool_t profile;
    const sorting_t sorting;

private:
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const = 0;
};

class primary_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = sorting_t::UNORDERED);
private:
    primary_readgen_t(const std::map<std::string, wire_func_t> &global_optargs,
                      datum_range_t range, profile_bool_t profile, sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;
    virtual boost::optional<read_t> sindex_sort_read(
        const key_range_t &active_range,
        const std::vector<rget_item_t> &items,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const;
    virtual key_range_t original_keyrange() const;
    virtual std::string sindex_name() const; // Used for error checking.
};

class sindex_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        const std::string &sindex,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = sorting_t::UNORDERED);
private:
    sindex_readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        const std::string &sindex, datum_range_t sindex_range,
        profile_bool_t profile, sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;
    virtual boost::optional<read_t> sindex_sort_read(
        const key_range_t &active_range,
        const std::vector<rget_item_t> &items,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const;
    virtual key_range_t original_keyrange() const;
    virtual std::string sindex_name() const; // Used for error checking.

    const std::string sindex;
};

class reader_t {
public:
    explicit reader_t(
        const rdb_namespace_access_t &ns_access,
        bool use_outdated,
        scoped_ptr_t<readgen_t> &&readgen);
    void add_transformation(transform_variant_t &&tv);
    void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    void accumulate_all(env_t *env, eager_acc_t *acc);
    std::vector<counted_t<const datum_t> >
    next_batch(env_t *env, const batchspec_t &batchspec);
    bool is_finished() const;
private:
    // Returns `true` if there's data in `items`.
    bool load_items(env_t *env, const batchspec_t &batchspec);
    rget_read_response_t do_read(env_t *env, const read_t &read);
    std::vector<rget_item_t> do_range_read(env_t *env, const read_t &read);

    rdb_namespace_access_t ns_access;
    const bool use_outdated;
    std::vector<transform_variant_t> transforms;

    bool started, shards_exhausted;
    const scoped_ptr_t<const readgen_t> readgen;
    key_range_t active_range;

    // We need this to handle the SINDEX_CONSTANT case.
    std::vector<rget_item_t> items;
    size_t items_index;
};

class lazy_datum_stream_t : public datum_stream_t {
public:
    lazy_datum_stream_t(
        rdb_namespace_access_t *_ns_access,
        bool _use_outdated,
        scoped_ptr_t<readgen_t> &&_readgen,
        const protob_t<const Backtrace> &bt_src);

    virtual bool is_array() { return false; }
    virtual counted_t<const datum_t> as_array(UNUSED env_t *env) {
        return counted_t<const datum_t>();  // Cannot be converted implicitly.
    }

    bool is_exhausted() const;
private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    virtual counted_t<datum_stream_t> add_transformation(
        env_t *env, transform_variant_t &&tv, const protob_t<const Backtrace> &bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    // We use these to cache a batch so that `next` works.  There are a lot of
    // things that are most easily written in terms of `next` that would
    // otherwise have to do this caching themselves.
    size_t current_batch_offset;
    std::vector<counted_t<const datum_t> > current_batch;

    reader_t reader;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_

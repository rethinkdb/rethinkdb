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
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/get.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {

// This is a more selective subset of the list at the top of protocol.cc.
typedef rdb_protocol_details::transform_t transform_t;
typedef rdb_protocol_details::terminal_t terminal_t;
typedef rdb_protocol_details::rget_item_t rget_item_t;
typedef rdb_protocol_details::transform_variant_t transform_variant_t;
typedef rdb_protocol_details::terminal_variant_t terminal_variant_t;
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

    // stream -> stream
    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val) = 0;
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f) = 0;
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f) = 0;

    // stream -> atom
    virtual counted_t<const datum_t> count(env_t *env) = 0;
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f) = 0;
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> d,
                                         counted_t<func_t> r) = 0;

    // stream -> stream (always eager)
    counted_t<datum_stream_t> slice(size_t l, size_t r);
    counted_t<datum_stream_t> zip();
    counted_t<datum_stream_t> indexes_of(counted_t<func_t> f);

    // Returns false or NULL respectively if stream is lazy.
    virtual bool is_array() = 0;
    virtual counted_t<const datum_t> as_array(env_t *env) = 0;

    // Gets the next elements from the stream.  (Returns zero elements only when
    // the end of the stream has been reached.  Otherwise, returns at least one
    // element.)  (Wrapper around `next_batch_impl`.)
    std::vector<counted_t<const datum_t> >
    next_batch(env_t *env, const batcher_t &batcher);
    // Prefer `next_batch`.  Cannot be used in conjunction with `next_batch`.
    virtual counted_t<const datum_t> next(env_t *env, const batcher_t &batcher);
    virtual bool is_exhausted() const = 0;

protected:
    explicit datum_stream_t(const protob_t<const Backtrace> &bt_src);

private:
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher) = 0;
    std::vector<counted_t<const datum_t> > batch_cache;
    size_t batch_cache_index;
};

class eager_datum_stream_t : public datum_stream_t {
protected:
    explicit eager_datum_stream_t(const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src) { }

    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f);
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f);

    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> d,
                                         counted_t<func_t> r);

    virtual bool is_array() = 0;
    virtual counted_t<const datum_t> as_array(env_t *env);
};

class wrapper_datum_stream_t : public eager_datum_stream_t {
public:
    explicit wrapper_datum_stream_t(counted_t<datum_stream_t> _source)
        : eager_datum_stream_t(_source->backtrace()), source(_source) { }
    virtual bool is_array() { return source->is_array(); }
    virtual counted_t<const datum_t> as_array(env_t *env) {
        return is_array()
            ? eager_datum_stream_t::as_array(env)
            : counted_t<const datum_t>();
    }
    virtual bool is_exhausted() const {
        return source->is_exhausted();
    }

protected:
    const counted_t<datum_stream_t> source;
};

class map_datum_stream_t : public wrapper_datum_stream_t {
public:
    map_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);

    counted_t<func_t> f;
};

class indexes_of_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexes_of_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);

    counted_t<func_t> f;
    int64_t index;
};

class filter_datum_stream_t : public wrapper_datum_stream_t {
public:
    filter_datum_stream_t(counted_t<func_t> _f,
                          counted_t<func_t> _default_filter_val,
                          counted_t<datum_stream_t> _source);

private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);

    counted_t<func_t> f;
    counted_t<func_t> default_filter_val;
};

class concatmap_datum_stream_t : public wrapper_datum_stream_t {
public:
    concatmap_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);

    counted_t<func_t> f;
    counted_t<datum_stream_t> subsource;
};

class readgen_t {
public:
    explicit readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        const datum_range_t &original_datum_range,
        sorting_t sorting);
    virtual ~readgen_t() { }
    read_t terminal_read(
        const transform_t &transform,
        terminal_t &&_terminal,
        const batcher_t &batcher) const;
    // This has to be on `readgen_t` because we sort differently depending on
    // the kinds of reads we're doing.
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const = 0;

    virtual read_t next_read(
        const key_range_t &active_range,
        const transform_t &transform,
        const batcher_t &batcher) const;
    // This generates a read that will read as many rows as we need to be able
    // to do an sindex sort, or nothing if no such read is necessary.  Such a
    // read should only be necessary when we're ordering by a secondary index
    // and the last element read has a truncated value for that secondary index.
    virtual boost::optional<read_t> sindex_sort_read(
        const std::vector<rget_item_t> &items,
        const transform_t &transform,
        const batcher_t &batcher) const = 0;
    virtual key_range_t original_keyrange() const = 0;

    // Returns `true` if there is no more to read.
    bool update_range(key_range_t *active_range,
                      const store_key_t &last_considered_key) const;
protected:
    const std::map<std::string, wire_func_t> global_optargs;
    const datum_range_t original_datum_range;
    const sorting_t sorting;

private:
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const transform_t &transform,
        const batcher_t &batcher) const = 0;
};

class primary_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = UNORDERED);
private:
    primary_readgen_t(const std::map<std::string, wire_func_t> &global_optargs,
                      datum_range_t range, sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const transform_t &transform,
        const batcher_t &batcher) const;
    virtual boost::optional<read_t> sindex_sort_read(
        const std::vector<rget_item_t> &items,
        const transform_t &transform,
        const batcher_t &batcher) const;
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const;
    virtual key_range_t original_keyrange() const;
};

class sindex_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        const std::string &sindex,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = UNORDERED);
private:
    sindex_readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        const std::string &sindex, datum_range_t sindex_range, sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const transform_t &transform,
        const batcher_t &batcher) const;
    virtual boost::optional<read_t> sindex_sort_read(
        const std::vector<rget_item_t> &items,
        const transform_t &transform,
        const batcher_t &batcher) const;
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const;
    virtual key_range_t original_keyrange() const;

    const std::string sindex;
};

class reader_t {
public:
    explicit reader_t(
        const namespace_repo_t<rdb_protocol_t>::access_t &ns_access,
        bool use_outdated,
        scoped_ptr_t<readgen_t> &&readgen);
    void add_transformation(transform_variant_t &&tv);
    rget_read_response_t::result_t run_terminal(env_t *env, terminal_variant_t &&tv);
    std::vector<counted_t<const datum_t> >
    next_batch(env_t *env, const batcher_t &batcher);
    bool is_finished() const;
private:
    // Returns `true` if there's data in `items`.
    bool load_items(env_t *env, const batcher_t &batcher);
    rget_read_response_t do_read(env_t *env, const read_t &read);
    std::vector<rget_item_t> do_range_read(env_t *env, const read_t &read);

    namespace_repo_t<rdb_protocol_t>::access_t ns_access;
    const bool use_outdated;
    transform_t transform;

    bool started, finished;
    const scoped_ptr_t<const readgen_t> readgen;
    key_range_t active_range;

    // We need this to handle the SINDEX_CONSTANT case.
    std::vector<rget_item_t> items;
    size_t items_index;
};

class lazy_datum_stream_t : public datum_stream_t {
public:
    lazy_datum_stream_t(
        namespace_repo_t<rdb_protocol_t>::access_t *_ns_access,
        bool _use_outdated,
        scoped_ptr_t<readgen_t> &&_readgen,
        const protob_t<const Backtrace> &bt_src);

    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f);
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f);

    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> base,
                                         counted_t<func_t> r);
    virtual bool is_array() { return false; }
    virtual counted_t<const datum_t> as_array(UNUSED env_t *env) {
        return counted_t<const datum_t>();  // Cannot be converted implicitly.
    }

    bool is_exhausted() const;
private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);

    rdb_protocol_t::rget_read_response_t::result_t run_terminal(
        env_t *env, const rdb_protocol_details::terminal_variant_t &t);

    // We use these to cache a batch so that `next` works.  There are a lot of
    // things that are most easily written in terms of `next` that would
    // otherwise have to do this caching themselves.
    size_t current_batch_offset;
    std::vector<counted_t<const datum_t> > current_batch;

    reader_t reader;
};

class array_datum_stream_t : public eager_datum_stream_t {
public:
    array_datum_stream_t(counted_t<const datum_t> _arr,
                         const protob_t<const Backtrace> &bt_src);
    virtual bool is_exhausted() const;

private:
    virtual bool is_array();
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, UNUSED const batcher_t &batcher);
    counted_t<const datum_t> next(env_t *env, const batcher_t &batcher);

    size_t index;
    counted_t<const datum_t> arr;
};

class slice_datum_stream_t : public wrapper_datum_stream_t {
public:
    slice_datum_stream_t(size_t left, size_t right, counted_t<datum_stream_t> src);
private:
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);
    uint64_t index, left, right;
};

class zip_datum_stream_t : public wrapper_datum_stream_t {
public:
    explicit zip_datum_stream_t(counted_t<datum_stream_t> src);
private:
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);
};

class indexed_sort_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexed_sort_datum_stream_t(
        counted_t<datum_stream_t> stream, // Must be a table with a sorting applied.
        std::function<bool(env_t *,
                           const counted_t<const datum_t> &,
                           const counted_t<const datum_t> &)> lt_cmp);
private:
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);

    std::function<bool(env_t *,
                       const counted_t<const datum_t> &,
                       const counted_t<const datum_t> &)> lt_cmp;
    size_t index;
    std::vector<counted_t<const datum_t> > data;
};

class union_datum_stream_t : public datum_stream_t {
public:
    union_datum_stream_t(const std::vector<counted_t<datum_stream_t> > &_streams,
                         const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src), streams(_streams), streams_index(0) { }

    // stream -> stream
    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f);
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f);

    // stream -> atom
    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> base,
                                         counted_t<func_t> r);
    virtual bool is_array();
    virtual counted_t<const datum_t> as_array(env_t *env);
    virtual bool is_exhausted() const;

private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batcher_t &batcher);

    std::vector<counted_t<datum_stream_t> > streams;
    size_t streams_index;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_

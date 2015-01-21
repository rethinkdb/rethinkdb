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

#include "containers/scoped.hpp"
#include "rdb_protocol/context.hpp"
//#include "rdb_protocol/func.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/real_table.hpp"
#include "rdb_protocol/shards.hpp"

namespace ql {

class env_t;
class scope_env_t;
class func_t;

class datum_stream_t : public single_threaded_countable_t<datum_stream_t>,
                       public pb_rcheckable_t {
public:
    virtual ~datum_stream_t() { }

    virtual changefeed::keyspec_t get_change_spec() = 0;
    virtual void add_transformation(transform_variant_t &&tv,
                                    const protob_t<const Backtrace> &bt) = 0;
    void add_grouping(transform_variant_t &&tv,
                      const protob_t<const Backtrace> &bt);

    scoped_ptr_t<val_t> run_terminal(env_t *env, const terminal_variant_t &tv);
    scoped_ptr_t<val_t> to_array(env_t *env);

    // stream -> stream (always eager)
    counted_t<datum_stream_t> slice(size_t l, size_t r);
    counted_t<datum_stream_t> indexes_of(counted_t<const func_t> f);
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
    virtual bool is_cfeed() const = 0;
    virtual bool is_infinite() const = 0;

    virtual void accumulate(
        env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) = 0;
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;

protected:
    bool batch_cache_exhausted() const;
    void check_not_grouped(const char *msg);
    explicit datum_stream_t(const protob_t<const Backtrace> &bt_src);

private:
    virtual std::vector<datum_t>
    next_batch_impl(env_t *env, const batchspec_t &batchspec) = 0;

    std::vector<datum_t> batch_cache;
    size_t batch_cache_index;
    bool grouped;
};

class eager_datum_stream_t : public datum_stream_t {
protected:
    explicit eager_datum_stream_t(const protob_t<const Backtrace> &bt)
        : datum_stream_t(bt) { }
    virtual datum_t as_array(env_t *env);
    bool ops_to_do() { return ops.size() != 0; }

protected:
    virtual changefeed::keyspec_t get_change_spec() {
        rfail(base_exc_t::GENERIC, "%s", "Cannot call `changes` on an eager stream.");
    }
    std::vector<transform_variant_t> transforms;

    virtual void add_transformation(transform_variant_t &&tv,
                                    const protob_t<const Backtrace> &bt);

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
    virtual bool is_cfeed() const {
        return source->is_cfeed();
    }
    virtual bool is_infinite() const {
        return source->is_infinite();
    }

protected:
    const counted_t<datum_stream_t> source;
};

class indexes_of_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexes_of_datum_stream_t(counted_t<const func_t> _f, counted_t<datum_stream_t> _source);

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
                         const protob_t<const Backtrace> &bt_src);
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const;
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
    virtual changefeed::keyspec_t get_change_spec();
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const;
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

class union_datum_stream_t : public datum_stream_t {
public:
    union_datum_stream_t(std::vector<counted_t<datum_stream_t> > &&_streams,
                         const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src), streams(_streams), streams_index(0),
          is_cfeed_union(false),
          is_infinite_union(false) {
        for (auto const &stream : streams) {
            is_cfeed_union |= stream->is_cfeed();
            is_infinite_union |= stream->is_infinite();
        }
    }

    virtual void add_transformation(transform_variant_t &&tv,
                                    const protob_t<const Backtrace> &bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    virtual bool is_array() const;
    virtual datum_t as_array(env_t *env);
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const;
    virtual bool is_infinite() const;

private:
    virtual changefeed::keyspec_t get_change_spec() {
        rfail(base_exc_t::GENERIC, "%s", "Cannot call `changes` on a union stream.");
    }
    std::vector<datum_t >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    std::vector<counted_t<datum_stream_t> > streams;
    size_t streams_index;
    bool is_cfeed_union, is_infinite_union;
};

class range_datum_stream_t : public eager_datum_stream_t {
public:
    range_datum_stream_t(bool _is_infite_range,
                         int64_t _start,
                         int64_t _stop,
                         const protob_t<const Backtrace> &);

    virtual std::vector<datum_t>
    next_raw_batch(env_t *, const batchspec_t &batchspec);

    virtual bool is_array() const {
        return false;
    }
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const {
        return false;
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
                       const protob_t<const Backtrace> &bt_src);

    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    virtual bool is_array() const {
        return is_array_map;
    }
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const {
        return is_cfeed_map;
    }
    virtual bool is_infinite() const {
        return is_infinite_map;
    }

private:
    std::vector<counted_t<datum_stream_t> > streams;
    counted_t<const func_t> func;
    bool is_array_map, is_cfeed_map, is_infinite_map;
};

// This class generates the `read_t`s used in range reads.  It's used by
// `reader_t` below.  Its subclasses are the different types of range reads we
// need to do.
class readgen_t {
public:
    explicit readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        std::string table_name,
        profile_bool_t profile,
        sorting_t sorting);
    virtual ~readgen_t() { }

    virtual read_t terminal_read(
        const std::vector<transform_variant_t> &transform,
        const terminal_variant_t &_terminal,
        const batchspec_t &batchspec) const = 0;
    // This has to be on `readgen_t` because we sort differently depending on
    // the kinds of reads we're doing.
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const = 0;

    virtual read_t next_read(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const = 0;
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
    virtual boost::optional<std::string> sindex_name() const = 0;

    // Returns `true` if there is no more to read.
    bool update_range(key_range_t *active_range,
                      const store_key_t &last_key) const;

    virtual changefeed::keyspec_t::range_t get_change_spec(
        std::vector<transform_variant_t>) const = 0;

    const std::string &get_table_name() const { return table_name; }
protected:
    const std::map<std::string, wire_func_t> global_optargs;
    const std::string table_name;
    const profile_bool_t profile;
    const sorting_t sorting;
};

class rget_readgen_t : public readgen_t {
public:
    explicit rget_readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        std::string table_name,
        const datum_range_t &original_datum_range,
        profile_bool_t profile,
        sorting_t sorting);

    virtual read_t terminal_read(
        const std::vector<transform_variant_t> &transform,
        const terminal_variant_t &_terminal,
        const batchspec_t &batchspec) const;

    virtual read_t next_read(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;

    virtual changefeed::keyspec_t::range_t get_change_spec(
        std::vector<transform_variant_t> transforms) const {
        return changefeed::keyspec_t::range_t{
            std::move(transforms), sindex_name(), sorting, original_datum_range};
    }
private:
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const = 0;

protected:
    const datum_range_t original_datum_range;
};

class primary_readgen_t : public rget_readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        std::string table_name,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = sorting_t::UNORDERED);

private:
    primary_readgen_t(const std::map<std::string, wire_func_t> &global_optargs,
                      std::string table_name,
                      datum_range_t range,
                      profile_bool_t profile,
                      sorting_t sorting);
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
    virtual boost::optional<std::string> sindex_name() const;
};

class sindex_readgen_t : public rget_readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        std::string table_name,
        const std::string &sindex,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = sorting_t::UNORDERED);

    virtual boost::optional<read_t> sindex_sort_read(
        const key_range_t &active_range,
        const std::vector<rget_item_t> &items,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;
    virtual void sindex_sort(std::vector<rget_item_t> *vec) const;
    virtual key_range_t original_keyrange() const;
    virtual boost::optional<std::string> sindex_name() const;
private:
    sindex_readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        std::string table_name,
        const std::string &sindex,
        datum_range_t sindex_range,
        profile_bool_t profile,
        sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transform,
        const batchspec_t &batchspec) const;

    const std::string sindex;
};

// For geospatial intersection queries
class intersecting_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        env_t *env,
        std::string table_name,
        const std::string &sindex,
        const datum_t &query_geometry);

    virtual read_t terminal_read(
        const std::vector<transform_variant_t> &transform,
        const terminal_variant_t &_terminal,
        const batchspec_t &batchspec) const;

    virtual read_t next_read(
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
    virtual boost::optional<std::string> sindex_name() const;

    virtual changefeed::keyspec_t::range_t get_change_spec(
        std::vector<transform_variant_t>) const {
        rfail_datum(base_exc_t::GENERIC,
                    "%s", "Cannot call `changes` on an intersection read.");
        unreachable();
    }
private:
    intersecting_readgen_t(
        const std::map<std::string, wire_func_t> &global_optargs,
        std::string table_name,
        const std::string &sindex,
        const datum_t &query_geometry,
        profile_bool_t profile);

    // Analogue to rget_readgen_t::next_read_impl(), but generates an intersecting
    // geo read.
    intersecting_geo_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<transform_variant_t> &transforms,
        const batchspec_t &batchspec) const;

    const std::string sindex;
    const datum_t query_geometry;
};

class reader_t {
public:
    virtual ~reader_t() { }
    virtual void add_transformation(transform_variant_t &&tv) = 0;
    virtual void accumulate(env_t *env, eager_acc_t *acc,
                            const terminal_variant_t &tv) = 0;
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;
    virtual std::vector<datum_t> next_batch(env_t *env,
                                            const batchspec_t &batchspec) = 0;
    virtual bool is_finished() const = 0;

    virtual changefeed::keyspec_t get_change_spec() const = 0;
};

// For reads that generate read_response_t results
class rget_response_reader_t : public reader_t {
public:
    rget_response_reader_t(
        const counted_t<real_table_t> &table,
        bool use_outdated,
        scoped_ptr_t<readgen_t> &&readgen);
    void add_transformation(transform_variant_t &&tv);
    void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;
    std::vector<datum_t> next_batch(env_t *env, const batchspec_t &batchspec);
    bool is_finished() const;

    virtual changefeed::keyspec_t get_change_spec() const {
        return changefeed::keyspec_t(
            readgen->get_change_spec(transforms),
            table,
            readgen->get_table_name());
    }
protected:
    // Returns `true` if there's data in `items`.
    // Overwrite this in an implementation
    virtual bool load_items(env_t *env, const batchspec_t &batchspec) = 0;
    rget_read_response_t do_read(env_t *env, const read_t &read);

    counted_t<real_table_t> table;
    const bool use_outdated;
    std::vector<transform_variant_t> transforms;

    bool started, shards_exhausted;
    const scoped_ptr_t<const readgen_t> readgen;
    key_range_t active_range;

    // We need this to handle the SINDEX_CONSTANT case.
    std::vector<rget_item_t> items;
    size_t items_index;
};

class rget_reader_t : public rget_response_reader_t {
public:
    rget_reader_t(
        const counted_t<real_table_t> &_table,
        bool use_outdated,
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
        bool use_outdated,
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
        const protob_t<const Backtrace> &bt_src);

    virtual bool is_array() const { return false; }
    virtual datum_t as_array(UNUSED env_t *env) {
        return datum_t();  // Cannot be converted implicitly.
    }

    bool is_exhausted() const;
    virtual bool is_cfeed() const;
    virtual bool is_infinite() const;

private:
    virtual changefeed::keyspec_t get_change_spec() {
        return reader->get_change_spec();
    }

    std::vector<datum_t >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    virtual void add_transformation(transform_variant_t &&tv,
                                    const protob_t<const Backtrace> &bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    // We use these to cache a batch so that `next` works.  There are a lot of
    // things that are most easily written in terms of `next` that would
    // otherwise have to do this caching themselves.
    size_t current_batch_offset;
    std::vector<datum_t> current_batch;

    scoped_ptr_t<reader_t> reader;
};

class vector_datum_stream_t : public eager_datum_stream_t
{
public:
    vector_datum_stream_t(
            const protob_t<const Backtrace> &bt_source,
            std::vector<datum_t> &&_rows,
            boost::optional<ql::changefeed::keyspec_t> &&_changespec);
private:
    datum_t next(env_t *env, const batchspec_t &bs);
    datum_t next_impl(env_t *);
    std::vector<datum_t> next_raw_batch(env_t *env, const batchspec_t &bs);

    void add_transformation(
        transform_variant_t &&tv, const protob_t<const Backtrace> &bt);

    bool is_exhausted() const;
    bool is_cfeed() const;
    bool is_array() const;
    bool is_infinite() const;

    changefeed::keyspec_t get_change_spec();

    std::vector<datum_t> rows;
    size_t index;
    boost::optional<ql::changefeed::keyspec_t> changespec;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_

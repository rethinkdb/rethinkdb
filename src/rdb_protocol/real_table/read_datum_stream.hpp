// Copyright 2010-2014 RethinkDB, all rights reserved
#ifndef RDB_PROTOCOL_REAL_TABLE_LAZY_DATUM_STREAM_HPP_
#define RDB_PROTOCOL_REAL_TABLE_LAZY_DATUM_STREAM_HPP_

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/real_table/protocol.hpp"
#include "rdb_protocol/real_table/real_table.hpp"
#include "rdb_protocol/wire_func.hpp"

// This class generates the `read_t`s used in range reads.  It's used by
// `table_reader_t` below.  Its subclasses are the different types of range reads we
// need to do.
class readgen_t {
public:
    explicit readgen_t(
        const std::map<std::string, ql::wire_func_t> &global_optargs,
        std::string table_name,
        const datum_range_t &original_datum_range,
        profile_bool_t profile,
        sorting_t sorting);
    virtual ~readgen_t() { }
    read_t terminal_read(
        const std::vector<ql::transform_variant_t> &transform,
        const ql::terminal_variant_t &_terminal,
        const ql::batchspec_t &batchspec) const;
    // This has to be on `readgen_t` because we sort differently depending on
    // the kinds of reads we're doing.
    virtual void sindex_sort(std::vector<ql::rget_item_t> *vec) const = 0;

    virtual read_t next_read(
        const key_range_t &active_range,
        const std::vector<ql::transform_variant_t> &transform,
        const ql::batchspec_t &batchspec) const;
    // This generates a read that will read as many rows as we need to be able
    // to do an sindex sort, or nothing if no such read is necessary.  Such a
    // read should only be necessary when we're ordering by a secondary index
    // and the last element read has a truncated value for that secondary index.
    virtual boost::optional<read_t> sindex_sort_read(
        const key_range_t &active_range,
        const std::vector<ql::rget_item_t> &items,
        const std::vector<ql::transform_variant_t> &transform,
        const ql::batchspec_t &batchspec) const = 0;

    virtual key_range_t original_keyrange() const = 0;
    virtual std::string sindex_name() const = 0; // Used for error checking.

    // Returns `true` if there is no more to read.
    bool update_range(key_range_t *active_range,
                      const store_key_t &last_key) const;
protected:
    const std::map<std::string, ql::wire_func_t> global_optargs;
    const std::string table_name;
    const datum_range_t original_datum_range;
    const profile_bool_t profile;
    const sorting_t sorting;

private:
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<ql::transform_variant_t> &transform,
        const ql::batchspec_t &batchspec) const = 0;
};

class primary_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        ql::env_t *env,
        std::string table_name,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = sorting_t::UNORDERED);
private:
    primary_readgen_t(const std::map<std::string, ql::wire_func_t> &global_optargs,
                      std::string table_name,
                      datum_range_t range,
                      profile_bool_t profile,
                      sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<ql::transform_variant_t> &transform,
        const ql::batchspec_t &batchspec) const;
    virtual boost::optional<read_t> sindex_sort_read(
        const key_range_t &active_range,
        const std::vector<ql::rget_item_t> &items,
        const std::vector<ql::transform_variant_t> &transform,
        const ql::batchspec_t &batchspec) const;
    virtual void sindex_sort(std::vector<ql::rget_item_t> *vec) const;
    virtual key_range_t original_keyrange() const;
    virtual std::string sindex_name() const; // Used for error checking.
};

class sindex_readgen_t : public readgen_t {
public:
    static scoped_ptr_t<readgen_t> make(
        ql::env_t *env,
        std::string table_name,
        const std::string &sindex,
        datum_range_t range = datum_range_t::universe(),
        sorting_t sorting = sorting_t::UNORDERED);
private:
    sindex_readgen_t(
        const std::map<std::string, ql::wire_func_t> &global_optargs,
        std::string table_name,
        const std::string &sindex,
        datum_range_t sindex_range,
        profile_bool_t profile,
        sorting_t sorting);
    virtual rget_read_t next_read_impl(
        const key_range_t &active_range,
        const std::vector<ql::transform_variant_t> &transform,
        const ql::batchspec_t &batchspec) const;
    virtual boost::optional<read_t> sindex_sort_read(
        const key_range_t &active_range,
        const std::vector<ql::rget_item_t> &items,
        const std::vector<ql::transform_variant_t> &transform,
        const ql::batchspec_t &batchspec) const;
    virtual void sindex_sort(std::vector<ql::rget_item_t> *vec) const;
    virtual key_range_t original_keyrange() const;
    virtual std::string sindex_name() const; // Used for error checking.

    const std::string sindex;
};

class table_reader_t {
public:
    explicit table_reader_t(
        const real_table_t &_table,
        bool use_outdated,
        scoped_ptr_t<readgen_t> &&readgen);
    void add_transformation(ql::transform_variant_t &&tv);
    void accumulate(ql::env_t *env, ql::eager_acc_t *acc,
        const ql::terminal_variant_t &tv);
    void accumulate_all(ql::env_t *env, ql::eager_acc_t *acc);
    std::vector<counted_t<const ql::datum_t> >
    next_batch(ql::env_t *env, const ql::batchspec_t &batchspec);
    bool is_finished() const;
private:
    // Returns `true` if there's data in `items`.
    bool load_items(ql::env_t *env, const ql::batchspec_t &batchspec);
    rget_read_response_t do_read(ql::env_t *env, const read_t &read);
    std::vector<ql::rget_item_t> do_range_read(ql::env_t *env, const read_t &read);

    real_table_t table;
    const bool use_outdated;
    std::vector<ql::transform_variant_t> transforms;

    bool started, shards_exhausted;
    const scoped_ptr_t<const readgen_t> readgen;
    key_range_t active_range;

    // We need this to handle the SINDEX_CONSTANT case.
    std::vector<ql::rget_item_t> items;
    size_t items_index;
};

class read_datum_stream_t : public ql::datum_stream_t {
public:
    read_datum_stream_t(
        const real_table_t &_table,
        bool _use_outdated,
        scoped_ptr_t<readgen_t> &&_readgen,
        const ql::protob_t<const Backtrace> &bt_src);

    virtual bool is_array() { return false; }
    virtual counted_t<const ql::datum_t> as_array(UNUSED ql::env_t *env) {
        return counted_t<const ql::datum_t>();  // Cannot be converted implicitly.
    }

    bool is_exhausted() const;
    virtual bool is_cfeed() const;
private:
    std::vector<counted_t<const ql::datum_t> >
    next_batch_impl(ql::env_t *env, const ql::batchspec_t &batchspec);

    virtual void add_transformation(ql::transform_variant_t &&tv,
                                    const ql::protob_t<const Backtrace> &bt);
    virtual void accumulate(ql::env_t *env, ql::eager_acc_t *acc,
        const ql::terminal_variant_t &tv);
    virtual void accumulate_all(ql::env_t *env, ql::eager_acc_t *acc);

    // We use these to cache a batch so that `next` works.  There are a lot of
    // things that are most easily written in terms of `next` that would
    // otherwise have to do this caching themselves.
    size_t current_batch_offset;
    std::vector<counted_t<const ql::datum_t> > current_batch;
    table_reader_t reader;
};

#endif   /* RDB_PROTOCOL_REAL_TABLE_LAZY_DATUM_STREAM_HPP_ */


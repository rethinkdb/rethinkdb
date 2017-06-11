#ifndef RDB_PROTOCOL_DATUM_STREAM_READERS_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_READERS_HPP_

#include "containers/optional.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/datum_stream/readgens.hpp"
#include "rdb_protocol/real_table.hpp"

namespace ql {

class reader_t {
public:
    virtual ~reader_t() { }
    virtual void add_transformation(transform_variant_t &&tv) = 0;
    virtual bool add_stamp(changefeed_stamp_t stamp) = 0;
    virtual optional<active_state_t> get_active_state() = 0;
    virtual void accumulate(env_t *env, eager_acc_t *acc,
                            const terminal_variant_t &tv) = 0;
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;
    virtual std::vector<datum_t> next_batch(
        env_t *env, const batchspec_t &batchspec) = 0;
    virtual std::vector<rget_item_t> raw_next_batch(
        env_t *, const batchspec_t &) { unreachable(); }
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
    virtual optional<active_state_t> get_active_state() {
        r_sanity_fail();
    }
    virtual void accumulate(env_t *, eager_acc_t *, const terminal_variant_t &) {}
    virtual void accumulate_all(env_t *, eager_acc_t *) {}
    virtual std::vector<datum_t> next_batch(env_t *, const batchspec_t &) {
        return std::vector<datum_t>();
    }
    std::vector<rget_item_t> raw_next_batch(
        env_t *, const batchspec_t &) final {
        return std::vector<rget_item_t>{};
    }
    virtual bool is_finished() const {
        return true;
    }
    virtual changefeed::keyspec_t get_changespec() const;

private:
    counted_t<real_table_t> table;
    std::string table_name;
};

class vector_reader_t : public reader_t {
public:
    explicit vector_reader_t(std::vector<datum_t> &&_items) :
        finished(false),
        items(std::move(_items)) {
    }
    virtual ~vector_reader_t() {}
    virtual void add_transformation(transform_variant_t &&) {
        r_sanity_fail();
    }
    virtual bool add_stamp(changefeed_stamp_t) {
        r_sanity_fail();
    }
    virtual optional<active_state_t> get_active_state() {
        r_sanity_fail();
    }
    virtual void accumulate(env_t *, eager_acc_t *, const terminal_variant_t &) {
        r_sanity_fail();
    }
    virtual void accumulate_all(env_t *, eager_acc_t *) {
        r_sanity_fail();
    }
    virtual std::vector<datum_t> next_batch(env_t *, const batchspec_t &) {
        r_sanity_check(!finished);
        finished = true;
        return std::move(items);
    }
    std::vector<rget_item_t> raw_next_batch(
        env_t *, const batchspec_t &) final {
        r_sanity_check(!finished);
        std::vector<rget_item_t> rget_items;
        for (auto it = items.begin(); it != items.end(); ++it) {
            rget_item_t item;
            item.data = std::move(*it);
            rget_items.push_back(std::move(item));
        }
        items.clear();
        finished = true;
        return rget_items;
    }
    virtual bool is_finished() const {
        return finished;
    }
    virtual changefeed::keyspec_t get_changespec() const {
        r_sanity_fail();
    }

private:
    bool finished;
    std::vector<datum_t> items;
};

// For reads that generate read_response_t results.
class rget_response_reader_t : public reader_t {
public:
    rget_response_reader_t(
        const counted_t<real_table_t> &table,
        scoped_ptr_t<readgen_t> &&readgen);
    virtual void add_transformation(transform_variant_t &&tv);
    virtual bool add_stamp(changefeed_stamp_t stamp);
    virtual optional<active_state_t> get_active_state();
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;
    virtual std::vector<datum_t> next_batch(env_t *env, const batchspec_t &batchspec);
    virtual std::vector<rget_item_t> raw_next_batch(env_t *env,
                                                    const batchspec_t &batchspec);
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
        active_ranges.set(active_ranges_t());
    }

    // Returns `true` if there's data in `items`.
    // Overwrite this in an implementation
    virtual bool load_items(env_t *env, const batchspec_t &batchspec) = 0;
    rget_read_response_t do_read(env_t *env, const read_t &read);

    counted_t<real_table_t> table;
    std::vector<transform_variant_t> transforms;
    optional<changefeed_stamp_t> stamp;

    bool started;
    const scoped_ptr_t<const readgen_t> readgen;
    optional<active_ranges_t> active_ranges;
    optional<reql_version_t> reql_version;
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

    // Each secondary index value might be inserted into a geospatial index multiple
    // times, and we need to remove those duplicates across batches.
    // We keep track of pairs of primary key and optional multi-index tags in order
    // to detect and remove such duplicates.
    std::set<std::pair<std::string, optional<uint64_t> > > processed_pkey_tags;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_READERS_HPP_

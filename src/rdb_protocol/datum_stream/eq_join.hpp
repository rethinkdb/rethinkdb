#ifndef RDB_PROTOCOL_DATUM_STREAM_EQ_JOIN_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_EQ_JOIN_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class eq_join_datum_stream_t : public eager_datum_stream_t {
public:
    eq_join_datum_stream_t(counted_t<datum_stream_t> _stream,
                           counted_t<table_t> _table,
                           datum_string_t _join_index,
                           counted_t<const func_t> _predicate,
                           bool _ordered,
                           backtrace_id_t bt);

    bool is_array() const final {
        return is_array_eq_join;
    }
    bool is_infinite() const final {
        return is_infinite_eq_join;
    }
    bool is_exhausted() const final;

    std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    feed_type_t cfeed_type() const final {
        return eq_join_type;
    }

private:
    counted_t<datum_stream_t> stream;
    scoped_ptr_t<reader_t> get_all_reader;
    std::vector<rget_item_t> get_all_items;

    counted_t<table_t> table;
    datum_string_t join_index;

    std::multimap<ql::datum_t,
                  ql::datum_t> sindex_to_datum;

    counted_t<const func_t> predicate;

    bool ordered;

    bool is_array_eq_join;
    bool is_infinite_eq_join;
    feed_type_t eq_join_type;
};



}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_EQ_JOIN_HPP_

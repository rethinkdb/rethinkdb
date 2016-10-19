#ifndef RDB_PROTOCOL_DATUM_STREAM_VECTOR_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_VECTOR_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class vector_datum_stream_t : public eager_datum_stream_t {
public:
    vector_datum_stream_t(
            backtrace_id_t bt,
            std::vector<datum_t> &&_rows,
            optional<changefeed::keyspec_t> &&_changespec);
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
    optional<changefeed::keyspec_t> changespec;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_VECTOR_HPP_

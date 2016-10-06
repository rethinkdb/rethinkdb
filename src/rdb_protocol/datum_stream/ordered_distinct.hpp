#ifndef RDB_PROTOCOL_DATUM_STREAM_ORDERED_DISTINCT_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_ORDERED_DISTINCT_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class ordered_distinct_datum_stream_t : public wrapper_datum_stream_t {
public:
    explicit ordered_distinct_datum_stream_t(counted_t<datum_stream_t> _source);
private:
    std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
    datum_t last_val;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_ORDERED_DISTINCT_HPP_

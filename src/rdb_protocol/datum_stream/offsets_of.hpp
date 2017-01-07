#ifndef RDB_PROTOCOL_DATUM_STREAM_OFFSETS_OF_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_OFFSETS_OF_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class offsets_of_datum_stream_t : public wrapper_datum_stream_t {
public:
    offsets_of_datum_stream_t(counted_t<const func_t> _f, counted_t<datum_stream_t> _source);

private:
    std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    counted_t<const func_t> f;
    int64_t index;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_OFFSETS_OF_HPP_

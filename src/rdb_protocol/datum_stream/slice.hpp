#ifndef RDB_PROTOCOL_DATUM_STREAM_SLICE_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_SLICE_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

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

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_SLICE_HPP_

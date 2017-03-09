#ifndef RDB_PROTOCOL_DATUM_STREAM_RANGE_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_RANGE_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class range_datum_stream_t : public eager_datum_stream_t {
public:
    range_datum_stream_t(bool _is_infite_range,
                         int64_t _start,
                         int64_t _stop,
                         backtrace_id_t);

    virtual std::vector<datum_t>
    next_raw_batch(env_t *, const batchspec_t &batchspec);

    virtual bool is_array() const {
        return false;
    }
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const {
        return feed_type_t::not_feed;
    }
    virtual bool is_infinite() const {
        return is_infinite_range;
    }

private:
    bool is_infinite_range;
    int64_t start, stop;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_RANGE_HPP_

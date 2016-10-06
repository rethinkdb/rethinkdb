#ifndef RDB_PROTOCOL_DATUM_STREAM_MAP_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_MAP_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class map_datum_stream_t : public eager_datum_stream_t {
public:
    map_datum_stream_t(std::vector<counted_t<datum_stream_t> > &&_streams,
                       counted_t<const func_t> &&_func,
                       backtrace_id_t bt);

    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    virtual bool is_array() const {
        return is_array_map;
    }
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const {
        return union_type;
    }
    virtual bool is_infinite() const {
        return is_infinite_map;
    }

private:
    std::vector<counted_t<datum_stream_t> > streams;
    counted_t<const func_t> func;
    feed_type_t union_type;
    bool is_array_map, is_infinite_map;

    // We need to preserve this between calls because we might have gotten data
    // from a few substreams before having to abort because we timed out on a
    // changefeed stream and return a partial batch.  (We still time out and
    // return partial batches, or even empty batches, for the web UI in the case
    // of a changefeed stream.)
    std::vector<std::deque<datum_t> > cache;
    std::vector<datum_t> args;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_MAP_HPP_

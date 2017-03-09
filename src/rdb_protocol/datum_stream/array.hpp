#ifndef RDB_PROTOCOL_DATUM_STREAM_ARRAY_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_ARRAY_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class array_datum_stream_t : public eager_datum_stream_t {
public:
    array_datum_stream_t(datum_t _arr,
                         backtrace_id_t bt);
    virtual bool is_exhausted() const;
    virtual feed_type_t cfeed_type() const;
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

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_ARRAY_HPP_

#ifndef RDB_PROTOCOL_DATUM_STREAM_INDEXED_SORT_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_INDEXED_SORT_HPP_

#include "rdb_protocol/datum_stream.hpp"

namespace ql {

class indexed_sort_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexed_sort_datum_stream_t(
        counted_t<datum_stream_t> stream, // Must be a table with a sorting applied.
        std::function<bool(env_t *,  // NOLINT(readability/casting)
                           profile::sampler_t *,
                           const datum_t &,
                           const datum_t &)> lt_cmp);
private:
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    std::function<bool(env_t *,  // NOLINT(readability/casting)
                       profile::sampler_t *,
                       const datum_t &,
                       const datum_t &)> lt_cmp;
    size_t index;
    std::vector<datum_t> data;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_DATUM_STREAM_INDEXED_SORT_HPP_

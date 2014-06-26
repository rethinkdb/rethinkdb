#ifndef RDB_PROTOCOL_SERIALIZE_DATUM_HPP_
#define RDB_PROTOCOL_SERIALIZE_DATUM_HPP_

#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"

namespace ql {

class datum_t;

// More stable versions of datum serialization, unlike the versioned serialization
// functions, so that we don't have to update every row on disk just because
// unrelated cluster serialization has changed.
size_t datum_serialized_size(const counted_t<const datum_t> &datum);
void datum_serialize(write_message_t *wm, const counted_t<const datum_t> &datum);
archive_result_t datum_deserialize(read_stream_t *s, counted_t<const datum_t> *datum);

// The versioned serialization functions.
template <cluster_version_t W>
size_t serialized_size(const counted_t<const datum_t> &datum) {
    return datum_serialized_size(datum);
}
template <cluster_version_t W>
void serialize(write_message_t *wm, const counted_t<const datum_t> &datum) {
    datum_serialize(wm, datum);
}
template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, counted_t<const datum_t> *datum) {
    return datum_deserialize(s, datum);
}

template <cluster_version_t W>
void serialize(write_message_t *wm, const empty_ok_t<const counted_t<const datum_t> > &datum);
template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, empty_ok_ref_t<counted_t<const datum_t> > datum);

}  // namespace ql



#endif /* RDB_PROTOCOL_SERIALIZE_DATUM_HPP_ */

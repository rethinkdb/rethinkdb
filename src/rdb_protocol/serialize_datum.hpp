#ifndef RDB_PROTOCOL_SERIALIZE_DATUM_HPP_
#define RDB_PROTOCOL_SERIALIZE_DATUM_HPP_

#include "containers/archive/archive.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/counted.hpp"

namespace ql {

class datum_t;

// More stable versions of datum serialization, kept separate from the versioned
// serialization functions.  Don't change these in a backwards-uncompatible way!  See
// the FAQ at the end of this file.
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

// Q. Why do we have datum_serialize/datum_deserialize?
//
// A. We don't want the datum serialization format to silently change underneath us,
// because dealing with that possibility would require storing the serialization
// format number for every datum, like we did (using the block magic value) with the
// persist metadata and auth metadata and sindex block metadata.
//
// We would have to store the serialization format number using unused bits in the
// pair_offsets array in the leaf node, which would suck. Also, when we add a feature
// to bring all the database files up to a specific version (because eventually we'll
// drop support for the oldest version of serialization format), we'll want datums to
// be outside that system -- we don't want to be held back by the requirement that we
// update every row of the databases to achieve such an upgrade.
//
// Also, datums have their own inherent backwards-compatible upgradeability because
// their serialization format starts with a 1-byte type tag. That is where some
// change to the serialization format should be indicated.
//
// I think the unused bits in the pair offsets array should be reserved for
// indicating a different kind of change to the blob value format. For example, if we
// want to support loading only parts of the object, without loading the whole thing,
// that would be appropriate for the bits in the pair offsets array, which is a
// relatively scarce resource.

#endif /* RDB_PROTOCOL_SERIALIZE_DATUM_HPP_ */

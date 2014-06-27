#ifndef CONTAINERS_ARCHIVE_VERSIONED_HPP_
#define CONTAINERS_ARCHIVE_VERSIONED_HPP_

#include "containers/archive/archive.hpp"
#include "version.hpp"

namespace archive_internal {

// This is just used to implement serialize_cluster_version and
// deserialize_cluster_version.  (cluster_version_t conveniently has a contiguous set
// of valid representation, from v1_13 to v1_13_is_latest).
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_version_t, int8_t,
                                      cluster_version_t::v1_13,
                                      cluster_version_t::v1_13_is_latest);

class bogus_made_up_type_t;

}  // namespace archive_details

// These are generally universal.  They must not have their behavior change -- except
// if we remove some cluster_version_t value, in which case... maybe would fail on a
// range error with the specific removed values.  Or maybe, we would do something
// differently.
inline void serialize_cluster_version(write_message_t *wm, cluster_version_t v) {
    archive_internal::serialize<cluster_version_t::LATEST>(wm, v);
}

inline archive_result_t deserialize_cluster_version(read_stream_t *s,
                                                    cluster_version_t *thing) {
    return archive_internal::deserialize<cluster_version_t::LATEST>(s, thing);
}


// Serializes a value for a given version.  DOES NOT SERIALIZE THE VERSION NUMBER!
template <class T>
void serialize_for_version(cluster_version_t version, write_message_t *wm,
                           const T &value) {
    switch (version) {
    case cluster_version_t::v1_13_is_latest:
        serialize<cluster_version_t::v1_13_is_latest>(wm, value);
        break;
    default:
        unreachable();
    }
}

// Deserializes a value, assuming it's serialized for a given version.  (This doesn't
// deserialize any version numbers.)
template <class T>
archive_result_t deserialize_for_version(cluster_version_t version,
                                         read_stream_t *s,
                                         T *thing) {
    switch (version) {
    case cluster_version_t::v1_13_is_latest:
        return deserialize<cluster_version_t::v1_13_is_latest>(s, thing);
    default:
        unreachable();
    }
}


// Some serialized_size needs to be visible, apparently, so that
// serialized_size_for_version will actually parse.
template <cluster_version_t W>
size_t serialized_size(const archive_internal::bogus_made_up_type_t &);

template <class T>
size_t serialized_size_for_version(cluster_version_t version,
                                   const T &thing) {
    switch (version) {
    case cluster_version_t::v1_13_is_latest:
        return serialized_size<cluster_version_t::v1_13_is_latest>(thing);
    default:
        unreachable();
    }
}

#define INSTANTIATE_SERIALIZE_SINCE_v1_13(typ)                 \
    template void serialize<cluster_version_t::v1_13_is_latest>(  \
            write_message_t *, const typ &)

#define INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)                        \
    template archive_result_t deserialize<cluster_version_t::v1_13_is_latest>( \
            read_stream_t *, typ *)

#define INSTANTIATE_SERIALIZED_SIZE_SINCE_v1_13(typ) \
    template size_t serialized_size<cluster_version_t::v1_13_is_latest>(const typ &)

#define INSTANTIATE_SINCE_v1_13(typ)            \
    INSTANTIATE_SERIALIZE_SINCE_v1_13(typ);     \
    INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)

#define INSTANTIATE_SELF_SINCE_v1_13(typ)                               \
    template void typ::rdb_serialize<cluster_version_t::v1_13_is_latest>( \
            write_message_t *) const;                                   \
    template archive_result_t typ::rdb_deserialize<cluster_version_t::v1_13_is_latest>( \
            read_stream_t *s)


#endif  // CONTAINERS_ARCHIVE_VERSIONED_HPP_

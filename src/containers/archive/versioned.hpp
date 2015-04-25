#ifndef CONTAINERS_ARCHIVE_VERSIONED_HPP_
#define CONTAINERS_ARCHIVE_VERSIONED_HPP_

#include "containers/archive/archive.hpp"
#include "version.hpp"

namespace archive_internal {

class bogus_made_up_type_t;

}  // namespace archive_internal

// These are generally universal.  They must not have their behavior change -- except
// if we remove some cluster_version_t value, in which case... maybe would fail on a
// range error with the specific removed values.  Or maybe, we would do something
// differently.
inline void serialize_cluster_version(write_message_t *wm, cluster_version_t v) {
    int8_t raw = static_cast<int8_t>(v);
    serialize<cluster_version_t::LATEST_OVERALL>(wm, raw);
}

inline MUST_USE archive_result_t deserialize_cluster_version(
    read_stream_t *s, cluster_version_t *thing, const char *v1_13_msg) {
    // Initialize `thing` to *something* because GCC 4.6.3 thinks that `thing`
    // could be used uninitialized, even when the return value of this function
    // is checked through `guarantee_deserialization()`.
    // See https://github.com/rethinkdb/rethinkdb/issues/2640
    *thing = cluster_version_t::LATEST_OVERALL;
    int8_t raw;
    archive_result_t res = deserialize<cluster_version_t::LATEST_OVERALL>(s, &raw);
    if (raw == static_cast<int8_t>(obsolete_cluster_version_t::v1_13)
        || raw == static_cast<int8_t>(obsolete_cluster_version_t::v1_13_2)) {
        crash("%s", v1_13_msg);
    } else {
        // This is the same rassert in `ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE`.
        rassert(raw >= static_cast<int8_t>(cluster_version_t::v1_14)
                && raw <= static_cast<int8_t>(cluster_version_t::v2_0_is_latest));
        *thing = static_cast<cluster_version_t>(raw);
    }
    return res;
}


// Serializes a value for a given version.  DOES NOT SERIALIZE THE VERSION NUMBER!
template <class T>
void serialize_for_version(cluster_version_t version, write_message_t *wm,
                           const T &value) {
    // We currently only support serializing either the current disk or current
    // cluster version, no previous versions.
    if (version == cluster_version_t::CLUSTER) {
        serialize<cluster_version_t::CLUSTER>(wm, value);
    } else if (version == cluster_version_t::LATEST_DISK) {
        serialize<cluster_version_t::LATEST_DISK>(wm, value);
    } else {
        crash("Attempted to serialize for a non-current version");
    }
}

// Deserializes a value, assuming it's serialized for a given version.  (This doesn't
// deserialize any version numbers.)
template <class T>
archive_result_t deserialize_for_version(cluster_version_t version,
                                         read_stream_t *s,
                                         T *thing) {
    switch (version) {
    case cluster_version_t::v1_14:
        return deserialize<cluster_version_t::v1_14>(s, thing);
    case cluster_version_t::v1_15:
        return deserialize<cluster_version_t::v1_15>(s, thing);
    case cluster_version_t::v1_16:
        return deserialize<cluster_version_t::v1_16>(s, thing);
    case cluster_version_t::v2_0_is_latest:
        return deserialize<cluster_version_t::v2_0_is_latest>(s, thing);
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
    case cluster_version_t::v1_14:
        return serialized_size<cluster_version_t::v1_14>(thing);
    case cluster_version_t::v1_15:
        return serialized_size<cluster_version_t::v1_15>(thing);
    case cluster_version_t::v1_16:
        return serialized_size<cluster_version_t::v1_16>(thing);
    case cluster_version_t::v2_0_is_latest:
        return serialized_size<cluster_version_t::v2_0_is_latest>(thing);
    default:
        unreachable();
    }
}

// We want to express explicitly whether a given serialization function
// is used for cluster messages or disk serialization in case the latest cluster
// and latest disk versions diverge.
//
// If you see either the INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK
// of INSTANTIATE_SERIALIZE_FOR_DISK macro used somewhere, you know that if you
// change the serialization format of that type that will break the disk format,
// and you should consider writing a deserialize function for the older versions.

#define INSTANTIATE_SERIALIZE_FOR_DISK(typ)                  \
    template void serialize<cluster_version_t::LATEST_DISK>( \
            write_message_t *, const typ &)

#define INSTANTIATE_SERIALIZE_FOR_CLUSTER(typ)           \
    template void serialize<cluster_version_t::CLUSTER>( \
            write_message_t *, const typ &)

#define INSTANTIATE_DESERIALIZE_FOR_CLUSTER(typ)                       \
    template archive_result_t deserialize<cluster_version_t::CLUSTER>( \
                            read_stream_t *, typ *)

#ifdef CLUSTER_AND_DISK_VERSIONS_ARE_SAME
#define INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(typ)  \
    template void serialize<cluster_version_t::CLUSTER>( \
            write_message_t *, const typ &)
#else
#define INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(typ)      \
    template void serialize<cluster_version_t::CLUSTER>(     \
            write_message_t *, const typ &);                 \
    template void serialize<cluster_version_t::LATEST_DISK>( \
            write_message_t *, const typ &)
#endif

#define INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)                                 \
    template archive_result_t deserialize<cluster_version_t::v1_14>(             \
            read_stream_t *, typ *);                                             \
    template archive_result_t deserialize<cluster_version_t::v1_15>(             \
            read_stream_t *, typ *);                                             \
    template archive_result_t deserialize<cluster_version_t::v1_16>(             \
            read_stream_t *, typ *);                                             \
    template archive_result_t deserialize<cluster_version_t::v2_0_is_latest>(    \
            read_stream_t *, typ *)

#define INSTANTIATE_SERIALIZABLE_SINCE_v1_13(typ)        \
    INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(typ);     \
    INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)

#define INSTANTIATE_DESERIALIZE_SINCE_v1_16(typ)                                 \
    template archive_result_t deserialize<cluster_version_t::v1_16>(             \
            read_stream_t *, typ *);                                             \
    template archive_result_t deserialize<cluster_version_t::v2_0_is_latest>(    \
            read_stream_t *, typ *)

#define INSTANTIATE_SERIALIZABLE_SINCE_v1_16(typ)        \
    INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(typ);     \
    INSTANTIATE_DESERIALIZE_SINCE_v1_16(typ)

#define INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(typ)                      \
    INSTANTIATE_SERIALIZE_FOR_CLUSTER(typ);                            \
    template archive_result_t deserialize<cluster_version_t::CLUSTER>( \
            read_stream_t *, typ *)

#endif  // CONTAINERS_ARCHIVE_VERSIONED_HPP_

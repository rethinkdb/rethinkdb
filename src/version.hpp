#ifndef VERSION_HPP_
#define VERSION_HPP_

// An enumeration of the current and previous recognized versions.  Every
// serialization / deserialization happens relative to one of these versions.
enum class cluster_version_t {
    // The versions are _currently_ contiguously numbered.  If this becomes untrue,
    // be sure to carefully replace the ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE line
    // that implements serialization.
    v1_13 = 0,

    // See CLUSTER_VERSION, which should always be the latest version.
    LATEST_VERSION = v1_13,

    // ONLY_VERSION should only exist as long as there's only one version.  A few
    // assertions check it -- that code should be fixed to explicitly handle multiple
    // versions, but we're procrastinating that for now.
    ONLY_VERSION = v1_13,
};

// We will not (barring a bug) even attempt to deserialize a version number that we
// do not support.  Cluster nodes connecting to us make sure that they are
// communicating with a version number that we also support (the max of our two
// versions) and we don't end up deserializing version numbers in that situation
// anyway (yet).  Files on disk will not (barring corruption, or a bug) cause us to
// try to use a version number that we do not support -- we'll see manually whether
// we can read the serializer file by looking at the disk_format_version field in the
// metablock.
//
// At some point the set of cluster versions and disk versions that we support might
// diverge.  It's likely that we'd support a larger window of serialization versions
// in the on-disk format.
//
// Also, note: it's possible that versions will not be linearly ordered: Suppose we
// release v1.17 and then v1.18.  Perhaps v1.17 supports v1_16 and v1_17 and v1.18
// supports v1_17 and v1_18.  Suppose we then need to make a point-release, v1.17.1,
// that changes a serialization format, and we add a new cluster version v1_17_1.
// Then, v1.18 would still only support v1_17 and v1_18.  However, v1.18.1 might
// support v1_17, v1_17_1, and v1_18 (and v1_18_1 if that needs to be created).

#endif  // VERSION_HPP_

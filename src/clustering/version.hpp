#ifndef __CLUSTERING_VERSION_HPP__
#define __CLUSTERING_VERSION_HPP__

class version_t {
public:
    version_t() { }
    version_t(branch_id_t bid, state_timestamp_t ts) :
        branch(bid), timestamp(ts) { }

    bool operator==(const version_t &v) {
        return branch == v.branch && timestamp == v.timestamp;
    }
    bool operator!=(const version_t &v) {
        return !(*this == v);
    }

    branch_id_t branch;
    state_timestamp_t timestamp;
};

class version_range_t {
public:
    version_range_t();
    version_range_t(const version_t &e, const version_t &l) :
        earliest(e), latest(l) { }

    bool is_coherent() {
        return earliest == latest;
    }
    bool operator==(const version_range_t &v) {
        return earliest == v.earliest && latest == v.latest;
    }
    bool operator!=(const version_range_t &v) {
        return !(*this == v);
    }

    version_t earliest, latest;
};

#endif /* __CLUSTERING_VERSION_HPP__ */

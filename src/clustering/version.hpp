#ifndef __CLUSTERING_VERSION_HPP__
#define __CLUSTERING_VERSION_HPP__

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>

#include "timestamps.hpp"

typedef boost::uuids::uuid branch_id_t;

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

    RDB_MAKE_ME_SERIALIZABLE_2(branch, timestamp);
};

class version_range_t {
public:
    version_range_t() { }
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

    RDB_MAKE_ME_SERIALIZABLE_2(earliest, latest);
};

/* `branch_history_database_t` is abstract for ease of mocking */
template<class protocol_t>
class branch_history_database_t {
public:
    virtual bool version_is_ancestor(version_t v1, version_t v2, typename protocol_t::region_t where) = 0;
protected:
    virtual ~branch_history_database_t() { }
};

#endif /* __CLUSTERING_VERSION_HPP__ */

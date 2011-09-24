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

template<class protocol_t>
class version_map_t {
public:
    version_map_t() { }
    version_map_t(typename protocol_t::region_t r, version_range_t v) {
        regions.push_back(std::make_pair(r, v));
    }
    explicit version_map_t(std::vector<std::pair<typename protocol_t::region_t, version_range_t> > rs) :
        regions(rs)
    {
        /* Make sure it's contiguous and nonoverlapping */
        DEBUG_ONLY(get_region());
    }

    typename protocol_t::region_t get_region() THROWS_NOTHING {
        std::vector<typename protocol_t::region_t> rs;
        for (int i = 0; i < (int)regions.size(); i++) rs.push_back(regions[i].first);
        return region_join(rs);
    }

    std::vector<std::pair<typename protocol_t::region_t, version_range_t> > regions;

    RDB_MAKE_ME_SERIALIZABLE_1(regions);
};

template<class protocol_t>
class version_map_view_t {
public:
    virtual ~version_map_view_t() { }

    typename protocol_t::region_t get_region() {
        return region;
    }

    version_map_t<protocol_t> read() {
        ASSERT_FINITE_CORO_WAITING;
        version_map_t<protocol_t> map = do_read();
        rassert(map.get_region() == region);
        return map;
    }

    void write(version_range_t value) {
        ASSERT_FINITE_CORO_WAITING;
        do_write(value);
        rassert(read().regions.size() == 1);
        rassert(read().regions[0].second == value);
    }

protected:
    version_map_view_t(typename protocol_t::region_t r) : region(r) { }
    virtual version_map_t<protocol_t> do_read() = 0;
    virtual void do_write(version_range_t value) = 0;

private:
    typename protocol_t::region_t region;
};

#endif /* __CLUSTERING_VERSION_HPP__ */

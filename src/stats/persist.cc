#include "stats/persist.hpp"

#include <vector>
#include <sstream>

#include <stdlib.h>

#include <boost/numeric/conversion/cast.hpp>

#include "stats/serialize.hpp"
#include "utils.hpp"
#include "concurrency/pmap.hpp"


static intrusive_list_t<persistent_stat_t> &pstat_list() {
    /* Getter function so that we can be sure that var_list is initialized before it is needed,
    as advised by the C++ FAQ. Otherwise, a perfmon_t might be initialized before the var list
    was initialized. */
    static intrusive_list_t<persistent_stat_t> list;
    return list;
}

persistent_stat_t::persistent_stat_t() {
    pstat_list().push_back(this);
}

persistent_stat_t::~persistent_stat_t() {
    pstat_list().remove(this);
}

static void co_persist_visit(int thread, const std::vector<void*> &data) {
    on_thread_t moving(thread);
    int i = 0;
    for (persistent_stat_t *p = pstat_list().head(); p; p = pstat_list().next(p))
        p->visit_persist(data[i++]);
}

void persistent_stat_t::persist_all(metadata_store_t *store) {
    std::vector<void*> data;
    data.reserve(pstat_list().size());
    for (persistent_stat_t *p = pstat_list().head(); p; p = pstat_list().next(p))
        data.push_back(p->begin_persist());

    pmap(get_num_threads(), boost::bind(&co_persist_visit, _1, data));

    int i = 0;
    for (persistent_stat_t *p = pstat_list().head(); p; p = pstat_list().next(p)) {
        std::string key = p->persist_key();
        std::string value = p->end_persist(data[i++]);
        store->set_meta(key, value);
    }
}

void persistent_stat_t::unpersist_all(metadata_store_t *store) {
    std::string value;
    for (persistent_stat_t *p = pstat_list().head(); p; p = pstat_list().next(p)) {
        std::string key = p->persist_key();
        if (store->get_meta(key, &value)) {
            p->unpersist(value);
        }
    }
}


// perfmon_persistent_counter_t
perfmon_persistent_counter_t::perfmon_persistent_counter_t(std::string name, bool internal)
    : perfmon_counter_t(name, internal)
    , unpersisted_value(0) { }

std::string perfmon_persistent_counter_t::persist_key() { return name; }

void perfmon_persistent_counter_t::get_thread_persist(padded_int64_t *stat) { stat->value = get(); }

int64_t perfmon_persistent_counter_t::combine_stats(padded_int64_t *stats) {
    return perfmon_counter_t::combine_stats(stats) + unpersisted_value;
}

std::string perfmon_persistent_counter_t::combine_persist(padded_int64_t *stats) {
    int64_t total = combine_stats(stats);
    std::ostringstream ss;
    serialize_i64(ss, total);
    return ss.str();
}

void perfmon_persistent_counter_t::unpersist(const std::string &value) {
    rassert(unpersisted_value == 0, "unpersist called multiple times");
    std::istringstream ss(value);
    unpersisted_value = unserialize_i64(ss);
}


// perfmon_persistent_stddev_t
perfmon_persistent_stddev_t::perfmon_persistent_stddev_t(std::string name, bool internal)
    : perfmon_stddev_t(name, internal) { }

std::string perfmon_persistent_stddev_t::persist_key() { return name; }

void perfmon_persistent_stddev_t::get_thread_persist(stddev_t *stat) { get_thread_stat(stat); }

stddev_t perfmon_persistent_stddev_t::combine_stats(stddev_t *stats) {
    stddev_t devs[2] = { unpersisted_value, perfmon_stddev_t::combine_stats(stats) };
    return stddev_t::combine(2, devs);
}

std::string perfmon_persistent_stddev_t::combine_persist(stddev_t *stats) {
    stddev_t total = combine_stats(stats);
    std::ostringstream ss;
    serialize_u64(ss, boost::numeric_cast<uint64_t>(total.datapoints()));
    serialize_float(ss, total.mean());
    serialize_float(ss, total.standard_variance());
    return ss.str();
}

void perfmon_persistent_stddev_t::unpersist(const std::string &value) {
    std::istringstream ss(value);
    size_t datapoints = boost::numeric_cast<size_t>(unserialize_u64(ss));
    float mean = unserialize_float(ss);
    float variance = unserialize_float(ss);
    unpersisted_value = stddev_t(datapoints, mean, variance);
}

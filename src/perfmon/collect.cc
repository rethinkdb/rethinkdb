#include "utils.hpp"
#include <boost/bind.hpp>

#include "perfmon/collect.hpp"
#include "concurrency/pmap.hpp"

/* This is the function that actually gathers the stats. It is illegal to create or destroy
perfmon_t objects while perfmon_get_stats is active. */
static void co_perfmon_visit(int thread, void *data) {
    on_thread_t moving(thread);
    get_global_perfmon_collection().visit_stats(data);
}

int get_num_threads();

perfmon_result_t *perfmon_get_stats() {
    void *data = get_global_perfmon_collection().begin_stats();
    pmap(get_num_threads(), boost::bind(&co_perfmon_visit, _1, data));
    return get_global_perfmon_collection().end_stats(data);
}


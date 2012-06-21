#ifndef MEMCACHED_STATS_HPP_
#define MEMCACHED_STATS_HPP_

#include "perfmon/perfmon.hpp"

struct memcached_stats_t {
    explicit memcached_stats_t(perfmon_collection_t *parent)
        : parser_collection("parser", parent, true, true),
          pm_cmd_set("cmd_set", secs_to_ticks(1.0), &parser_collection),
          pm_cmd_get("cmd_get", secs_to_ticks(1.0), &parser_collection),
          pm_cmd_rget("cmd_rget", secs_to_ticks(1.0), &parser_collection),
          pm_delete_key_size("cmd_delete_key_size", &parser_collection),
          pm_get_key_size("cmd_get_key_size", &parser_collection),
          pm_storage_key_size("cmd_set_key_size", &parser_collection),
          pm_storage_value_size("cmd_set_val_size", &parser_collection),
          pm_conns_reading("conns_reading", secs_to_ticks(1), &parser_collection),
          pm_conns_writing("conns_writing", secs_to_ticks(1), &parser_collection),
          pm_conns_acting("conns_acting", secs_to_ticks(1), &parser_collection),
          pm_conns("pm_conns", secs_to_ticks(1), &parser_collection)
    { }

    perfmon_collection_t parser_collection;

    perfmon_duration_sampler_t
        pm_cmd_set,
        pm_cmd_get,
        pm_cmd_rget;

    perfmon_stddev_t
        pm_delete_key_size,
        pm_get_key_size,
        pm_storage_key_size,
        pm_storage_value_size;

    perfmon_duration_sampler_t
        pm_conns_reading,
        pm_conns_writing,
        pm_conns_acting;

    perfmon_duration_sampler_t pm_conns;
};


#endif /* MEMCACHED_STATS_HPP_ */

#ifndef MEMCACHED_STATS_HPP_
#define MEMCACHED_STATS_HPP_

#include "perfmon/perfmon.hpp"

struct memcached_stats_t {
    explicit memcached_stats_t(perfmon_collection_t *parent)
        : parser_collection(),
          pm_cmd_set(secs_to_ticks(1.0)),
          pm_cmd_get(secs_to_ticks(1.0)),
          pm_cmd_rget(secs_to_ticks(1.0)),
          pm_delete_key_size(),
          pm_get_key_size(),
          pm_storage_key_size(),
          pm_storage_value_size(),
          pm_conns_reading(secs_to_ticks(1)),
          pm_conns_writing(secs_to_ticks(1)),
          pm_conns_acting(secs_to_ticks(1)),
          pm_conns(secs_to_ticks(1)),
          parent_membership(parent, &parser_collection, "parser"),
          stats_membership(&parser_collection,
              &pm_cmd_set, "cmd_set",
              &pm_cmd_get, "cmd_get",
              &pm_cmd_rget, "cmd_rget",
              &pm_delete_key_size, "cmd_delete_key_size",
              &pm_get_key_size, "cmd_get_key_size",
              &pm_storage_key_size, "cmd_set_key_size",
              &pm_storage_value_size, "cmd_set_val_size",
              &pm_conns_reading, "conns_reading",
              &pm_conns_writing, "conns_writing",
              &pm_conns_acting, "conns_acting",
              &pm_conns, "pm_conns",
              NULLPTR)
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

    perfmon_membership_t parent_membership;
    perfmon_multi_membership_t stats_membership;
};


#endif /* MEMCACHED_STATS_HPP_ */

#ifndef BTREE_STATS_HPP_
#define BTREE_STATS_HPP_

#include "perfmon/perfmon.hpp"

class btree_stats_t {
public:
    explicit btree_stats_t(perfmon_collection_t *parent,
                           const std::string &identifier)
        : btree_collection(),
          pm_keys_read(secs_to_ticks(1)),
          pm_keys_set(secs_to_ticks(1)),
          pm_keys_membership(&btree_collection,
              &pm_keys_read, "keys_read",
              &pm_total_keys_read, "total_keys_read",
              &pm_keys_set, "keys_set",
              &pm_total_keys_set, "total_keys_set") {
        if (parent != nullptr) {
            rename(parent, identifier);
        }
    }

    void hide() {
        btree_collection_membership.reset();
    }

    void rename(perfmon_collection_t *parent,
                const std::string &identifier) {
        btree_collection_membership.reset();
        btree_collection_membership.init(new perfmon_membership_t(
            parent,
            &btree_collection,
            "btree-" + identifier));
    }

    perfmon_collection_t btree_collection;
    scoped_ptr_t<perfmon_membership_t> btree_collection_membership;
    perfmon_rate_monitor_t
        pm_keys_read,
        pm_keys_set;
    perfmon_counter_t
        pm_total_keys_read,
        pm_total_keys_set;
    perfmon_multi_membership_t pm_keys_membership;
};


#endif  // BTREE_STATS_HPP_

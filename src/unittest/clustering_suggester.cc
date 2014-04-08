// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/suggester/suggest_blueprint.hpp"
#include "clustering/generic/nonoverlapping_regions.hpp"
#include "rdb_protocol/protocol.hpp"

namespace unittest {

TEST(ClusteringSuggester, NewNamespace) {
    datacenter_id_t primary_datacenter = generate_uuid(),
        secondary_datacenter = generate_uuid();

    std::vector<machine_id_t> machines;
    for (int i = 0; i < 10; i++) {
        machines.push_back(generate_uuid());
    }

    std::map<machine_id_t, reactor_business_card_t> directory;
    for (int i = 0; i < 10; i++) {
        reactor_business_card_t rb;
        rb.activities[generate_uuid()] = reactor_business_card_t::activity_entry_t(region_t::universe(), reactor_business_card_t::nothing_t());
        directory[machines[i]] = rb;
    }

    std::map<machine_id_t, datacenter_id_t> machine_data_centers;
    for (int i = 0; i < 10; i++) {
        machine_data_centers[machines[i]] = i % 2 == 0 ? primary_datacenter : secondary_datacenter;
    }

    std::map<datacenter_id_t, int> affinities;
    affinities[primary_datacenter] = 2;
    affinities[secondary_datacenter] = 3;

    nonoverlapping_regions_t shards;
    bool success = shards.add_region(hash_region_t<key_range_t>(key_range_t(key_range_t::closed, store_key_t(""),
                                                                            key_range_t::open, store_key_t("n"))));
    ASSERT_TRUE(success);
    success = shards.add_region(hash_region_t<key_range_t>(key_range_t(key_range_t::closed, store_key_t("n"),
                                                                       key_range_t::none, store_key_t())));
    ASSERT_TRUE(success);

    std::map<machine_id_t, int> usage;
    persistable_blueprint_t blueprint = suggest_blueprint(
        directory,
        primary_datacenter,
        affinities,
        shards,
        machine_data_centers,
        region_map_t<machine_id_t>(region_t::universe(), nil_uuid()),
        region_map_t<std::set<machine_id_t> >(),
        &usage,
        true);

    EXPECT_EQ(machines.size(), blueprint.machines_roles.size());
}

}  // namespace unittest

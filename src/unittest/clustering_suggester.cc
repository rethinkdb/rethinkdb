#include "unittest/gtest.hpp"

#include "clustering/suggester/suggester.hpp"
#include "mock/dummy_protocol.hpp"

namespace unittest {

using namespace mock;

TEST(ClusteringSuggester, NewNamespace) {
    datacenter_id_t primary_datacenter = generate_uuid(),
        secondary_datacenter = generate_uuid();

    std::vector<peer_id_t> peers;
    for (int i = 0; i < 10; i++) {
        peers.push_back(peer_id_t(generate_uuid()));
    }

    std::map<peer_id_t, reactor_business_card_t<dummy_protocol_t> > directory;
    for (int i = 0; i < 10; i++) {
        reactor_business_card_t<dummy_protocol_t> rb;
        rb.activities[generate_uuid()] = std::make_pair(a_thru_z_region(), reactor_business_card_t<dummy_protocol_t>::nothing_t());
        directory[peers[i]] = rb;
    }

    std::map<peer_id_t, datacenter_id_t> machine_data_centers;
    for (int i = 0; i < 10; i++) {
        machine_data_centers[peers[i]] = i % 2 == 0 ? primary_datacenter : secondary_datacenter;
    }

    std::map<datacenter_id_t, int> affinities;
    affinities[primary_datacenter] = 2;
    affinities[secondary_datacenter] = 3;

    std::set<dummy_protocol_t::region_t> shards;
    shards.insert(dummy_protocol_t::region_t('a', 'm'));
    shards.insert(dummy_protocol_t::region_t('n', 'z'));

    blueprint_t<dummy_protocol_t> blueprint = suggest_blueprint<dummy_protocol_t>(
        directory,
        primary_datacenter,
        affinities,
        shards,
        machine_data_centers);

    EXPECT_EQ(peers.size(), blueprint.peers_roles.size());
}

}  // namespace unittest

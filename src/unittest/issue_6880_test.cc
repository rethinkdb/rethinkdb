// Copyright 2010-2015 RethinkDB, all rights reserved.
// Unit tests for Issue #6880 - Re-provisioned Server crash fix
// Tests that empty IP addresses are handled gracefully when joining

#include "unittest/gtest.hpp"

#include "arch/runtime/thread_pool.hpp"
#include "arch/timing.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/connectivity/cluster.hpp"

namespace unittest {

// Test that attempting to join a peer with empty IP addresses is handled gracefully
// This simulates the case when a server was removed from the cluster and its
// address list becomes empty
TPTEST(Issue6880, EmptyPeerIpsHandled) {
    connectivity_cluster_t c1;
    test_cluster_run_t cr1(&c1);
    
    // Create a peer_address_t with an empty set of IPs
    // This simulates the case of a re-provisioned/removed server
    std::set<ip_and_port_t> empty_ips;
    peer_address_t empty_peer_address(empty_ips);
    
    // Attempt to join the empty peer address
    // The fix for #6880 should handle this gracefully without crashing
    cr1.join(empty_peer_address, 0);
    
    let_stuff_happen();
    
    // The cluster node should still be operational
    EXPECT_EQ(c1.get_me().is_nil(), false);
    
    // Should only have connection to self
    EXPECT_EQ(c1.get_connections()->get_all().size(), 1u);
}

// Test that a peer with empty canonical addresses is handled correctly
TPTEST(Issue6880, EmptyCanonicalAddressesHandled) {
    connectivity_cluster_t c1;
    // Create with empty canonical addresses
    peer_address_t empty_canonical;
    test_cluster_run_t cr1(&c1, empty_canonical);
    
    // The server should still start up correctly
    EXPECT_EQ(c1.get_me().is_nil(), false);
    EXPECT_EQ(c1.get_connections()->get_all().size(), 1u);
    
    // Should be able to get the port
    EXPECT_GT(cr1.get_port(), 0);
}

// Test reconnection logic with a peer that has changing addresses
TPTEST(Issue6880, ServerReconnectionAfterRemoval) {
    connectivity_cluster_t c1, c2;
    test_cluster_run_t cr1(&c1);
    test_cluster_run_t cr2(&c2);
    
    // First join - normal connection
    cr1.join(get_cluster_local_address(&c2), 0);
    let_stuff_happen();
    
    // Verify connection
    EXPECT_EQ(c1.get_connections()->get_all().size(), 2u);
    EXPECT_EQ(c2.get_connections()->get_all().size(), 2u);
    
    // Now simulate what happens when c2 is "removed" and re-provisioned
    // by creating a new cluster node that tries to join c1
    {
        connectivity_cluster_t c3;
        test_cluster_run_t cr3(&c3);
        
        // c3 joins c1
        cr3.join(get_cluster_local_address(&c1), 0);
        let_stuff_happen();
        
        // Verify the new connection works
        EXPECT_EQ(c1.get_connections()->get_all().size(), 3u);
        EXPECT_EQ(c3.get_connections()->get_all().size(), 2u);
    }
    
    // After c3 is destroyed, c1 should still be stable
    let_stuff_happen();
    EXPECT_EQ(c1.get_me().is_nil(), false);
    EXPECT_GE(c1.get_connections()->get_all().size(), 2u);
}

// Test that multiple empty join attempts don't cause issues
TPTEST(Issue6880, MultipleEmptyJoinAttempts) {
    connectivity_cluster_t c1;
    test_cluster_run_t cr1(&c1);
    
    std::set<ip_and_port_t> empty_ips;
    peer_address_t empty_peer_address(empty_ips);
    
    // Try joining empty addresses multiple times
    for (int i = 0; i < 5; ++i) {
        cr1.join(empty_peer_address, 0);
        let_stuff_happen();
        
        // Should remain stable after each attempt
        EXPECT_EQ(c1.get_me().is_nil(), false);
        EXPECT_EQ(c1.get_connections()->get_all().size(), 1u);
    }
}

// Test that normal operation continues after handling empty peer
TPTEST(Issue6880, NormalOperationAfterEmptyPeer) {
    connectivity_cluster_t c1, c2;
    recording_test_application_t a1(&c1, 'T'), a2(&c2, 'T');
    test_cluster_run_t cr1(&c1);
    test_cluster_run_t cr2(&c2);
    
    // First try to join an empty peer (should be handled gracefully)
    std::set<ip_and_port_t> empty_ips;
    peer_address_t empty_peer_address(empty_ips);
    cr1.join(empty_peer_address, 0);
    let_stuff_happen();
    
    // Then do a normal join
    cr1.join(get_cluster_local_address(&c2), 0);
    let_stuff_happen();
    
    // Verify normal communication works
    a1.send(456, c2.get_me());
    let_stuff_happen();
    a2.expect(456, c1.get_me());
    
    // Both should be connected
    EXPECT_EQ(c1.get_connections()->get_all().size(), 2u);
    EXPECT_EQ(c2.get_connections()->get_all().size(), 2u);
}

}  // namespace unittest

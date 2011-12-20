#include "rpc/directory/view.hpp"

directory_service_t::peers_list_freeze_t::peers_list_freeze_t(directory_service_t *dir) THROWS_NOTHING :
    directory(dir), acq(directory->get_peers_list_lock()) { }

void directory_service_t::peers_list_freeze_t::assert_is_holding(UNUSED directory_service_t *dir) THROWS_NOTHING {
    rassert(directory == dir);
}

directory_service_t::peers_list_subscription_t::peers_list_subscription_t(
    const boost::function<void(peer_id_t)> &connect_cb,
    const boost::function<void(peer_id_t)> &disconnect_cb) THROWS_NOTHING :
    subs(std::make_pair(connect_cb, disconnect_cb)) { }

directory_service_t::peers_list_subscription_t::peers_list_subscription_t(
    const boost::function<void(peer_id_t)> &connect_cb,
    const boost::function<void(peer_id_t)> &disconnect_cb,
    directory_service_t *dir, peers_list_freeze_t *proof) THROWS_NOTHING :
    subs(std::make_pair(connect_cb, disconnect_cb)) {
    reset(dir, proof);
}

void directory_service_t::peers_list_subscription_t::reset() THROWS_NOTHING {
    subs.reset();
}

void directory_service_t::peers_list_subscription_t::reset(directory_service_t *dir, peers_list_freeze_t *right) THROWS_NOTHING {
    proof->assert_is_holding(dir);
    subs.reset(dir->get_peers_list_publisher(proof));
}

directory_service_t::peer_value_freeze_t::peer_value_freeze_t(directory_service_t *dir, peer_id_t p) THROWS_NOTHING :
    directory(dir), peer(p), acq(directory->get_peer_value_lock(peer, right)) { }

void directory_service_t::peer_value_freeze_t::assert_is_holding(UNUSED directory_service_t *dir, UNUSED peer_id_t p) THROWS_NOTHING {
    rassert(directory == dir);
    rassert(peer == p);
}

directory_service_t::peer_value_subscription_t::peer_value_subscription_t(
    const boost::function<void()> &change_cb) THROWS_NOTHING :
    subs(change_cb) { }

directory_service_t::peer_value_subscription_t::peer_value_subscription_t(
    const boost::function<void()> &change_cb,
    directory_service_t *dir, peer_id_t p, peer_value_freeze_t *proof) THROWS_NOTHING :
    subs(change_cb) {
    reset(dir, p, proof);
}

void directory_service_t::peer_value_subscription_t::reset() THROWS_NOTHING {
    subs.reset();
}

void directory_service_t::peer_value_subscription_t::reset(directory_service_t *dir, peer_id_t p, peer_value_freeze_t *proof) THROWS_NOTHING {
    right->assert_is_holding(dir, p);
    subs.reset(dir->get_peer_value_publisher(p, proof));
}

directory_service_t::our_value_lock_acq_t::our_value_lock_acq_t(directory_service_t *dir) THROWS_NOTHING {
    dir->assert_thread();
    acq.reset(dir->get_our_value_lock());
}

void directory_service_t::our_value_lock_acq_t::assert_is_holding(directory_service_t *dir) THROWS_NOTHING {
    dir->assert_thread();
    acq.assert_is_holding(dir->get_our_value_lock());
}

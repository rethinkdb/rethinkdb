#include "rpc/mailbox/raw_mailbox.hpp"

#include <inttypes.h>

#include "rpc/mailbox/mailbox.hpp"
#include "utils.hpp"

/* raw_mailbox_t */

raw_mailbox_t::address_t::address_t() :
    peer(peer_id_t()), thread(-1), mailbox_id(0) { }

bool raw_mailbox_t::address_t::is_nil() const {
    return peer.is_nil();
}

peer_id_t raw_mailbox_t::address_t::get_peer() const {
    guarantee(!is_nil(), "A nil address has no peer");
    return peer;
}

std::string raw_mailbox_t::address_t::human_readable() const {
    return strprintf("%s:%d:%" PRIu64, uuid_to_str(peer.get_uuid()).c_str(), thread, mailbox_id);
}

raw_mailbox_t::raw_mailbox_t(mailbox_manager_t *m, mailbox_read_callback_t *_callback) :
    manager(m),
    mailbox_id(manager->register_mailbox(this)),
    callback(_callback) {
    guarantee(callback != nullptr);
}

raw_mailbox_t::~raw_mailbox_t() {
    assert_thread();
    if (callback != nullptr) {
        begin_shutdown();
    }
}

void raw_mailbox_t::begin_shutdown() {
    assert_thread();
    guarantee(callback != nullptr);
    callback = nullptr;
    manager->unregister_mailbox(mailbox_id);
    drainer.begin_draining();
}

raw_mailbox_t::address_t raw_mailbox_t::get_address() const {
    address_t a;
    a.peer = manager->get_connectivity_cluster()->get_me();
    a.thread = home_thread().threadnum;
    a.mailbox_id = mailbox_id;
    return a;
}


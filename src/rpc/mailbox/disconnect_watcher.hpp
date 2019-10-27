#ifndef RPC_MAILBOX_DISCONNECT_WATCHER_HPP_
#define RPC_MAILBOX_DISCONNECT_WATCHER_HPP_

#include "concurrency/auto_drainer.hpp"
#include "concurrency/signal.hpp"

class mailbox_manager_t;
class peer_id_t;

/* Note: disconnect_watcher_t keeps the connection alive for as long as it
exists, blocking reconnects from getting through. Avoid keeping the
disconnect_watcher_t around for long after it gets pulsed. */
class disconnect_watcher_t : public signal_t, private signal_t::subscription_t {
public:
    disconnect_watcher_t(mailbox_manager_t *mailbox_manager, peer_id_t peer);
private:
    void run();
    auto_drainer_t::lock_t connection_keepalive;
};

#endif  // RPC_MAILBOX_DISCONNECT_WATCHER_HPP_

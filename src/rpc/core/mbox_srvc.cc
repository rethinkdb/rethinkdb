#include "rpc/core/mbox_srvc.hpp"
#include "rpc/core/cluster.hpp"

void mailbox_srvc_t::handle(cluster_peer_t *sndr) {
    if (cluster_mailbox_t *mbox = get_cluster()->get_mailbox(msg.id())) {
#ifndef NDEBUG
        fprintf(stderr, "Got a message\n");
        if (msg.has_type()) {
            fprintf(stderr, "Type: %s\n", msg.type().c_str());
            std::string realname = demangle_cpp_name(mbox->expected_type().name());
            if (realname != msg.type()) {
                logERR("Error, mailbox type mismatch. Mailbox msg of type: %s "
                        "from peer: %d sent to mailbox: %d which is of type: "
                        "%s\n", msg.type().c_str(), sndr->id,  msg.id(),
                        realname.c_str()); 
                goto ERROR_BREAKOUT;
            }
        } else {
            logWRN("Peer: %d did not specify type information but I'm supposed"
                    "to be enforcing type safety. If you want me to actually do"
                    "checks compile him in debug mode, if you don't want to see"
                    "this message compile me in release mode\n", sndr->id);
        }
#endif

        cluster_peer_inpipe_t inpipe(sndr->conn.get(), msg.length());
        cond_t to_signal_when_done;
        fprintf(stderr, "Handing over to mailbox\n");
        coro_t::spawn_now(boost::bind(
            &cluster_mailbox_t::unserialize, mbox, boost::ref(inpipe.get_archive()),
            // Wrap in `boost::function` to avoid a weird `boost::bind()` feature
            boost::function<void()>(boost::bind(&cond_t::pulse, &to_signal_when_done))
            ));
        to_signal_when_done.wait();

    } else {
        if (msg.has_type()) { 
            logERR("Unknown mailbox. Mailbox msg of type: %s from peer: %d, sent to nonexistant mailbox:%d\n", msg.type().c_str(), sndr->id, msg.id());
        } else { 
            logERR("Unknown mailbox. Mailbox msg of unspecified type from peer: %d, sent to nonexistant mailbox:%d\n", sndr->id, msg.id());
        }
        goto ERROR_BREAKOUT;
    }
    return;

ERROR_BREAKOUT:
        /* consume the msg so that we can keep going */
        sndr->conn->get_raw_conn()->pop(msg.length());
}

#include "rpc/core/mbox_srvc.hpp"
#include "rpc/core/cluster.hpp"
#include <cxxabi.h>

void mailbox_srvc_t::handle(cluster_peer_t *sndr) {
    if (cluster_mailbox_t *mbox = get_cluster().get_mailbox(msg.id())) {
#ifndef NDEBUG
        if (msg.has_type()) {
            int status; 
            char *realname = abi::__cxa_demangle(mbox->expected_type().name(), 0, 0, &status);
            rassert(status == 0);

            if (strcmp(realname, msg.type().c_str()) != 0) {
                logERR("Error, mailbox type mismatch. Mailbox msg of type: %s "
                        "from peer: %d sent to mailbox: %d which is of type: "
                        "%s\n", msg.type().c_str(), sndr->id,  msg.id(),
                        realname); 
                free(realname);
                goto ERROR_BREAKOUT;
            } else {
                free(realname);
            }
        } else {
            logWRN("Peer: %d did not specify type information but I'm supposed"
                    "to be enforcing type safety. If you want me to actually do"
                    "checks compile him in debug mode, if you don't want to see"
                    "this message compile me in release mode\n", sndr->id);
        }
#endif
        cluster_inpipe_t inpipe(sndr->conn.get(), msg.length());
        coro_t::spawn_now(boost::bind(&cluster_mailbox_t::unserialize, mbox, &inpipe));
        inpipe.to_signal_when_done.wait();
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
        sndr->conn->pop(msg.length());
}
